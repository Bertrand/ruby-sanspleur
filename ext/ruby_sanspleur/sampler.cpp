/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include "sampler.h"
#include "dumper_file.h"
#include "ruby_sanspleur.h"
#include "stack_trace_sample.h"
#include "thread_ticker.h"
#include "signal_ticker.h"
#include "info_header.h"


#ifdef RUBY_VM /* ruby 1.9 and above */

#include <ruby/st.h>
#include <ruby/intern.h>

#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())

#else /* ruby 1.8*/

#include <st.h>

 extern "C" {
    struct FRAME *ruby_frame;
    rb_thread_t rb_curr_thread;
    rb_thread_t rb_main_thread;
}

#define THREAD_TYPE rb_thread_t
#define CURRENT_THREAD (rb_curr_thread)
#define MAIN_THREAD (rb_main_thread)

#endif /* RUBY_VM */



#define STRING_BUFFER_SIZE 1024


static THREAD_TYPE thread_to_sample = NULL;

static GenericTicker *ticker = NULL;
static StackTraceSample *sample = NULL;
static DumperFile *dumper = NULL;
static int skip_writting = 0;
static double start_sample_date = 0;
static st_table *file_name_table = NULL;
static st_table *function_name_table = NULL;
static st_table *class_name_table = NULL;

void sanspleur_init()
{
    file_name_table = st_init_strtable();
    function_name_table = st_init_strtable();
    class_name_table = st_init_strtable();
}

typedef void sanspleur_backtrace_iter_func(void *, const char* /* filename */, int /* line number */, const char* /* function name */, ID /* function id */, const char* /* class name */, ID /* class id */);


#ifdef RUBY_VM /* ruby 1.9 and above */

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
    void /*rb_iseq_t*/ *iseq;        /* cfp[3] */
    VALUE flag;         /* cfp[4] */
    VALUE self;         /* cfp[5] / block[0] */
    VALUE *lfp;         /* cfp[6] / block[1] */
    VALUE *dfp;         /* cfp[7] / block[2] */
    void /*rb_iseq_t*/ *block_iseq;  /* cfp[8] / block[3] */
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

//extern "C" {
     extern rb_thread_t *ruby_current_thread;
//}

static void _stacktrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{
    ID function_id = 0; 
    VALUE klass = NULL;

    //fprintf(stderr, "in current_thread : %p\n", (void*)ruby_current_threadd);
    
    fprintf(stderr, "in current_thread : %p %p\n", (void*)rb_thread_current(), ruby_current_thread);
return; 

    // rb_control_frame_t *cfp = ruby_current_threadd->cfp;
    // void /*rb_iseq_t*/ *iseq = cfp->iseq;
    // if (!iseq && cfp->me) {
    //     function_id = cfp->me->def->original_id;
    //     klass = cfp->me->klass;

    //     fprintf(stderr, "in _stacktrace_each, function_id : %p\n", (void*)function_id);
    // }


    // while (iseq) {
    // if (RUBY_VM_IFUNC_P(iseq)) {
    //     if (idp) CONST_ID(*idp, "<ifunc>");
    //     if (klassp) *klassp = 0;
    //     return;
    // }
    // if (iseq->defined_method_id) {
    //     if (idp) *idp = iseq->defined_method_id;
    //     if (klassp) *klassp = iseq->klass;
    //     return;
    // }
    // if (iseq->local_iseq == iseq) {
    //     break;
    // }
    // iseq = iseq->parent_iseq;
    // }
}

#else /* ruby 1.8 */

static void _stacktrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{
    struct FRAME *frame = ruby_frame;
    NODE *n;
    fprintf(stderr, "in _stacktrace_each\n");
    fprintf(stderr, "frame : %p, node : %p\n", frame, n);

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

#define safe_string(__s__) (__s__ ? __s__ : "")

static void add_line_to_trace(void* anonymous_trace, const char* file_name, int line_number, const char* function_name, ID function_id, const char* class_name, ID class_id)
{
    fprintf(stderr, "New backtrace line : %s:%d %s::%s\n", safe_string(file_name), line_number, safe_string(class_name), safe_string(function_name));

    struct stack_trace *trace = (struct stack_trace*)anonymous_trace;
    StackLine *new_line = new StackLine();
    
    new_line->next_stack_line = trace->stack_line;
    trace->stack_line = new_line;

    new_line->line_number = line_number;
    new_line->function_id = function_id;

    if (file_name && !st_lookup(file_name_table, (st_data_t)file_name, (st_data_t *)&new_line->file_name)) {
        file_name = sanspleur_copy_string(file_name);
        st_insert(file_name_table, (st_data_t)file_name, (st_data_t)file_name);
        new_line->file_name = file_name;
    }
    if (function_name && !st_lookup(function_name_table, (st_data_t)function_name, (st_data_t *)&new_line->function_name)) {
        function_name = sanspleur_copy_string(function_name);
        st_insert(function_name_table, (st_data_t)function_name, (st_data_t)function_name);
        new_line->function_name = function_name;
    }
    if (class_name && !st_lookup(class_name_table, (st_data_t)class_name, (st_data_t *)&new_line->class_name)) {
        class_name = sanspleur_copy_string(class_name);
        st_insert(class_name_table, (st_data_t)class_name, (st_data_t)class_name);
        new_line->class_name = function_name;
    }    
}

#ifdef RUBY_VM
static void sanspleur_sampler_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
static void sanspleur_sampler_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
    double sample_duration = 0;
    
    if (thread_to_sample == CURRENT_THREAD && sample) {
        sample->thread_called();
    }
    if (thread_to_sample == CURRENT_THREAD && ticker) {
        sample_duration = ticker->time_since_anchor();
    }
    if (sample_duration != 0) {
        struct stack_trace *new_trace;
        
        new_trace = (struct stack_trace *)calloc(1, sizeof(*new_trace));
        new_trace->sample_duration = sample_duration;
        new_trace->sample_tick_count = ticker->ticks_since_anchor();
        new_trace->ruby_event = event;
        new_trace->call_method = sanspleur_copy_string(rb_id2name(mid));

        _stacktrace_each(add_line_to_trace, (void*)new_trace); 

        if (sample) {
            sample->add_new_stack_trace(new_trace);
        }
        if (dumper) {
            dumper->write_stack_trace(new_trace);
        }
        ticker->sync_anchor();
    }
}

static void sanspleur_install_sampler_hook()
{
#ifdef RUBY_VM
    rb_add_event_hook(sanspleur_sampler_event_hook,
          RUBY_EVENT_CALL | RUBY_EVENT_RETURN |
          RUBY_EVENT_C_CALL | RUBY_EVENT_C_RETURN
            | RUBY_EVENT_LINE, Qnil); // RUBY_EVENT_SWITCH
#else
    rb_add_event_hook(sanspleur_sampler_event_hook,
          RUBY_EVENT_CALL | RUBY_EVENT_RETURN |
          RUBY_EVENT_C_CALL | RUBY_EVENT_C_RETURN
          | RUBY_EVENT_LINE);
#endif

#if defined(TOGGLE_GC_STATS)
    rb_gc_enable_stats();
#endif
}

static void sanspleur_remove_sampler_hook()
{
#if defined(TOGGLE_GC_STATS)
    rb_gc_disable_stats();
#endif

    /* Now unregister from event   */
    rb_remove_event_hook(sanspleur_sampler_event_hook);
}

extern "C" {

char *sanspleur_copy_string(const char *string)
{
    int length;
    char *result = NULL;
    
    if (string) {
        length = strlen(string);
        result = (char *)malloc(length + 1);
        strncpy(result, string, length);
        result[length] = 0;
    }
    return result;
}

double sanspleur_get_current_time()
{
    struct timeval current_date;
    double result;
    
    gettimeofday(&current_date, NULL);
    result = current_date.tv_sec + (current_date.tv_usec / 1000000.0);
    return result;
}

VALUE sanspleur_set_current_thread_to_sample(VALUE self)
{
    thread_to_sample = CURRENT_THREAD;
    return Qnil;
}

VALUE sanspleur_start_sample(VALUE self, VALUE url, VALUE usleep_value, VALUE file_name, VALUE extra_info)
{
    fprintf(stderr, "Sampler: starting sampling session\n");
    int count = 0;
    const char *url_string = NULL;
    const char *extra_info_string = NULL;
    int usleep_int = NUM2INT(usleep_value);
    InfoHeader *info_header;
    const char *start_date;
    
    if (url && url != Qnil) {
        url_string = StringValueCStr(url);
    }
    if (extra_info && extra_info != Qnil) {
        extra_info_string = StringValueCStr(extra_info);
    }
    if (!thread_to_sample) {
        thread_to_sample = CURRENT_THREAD;
    }
    if (sample) {
        delete sample;
        sample = NULL;
    }
    if (dumper) {
        delete dumper;
        dumper = NULL;
    }
    start_date = StackTraceSample::current_date_string();
    info_header = new InfoHeader(url_string, usleep_int, start_date, extra_info_string);
    free((void *)start_date);
    
    if (file_name != Qnil) {
        dumper = new DumperFile(StringValueCStr(file_name));
        dumper->open_file_with_header(info_header);
        if (skip_writting) {
            dumper->skip_writting(true);
        }
    } else {
        sample = new StackTraceSample(info_header);
    }
    start_sample_date = DumperFile::get_current_time();
    if (!ticker) {
        ticker = new ThreadTicker(usleep_int);
        ticker->start();
    } else {
        ticker->reset();
        ticker->resume();
    }
    sanspleur_install_sampler_hook();
    delete info_header;
    return Qnil;
}

VALUE sanspleur_stop_sample(VALUE self, VALUE extra_info)
{
    const char *extra_info_string = NULL;
    long long total_ticker_count = 0;
    double total_sampling_time = 0; 
    
    int count = 0;
    
    sanspleur_remove_sampler_hook();
    if (ticker) {
        total_ticker_count = ticker->total_tick_count();
        total_sampling_time = DumperFile::get_current_time() - start_sample_date;
        ticker->pause();
    }
    
    if (extra_info && extra_info != Qnil) {
        extra_info_string = StringValueCStr(extra_info);
    }
    if (sample) {
        sample->set_total_tick_count(total_ticker_count);
        sample->set_extra_ending_info(extra_info_string);
    }
    if (dumper) {
        dumper->close_file_with_info(total_sampling_time, total_ticker_count, extra_info_string);
        delete dumper;
        dumper = NULL;
    }
    skip_writting = 0;
    
    fprintf(stderr, "Sampler: stoping sampling session\nTotal time: %lf\nTotal ticks:%lld\n", total_sampling_time, total_ticker_count);
    
    DEBUG_PRINTF("thread called %d, stack trace %d\n", sample.thread_called_count, sample.stack_trace_record_count);
    return Qnil;
}

VALUE sanspleur_sample(VALUE self, VALUE url, VALUE usleep_value, VALUE file_name, VALUE beginning_extra_info, VALUE end_extra_info)
{
    int result;

    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "A block must be provided to the profile method.");
    }
    
    sanspleur_start_sample(self, url, usleep_value, file_name, beginning_extra_info);
    rb_protect(rb_yield, self, &result);
    return sanspleur_stop_sample(self, end_extra_info);
}

static void write_sample_to_disk(StackTraceSample *sample, char *filename, double duration)
{
    DumperFile *dumper;
    
    dumper = new DumperFile(filename);
    dumper->open_file_with_header(sample->get_info_header());
    dumper->write_stack_trace_sample(sample);
    dumper->close_file_with_info(duration, sample->get_total_tick_count(), sample->get_extra_ending_info());
    delete dumper;
}

struct data {
    StackTraceSample *sample;
    char *filename;
    double duration;
};

static void *write_sample_to_disk_in_thread(void *values)
{
    struct data *values_data;
    
    values_data = (struct data *)values;
    write_sample_to_disk(values_data->sample, values_data->filename, values_data->duration);
    delete values_data->sample;
    free(values_data->filename);
    free(values_data);
    pthread_exit(NULL);
    return NULL;
}

VALUE sanspleur_save_current_sample(VALUE self, VALUE filename, VALUE in_thread)
{
    if (sample) {
        int in_thread_int;
        
        in_thread_int = NUM2INT(in_thread);
        if (in_thread_int) {
            struct data *values;
            pthread_t thread;
            
            values = (struct data *)malloc(sizeof(values));
            values->sample = sample;
            values->filename = sanspleur_copy_string(StringValueCStr(filename));
            values->duration = DumperFile::get_current_time() - start_sample_date;
            pthread_create(&thread, NULL, write_sample_to_disk_in_thread, (void *)values);
            sample = NULL;
        } else {
            write_sample_to_disk(sample, StringValueCStr(filename), DumperFile::get_current_time() - start_sample_date);
            delete sample;
            sample = NULL;
        }
    }
    return Qnil;
}

VALUE sanspleur_cancel_current_sample(VALUE self)
{
    if (sample) {
        delete sample;
        sample = NULL;
    }
    return Qnil;
}

VALUE sanspleur_skip_writting_to_debug(VALUE self, VALUE skip)
{
    skip_writting = NUM2INT(skip);
//  skip_writting = strcmp("true", StringValueCStr(skip));
    return Qnil;
}

}
