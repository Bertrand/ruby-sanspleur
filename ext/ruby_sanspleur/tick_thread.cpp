/*
 *  tick_thread.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "tick_thread.h"
#include "sampler.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

static void *thread_function(void *instance)
{
	void *result;
	
	result = ((TickThread *)instance)->_thread_action();
	pthread_exit(result);
	return result;
}

TickThread::TickThread(int usleep_value)
{
	_thread_time = sanspleur_get_current_time();
	_anchor_time = _thread_time;
	_usleep_value = usleep_value;
	_thread_running = 0;
	_tick_count = 0;
}

TickThread::~TickThread()
{
}

double TickThread::anchor_difference()
{
	return _thread_time - _anchor_time;
}

void TickThread::update_anchor()
{
	_anchor_time = _thread_time;
	_tick_count = 0;
}

double TickThread::anchor_value()
{
	return _anchor_time;
}

int TickThread::anchor_tick_value()
{
	return _tick_count;
}

void TickThread::start()
{
	_thread_running = 1;
	pthread_create(&_thread, NULL, thread_function, this);
}

void TickThread::stop()
{
	_thread_running = 0;
}

void *TickThread::_thread_action()
{
	double last_time;
	
	last_time = sanspleur_get_current_time();
	while (_thread_running) {
		double current_time;
		
		usleep(_usleep_value);
		_thread_time = sanspleur_get_current_time();
		last_time = current_time;
		_tick_count++;
	}
	free(this);
	return NULL;
}
