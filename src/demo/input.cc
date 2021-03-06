#include <pthread.h>
#include "demo.hh"

#include <ncpp/NotCurses.hh>

using namespace ncpp;

typedef struct nciqueue {
	ncinput ni;
	struct nciqueue *next;
} nciqueue;

static bool spawned;
static pthread_t tid;
static nciqueue* queue;
static nciqueue** enqueue = &queue;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// incoming timespec is relative (or even NULL, for blocking), but we need
// absolute deadline, so convert it up.
char32_t demo_getc (const struct timespec* ts, ncinput* ni)
{
	struct timespec now;
	uint64_t ns;
	struct timespec abstime;

	if (ts) {
		clock_gettime (CLOCK_MONOTONIC, &now);
		ns = timespec_to_ns (&now) + timespec_to_ns (ts);
		ns_to_timespec (ns, &abstime);
	} else {
		abstime.tv_sec = ~0;
		abstime.tv_nsec = ~0;
	}

	pthread_mutex_lock (&lock);
	while (!queue) {
		clock_gettime (CLOCK_MONOTONIC, &now);
		if(timespec_to_ns (&now) > timespec_to_ns (&abstime)) {
			pthread_mutex_unlock (&lock);
			return 0;
		}
		pthread_cond_timedwait (&cond, &lock, &abstime);
	}
	nciqueue *q = queue;
	queue = queue->next;
	if (queue == nullptr) {
		enqueue = &queue;
	}
	pthread_mutex_unlock (&lock);
	char32_t id = q->ni.id;
	if (ni) {
		memcpy (ni, &q->ni, sizeof(*ni));
	}
	free(q);

	return id;
}

static int
pass_along (const ncinput* ni)
{
	auto nq = new nciqueue;
	memcpy (&nq->ni, ni, sizeof(*ni));
	nq->next = nullptr;
	*enqueue = nq;
	enqueue = &nq->next;
	return 0;
}

static bool
handle_mouse (NotCurses *nc, const ncinput* ni)
{
	if (ni->id != NCKEY_BUTTON1 && ni->id != NCKEY_RELEASE) {
		return true;
	}

	int ret;
	if (ni->id == NCKEY_RELEASE) {
		ret = hud_release ();
	} else {
		ret = hud_grab (ni->y, ni->x);
	}

	if (ret) {
		ret = demo_render (*nc);
	}

	return ret;
}

static void *
ultramegaok_demo (void *vnc)
{
	ncinput ni;
	auto nc = static_cast<NotCurses*>(vnc);
	char32_t id;
	while ((id = nc->getc (true, &ni)) != (char32_t)-1) {
		pthread_testcancel ();
		if (id == 0) {
			continue;
		}

		if (NCKey::IsMouse (ni.id)) {
			handle_mouse (nc, &ni);
		} else {
			if (ni.id == 'q') {
				interrupt_demo ();
			}
			// go ahead and pass through the keyboard press, even if it was a 'q'
			// (this might cause the demo to exit immediately, as is desired)
			pass_along (&ni);
		}
	}

	return nullptr;
}

// listens for events, handling mouse events directly and making other ones
// available to demos
bool input_dispatcher (NotCurses &nc)
{
	spawned = true;
	if (pthread_create (&tid, nullptr, ultramegaok_demo, &nc)) {
		spawned = false;
		return false;
	}
	return true;
}

int stop_input (void)
{
	int ret = 0;
	if (spawned) {
		ret |= pthread_cancel (tid);
		ret |= pthread_join (tid, nullptr);
		spawned = false;
	}
	return ret;
}
