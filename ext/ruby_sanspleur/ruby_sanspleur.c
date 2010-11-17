
#include "ruby_sanspleur.h"
#include <stdio.h>
#include <assert.h>
#include "sampler.h"

static VALUE sanspleurModule;

#if RUBY_VERSION == 191 // accomodate for this: http://redmine.ruby-lang.org/issues/show/3748
# if defined(_WIN32)
  __declspec(dllexport)
# endif
#endif

void Init_ruby_sanspleur()
{
    sanspleurModule = rb_define_module("RubySanspleur");
    rb_define_const(sanspleurModule, "VERSION", rb_str_new2(RUBY_SANSPLEUR_VERSION));
	
    rb_define_module_function(sanspleurModule, "set_current_thread_to_sample", sanspleur_set_current_thread_to_sample, 0);
    rb_define_module_function(sanspleurModule, "start_sample", sanspleur_start_sample, 3);
    rb_define_module_function(sanspleurModule, "stop_sample", sanspleur_stop_sample, 0);
    rb_define_module_function(sanspleurModule, "sample", sanspleur_sample, 3);
    rb_define_module_function(sanspleurModule, "skip_writting_to_debug", sanspleur_skip_writting_to_debug, 1);
}
