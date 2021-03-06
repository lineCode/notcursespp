#include <config.hh>

#include <memory>
#include <cstdlib>
#include <cstring>

#include "demo.hh"

#include <ncpp/NotCurses.hh>

using namespace ncpp;

// FIXME do the bigger dimension on the screen's bigger dimension
constexpr int CHUNKS_VERT = 6;
constexpr int CHUNKS_HORZ = 12;
constexpr int MOVES = 20;

static bool
move_square (NotCurses &nc, std::shared_ptr<Plane> chunk, int* holey, int* holex, uint64_t movens)
{
	int newholex, newholey;
	chunk->get_yx (&newholey, &newholex);

	// we need to move from newhole to hole over the course of movetime
	int deltay, deltax;
	deltay = *holey - newholey;
	deltax = *holex - newholex;

	// divide movetime into abs(max(deltay, deltax)) units, and move delta
	int units = abs (deltay) > abs (deltax) ? abs (deltay) : abs (deltax);
	movens /= units;

	struct timespec movetime;
	ns_to_timespec (movens, &movetime);

	int i;
	int targy = newholey;
	int targx = newholex;
	deltay = deltay < 0 ? -1 : deltay == 0 ? 0 : 1;
	deltax = deltax < 0 ? -1 : deltax == 0 ? 0 : 1;

	// FIXME do an adaptive time, like our fades, so we whip along under load
	for (i = 0 ; i < units ; ++i) {
		targy += deltay;
		targx += deltax;
		chunk->move (targy, targx);
		if (!demo_render (nc)) {
			return false;
		}
		nanosleep (&movetime, nullptr);
	}

	*holey = newholey;
	*holex = newholex;
	return true;
}

// we take demodelay * 5 to play MOVES moves
static bool
play (NotCurses &nc, std::shared_ptr<Plane> chunks[CHUNKS_VERT * CHUNKS_HORZ])
{
	const uint64_t delayns = timespec_to_ns (&demodelay);
	constexpr int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;

	struct timespec cur;
	clock_gettime (CLOCK_MONOTONIC, &cur);

	// we don't want to spend more than demodelay * 5
	const uint64_t totalns = delayns * 5;
	const uint64_t deadline_ns = timespec_to_ns (&cur) + totalns;
	const uint64_t movens = totalns / MOVES;
	int hole = random () % chunkcount;
	int holex, holey;

	chunks[hole]->get_yx (&holey, &holex);
	chunks[hole].reset ();

	int m;
	int lastdir = -1;
	for (m = 0 ; m < MOVES ; ++m) {
		clock_gettime (CLOCK_MONOTONIC, &cur);

		uint64_t now = cur.tv_sec * GIG + cur.tv_nsec;
		if (now >= deadline_ns) {
			break;
		}

		int mover = chunkcount;
		int direction;
		do {
			direction = random () % 4;
			switch (direction) {
				case 3: // up
					if (lastdir != 1 && hole >= CHUNKS_HORZ) { mover = hole - CHUNKS_HORZ; } break;

				case 2: // right
					if (lastdir != 0 && hole % CHUNKS_HORZ < CHUNKS_HORZ - 1) { mover = hole + 1; } break;

				case 1: // down
					if (lastdir != 3 && hole < chunkcount - CHUNKS_HORZ) { mover = hole + CHUNKS_HORZ; } break;

				case 0: // left
					if (lastdir != 2 && hole % CHUNKS_HORZ) { mover = hole - 1; } break;
			}
		} while (mover == chunkcount);

		lastdir = direction;
		move_square (nc, chunks[mover], &holey, &holex, movens);
		chunks[hole] = chunks[mover];
		chunks[mover] = nullptr;
		hole = mover;
	}

	return true;
}

static bool
fill_chunk (std::shared_ptr<Plane> n, int idx)
{
	const int hidx = idx % CHUNKS_HORZ;
	const int vidx = idx / CHUNKS_HORZ;
	char buf[4];
	int maxy, maxx;

	n->get_dim (&maxy, &maxx);
	snprintf (buf, sizeof(buf), "%02d", idx + 1); // don't zero-index to viewer

	uint64_t channels = 0;
	int r = 64 + hidx * 10;
	int b = 64 + vidx * 30;
	int g = 225 - ((hidx + vidx) * 12);

	channels_set_fg_rgb (&channels, r, g, b);
	if (!n->double_box (0, channels, maxy - 1, maxx - 1, 0)) {
		return false;
	}

	if (maxx >= 4 && maxy >= 3) {
		n->set_fg_rgb (0, 0, 0);
		n->set_bg_rgb (r, g, b);
		if (n->putstr ((maxy - 1) / 2, NCALIGN_CENTER, buf) <= 0) {
			return false;
		}
	}

	Cell style;
	style.set_fg_rgb (r, g, b);
	n->prime (style, "█", 0, channels);
	n->set_base (style);
	n->release (style);

	return true;
}

static bool
draw_bounding_box (Plane *n, int yoff, int xoff, int chunky, int chunkx)
{
	uint64_t channels = 0;

	channels_set_fg_rgb (&channels, 180, 80, 180);
	//channels_set_bg_rgb(&channels, 0, 0, 0);
	n->cursor_move (yoff, xoff);
	bool ret = n->rounded_box (0, channels,
							   CHUNKS_VERT * chunky + yoff + 1,
							   CHUNKS_HORZ * chunkx + xoff + 1, 0);
	return ret;
}

// break whatever's on the screen into panels and shift them around like a
// sliding puzzle. FIXME once we have copying, anyway. until then, just use
// background colors.
bool sliding_puzzle_demo (NotCurses &nc)
{
	bool ret = false;
	int maxx, maxy;
	int chunky, chunkx;

	nc.get_term_dim (&maxy, &maxx);
	// want at least 2x2 for each sliding chunk
	if (maxy < CHUNKS_VERT * 2 || maxx < CHUNKS_HORZ * 2) {
		return false;
	}

	// we want an 8x8 grid of chunks with a border. the leftover space will be unused
	chunky = (maxy - 2) / CHUNKS_VERT;
	chunkx = (maxx - 2) / CHUNKS_HORZ;

	// don't allow them to be too rectangular, but keep aspect ratio in mind!
	if (chunky > chunkx + 1) {
		chunky = chunkx + 1;
	} else if (chunkx > chunky * 2) {
		chunkx = chunky * 2;
	}

	int wastey = ((maxy - 2) - (CHUNKS_VERT * chunky)) / 2;
	int wastex = ((maxx - 2) - (CHUNKS_HORZ * chunkx)) / 2;
	Plane *n = nc.get_stdplane ();
	n->erase ();

	int averr = 0;
	char *path = find_data("lamepatents.jpg");
	std::unique_ptr<Visual> ncv (n->visual_open (path, &averr));
	delete[] path;

	if (!ncv) {
		return false;
	}

	if (ncv->decode (&averr) == nullptr) {
		return false;
	}

	if (!ncv->render (0, 0, 0, 0)) {
		return false;
	}

	constexpr int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
	auto chunks = new std::shared_ptr<Plane>[chunkcount];

	// draw the 72 boxes in a nice color pattern, in order
	int cy, cx;
	for (cy = 0 ; cy < CHUNKS_VERT ; ++cy) {
		for (cx = 0 ; cx < CHUNKS_HORZ ; ++cx) {
			const int idx = cy * CHUNKS_HORZ + cx;
			chunks[idx] = std::make_shared<Plane> (chunky, chunkx, cy * chunky + wastey + 1, cx * chunkx + wastex + 1);
			fill_chunk (chunks[idx], idx);
		}
	}

	// draw a box around the playing area
	if (!draw_bounding_box (n, wastey, wastex, chunky, chunkx)) {
		return false;
	}

	int i;
	struct timespec ts = { /*.tv_sec =*/ 0, /*.tv_nsec =*/ 1000000000, };
	if (!demo_render(nc)) {
		goto done;
	}

	// fade out each of the chunks in succession
	/*for(cy = 0 ; cy < CHUNKS_VERT ; ++cy){
	  for(cx = 0 ; cx < CHUNKS_HORZ ; ++cx){
      const int idx = cy * CHUNKS_HORZ + cx;
      if(ncplane_fadeout(chunks[idx], &ts)){
	  goto done;
      }
	  }
	  }*/
	// shuffle up the chunks

	clock_nanosleep (CLOCK_MONOTONIC, 0, &ts, nullptr);
	for (i = 0 ; i < 200 ; ++i) {
		int i0 = random () % chunkcount;
		int i1 = random () % chunkcount;
		while (i1 == i0) {
			i1 = random () % chunkcount;
		}

		int targy0, targx0;
		int targy, targx;

		chunks[i0]->get_yx (&targy0, &targx0);
		chunks[i1]->get_yx (&targy, &targx);

		std::shared_ptr<Plane> t = chunks[i0];
		t->move (targy, targx);
		chunks[i0] = chunks[i1];
		chunks[i0]->move (targy0, targx0);
		chunks[i1] = t;
		if (!demo_render(nc)) {
			goto done;
		}
	}

	if (!play (nc, chunks)) {
		goto done;
	}
	ret = true;

  done:
	delete[] chunks;
	return ret;
}
