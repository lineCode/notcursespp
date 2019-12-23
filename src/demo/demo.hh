#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <assert.h>
#include <limits.h>
#include <ncpp/NotCurses.hh>

#ifdef __cplusplus
extern "C" {
#endif

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;

// heap-allocated, caller must free. locates data files per command line args.
char* find_data(const char* datum);

bool unicodeblocks_demo (ncpp::NotCurses &nc);
bool witherworm_demo (ncpp::NotCurses &nc);
bool box_demo (ncpp::NotCurses &nc);
bool maxcolor_demo (ncpp::NotCurses &nc);
bool grid_demo (ncpp::NotCurses &nc);
bool sliding_puzzle_demo (ncpp::NotCurses &nc);
bool view_demo (ncpp::NotCurses &nc);
bool eagle_demo (ncpp::NotCurses &nc);
bool panelreel_demo (ncpp::NotCurses &nc);
bool xray_demo (ncpp::NotCurses &nc);
bool luigi_demo (ncpp::NotCurses &nc);
bool outro (ncpp::NotCurses &nc);

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

#ifdef __cplusplus
}
#endif

#endif
