/*
 *  sampler.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 15/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
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
#include "st.h"
#include "info_header.h"

#define STRING_BUFFER_SIZE 1024

extern "C" {
	struct FRAME *ruby_frame;
	rb_thread_t rb_curr_thread;
	rb_thread_t rb_main_thread;
}

static rb_thread_t thread_to_sample = NULL;

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

#ifdef RUBY_VM
static void sanspleur_sampler_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
static void sanspleur_sampler_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
	double sample_duration = 0;
    
	if (thread_to_sample == rb_curr_thread && sample) {
		sample->thread_called();
	}
	if (thread_to_sample == rb_curr_thread && ticker) {
		sample_duration = ticker->time_since_anchor();
	}
	if (sample_duration != 0) {
		struct FRAME *frame = ruby_frame;
    	NODE *n;
		struct stack_trace *new_trace;
		
		new_trace = (struct stack_trace *)calloc(1, sizeof(*new_trace));
        new_trace->sample_duration = sample_duration;
		new_trace->sample_tick_count = ticker->ticks_since_anchor();
		new_trace->ruby_event = event;
		new_trace->call_method = sanspleur_copy_string(rb_id2name(mid));
		
		for (; frame && (n = frame->node); frame = frame->prev) {
			const char *function_name = NULL;
			const char *file_name = n->nd_file;
			struct stack_line *new_line;
			ID function_id = 0;
			const char *class_name = NULL;
			
			if (frame->prev && frame->prev->last_func) {
				if (frame->prev->node == n) {
					if (frame->prev->last_func == frame->last_func) continue;
				}
				
				function_id = frame->prev->last_func;
				function_name = rb_id2name(frame->prev->last_func);
				class_name = rb_class2name(frame->prev->last_class);
			}
			if (function_name == NULL) {
				function_name = "";
			}
			if (file_name == NULL) {
				file_name = "";
			}
			if (class_name == NULL) {
				class_name = "";
			}
			
			new_line = (struct stack_line *)calloc(1, sizeof(*new_line));
			new_line->next_stack_line = new_trace->stack_line;
			new_trace->stack_line = new_line;
			new_line->line_number = nd_line(n);
			new_line->function_id = function_id;
#if COPY_RUBY_STRING
			new_line->file_name = sanspleur_copy_string(file_name);
			new_line->function_name = sanspleur_copy_string(function_name);
			new_line->class_name = sanspleur_copy_string(class_name);
#else
			if (!st_lookup(file_name_table, (st_data_t)file_name, (st_data_t *)&new_line->file_name)) {
				file_name = sanspleur_copy_string(file_name);
				st_insert(file_name_table, (st_data_t)file_name, (st_data_t)file_name);
				new_line->file_name = file_name;
			}
			if (!st_lookup(function_name_table, (st_data_t)function_name, (st_data_t *)&new_line->function_name)) {
				function_name = sanspleur_copy_string(function_name);
				st_insert(function_name_table, (st_data_t)function_name, (st_data_t)function_name);
				new_line->function_name = function_name;
			}
			if (!st_lookup(class_name_table, (st_data_t)class_name, (st_data_t *)&new_line->class_name)) {
				class_name = sanspleur_copy_string(class_name);
				st_insert(class_name_table, (st_data_t)class_name, (st_data_t)class_name);
				new_line->class_name = function_name;
			}
#endif
		}
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
	thread_to_sample = rb_curr_thread;
	return Qnil;
}

VALUE sanspleur_start_sample(VALUE self, VALUE url, VALUE usleep_value, VALUE file_name, VALUE extra_info)
{
	fprintf(stderr, "Sampler: starting sampling session\n");
	struct FRAME *test = ruby_frame;
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
		thread_to_sample = rb_curr_thread;
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
	struct FRAME *test = ruby_frame;
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
//	skip_writting = strcmp("true", StringValueCStr(skip));
	return Qnil;
}

}
