/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
// Changelog:
//
// (Ber 04/15/2011)
// Made the thread detached, no need to join it to release its state.
// Moved state to a condition lock .
//
// (Ber 04/17/2001)
// Sanitize variable and method names 
// Use nanosleep instead of usleep for a better estimation of actual time between 2 ticks

#include <pthread.h>
#include "generic_ticker.h"

class ThreadTicker : public GenericTicker {
	protected:
		// methods
		void _changeStateTo(unsigned int state, bool broadcast);
		
		// ivars
		double				_anchored_time;
		double				_elapsed_time;
		long long			_anchored_tick;
		long long			_current_tick;

		long				_microseconds_interval;
		
		pthread_t			_thread;
		pthread_mutex_t		_mutex;
		pthread_cond_t		_cond;
		unsigned int		_state;

		~ThreadTicker();
		
		void runTicks();
		
	public:
		ThreadTicker(long microseconds_interval);
		
		double time_since_anchor();
		void sync_anchor();
		long long ticks_since_anchor();
		long long total_tick_count();
		
		void start();
		void stop();
		void pause();
		void resume();
		void reset();

		void *_thread_action();
};