/*
 *  tick_thread.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include <pthread.h>

class TickThread {
	protected:
		double _anchor_time;
		double _thread_time;
		int _usleep_value;
		int _thread_running;
		pthread_t _thread;
		int _tick_count;

		~TickThread();
		
	public:
		TickThread(int usleep_value);
		
		double anchor_difference();
		void update_anchor();
		double anchor_value();
		int anchor_tick_value();
		
		void start();
		void stop();

		void *_thread_action();
};