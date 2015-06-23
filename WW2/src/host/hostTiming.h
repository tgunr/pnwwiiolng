#pragma once
#if defined(ENABLE_HOST_TIMERS)

#if defined(WIN32)
# include <intrin.h>
# define rdtsc(x) x = __rdtsc()
#else
static inline void
rdtsc(UINT64& x)
{
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
}
#endif

#include <vector>
namespace Timers
{
  extern std::vector< std::pair<const char*, UINT64> > timings;
  extern TeulTimeMS nextReport;
}

#define TIMER_USE_HERE UINT64 timer_start, timer_end;
#define TIMER_RESET { Timers::timings.clear(); if ( teulClock64 >= Timers::nextReport ) Timers::nextReport = 0ULL; }
#define TIMER_START rdtsc(timer_start);
#define TIMER_STOP(_label)  rdtsc(timer_end); if ( Timers::nextReport == 0ULL ) Timers::timings.push_back(std::make_pair(_label, timer_end - timer_start));
#define TIMER_ACCUMULATOR_NAME(_label) timer_acc_##_label
#define TIMER_ACCUMULATOR(_label) const size_t TIMER_ACCUMULATOR_NAME(_label) = Timers::timings.size(); Timers::timings.push_back(std::make_pair(#_label, 0ULL));
#define TIMER_STOP_ADD(_label)  rdtsc(timer_end); Timers::timings[TIMER_ACCUMULATOR_NAME(_label)].second += (timer_end - timer_start);
#define TIMER_REPORT(_interval) \
	if ( Timers::nextReport == 0ULL ) \
	{ \
		dbugLog(_FLI, "Timers:"); \
		for ( size_t i = 0; i < Timers::timings.size(); ++i ) \
		{ \
			dbugLog(_FLIB, " %s:%llu", Timers::timings[i].first, Timers::timings[i].second); \
		} \
		Timers::nextReport = teulClock64 + _interval; \
		TIMER_RESET \
	}
#else
# define TIMER_USE_HERE
# define TIMER_RESET
# define TIMER_START
# define TIMER_STOP(_label)
# define TIMER_REPORT(_interval)
# define TIMER_ACCUMULATOR(_label)
# define TIMER_STOP_ADD(_label)
#endif
