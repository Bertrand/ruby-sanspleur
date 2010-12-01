/*
 *  signal_ticker.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
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
		
		double anchor_difference();
		void update_anchor();
		double anchor_value();
		int anchor_tick_value();
		
		void start();
		void stop();
};