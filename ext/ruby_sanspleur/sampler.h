/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */


//#define DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG_PRINTF(...)

#define safe_string(__s__) (__s__ ? __s__ : "")

#if HAVE_TIMER_SETTIME
 #define HAVE_RT 1
 #else
 #define HAVE_RT 0
 #endif


#ifdef __cplusplus 
extern "C" {
#endif

#include <ruby.h>

#ifndef RUBY_VM  /* ruby 1.8 */
#include <env.h>
#include <node.h>
#include <st.h> 
#else /* ruby 1.9 and above */
#include <ruby/st.h> 
#endif

VALUE sanspleur_set_current_thread_to_sample(VALUE self);
VALUE sanspleur_start_sample(VALUE self, VALUE url, VALUE milliseconds_interval, VALUE file_name, VALUE extra_info);
VALUE sanspleur_stop_sample(VALUE self, VALUE extra_info);
VALUE sanspleur_sample(VALUE self, VALUE url, VALUE microseconds_interval, VALUE file_name, VALUE beginning_extra_info, VALUE end_extra_info);
VALUE sanspleur_save_current_sample(VALUE self, VALUE filename, VALUE in_thread);
VALUE sanspleur_cancel_current_sample(VALUE self);

double sanspleur_get_current_time();
void sanspleur_init();

	
#ifdef __cplusplus 
}
#endif
