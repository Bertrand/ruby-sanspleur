/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "clock_ticker.h"
#include "sampler.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <signal.h> 
#include <stdio.h> 
#include <string.h> 


    //struct itimerval timer;
    //getitimer(ITIMER, &timer);
    // fprintf(stderr, "pouet %ld %ld\n", timer.it_value.tv_sec, timer.it_value.tv_usec);

    //struct timespec tp;
    //clock_getrealtime(&tp);
    //fprintf(stderr, "gli %ld %ld\n", tp.tv_sec, tp.tv_nsec);

    //struct timeval tp;
    //gettimeofday(&tp, NULL);
    //fprintf(stderr, "gli %ld %ld\n", tp.tv_sec, tp.tv_usec);



ClockTicker::ClockTicker(int usleep_value)
{
	_tick_interval = usleep_value;
}

ClockTicker::~ClockTicker()
{
}

void ClockTicker::get_current_time(TIME_STRUCT_TYPE* time)
{
	gettimeofday(time, NULL);
}

double ClockTicker::time_interval_since(TIME_STRUCT_TYPE* time)
{
	TIME_STRUCT_TYPE now; 
	get_current_time(&now);

	return (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) * 0.000001;
}

double ClockTicker::time_since_anchor()
{
	return time_interval_since(&_anchor_time);
}

double ClockTicker::time_since_start()
{
	return time_interval_since(&_start_time);
}

double ClockTicker::tick_interval_in_seconds()
{
	return _tick_interval / 1000000.0;
}

void ClockTicker::sync_anchor()
{
	get_current_time(&_anchor_time);
}

long long ClockTicker::ticks_since_anchor()
{
	return (long long)(time_since_anchor() / tick_interval_in_seconds());
}

long long ClockTicker::total_tick_count()
{
	return (long long)(time_since_start() / tick_interval_in_seconds());
}

void ClockTicker::start()
{
	get_current_time(&_start_time);
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

}

void ClockTicker::reset()
{

}