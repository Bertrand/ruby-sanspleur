/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
 
#include "ruby_sanspleur.h"
#include <stdio.h>
#include <assert.h>
#include "sampler.h"

static VALUE sanspleurModule;

void Init_ruby_sanspleur()
{
    sanspleurModule = rb_define_module("RubySanspleur");
    rb_define_const(sanspleurModule, "VERSION", rb_str_new2(RUBY_SANSPLEUR_VERSION));
	
    rb_define_module_function(sanspleurModule, "set_current_thread_to_sample", sanspleur_set_current_thread_to_sample, 0);
    rb_define_module_function(sanspleurModule, "start_sample", sanspleur_start_sample, 4);
    rb_define_module_function(sanspleurModule, "stop_sample", sanspleur_stop_sample, 1);
    rb_define_module_function(sanspleurModule, "sample", sanspleur_sample, 5);
    rb_define_module_function(sanspleurModule, "save_current_sample", sanspleur_save_current_sample, 2);
    rb_define_module_function(sanspleurModule, "cancel_current_sample", sanspleur_cancel_current_sample, 0);
	
	sanspleur_init();
}



char *sanspleur_copy_string(const char *string)
{
    long length;
    char *result = NULL;
    
    if (string) {
        length = strlen(string);
        result = (char *)malloc(length + 1);
        strncpy(result, string, length);
        result[length] = 0;
    }
    return result;
}