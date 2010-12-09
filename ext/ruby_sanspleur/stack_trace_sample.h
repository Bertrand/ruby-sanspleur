/*
 *  stack_trace_sample.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 02/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "sampler.h"

struct stack_line;
struct stack_line {
	int line_number;
	const char *file_name;
	const char *function_name;
	const char *class_name;
	struct stack_line *next_stack_line;
	ID function_id;
};

struct stack_trace;
struct stack_trace {
	struct stack_line *stack_line;
	struct stack_trace *next_stack_trace;
    double sample_duration;
	int sample_tick_count;
	
	int ruby_event;
	const char *call_method;
};

class StackTraceSample {
	protected:
		struct stack_trace *_first_stack_trace;
		struct stack_trace *_last_stack_trace;
		int _thread_called_count;
		int _stack_trace_count;
		const char *_beginning_info;
		const char *_ending_info;
		const char *_url;
		const char *_start_date_string;
		int _interval;
	
	public:
		static const char *current_date_string();
		
		StackTraceSample(int interval, const char *url);
		~StackTraceSample();
		
		void thread_called();
		void set_extra_beginning_info(const char *info);
		const char* get_extra_beginning_info();
		void set_extra_ending_info(const char *info);
		const char *get_extra_ending_info();
		const char *get_start_date_string();
		int get_interval();
		struct stack_trace *get_first_stack_trace();
		const char* get_url();
		
		void add_new_stack_trace(struct stack_trace *new_trace);
};