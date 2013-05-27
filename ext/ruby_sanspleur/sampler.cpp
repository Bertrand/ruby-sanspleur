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
#include "clock_ticker.h"
#include "info_header.h"
#include "ruby_backtrace_walker.h"

#include <ruby/st.h>


#ifdef RUBY_VM /* ruby 1.9 and above */

#include <ruby/st.h>
#include <ruby/intern.h>

#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())

#if RUBY_API_VERSION_MAJOR >= 2
#include <ruby/debug.h>
#endif 

#else /* ruby 1.8*/

#include <st.h>

// #define THREAD_TYPE rb_thread_t
// #define CURRENT_THREAD ((rb_thread_t)DATA_PTR(rb_thread_current()))
// #define MAIN_THREAD (DATA_PTR(rb_thread_main()))

#define THREAD_TYPE VALUE
#define CURRENT_THREAD (rb_thread_current())
#define MAIN_THREAD (rb_thread_main())
 
#endif /* RUBY_VM */


#if HAVE_RB_THREAD_ADD_EVENT_HOOK
extern "C" {
    void rb_thread_add_event_hook(VALUE thval, rb_event_hook_func_t func, rb_event_flag_t events, VALUE data);
    int rb_thread_remove_event_hook(VALUE thval, rb_event_hook_func_t func);
}
#endif

void add_event_hook_for_thread(VALUE thval, rb_event_hook_func_t func, rb_event_flag_t events, VALUE data)
{
#if HAVE_RB_THREAD_ADD_EVENT_HOOK
    rb_thread_add_event_hook(thval, func, events, data);
#else 
    rb_add_event_hook(func, events, data);
#endif
}

void remove_event_hook_for_thread(VALUE thval, rb_event_hook_func_t func)
{
#if HAVE_RB_THREAD_ADD_EVENT_HOOK
    rb_thread_remove_event_hook(thval, func);
#else 
    rb_remove_event_hook(func);
#endif
}



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
        new_line->class_name = class_name;
    }
      
}

#ifdef RUBY_VM
static void sanspleur_sampler_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
static void sanspleur_sampler_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
    long long tick_count = 0;
    if (thread_to_sample == CURRENT_THREAD && sample) {
       sample->thread_called();
    }

    if (thread_to_sample == CURRENT_THREAD && ticker) {
        tick_count = ticker->ticks_since_anchor();
    }

    if (tick_count != 0) {
        DEBUG_PRINTF("tick\n");
        StackTrace *new_trace = new StackTrace();
        new_trace->sample_duration = ticker->time_since_anchor();
        new_trace->sample_tick_count = tick_count;
        new_trace->ruby_event = event;
        new_trace->call_method = sanspleur_copy_string(rb_id2name(mid));

        ruby_backtrace_each(add_line_to_trace, (void*)new_trace); 

        if (sample) {
            sample->add_new_stack_trace(new_trace);
        }
        if (dumper) {
            dumper->write_stack_trace(new_trace);
        }
        if (!sample) {
            delete new_trace;
        }
        ticker->sync_anchor();
        DEBUG_PRINTF("------------\n");

    }
}

static void sanspleur_install_sampler_hook()
{
    DEBUG_PRINTF("installing event hook\n");
#ifdef RUBY_VM
    add_event_hook_for_thread(thread_to_sample, sanspleur_sampler_event_hook, RUBY_EVENT_RETURN  | RUBY_EVENT_C_RETURN, Qnil); 
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
    DEBUG_PRINTF("removing event hook\n");

#if defined(TOGGLE_GC_STATS)
    rb_gc_disable_stats();
#endif

    /* Now unregister from event   */
#ifdef RUBY_VM
    remove_event_hook_for_thread(thread_to_sample, sanspleur_sampler_event_hook);
#else
    rb_remove_event_hook(sanspleur_sampler_event_hook);
#endif
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

VALUE sanspleur_start_sample(VALUE self, VALUE url, VALUE microseconds_interval, VALUE file_name, VALUE extra_info)
{
    DEBUG_PRINTF("Sampler: starting sampling session\n");
    int count = 0;
    const char *url_string = NULL;
    const char *extra_info_string = NULL;
    long usleep_int = NUM2INT(microseconds_interval);
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
        // old test
        // ticker = new ThreadTicker(usleep_int);

#if HAVE_RT
        ticker = new SignalTicker(usleep_int);
#else 
        ticker = new ClockTicker(usleep_int);
#endif
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
    
    DEBUG_PRINTF("tick count: %lld\n", total_ticker_count);

    if (sample) {
        DEBUG_PRINTF("total tick count: %lld\n", sample->get_total_tick_count());
    }

    thread_to_sample = NULL;

    return Qnil;
}

VALUE sanspleur_sample(VALUE self, VALUE url, VALUE microseconds_interval, VALUE file_name, VALUE beginning_extra_info, VALUE end_extra_info)
{
    int result;

    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "A block must be provided to the profile method.");
    }
    
    sanspleur_start_sample(self, url, microseconds_interval, file_name, beginning_extra_info);
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
