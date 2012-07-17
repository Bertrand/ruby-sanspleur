/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
 
#ifndef RUBY_BACKTRACE_WALKER_H
#define RUBY_BACKTRACE_WALKER_H


#ifdef __cplusplus 
extern "C" {
#endif

#include <ruby.h>

typedef void sanspleur_backtrace_iter_func(void *, const char* /* filename */, int /* line number */, const char* /* function name */, ID /* function id */, const char* /* class name */, ID /* class id */);

extern void ruby_backtrace_each(sanspleur_backtrace_iter_func* iterator, void* arg);


#ifdef __cplusplus 
}
#endif

#endif /* RUBY_BACKTRACE_WALKER_H */