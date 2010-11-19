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
		long long _current_anchor;
		long long _thread_tick;
		int _usleep_value;
		int _thread_running;
		pthread_t _thread;
		int user_count;

		~TickThread();
		
	public:
		TickThread(int usleep_value);
		
		int did_thread_tick(void);
		void update_current_anchor(void);
		
		void start(void);
		void stop(void);

		void *_thread_action(void);
};