/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
 
#ifndef RUBY_SANSPLEUR_H
#define RUBY_SANSPLEUR_H

#include <stdio.h>

#include <ruby.h>

#ifdef RUBY_VM /* ruby 1.9 and above */

#else /* ruby 1.8 */

#include <node.h>
typedef rb_event_t rb_event_flag_t;

#endif

#include "version.h"

#endif
