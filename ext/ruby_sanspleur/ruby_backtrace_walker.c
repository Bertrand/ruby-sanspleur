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

#else /* ruby 1.8*/

#include <ruby/env.h>
#include <st.h>
#include <node.h>
typedef rb_event_t rb_event_flag_t;

#endif /* RUBY_VM */


#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())



#ifdef RUBY_VM /* ruby 1.9 and above */

struct rb_iseq_struct;

struct rb_iseq_struct {

    enum iseq_type {
    ISEQ_TYPE_TOP,
    ISEQ_TYPE_METHOD,
    ISEQ_TYPE_BLOCK,
    ISEQ_TYPE_CLASS,
    ISEQ_TYPE_RESCUE,
    ISEQ_TYPE_ENSURE,
    ISEQ_TYPE_EVAL,
    ISEQ_TYPE_MAIN,
    ISEQ_TYPE_DEFINED_GUARD
    } type;              

    VALUE name;
    VALUE filename;
    VALUE filepath;
    VALUE *iseq;
    VALUE *iseq_encoded;
    unsigned long iseq_size;
    VALUE mark_ary; 
    VALUE coverage;     
    unsigned short line_no;

    void *insn_info_table;
    size_t insn_info_size;

    ID *local_table;
    int local_table_size;

    int local_size;

    void *ic_entries;
    int ic_size;

    int argc;
    int arg_simple;
    int arg_rest;
    int arg_block;
    int arg_opts;
    int arg_post_len;
    int arg_post_start;
    int arg_size;
    VALUE *arg_opt_table;

    size_t stack_max; 

    void *catch_table;
    int catch_table_size;

    struct rb_iseq_struct *parent_iseq;
    struct rb_iseq_struct *local_iseq;

    VALUE self;
    VALUE orig;        

    void *cref_stack;
    VALUE klass;

    ID defined_method_id; // this is the last one we need. Don't go beyond this, we only access frames as pointers.
};

typedef struct rb_iseq_struct rb_iseq_t;
typedef enum {
    NOEX_PUBLIC    = 0x00,
    NOEX_NOSUPER   = 0x01,
    NOEX_PRIVATE   = 0x02,
    NOEX_PROTECTED = 0x04,
    NOEX_MASK      = 0x06,
    NOEX_BASIC     = 0x08,
    NOEX_UNDEF     = NOEX_NOSUPER,
    NOEX_MODFUNC   = 0x12,
    NOEX_SUPER     = 0x20,
    NOEX_VCALL     = 0x40,
    NOEX_RESPONDS  = 0x80
} rb_method_flag_t;

typedef enum {
    VM_METHOD_TYPE_ISEQ,
    VM_METHOD_TYPE_CFUNC,
    VM_METHOD_TYPE_ATTRSET,
    VM_METHOD_TYPE_IVAR,
    VM_METHOD_TYPE_BMETHOD,
    VM_METHOD_TYPE_ZSUPER,
    VM_METHOD_TYPE_UNDEF,
    VM_METHOD_TYPE_NOTIMPLEMENTED,
    VM_METHOD_TYPE_OPTIMIZED, /* Kernel#send, Proc#call, etc */
    VM_METHOD_TYPE_MISSING   /* wrapper for method_missing(id) */
} rb_method_type_t;

typedef struct rb_method_definition_struct {
    rb_method_type_t type; /* method type */
    ID original_id;
} rb_method_definition_t;

typedef struct rb_method_entry_struct {
    rb_method_flag_t flag;
    char mark;
    rb_method_definition_t *def;
    ID called_id;
    VALUE klass;                    /* should be mark */
} rb_method_entry_t;

typedef struct {
    VALUE *pc;          /* cfp[0] */
    VALUE *sp;          /* cfp[1] */
    VALUE *bp;          /* cfp[2] */
    rb_iseq_t *iseq;        /* cfp[3] */
    VALUE flag;         /* cfp[4] */
    VALUE self;         /* cfp[5] / block[0] */
    VALUE *lfp;         /* cfp[6] / block[1] */
    VALUE *dfp;         /* cfp[7] / block[2] */
    rb_iseq_t *block_iseq;  /* cfp[8] / block[3] */
    VALUE proc;         /* cfp[9] / block[4] */
    const rb_method_entry_t *me;/* cfp[10] */
} rb_control_frame_t;


typedef struct rb_thread_struct {
    VALUE self;
    void /*rb_vm_t*/ *vm;

    VALUE *stack;       
    unsigned long stack_size;
    rb_control_frame_t *cfp;
} rb_thread_t;


int rb_vm_get_sourceline(const rb_control_frame_t *cfp);



#define RUBY_VM_IFUNC_P(ptr)        (BUILTIN_TYPE(ptr) == T_NODE)
#define VM_FRAME_MAGIC_MASK_BITS   8
#define VM_FRAME_MAGIC_MASK   (~(~0<<VM_FRAME_MAGIC_MASK_BITS))
#define VM_FRAME_TYPE(cfp) ((cfp)->flag & VM_FRAME_MAGIC_MASK)


void ruby_backtrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{

    rb_thread_t* ruby_current_thread = (rb_thread_t*)(DATA_PTR(rb_thread_current()));
    rb_control_frame_t *cfp = ruby_current_thread->cfp;

    const rb_control_frame_t *cfp_limit = (rb_control_frame_t*)(ruby_current_thread->stack + ruby_current_thread->stack_size);
    cfp_limit -= 2;
   
    while (cfp < cfp_limit) {
        const char* class_name = NULL; 
        ID function_id = 0; 
        VALUE klass = NULL;
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
                klass = NULL;
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
            file_name = RSTRING_PTR(iseq->filename);
        }

        const char* function_name = NULL;
        if (function_id) function_name = rb_id2name(function_id); 


        if (cfp->self) {
            if (TYPE(cfp->self) == RUBY_T_CLASS || TYPE(cfp->self) == RUBY_T_MODULE) {
                VALUE str = rb_obj_as_string(cfp->self);
                class_name = RSTRING_PTR(str);
            }        
        }
        if (klass && !class_name) {
            class_name = rb_class2name(klass); 
        }

        if (no_pos < 0) {
            line_number = rb_vm_get_sourceline(cfp);
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