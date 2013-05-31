/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
 
#include "ruby_backtrace_walker.h"

#ifdef RUBY_VM /* ruby 1.9 and above */

#include <ruby/st.h>
#include <ruby/intern.h>
#include <ruby/version.h>

#else /* ruby 1.8*/

#include <env.h>
#include <st.h>
#include <node.h>
typedef rb_event_t rb_event_flag_t;

#endif /* RUBY_VM */


#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())



#ifdef RUBY_VM /* ruby 1.9 and above */

#if RUBY_API_VERSION_MAJOR < 2
#include "private_ruby_1_9.h"
#else 
#include "private_ruby_2_0.h"
#endif


void ruby_backtrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{

    rb_thread_t* ruby_current_thread = (rb_thread_t*)(DATA_PTR(rb_thread_current()));
    rb_control_frame_t *cfp = ruby_current_thread->cfp;

    const rb_control_frame_t *cfp_limit = (rb_control_frame_t*)(ruby_current_thread->stack + ruby_current_thread->stack_size);
    cfp_limit -= 2;
    
    while (cfp < cfp_limit) {
        const char* class_name = NULL; 
        ID function_id = 0; 
        VALUE klass = 0;
        const char* file_name = NULL; 
        int line_number = -1; 
        int no_pos = -1;

        rb_iseq_t *iseq = cfp->iseq;
        if (!iseq && cfp->me) {
            function_id = cfp->me->def->original_id;
            klass = cfp->me->klass;
            no_pos = 1;
        }

        while (iseq) {
            if (RUBY_VM_IFUNC_P(iseq)) {
                CONST_ID(function_id, "<ifunc>");
                klass = 0;
                break;
            }
            klass = iseq->klass;

            if (iseq->defined_method_id) {
                function_id = iseq->defined_method_id;
                break;
            }

            if (iseq->local_iseq == iseq) {
                break;
            }
            iseq = iseq->parent_iseq;
        }

        if (iseq != 0 && cfp->pc != 0) {
            file_name = ISEQ_FILENAME(iseq);
        }

        const char* function_name = NULL;
        if (function_id) function_name = rb_id2name(function_id); 


        if (cfp->self) {
            if (TYPE(cfp->self) == RUBY_T_CLASS || TYPE(cfp->self) == RUBY_T_ICLASS) {
                class_name = rb_class2name(cfp->self);
            } else if (TYPE(cfp->self) == RUBY_T_MODULE) {
                VALUE str = rb_obj_as_string(cfp->self);
                char* object_description = RSTRING_PTR(str); 
                class_name = object_description;
            }        
        }
        if (klass && !class_name) {
            class_name = rb_class2name(klass); 
        }

        if (no_pos < 0) {
            line_number = CFP_LINENO(cfp);
        }
        ID class_id = 0; 

        if (VM_FRAME_TYPE(cfp) != 0x51) {
            iterator(arg, file_name, line_number, function_name, function_id, class_name, class_id);
        }

        cfp++;
    }
}

#else /* ruby 1.8 */

void ruby_backtrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{
    struct FRAME *frame = ruby_frame;
    NODE *n;
 
    for (; frame && (n = frame->node); frame = frame->prev) {
        ID          function_id = 0;
        const char* function_name = NULL;
        const char* file_name = NULL;
        int         line_number = 0;
        ID          class_id = 0;
        const char* class_name = NULL;

        function_id = frame->last_func;
        if (function_id) function_name = rb_id2name(function_id); 

        file_name = n->nd_file; 
        line_number = nd_line(n);
        class_id = frame->last_class;
        if (class_id) class_name = rb_class2name(class_id); 

        iterator(arg, file_name, line_number, function_name, function_id, class_name, class_id);
    }
}


#endif /* ruby 1.8 */