/*
 *  dumper_file.cpp
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 02/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "dumper_file.h"
#include "info_header.h"
#include "sampler.h"
#include "stack_trace_sample.h"
#include "version.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

const char *DumperFile::default_tmp_filename()
{
	time_t date;
	struct tm d;
	char *buffer;
	
	buffer = (char *)malloc(128);
	time(&date);
	localtime_r(&date, &d);
	strftime(buffer, sizeof(buffer), "/tmp/%c", &d);
	return buffer;
}

double DumperFile::get_current_time()
{
	struct timeval stop_date;
	
	gettimeofday(&stop_date, NULL);
	return stop_date.tv_sec + (stop_date.tv_usec / 1000000.0);
}

DumperFile::DumperFile(const char *filename)
{
	_file = NULL;
	_filename = sanspleur_copy_string(filename);
	_skip_writting = false;
}

DumperFile::~DumperFile()
{
	this->close_file_with_info(-1, 0, NULL);
	if (_filename) {
		free((void *)_filename);
	}
}

void DumperFile::open_file_with_header(const InfoHeader *info_header)
{
#ifdef USE_FOPEN
	_file = fopen(_filename, "w");
#else
	_file = ::open(_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | O_NONBLOCK);
#endif
	if (_file) {
		_start_time = DumperFile::get_current_time();
		this->write_info_header(info_header);
		_usleep_value = info_header->get_usleep_int();
	}
}

void DumperFile::close_file_with_info(double duration, long long tick_count, const char *extra_info)
{
	if (_file) {
		write_footer(duration, tick_count, extra_info);
#ifdef USE_FOPEN
		fclose(_file);
		_file = 0;
#else
		close(_file);
		_file = NULL;
#endif
	}
}

void DumperFile::write_info_header(const InfoHeader *info_header)
{
	if (_file) {
		if (info_header->get_extra_info_string()) {
	        write_string_in_file("%s\n%s\n%d\n%s\n%s\n--\n", RUBY_SANSPLEUR_VERSION, info_header->get_url_string(), info_header->get_usleep_int(), info_header->get_start_date(), info_header->get_extra_info_string());
		} else {
	        write_string_in_file("%s\n%s\n%d\n%s\n--\n", RUBY_SANSPLEUR_VERSION, info_header->get_url_string(), info_header->get_usleep_int(), info_header->get_start_date());
		}
	}
}

void DumperFile::write_footer(double duration, long long tick_count, const char *extra_info)
{
	if (_file) {
		struct timeval stop_date;
		
		gettimeofday(&stop_date, NULL);
		if (extra_info) {
			write_string_in_file("\n--\n%.2f\n%.2f\n%lld\n%s\n", duration, DumperFile::get_current_time() - _start_time, tick_count, extra_info);
		} else {
			write_string_in_file("\n--\n%.2f\n%.2f\n%lld\n", duration, DumperFile::get_current_time() - _start_time, tick_count);
		}
	}
}

void DumperFile::write_stack_trace_sample(StackTraceSample* sample)
{
	if (_file) {
		struct stack_trace *trace;
		
        trace = sample->get_first_stack_trace();
		while (trace) {
			this->write_stack_trace(trace);
			trace = trace->next_stack_trace;
		}
	}
}

void DumperFile::write_stack_trace(struct stack_trace *trace)
{
	if (_file) {
		struct stack_line *line;
		int depth = 0;

		line = trace->stack_line;
		while (line) {
			write_stack_line_in_file(line, trace, line->next_stack_line?"":"\n");
			line = line->next_stack_line;
			depth++;
		}
	}
}

void DumperFile::write_stack_line_in_file(struct stack_line *line, struct stack_trace *trace, const char *suffix)
{
	if (_skip_writting) {
		return;
	}
	// thread id, time, file, stack depth, type, ns, function, symbol
	write_string_in_file(
	"1\t" // 0: thread id
	"%d\t" // 1: tick
	"%s\t" // 2: file
	"%d\t" // 3: line number
	"%p\t" // 4: type
	"%p\t" // 5: function id
	"%s\t" // 6: function name
	"%s\t" // 7: call method
	"%f\t" // 8: sample duration
	"%s\t" // 9: class name
	"\n%s", // suffix
	trace->sample_tick_count, // 1: time
	line->file_name, // 2: file
	line->line_number, // 3: line number
	(void *)trace->ruby_event, // 4: type
	(void *)line->function_id, // 5: function id
	line->function_name, // 6: function name
	trace->call_method, // 7: call method
	trace->sample_duration, // 8: sample duration
	line->class_name, // 9: class name
	suffix
	);
}

void DumperFile::skip_writting(bool skip)
{
	_skip_writting = skip;
}


int DumperFile::write_string_in_file(const char *format, ...)
{
	int result;
	va_list list;
	
	va_start(list, format);
	if (format == NULL) {
		format = "(null)";
	}
#ifdef USE_FOPEN
	result = vfprintf(_file, format, list) > 0;
#else
#error not implemented
	result = write(_file, format, size) == size;
#endif
	va_end(list);
	return result;
}

int DumperFile::write_integer_in_file(int integer)
{
	return write_string_in_file("%d", integer);
}

int DumperFile::write_double_in_file(double number)
{
	return write_string_in_file("%f", number);
}

int DumperFile::write_pointer_in_file(const void *pointer)
{
	return write_string_in_file("%p", pointer);
}
