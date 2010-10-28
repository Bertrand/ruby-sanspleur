/*
 *  sampler.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 15/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include <ruby.h>

VALUE sanspleur_set_current_thread_to_sample(VALUE self);
VALUE sanspleur_start_sample(VALUE self, VALUE usleep_value, VALUE info);
VALUE sanspleur_stop_sample(VALUE self, VALUE file_name);
VALUE sanspleur_sample(VALUE self, VALUE usleep_value, VALUE info, VALUE file_name);
VALUE sanspleur_dump_last_sample(VALUE self);
