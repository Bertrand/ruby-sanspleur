/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "clock_ticker.h"
#include <time.h>

#ifdef __MACH__

#include <mach/mach_time.h>

uint64_t nanosecond_clock_value()
{
    uint64_t        elapsed;
    uint64_t        elapsedNano;
    static mach_timebase_info_data_t    sTimebaseInfo;

    elapsed = mach_absolute_time();

    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }

    elapsedNano = elapsed * sTimebaseInfo.numer / sTimebaseInfo.denom;

    return elapsedNano;
}

#else /* __MACH__ */

uint64_t nanosecond_clock_value()
{
	uint64_t elapsed;
	struct timespec ts;
	
	clock_gettime(CLOCK_REALTIME, &ts);
	elapsed = 1000000000 * ts.tv_sec + ts.tv_nsec;

	return elapsed;
}

#endif /* __MACH__ */

ClockTicker::ClockTicker(int usleep_value)
{
	_tick_interval = usleep_value * 1000;
}

ClockTicker::~ClockTicker()
{
}

void ClockTicker::sync_anchor()
{
	_anchor_time = nanosecond_clock_value();
}

long long ClockTicker::ticks_since_anchor()
{
	uint64_t elapsed = nanosecond_clock_value() - _anchor_time;
	return elapsed / _tick_interval;
}

long long ClockTicker::total_tick_count()
{
	uint64_t elapsed = nanosecond_clock_value() - _start_time;
	return elapsed / _tick_interval;
}

double ClockTicker::time_since_anchor()
{
	uint64_t elapsed = nanosecond_clock_value() - _anchor_time;
	return (double)elapsed / 1000000000.0;
}

void ClockTicker::start()
{
	_start_time = nanosecond_clock_value();
	_anchor_time = _start_time;
}

void ClockTicker::stop()
{
}

void ClockTicker::pause()
{

}

void ClockTicker::resume()
{
	start();
}

void ClockTicker::reset()
{
	start();
}