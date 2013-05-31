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
#include <time.h>
#include <sys/time.h>
#include <signal.h> 
#include <stdio.h> 
#include <string.h> 

#include "ruby_backtrace_walker.h"

 #if HAVE_RT
 #define USE_RT 1
 #else
 #define USE_RT 0
 #endif


#if USE_RT

#define SIGNAL_NUMBER SIGRTMIN
#define CLOCK_ID CLOCK_MONOTONIC

#else

// #define TIMER ITIMER_PROF
// #define SIGNAL SIGPROF

// #define TIMER ITIMER_VIRTUAL
// #define SIGNAL SIGVTALRM

#define TIMER_NUMBER ITIMER_REAL
#define SIGNAL_NUMBER SIGALRM

#endif




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
	_thread_running = 1;
	
	global_thread_time = 0;
	global_microseconds_interval = _microseconds_interval;
	_anchor_time = global_thread_time;

    struct sigaction sa;    
	memset (&sa, 0, sizeof (sa));
    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
	sa.sa_handler = &timer_handler;
    if (sigaction (SIGNAL_NUMBER, &sa, NULL) != 0) {
        fprintf(stderr, "Failed to register timer signal handler.\n");
        exit(-1);
    }

#if USE_RT

    struct sigevent evt;
    timer_t timer_id; 

    evt.sigev_notify = SIGEV_SIGNAL;
    evt.sigev_signo = SIGNAL_NUMBER;
    if (timer_create(CLOCK_ID, &evt, &timer_id) !=0) {
        fprintf(stderr, "unable to create timer\n");
    }

    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec = 0;
    timerspec.it_interval.tv_nsec = _microseconds_interval * 1000;
    timerspec.it_value.tv_sec = 0;
    timerspec.it_value.tv_nsec = _microseconds_interval * 1000;  

    if (timer_settime(timer_id, 0, &timerspec, NULL) != 0) {
        fprintf(stderr, "unable to set time on timer\n");
    };

#else

    struct itimerval timerspec;
    timerspec.it_interval.tv_sec = 0;
    timerspec.it_interval.tv_usec = _microseconds_interval;
    timerspec.it_value.tv_sec = 0;
    timerspec.it_value.tv_usec = _microseconds_interval;
    setitimer(TIMER_NUMBER, &timerspec, NULL);

#endif

}

void SignalTicker::stop()
{
	
	_thread_running = 0;

#if USE_RT

#else

	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(TIMER_NUMBER, &timer, NULL);

#endif

	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = NULL;
	sigaction (SIGNAL_NUMBER, &sa, NULL);
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