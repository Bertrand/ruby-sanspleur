/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "generic_ticker.h"
#include <sys/time.h>

#define TIME_STRUCT_TYPE struct timeval

class ClockTicker : public GenericTicker {
	protected:

		TIME_STRUCT_TYPE _start_time; 
		TIME_STRUCT_TYPE _anchor_time; 
		long _tick_interval; 

		void get_current_time(TIME_STRUCT_TYPE* time);
		double time_interval_since(TIME_STRUCT_TYPE* time);
		double time_since_start();
		double tick_interval_in_seconds();

		~ClockTicker();
		
	public:
		ClockTicker(int usleep_value);
		
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