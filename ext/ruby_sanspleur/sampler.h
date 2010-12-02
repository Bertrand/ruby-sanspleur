/*
 *  sampler.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 15/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#define COPY_RUBY_STRING 0

//#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTF(...)

#ifdef __cplusplus 
extern "C" {
#endif

#include <ruby.h>
#include <env.h>
#include <node.h>

	VALUE sanspleur_set_current_thread_to_sample(VALUE self);
	VALUE sanspleur_start_sample(VALUE self, VALUE url, VALUE usleep_value, VALUE file_name, VALUE extra_info);
	VALUE sanspleur_stop_sample(VALUE self, VALUE extra_info);
	VALUE sanspleur_sample(VALUE self, VALUE url, VALUE usleep_value, VALUE file_name, VALUE beginning_extra_info, VALUE end_extra_info);
	VALUE sanspleur_skip_writting_to_debug(VALUE self, VALUE skip);
	VALUE sanspleur_save_current_sample(VALUE self, VALUE filename, VALUE in_thread);
	VALUE sanspleur_cancel_current_sample(VALUE self);
	
	char *sanspleur_copy_string(const char *string);
	double sanspleur_get_current_time();
	
#ifdef __cplusplus 
}
#endif
