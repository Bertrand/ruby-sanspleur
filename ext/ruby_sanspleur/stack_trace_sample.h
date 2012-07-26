/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "sampler.h"

class InfoHeader;

class StackLine {
	public:
		int line_number;
		const char *file_name;
		const char *function_name;
		const char *class_name;
		StackLine *next_stack_line;
		ID function_id;
} ; 

class StackTrace {
	public:
		StackTrace();
		~StackTrace();

		StackLine *stack_line;
		StackTrace *next_stack_trace;
	    double sample_duration;
		long long sample_tick_count;
		
		int ruby_event;
		const char *call_method;
};

class StackTraceSample {
	protected:
		StackTrace *_first_stack_trace;
		StackTrace *_last_stack_trace;
		int _thread_called_count;
		int _stack_trace_count;
		long long _total_tick_count;
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
		StackTrace *get_first_stack_trace();
		void set_total_tick_count(long long count);
		long long get_total_tick_count();
		
		void add_new_stack_trace(StackTrace *new_trace);
};