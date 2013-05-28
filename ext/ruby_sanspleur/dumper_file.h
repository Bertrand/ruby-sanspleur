/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */
#include <stdio.h>
#include <sys/time.h>

#include "ruby_sanspleur.h"

class StackTraceSample;
class InfoHeader;
class StackLine;
class StackTrace;


#define USE_FOPEN

class DumperFile
{
	protected:
		StackTraceSample* _sample; 
		double _start_time;
#ifdef USE_FOPEN
		FILE *_file;
#else
		int _file;
#endif
		const char *_filename;
		long _microseconds_interval;
	
		void write_info_header(const InfoHeader *header);
		void write_stack_line_in_file(StackLine *line, StackTrace *trace, const char *suffix);
		int write_string_in_file(const char *string, ...);
		int write_integer_in_file(int integer);
		int write_pointer_in_file(const void *pointer);
		int write_double_in_file(double number);
		void write_footer(double duration, long long tick_count, const char *extra_info);
		
	public:
		static double get_current_time();
		static const char *default_tmp_filename();
		
		DumperFile(const char *filename, StackTraceSample* sample);
		~DumperFile();
		
		void open_file_with_header(const InfoHeader *header);
		void write_stack_trace_sample(StackTraceSample* sample);
		void close_file_with_info(double duration, long long tick_count, const char *extra_info);
		void write_stack_trace(StackTrace *trace);

		void write_symbol_index_entry(ID symbol_id, const char* symbol);
		void write_file_path_index();
		void write_class_name_index();

};