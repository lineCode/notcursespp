#include <config.hh>

#include <array>
#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <iostream>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h> // ffmpeg doesn't reliably "C"-guard itself
}

#include <ncpp/NotCurses.hh>
#include <ncpp/Visual.hh>

using namespace ncpp;

static void usage (std::ostream& os, const char* name, int exitcode)
	__attribute__ ((noreturn));

void usage (std::ostream& o, const char* name, int exitcode)
{
	o << "usage: " << name << " [ -h ] [ -l loglevel(0-9) ] files" << std::endl;
	exit (exitcode);
}

constexpr auto NANOSECS_IN_SEC = 1000000000ll;

static inline uint64_t
timespec_to_ns (const struct timespec* ts)
{
  return ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
}

// FIXME: make the callback use notcurses++ classes
// frame count is in the curry. original time is in the ncplane's userptr.
int perframe ([[maybe_unused]] struct notcurses* _nc, struct ncvisual* ncv, void* vframecount)
{
	NotCurses &nc = NotCurses::get_instance ();

	const struct timespec* start = static_cast<struct timespec*>(ncplane_userptr (ncvisual_plane (ncv)));
	Plane* stdn = nc.get_stdplane ();
	int* framecount = static_cast<int*>(vframecount);
	++*framecount;
	stdn->set_fg (0x80c080);
	stdn->cursor_move (0, 0);

	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	int64_t ns = timespec_to_ns (&now) - timespec_to_ns (start);
	stdn->printf ("Got frame %05d\u2026", *framecount);
	const int64_t h = ns / (60 * 60 * NANOSECS_IN_SEC);
	ns -= h * (60 * 60 * NANOSECS_IN_SEC);
	const int64_t m = ns / (60 * NANOSECS_IN_SEC);
	ns -= m * (60 * NANOSECS_IN_SEC);
	const int64_t s = ns / NANOSECS_IN_SEC;
	ns -= s * NANOSECS_IN_SEC;
	stdn->printf (0, NCAlign::Right, "%02ld:%02ld:%02ld.%04ld", h, m, s, ns / 1000000);
	if (!nc.render ()) {
		return -1;
	}

	int dimx, dimy, oldx, oldy, keepy, keepx;
	nc.get_term_dim (&dimy, &dimx);
	ncplane_dim_yx (ncvisual_plane (ncv), &oldy, &oldx);
	keepy = oldy > dimy ? dimy : oldy;
	keepx = oldx > dimx ? dimx : oldx;
	return ncplane_resize (ncvisual_plane (ncv), 0, 0, keepy, keepx, 0, 0, dimy, dimx);
}

// can exit() directly. returns index in argv of first non-option param.
int handle_opts (int argc, char **argv, notcurses_options &opts)
{
	int c;
	while ((c = getopt (argc, argv, "hl:")) != -1) {
		switch (c) {
			case 'h':
				usage (std::cout, argv[0], EXIT_SUCCESS);
				break;

			case 'l': {
				std::stringstream ss;
				ss << optarg;
				int ll;
				ss >> ll;
				if (ll < NCLogLevel::Silent || ll > NCLogLevel::Trace) {
					std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
					usage (std::cerr, argv[0], EXIT_FAILURE);
				}
				if (ll == 0 && strcmp(optarg, "0")) {
					std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
					usage (std::cerr, argv[0], EXIT_FAILURE);
				}
				opts.loglevel = static_cast<ncloglevel_e>(ll);
				break;
				}

			default:
				usage (std::cerr, argv[0], EXIT_FAILURE);
				break;
		}
	}
	// we require at least one free parameter
	if(argv[optind] == nullptr){
		usage(std::cerr, argv[0], EXIT_FAILURE);
	}
	return optind;
}

int main (int argc, char** argv)
{
	setlocale (LC_ALL, "");
	NotCurses &nc = NotCurses::get_instance ();
	auto nonopt = handle_opts (argc, argv, NotCurses::default_notcurses_options);
	nc.init ();
	if (!nc) {
		return EXIT_FAILURE;
	}
	int dimy, dimx;
	nc.get_term_dim (&dimy, &dimx);

	int frames;
	Plane ncp (dimy - 1, dimx, 1, 0, &frames);
	if (ncp == nullptr) {
		return EXIT_FAILURE;
	}

	for (int i = nonopt ; i < argc ; ++i) {
		std::array<char, 128> errbuf;
		int averr;
		frames = 0;
		Visual *ncv = ncp.visual_open (argv[i], &averr);
		if (ncv == nullptr) {
			av_make_error_string (errbuf.data (), errbuf.size (), averr);
			std::cerr << "Error opening " << argv[i] << ": " << errbuf.data () << std::endl;
			return EXIT_FAILURE;
		}

		if (ncv->stream (&averr, perframe, &frames)) {
			av_make_error_string (errbuf.data (), errbuf.size (), averr);
			std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data () << std::endl;
			return EXIT_FAILURE;
		}
		char32_t ie = nc.getc (true);
		delete ncv;
		if (ie == static_cast<char32_t>(-1)) {
			break;
		}

		if (ie == NCKEY_RESIZE){
			--i; // rerun with the new size
			if (!nc.resize (&dimy, &dimx)) {
				return EXIT_FAILURE;
			}

			if (!ncp.resize(dimy, dimx)) {
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;
}
