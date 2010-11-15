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
#include "thread.h"

#define STRING_BUFFER_SIZE 1024

extern "C" {
	struct FRAME *ruby_frame;
	rb_thread_t rb_curr_thread;
	rb_thread_t rb_main_thread;
}

static rb_thread_t thread_to_sample = NULL;

StackTraceSample *sample = NULL;
DumperFile *dumper = NULL;
int usleep_value = 0;

#ifdef RUBY_VM
static void sanspleur_sampler_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
static void sanspleur_sampler_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
	int ticks_count;
    
	if (sample) {
		sample->thread_called();
	}
	if (thread_to_sample == rb_curr_thread && (ticks_count = sanspleur_did_thread_tick()) != 0) {
		struct FRAME *frame = ruby_frame;
    	NODE *n;
		struct stack_trace *new_trace;
		
		new_trace = (struct stack_trace *)calloc(1, sizeof(*new_trace));
        new_trace->thread_ticks = ticks_count;
		new_trace->ruby_event = event;
		new_trace->call_method = sanspleur_copy_string(rb_id2name(mid));
		
		for (; frame && (n = frame->node); frame = frame->prev) {
			const char *function_name = NULL;
			struct stack_line *new_line;
			ID function_id = 0;
			
			if (frame->prev && frame->prev->last_func) {
				if (frame->prev->node == n) {
					if (frame->prev->last_func == frame->last_func) continue;
				}
				
				function_id = frame->prev->last_func;
				function_name = rb_id2name(frame->prev->last_func);
			}
			
			new_line = (struct stack_line *)calloc(1, sizeof(*new_line));
			new_line->next_stack_line = new_trace->stack_line;
			new_trace->stack_line = new_line;
			new_line->line_number = nd_line(n);
			new_line->function_id = function_id;
#if COPY_RUBY_STRING
			new_line->file_name = sanspleur_copy_string(n->nd_file);
			new_line->function_name = sanspleur_copy_string(function_name);
#else
			new_line->file_name = n->nd_file;
			new_line->function_name = function_name;
#endif
		}
		if (sample) {
			sample->add_new_stack_trace(new_trace);
		}
		if (dumper) {
			dumper->write_stack_trace(new_trace);
		}
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

VALUE sanspleur_set_current_thread_to_sample(VALUE self)
{
	thread_to_sample = rb_curr_thread;
	return Qnil;
}

VALUE sanspleur_start_sample(VALUE self, VALUE usleep_value, VALUE file_name, VALUE info)
{
	struct FRAME *test = ruby_frame;
	int count = 0;
	const char *info_string = NULL;
	
	if (info) {
		info_string = StringValueCStr(info);
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
	usleep_value = NUM2INT(usleep_value);
	dumper = new DumperFile(StringValueCStr(file_name));
	dumper->open_file_with_sample(usleep_value, info_string);
	sanspleur_start_thread(usleep_value);
	sanspleur_install_sampler_hook();
	return Qnil;
}

VALUE sanspleur_stop_sample(VALUE self)
{
	struct FRAME *test = ruby_frame;
	int count = 0;
	
	sanspleur_remove_sampler_hook();
	sanspleur_stop_thread();
	
	if (dumper) {
		delete dumper;
		dumper = NULL;
	}
	
	DEBUG_PRINTF("thread called %d, stack trace %d\n", sample.thread_called_count, sample.stack_trace_record_count);
	return Qnil;
}

VALUE sanspleur_sample(VALUE self, VALUE usleep_value, VALUE file_name, VALUE info)
{
	int result;

	if (!rb_block_given_p()) {
		rb_raise(rb_eArgError, "A block must be provided to the profile method.");
	}
	
	sanspleur_start_sample(self, usleep_value, file_name, info);
	rb_protect(rb_yield, self, &result);
	return sanspleur_stop_sample(self);
}

}
