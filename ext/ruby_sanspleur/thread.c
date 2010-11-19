/*
 *  thread.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "thread.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

static volatile long long sanspleur_thread_tick = 0;
static long long sanspleur_last_sample_tick = 0;
static pthread_t sanspleur_thread;
static int sanspleur_thread_running = 0;

static void *sanspleur_thread_process(void *arg)
{
	unsigned long long usleep_value;
	
	usleep_value = *(unsigned long long *)arg;
	free(arg);
	while (sanspleur_thread_running) {
		usleep(usleep_value);
		sanspleur_thread_tick++;
	}
	pthread_exit(0);
}

int sanspleur_start_thread(unsigned long long usleep_value)
{
	unsigned long long *usleep_value_ptr;
	int result = 0;
	
	if (usleep_value < 0) {
		usleep_value = 0;
	}
	if (usleep_value > 0) {
		usleep_value_ptr = malloc(sizeof(*usleep_value_ptr));
		*usleep_value_ptr = usleep_value;
		sanspleur_thread_running = 1;
		sanspleur_last_sample_tick = sanspleur_thread_tick;
		result = pthread_create(&sanspleur_thread, NULL, sanspleur_thread_process, usleep_value_ptr);
		
		if (result != 0) {
			free(usleep_value_ptr);
		}
	}
	return result;
}

int sanspleur_stop_thread(void)
{
	if (sanspleur_thread_running) {
		sanspleur_thread_running = 0;
		pthread_join(sanspleur_thread, NULL);
	}
}

int sanspleur_did_thread_tick(void)
{
	int value = 0;
	
	if (sanspleur_thread_running) {
		value = sanspleur_thread_tick - sanspleur_last_sample_tick;
	}
	return value;
}

void sanspleur_reset_thread_tick(void)
{
	sanspleur_last_sample_tick = sanspleur_thread_tick;
}
