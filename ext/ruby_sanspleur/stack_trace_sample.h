/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "sampler.h"
#include <ruby/st.h> // for hash tables

class InfoHeader;
class StackTraceSample;
class StackTrace;

typedef void SymbolIteratorFunction(void * /* closure */, ID /* file id */, const char* /* symbol */);

class StackLine {
	public:
		unsigned long long line_number;
		const char *file_name;
		const char *function_name;
		const char *class_name;
		StackLine *next_stack_line;
		ID function_id;

		ID sample_local_function_id;
		ID sample_local_class_id;
		ID sample_local_file_path_id;
} ; 

class StackTrace {
	public:
		StackTrace();
		~StackTrace();

		void push_stack_frame(const char* file_path, unsigned long long line_number, const char* class_name, ID function_id);

		StackTraceSample* sample; 
		StackLine *stack_line;
		StackTrace *next_stack_trace;
	  double sample_duration;
		long long sample_tick_count;
		int depth;
		
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
		
		st_table* _local_function_ids;
		st_table* _local_class_ids;
		st_table* _file_paths_ids;

		ID _next_local_function_id;
		ID _next_local_class_id;
		ID _next_file_path_id;

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
		
		StackTrace* new_stack_trace();
		void add_new_stack_trace(StackTrace *new_trace);

		ID local_function_id_for_id(ID function_id);
		ID local_class_id_for_class_name(const char *class_name);
		ID local_file_id_for_path(const char *file_path);

		void sample_file_paths_each(SymbolIteratorFunction* iter, void* closure);
		void sample_class_each(SymbolIteratorFunction* iter, void* closure);
		void sample_function_each(SymbolIteratorFunction* iter, void* closure);

};