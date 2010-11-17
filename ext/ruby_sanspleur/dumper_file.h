/*
 *  dumper_file.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 02/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */
#include <stdio.h>

class StackTraceSample;

#define USE_FOPEN

class DumperFile
{
	protected:
#ifdef USE_FOPEN
		FILE *_file;
#else
		int _file;
#endif
		const char *_filename;
		int _usleep_value;
		bool _skip_writting;
	
		void write_header(const char *info);
		void write_stack_line_in_file(struct stack_line *line, struct stack_trace *trace);
		void write_stack_trace_sample_header(StackTraceSample* sample);
//		void write_stack_trace_sample(StackTraceSample* sample);
		int write_string_in_file(const char *string);
		int write_integer_in_file(int integer);
		int write_pointer_in_file(const void *pointer);
		
	public:
		DumperFile(const char *filename);
		~DumperFile();
		
		void open_file_with_sample(int usleep_value, const char *info);
		void close_file();
		void write_stack_trace(struct stack_trace *trace);
		void skip_writting(bool skip);
};