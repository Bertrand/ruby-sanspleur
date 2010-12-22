/*
 *  stack_trace_sample.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 02/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "sampler.h"

class InfoHeader;

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
		int _total_tick_count;
		const char *_ending_info;
		const InfoHeader *_info_header;
	
	public:
		static const char *current_date_string();
		
		StackTraceSample(const InfoHeader *info_header);
		~StackTraceSample();
		
		const InfoHeader *get_info_header();
		void thread_called();
		void set_extra_ending_info(const char *info);
		const char *get_extra_ending_info();
		const char *get_start_date_string();
		struct stack_trace *get_first_stack_trace();
		void set_total_tick_count(int count);
		int get_total_tick_count();
		
		void add_new_stack_trace(struct stack_trace *new_trace);
};