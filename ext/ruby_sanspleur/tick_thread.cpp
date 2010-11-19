/*
 *  tick_thread.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "tick_thread.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

static volatile long long sanspleur_thread_tick = 0;
static long long sanspleur_last_sample_tick = 0;
static pthread_t sanspleur_thread;
static int sanspleur_thread_running = 0;

static void *thread_function(void *instance)
{
	return ((TickThread *)instance)->_thread_action();
}

long long current_anchor;
long long thread_tick;
int usleep_value;
int should_stop;
TickThread::TickThread(int usleep_value)
{
	_current_anchor = 0;
	_thread_tick = 0;
	_usleep_value = usleep_value;
	_thread_running = 0;
}

TickThread::~TickThread()
{
}


int TickThread::did_thread_tick(void)
{
	return _thread_tick - _current_anchor;
}

void TickThread::update_current_anchor(void)
{
	_current_anchor = _thread_tick;
}

void TickThread::start(void)
{
	_thread_running = 1;
	pthread_create(&_thread, NULL, thread_function, this);
}

void TickThread::stop(void)
{
	_thread_running = 0;
}

void *TickThread::_thread_action(void)
{
	while (_thread_running) {
		usleep(_usleep_value);
		_thread_tick++;
	}
	free(this);
	pthread_exit(0);
	return NULL;
}
