#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <assert.h>
#include <limits.h>
#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;

// heap-allocated, caller must free. locates data files per command line args.
char* find_data(const char* datum);

bool unicodeblocks_demo (ncpp::NotCurses &nc);
bool witherworm_demo (ncpp::NotCurses &nc);
bool box_demo (ncpp::NotCurses &nc);
bool trans_demo (ncpp::NotCurses &nc);
bool chunli_demo (ncpp::NotCurses &nc);
bool grid_demo (ncpp::NotCurses &nc);
bool sliding_puzzle_demo (ncpp::NotCurses &nc);
bool view_demo (ncpp::NotCurses &nc);
bool eagle_demo (ncpp::NotCurses &nc);
bool panelreel_demo (ncpp::NotCurses &nc);
bool xray_demo (ncpp::NotCurses &nc);
bool luigi_demo (ncpp::NotCurses &nc);
bool outro (ncpp::NotCurses &nc);

/*------------------------------- demo input API --------------------------*/
bool input_dispatcher (ncpp::NotCurses &nc);
int stop_input (void);

// if 'q' is pressed at any time during the demo, gracefully interrupt/exit
void interrupt_demo (void);

// demos should not call notcurses_getc() directly, as it's being monitored by
// the toplevel event listener. instead, call this intermediate API. just
// replace 'notcurses' with 'demo'.
char32_t demo_getc (const struct timespec* ts, ncinput* ni);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
demo_getc_nblock (ncinput* ni)
{
	struct timespec ts = { /*.tv_sec =*/ 0, /*.tv_nsec =*/ 0 };
	return demo_getc (&ts, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
demo_getc_blocking (ncinput* ni)
{
	return demo_getc (nullptr, ni);
}
/*----------------------------- end demo input API -------------------------*/

/*-------------------------------time helpers----------------------------*/
int timespec_subtract(struct timespec *result, const struct timespec *time1,
                      struct timespec *time0);

#define GIG 1000000000ul

static inline uint64_t
timespec_to_ns(const struct timespec* ts){
  return ts->tv_sec * GIG + ts->tv_nsec;
}

static inline struct timespec*
ns_to_timespec(uint64_t ns, struct timespec* ts){
  ts->tv_sec = ns / GIG;
  ts->tv_nsec = ns % GIG;
  return ts;
}

static inline int64_t
timespec_subtract_ns(const struct timespec* time1, const struct timespec* time0){
  int64_t ns = timespec_to_ns(time1);
  ns -= timespec_to_ns(time0);
  return ns;
}

// divide the provided timespec 'ts' by 'divisor' into 'quots'
static inline void
timespec_div(const struct timespec* ts, unsigned divisor, struct timespec* quots){
  uint64_t ns = timespec_to_ns(ts);
  ns /= divisor;
  quots->tv_nsec = ns % GIG;
  quots->tv_sec = ns / GIG;
}

// divide the provided timespec 'ts' by 'multiplier' into 'product'
static inline void
timespec_mul(const struct timespec* ts, unsigned multiplier, struct timespec* product){
  uint64_t ns = timespec_to_ns(ts);
  ns *= multiplier;
  product->tv_nsec = ns % GIG;
  product->tv_sec = ns / GIG;
}
/*-------------------------------time helpers----------------------------*/

/*----------------------------------HUD----------------------------------*/
extern ncpp::Plane* hud;
struct ncpp::Plane* hud_create (ncpp::NotCurses &nc);
bool hud_destroy (void);

// let the HUD know about an upcoming demo
bool hud_schedule (const char* demoname);

// demos should not call notcurses_render() themselves, but instead call
// demo_render(), which will ensure the HUD stays on the top of the z-stack.
bool demo_render (ncpp::NotCurses &nc);

int demo_fader (struct notcurses *nc, struct ncplane *ncp);

// grab the hud with the mouse
bool hud_grab (int y, int x);

// release the hud
bool hud_release (void);

typedef struct demoresult {
	char selector;
	struct ncstats stats;
	uint64_t timens;
	bool failed;
} demoresult;

// let the HUD know that a demo has completed, reporting the stats
bool hud_completion_notify (const demoresult* result);

// HUD retrieves results on demand from core
const demoresult* demoresult_lookup (int idx);
/*----------------------------------HUD----------------------------------*/

#endif
