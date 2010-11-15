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

class DumperFile
{
	protected:
		FILE *_file;
		const char *_filename;
		int _usleep_value;
	
		void write_header(const char *info);
		void write_stack_line_in_file(struct stack_line *line, struct stack_trace *trace);
		void write_stack_trace_sample_header(StackTraceSample* sample);
//		void write_stack_trace_sample(StackTraceSample* sample);
		
	public:
		DumperFile(const char *filename);
		~DumperFile();
		
		void open_file_with_sample(int usleep_value, const char *info);
		void close_file();
		void write_stack_trace(struct stack_trace *trace);
};