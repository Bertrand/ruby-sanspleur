/*
 *  thread.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 13/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

int sanspleur_start_thread(unsigned long long usleep_value);
int sanspleur_stop_thread(void);
int sanspleur_did_thread_tick(void);
