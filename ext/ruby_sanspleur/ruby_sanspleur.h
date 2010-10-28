
#ifndef RUBY_SANSPLEUR_H
#define RUBY_SANSPLEUR_H

#include <stdio.h>

#include <ruby.h>

#ifndef RUBY_VM
#include <node.h>
#include <st.h>
typedef rb_event_t rb_event_flag_t;
#define rb_sourcefile() (node ? node->nd_file : 0)
#define rb_sourceline() (node ? nd_line(node) : 0)
#endif

#include "version.h"

#endif
