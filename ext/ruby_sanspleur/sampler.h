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
	VALUE sanspleur_start_sample(VALUE self, VALUE usleep_value, VALUE file_name, VALUE info);
	VALUE sanspleur_stop_sample(VALUE self);
	VALUE sanspleur_sample(VALUE self, VALUE usleep_value, VALUE file_name, VALUE info);
	VALUE sanspleur_skip_writting_to_debug(VALUE self, VALUE skip);
	
	char *sanspleur_copy_string(const char *string);
#ifdef __cplusplus 
}
#endif
