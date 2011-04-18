/*
 *  thread_ticker.c
 *  ruby-sanspleur
 *
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "thread_ticker.h"
#include "sampler.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

enum {
	_ThreadWait,
	_ThreadRun,
	_ThreadStop,
};

static void *thread_function(void *instance)
{
	void *result = NULL;
	
	// 
	// This might require setting CAP_SYS_NICE on process.
	// see http://linux.die.net/man/7/capabilities
	// -- ber 
	// pthread_setschedparam(pthread_self(), SCHED_FIFO, NULL);
	
	result = ((ThreadTicker *)instance)->_thread_action();
	pthread_exit(result);
	return result;
}

ThreadTicker::ThreadTicker(int usleep_value)
{
	_elapsed_time = 0;
	_anchored_time = 0;
	_usleep_value = usleep_value;
	_anchored_tick = 0;
	_current_tick = 0;
	
	// Create condition lock
	if (pthread_mutex_init(&_mutex, NULL) != 0) {
		fprintf(stderr, "Thread ticker warning: unable to init mutex");
	}

	if (pthread_cond_init(&_cond, NULL) != 0) {
		fprintf(stderr, "Thread ticker warning: unable to init condition");
	}
	
	_state = _ThreadWait;
}

ThreadTicker::~ThreadTicker()
{

}

double ThreadTicker::time_since_anchor()
{
	return _elapsed_time - _anchored_time;
}

void ThreadTicker::sync_anchor()
{
	_anchored_time = _elapsed_time;
	_anchored_tick = _current_tick;
}

long long ThreadTicker::ticks_since_anchor()
{
	return _current_tick - _anchored_tick;
}

long long ThreadTicker::total_tick_count()
{
	return _current_tick;
}

void ThreadTicker::start()
{	
	_state = _ThreadRun;
	pthread_create(&_thread, NULL, thread_function, this);
}

void ThreadTicker::_changeStateTo(unsigned int state, bool broadcast)
{
	
	pthread_mutex_lock(&_mutex);
	
	_state = state;
	pthread_mutex_unlock(&_mutex);
	if (broadcast) {
		// wake up waiting thread
		pthread_cond_broadcast(&_cond);
	}
}

void ThreadTicker::stop()
{
	this->_changeStateTo(_ThreadStop, /* broadcast = */ true);
}

void ThreadTicker::pause()
{
	this->_changeStateTo(_ThreadWait, /* broadcast = */ false);
}

void ThreadTicker::resume()
{

	this->_changeStateTo(_ThreadRun, /* broadcast = */ true);	
}

void ThreadTicker::reset()
{
	_anchored_time = 0;
	_elapsed_time = 0;
	_anchored_tick = 0;
	_current_tick = 0;
}

void *ThreadTicker::_thread_action()
{			
	bool shouldStop = false;

	pthread_mutex_lock(&_mutex);

	while (!shouldStop) {
		switch (_state) {
			case _ThreadWait:
				// free lock and wait for condition variable to change
				pthread_cond_wait(&_cond, &_mutex);
				break;
			case _ThreadRun:
				// simply run the tick count (it will also look at _state to see if it must return).
				pthread_mutex_unlock(&_mutex);
				this->runTicks();
				pthread_mutex_lock(&_mutex);
				break;
			case _ThreadStop:
				// we're told to leave. ok. 
				pthread_mutex_unlock(&_mutex);
				shouldStop = true;
				break;
			default:
				fprintf(stderr, "Unknow Thread state. Things are certainly going to get worse now.\n");

		}
	}
	fprintf(stderr, "Leaving ticker thread\n");
	return NULL;
}

void ThreadTicker::runTicks()
{
	bool shouldLoop = true;

	struct timespec waitRequest;
	struct timespec remainingTime;
	
	waitRequest.tv_sec = 0;
	waitRequest.tv_nsec = _usleep_value * 1000;
	
	do {
		remainingTime.tv_sec = 0;
		remainingTime.tv_nsec = 0;
		
		int waitResult = nanosleep(&waitRequest, &remainingTime);
		long timeSlept = waitRequest.tv_nsec / 1000;
		if ((waitResult == -1) && (errno == EINTR)) {
			timeSlept -= remainingTime.tv_nsec / 1000;
		}
		
		pthread_mutex_lock(&_mutex);
		_elapsed_time += timeSlept; 
		_current_tick++;
		shouldLoop = (_state == _ThreadRun);
		pthread_mutex_unlock(&_mutex);
		
	} while (shouldLoop);
}
