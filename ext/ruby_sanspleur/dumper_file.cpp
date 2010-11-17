/*
 *  dumper_file.cpp
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 02/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "dumper_file.h"
#include "sampler.h"
#include "stack_trace_sample.h"
#include "version.h"
#include <fcntl.h>
#include <unistd.h>

DumperFile::DumperFile(const char *filename)
{
	_file = NULL;
	_filename = sanspleur_copy_string(filename);
	_skip_writting = false;
}

DumperFile::~DumperFile()
{
	this->close_file();
	if (_filename) {
		free((void *)_filename);
	}
}

void DumperFile::open_file_with_sample(int usleep_value, const char *info)
{
#ifdef USE_FOPEN
	_file = fopen(_filename, "w");
#else
	_file = ::open(_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | O_NONBLOCK);
#endif
	if (_file) {
		this->write_header(info);
		_usleep_value = usleep_value;
	}
}

void DumperFile::close_file()
{
	if (_file) {
#ifdef USE_FOPEN
		fclose(_file);
#else
		close(_file);
#endif
	}
}

void DumperFile::write_header(const char *info)
{
	if (_file) {
        write_string_in_file("version: ");
        write_string_in_file(RUBY_SANSPLEUR_VERSION);
        write_string_in_file("\n");
        write_string_in_file(info);
        write_string_in_file("\n--\n");
	}
}

void DumperFile::write_stack_trace_sample_header(StackTraceSample* sample)
{
	if (_file) {
		this->write_header(sample->get_info());
	}
}

//void DumperFile::write_stack_trace_sample(StackTraceSample* sample)
//{
//	if (_file) {
//		struct stack_trace *trace;
//		
//		_usleep_value = sample->get_usleep_value();
//		this->write_stack_trace_sample_header(sample);
//        trace = sample->get_first_stack_trace();
//		while (trace) {
//			this->write_stack_trace(trace);
//			trace = trace->next_stack_trace;
//		}
//	}
//}

void DumperFile::write_stack_trace(struct stack_trace *trace)
{
	if (_file) {
		struct stack_line *line;
		int depth = 0;

		line = trace->stack_line;
		while (line) {
			write_stack_line_in_file(line, trace);
			line = line->next_stack_line;
			depth++;
		}
		write_string_in_file("\n");
	}
}

void DumperFile::write_stack_line_in_file(struct stack_line *line, struct stack_trace *trace)
{
	if (_skip_writting) {
		return;
	}
	// thread id, time, file, stack depth, type, ns, function, symbol
	write_string_in_file("1"); // 0: thread id
	write_string_in_file("\t");
	write_integer_in_file(trace->thread_ticks * _usleep_value);  // 1: time
	write_string_in_file("\t");
	if (line->file_name) {
		write_string_in_file(line->file_name);  // 2: file
	}
	write_string_in_file("\t");
	write_integer_in_file(line->line_number);  // 3: line number
	write_string_in_file("\t");
	write_pointer_in_file((void *)trace->ruby_event);  // 4: type
	write_string_in_file("\t");
	if (line->function_id) {
		write_pointer_in_file((void *)line->function_id);  // 5: function id
	}
	write_string_in_file("\t");
	if (line->function_name) {
		write_string_in_file(line->function_name);  // 6: function name
	}
	write_string_in_file("\t");
	write_string_in_file(trace->call_method); // 7: call method
	write_string_in_file("\n");
}

void DumperFile::skip_writting(bool skip)
{
	_skip_writting = skip;
}


int DumperFile::write_string_in_file(const char *string)
{
	int size;
	int result;
	
	if (string == NULL) {
		string = "(null)";
	}
	size = strlen(string);
#ifdef USE_FOPEN
	result = fprintf(_file, "%s", string);
#else
	result = write(_file, string, size);
#endif
	return result == size;
}

int DumperFile::write_integer_in_file(int integer)
{
	char buffer[256];
	
	sprintf(buffer, "%d", integer);
	return write_string_in_file(buffer);
}

int DumperFile::write_pointer_in_file(const void *pointer)
{
	char buffer[256];
	
	sprintf(buffer, "%p", pointer);
	return write_string_in_file(buffer);
}
