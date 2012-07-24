/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "generic_ticker.h"
#include <stdint.h>



class ClockTicker : public GenericTicker {
	protected:

		uint64_t _start_time; 
		uint64_t _anchor_time; 
		uint64_t _tick_interval; 

		~ClockTicker();
		
	public:
		ClockTicker(int microseconds_interval);
		
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