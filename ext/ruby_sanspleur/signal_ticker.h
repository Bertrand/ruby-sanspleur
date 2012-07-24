/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "generic_ticker.h"

class SignalTicker : public GenericTicker {
	protected:
		double _anchor_time;
		double _thread_time;
		int _microseconds_interval;
		int _thread_running;
		int _tick_count;

		~SignalTicker();
		
	public:
		SignalTicker(int microseconds_interval);
		
		double time_since_anchor();
		void sync_anchor();
		long long ticks_since_anchor();
		long long total_tick_count();
		
		void start();
		void stop();
		void pause();
		void resume();
		void reset();
};