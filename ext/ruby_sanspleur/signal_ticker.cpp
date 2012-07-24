/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "signal_ticker.h"
#include "sampler.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <signal.h> 
#include <stdio.h> 
#include <string.h> 

#include "ruby_backtrace_walker.h"

#define SIGNAL SIGALRM
#define ITIMER ITIMER_REAL
 


double global_thread_time = 0;
int global_tick_count = 0;
int global_microseconds_interval = 0;

static void add_line_to_trace(void* anonymous_trace, const char* file_name, int line_number, const char* function_name, ID function_id, const char* class_name, ID class_id)
{
    //fprintf(stderr, "New backtrace line : %s:%d %s::%s\n", safe_string(file_name), line_number, safe_string(class_name), safe_string(function_name));
}

static void timer_handler(int signal)
{
	global_thread_time += global_microseconds_interval / 1000000.0;
	global_tick_count++;

    //ruby_backtrace_each(add_line_to_trace, (void*)NULL); 
    //fprintf(stderr, "--------\n");
}

SignalTicker::SignalTicker(int microseconds_interval)
{
	_thread_time = sanspleur_get_current_time();
	_anchor_time = _thread_time;
	_microseconds_interval = microseconds_interval;
	_thread_running = 0;
	_tick_count = 0;
}

SignalTicker::~SignalTicker()
{
}

double SignalTicker::time_since_anchor()
{
	return global_thread_time - _anchor_time;
}

void SignalTicker::sync_anchor()
{
	_anchor_time = global_thread_time;
	_tick_count = global_tick_count;
}

long long SignalTicker::ticks_since_anchor()
{
	return global_tick_count - _tick_count;
}

long long SignalTicker::total_tick_count()
{
	return global_tick_count;
}

void SignalTicker::start()
{
	struct sigaction sa;
	struct itimerval timer;
	
	_thread_running = 1;
	
	global_thread_time = 0;
	global_microseconds_interval = _microseconds_interval;
	_anchor_time = global_thread_time;

	memset (&sa, 0, sizeof (sa));
    sa.sa_flags = SA_RESTART;
	sa.sa_handler = &timer_handler;
	sigaction (SIGNAL, &sa, NULL);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = _microseconds_interval;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = _microseconds_interval;
	setitimer(ITIMER, &timer, NULL);
}

void SignalTicker::stop()
{
	struct sigaction sa;
	struct itimerval timer;
	
	_thread_running = 0;
	
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER, &timer, NULL);

	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = NULL;
	sigaction (SIGNAL, &sa, NULL);
}

void SignalTicker::pause()
{

}

void SignalTicker::resume()
{

}

void SignalTicker::reset()
{

}