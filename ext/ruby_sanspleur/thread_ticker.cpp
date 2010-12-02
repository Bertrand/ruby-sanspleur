/*
 *  thread_ticker.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "thread_ticker.h"
#include "sampler.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

static void *thread_function(void *instance)
{
	void *result;
	
	result = ((ThreadTicker *)instance)->_thread_action();
	pthread_exit(result);
	return result;
}

ThreadTicker::ThreadTicker(int usleep_value)
{
	_thread_time = sanspleur_get_current_time();
	_anchor_time = _thread_time;
	_usleep_value = usleep_value;
	_thread_running = 0;
	_tick_count = 0;
	_total_tick_count = 0;
}

ThreadTicker::~ThreadTicker()
{
}

double ThreadTicker::anchor_difference()
{
	return _thread_time - _anchor_time;
}

void ThreadTicker::update_anchor()
{
	_anchor_time = _thread_time;
	_tick_count = _total_tick_count;
}

double ThreadTicker::anchor_value()
{
	return _anchor_time;
}

long long ThreadTicker::anchor_tick_value()
{
	return _total_tick_count - _tick_count;
}

long long ThreadTicker::total_tick_count()
{
	return _total_tick_count;
}

void ThreadTicker::start()
{
	_thread_running = 1;
	pthread_create(&_thread, NULL, thread_function, this);
}

void ThreadTicker::stop()
{
	_thread_running = 0;
}

void *ThreadTicker::_thread_action()
{
	while (_thread_running) {
		double current_time;
		
		usleep(_usleep_value);
//		_thread_time = sanspleur_get_current_time();
		_thread_time += _usleep_value;
		_total_tick_count++;
	}
	free(this);
	return NULL;
}
