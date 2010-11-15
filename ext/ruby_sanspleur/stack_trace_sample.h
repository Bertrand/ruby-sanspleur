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
	struct stack_line *next_stack_line;
	ID function_id;
};

struct stack_trace;
struct stack_trace {
	struct stack_line *stack_line;
	struct stack_trace *next_stack_trace;
    int thread_ticks;
	
	int ruby_event;
	const char *call_method;
};

class StackTraceSample {
	protected:
		struct stack_trace *_first_stack_trace;
		struct stack_trace *_last_stack_trace;
		int _thread_called_count;
		int _stack_trace_count;
		char *_info;
	
	public:
		StackTraceSample();
		~StackTraceSample();
		
		void thread_called(void);
		void set_info(const char *info);
		const char* get_info(void);
		struct stack_trace *get_first_stack_trace();
		
		void add_new_stack_trace(struct stack_trace *new_trace);
};