/*
 *  signal_ticker.h
 *  ruby-sanspleur
 *
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "generic_ticker.h"

class SignalTicker : public GenericTicker {
	protected:
		double _anchor_time;
		double _thread_time;
		int _usleep_value;
		int _thread_running;
		int _tick_count;

		~SignalTicker();
		
	public:
		SignalTicker(int usleep_value);
		
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