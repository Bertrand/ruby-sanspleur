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


#define ITIMER ITIMER_REAL


#ifdef RUBY_VM /* ruby 1.9 and above */

#include <ruby/st.h>
#include <ruby/intern.h>

#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())

#else /* ruby 1.8*/

#include <st.h>

// #define THREAD_TYPE rb_thread_t
// #define CURRENT_THREAD ((rb_thread_t)DATA_PTR(rb_thread_current()))
// #define MAIN_THREAD (DATA_PTR(rb_thread_main()))

#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())
 
#endif /* RUBY_VM */



#include <time.h>
#include <sys/time.h>

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time

#include <mach/clock.h>
#include <mach/mach.h>

void clock_getrealtime(struct timespec* ts) {
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
}

#else
#define clock_getrealtime(ts) clock_gettime(CLOCK_REALTIME, ts)
#endif

#define STRING_BUFFER_SIZE 1024


static THREAD_TYPE thread_to_sample = NULL;

static GenericTicker *ticker = NULL;
static StackTraceSample *sample = NULL;
static DumperFile *dumper = NULL;
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

#define safe_string(__s__) (__s__ ? __s__ : "")

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

extern "C" {
    int rb_vm_get_sourceline(const rb_control_frame_t *cfp);
}


#define RUBY_VM_IFUNC_P(ptr)        (BUILTIN_TYPE(ptr) == T_NODE)
#define VM_FRAME_MAGIC_MASK_BITS   8
#define VM_FRAME_MAGIC_MASK   (~(~0<<VM_FRAME_MAGIC_MASK_BITS))
#define VM_FRAME_TYPE(cfp) ((cfp)->flag & VM_FRAME_MAGIC_MASK)


static void _stacktrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
{

    rb_thread_t* ruby_current_thread = (rb_thread_t*)(DATA_PTR(rb_thread_current()));
    rb_control_frame_t *cfp = ruby_current_thread->cfp;

    const rb_control_frame_t *cfp_limit = (rb_control_frame_t*)(ruby_current_thread->stack + ruby_current_thread->stack_size);
    cfp_limit -= 2;
   
    // This is a temporary hack on 1.9. We compute the stacktrace while 
    // receiving a function return event. The current frame is not the 
    // function from which we return, but its caller. 
    // We should rather get info about returning function in the event
    // itself, but for the moment, let's just walk a frame back. 
    cfp--; 

    while (cfp < cfp_limit) {

        ID function_id = 0; 
        VALUE klass = NULL;
        const char* file_name = NULL; 
        int line_number = -1; 
        bool no_pos = false;

        rb_iseq_t *iseq = cfp->iseq;
        if (!iseq && cfp->me) {
            function_id = cfp->me->def->original_id;
            klass = cfp->me->klass;
            no_pos = true;
        }
        if (cfp->me) {
            function_id = cfp->me->def->original_id;
            klass = cfp->me->klass;
        }

        while (iseq) {
            if (RUBY_VM_IFUNC_P(iseq)) {
                CONST_ID(function_id, "<ifunc>");
                klass = NULL;
                break;
            }
            if (iseq->defined_method_id) {
                function_id = iseq->defined_method_id;
                klass = iseq->klass;
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

        const char* class_name = NULL; 
        if (klass) class_name = rb_class2name(klass); 

        if (!no_pos) {
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

static void _stacktrace_each(sanspleur_backtrace_iter_func* iterator, void* arg)
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


static void add_line_to_trace(void* anonymous_trace, const char* file_name, int line_number, const char* function_name, ID function_id, const char* class_name, ID class_id)
{
    DEBUG_PRINTF("New backtrace line : %s:%d %s::%s\n", safe_string(file_name), line_number, safe_string(class_name), safe_string(function_name));

    StackTrace *trace = (StackTrace*)anonymous_trace;
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
    //struct itimerval timer;
    //getitimer(ITIMER, &timer);
    // fprintf(stderr, "pouet %ld %ld\n", timer.it_value.tv_sec, timer.it_value.tv_usec);

    struct timespec tp;
    clock_getrealtime(&tp);
    //fprintf(stderr, "gli %ld %ld\n", tp.tv_sec, tp.tv_nsec);

    //struct timeval tp;
    //gettimeofday(&tp, NULL);
    //fprintf(stderr, "gli %ld %ld\n", tp.tv_sec, tp.tv_usec);


    double sample_duration = 0;

    if (thread_to_sample == CURRENT_THREAD && sample) {
       sample->thread_called();
    }

    if (thread_to_sample == CURRENT_THREAD && ticker) {
        sample_duration = ticker->time_since_anchor();
    }


    if (sample_duration != 0) {
        StackTrace *new_trace = new StackTrace();
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
        DEBUG_PRINTF("------------\n");

    }
}

static void sanspleur_install_sampler_hook()
{
#ifdef RUBY_VM
    rb_add_event_hook(sanspleur_sampler_event_hook,
        // RUBY_EVENT_CALL    | RUBY_EVENT_C_CALL      |
        RUBY_EVENT_RETURN  | RUBY_EVENT_C_RETURN    |
        RUBY_EVENT_LINE, Qnil); // RUBY_EVENT_SWITCH
#else
    rb_add_event_hook(sanspleur_sampler_event_hook,
        RUBY_EVENT_CALL   | RUBY_EVENT_C_CALL     |
        RUBY_EVENT_RETURN | RUBY_EVENT_C_RETURN   | 
        RUBY_EVENT_LINE );
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
    DEBUG_PRINTF("Sampler: starting sampling session\n");
    int count = 0;
    const char *url_string = NULL;
    const char *extra_info_string = NULL;
    long usleep_int = NUM2INT(usleep_value);
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
    } else {
        sample = new StackTraceSample(info_header);
    }
    start_sample_date = DumperFile::get_current_time();
    if (!ticker) {
        ticker = new ThreadTicker(usleep_int);
        //ticker = new SignalTicker(usleep_int);
        //ticker->start();
    } else {
        ticker->reset();
        ticker->resume();
    }


    // struct itimerval timer;
    // timer.it_value.tv_sec = 0;
    // timer.it_value.tv_usec = usleep_int;
    // timer.it_interval.tv_sec = 0;
    // timer.it_interval.tv_usec = usleep_int;
    // setitimer(ITIMER, &timer, NULL);


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
    
    if (sample) {
        DEBUG_PRINTF("tick count: %lld\n", sample->get_total_tick_count());
    }
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

}
