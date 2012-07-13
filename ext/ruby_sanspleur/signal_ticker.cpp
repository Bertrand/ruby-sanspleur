/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "signal_ticker.h"
#include "sampler.h"
#include "debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <signal.h> 
#include <stdio.h> 
#include <string.h> 


#define SIGNAL SIGALRM
#define ITIMER ITIMER_REAL
 

double global_thread_time = 0;
int global_tick_count = 0;
int global_usleep_value = 0;

static void timer_handler(int signal)
{
	global_thread_time += global_usleep_value / 1000000.0;
	global_tick_count++;
}

SignalTicker::SignalTicker(int usleep_value)
{
	_thread_time = sanspleur_get_current_time();
	_anchor_time = _thread_time;
	_usleep_value = usleep_value;
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
	global_usleep_value = _usleep_value;
	_anchor_time = global_thread_time;

	memset (&sa, 0, sizeof (sa));
    sa.sa_flags = SA_RESTART;
	sa.sa_handler = &timer_handler;
	sigaction (SIGNAL, &sa, NULL);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = _usleep_value;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = _usleep_value;
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