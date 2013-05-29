/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
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

DumperFile::DumperFile(const char *filename, StackTraceSample* sample)
{
	_file = NULL;
	_filename = sanspleur_copy_string(filename);
	_sample = sample;
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
		_microseconds_interval = info_header->get_usleep_int();
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
	        write_string_in_file("%s\t%s\t%d\t%s\t%s\t\n", RUBY_SANSPLEUR_VERSION, info_header->get_url_string(), info_header->get_usleep_int(), info_header->get_start_date(), info_header->get_extra_info_string());
		} else {
	        write_string_in_file("%s\t%s\t%d\t%s\t\n", RUBY_SANSPLEUR_VERSION, info_header->get_url_string(), info_header->get_usleep_int(), info_header->get_start_date());
		}
		write_string_in_file("-- Traces --\n");
	}
}

static void _write_symbol_index_entry(void* closure, ID symbol_id, const char* symbol)
{
	DumperFile* file = (DumperFile*)closure;
	file->write_symbol_index_entry(symbol_id, symbol);
}

void DumperFile::write_symbol_index_entry(ID symbol_id, const char* symbol)
{
	const char* safe_symbol = symbol ? symbol : "";
	write_string_in_file("%lld\t%s\n", symbol_id, safe_symbol);
}

void DumperFile::write_file_path_index()
{
	write_string_in_file("-- File Index --\n");
	_sample->sample_file_paths_each(_write_symbol_index_entry, this);
}

void DumperFile::write_class_name_index()
{
	write_string_in_file("-- Class Index --\n");
	_sample->sample_class_each(_write_symbol_index_entry, this);
}

void DumperFile::write_function_name_index()
{
	write_string_in_file("-- Function Index --\n");
	_sample->sample_function_each(_write_symbol_index_entry, this);
}

void DumperFile::write_footer(double duration, long long tick_count, const char *extra_info)
{
	if (_file) {
		write_file_path_index();
		write_class_name_index();
		write_function_name_index();

		struct timeval stop_date;
		
		gettimeofday(&stop_date, NULL);
		write_string_in_file("-- Footer --\n");
		if (extra_info) {
			write_string_in_file("%.2f\t%.2f\t%lld\t%s\n", duration, DumperFile::get_current_time() - _start_time, tick_count, extra_info);
		} else {
			write_string_in_file("%.2f\t%.2f\t%lld\n", duration, DumperFile::get_current_time() - _start_time, tick_count);
		}
	}
}

void DumperFile::write_stack_trace_sample(StackTraceSample* sample)
{
	if (_file) {
		StackTrace *trace;
		
        trace = sample->get_first_stack_trace();
		while (trace) {
			this->write_stack_trace(trace);
			trace = trace->next_stack_trace;
		}
	}
}

void DumperFile::write_stack_trace(StackTrace *trace)
{
	if (_file) {
		StackLine *line;
		int depth = 0;

		write_string_in_file("%d\t%lld\t%lf\n", trace->depth, trace->sample_tick_count, trace->sample_duration);
		line = trace->stack_line;
		while (line) {
			write_stack_line_in_file(line, trace, line->next_stack_line?"":"\n");
			line = line->next_stack_line;
			depth++;
		}

		// Fixme: check that depth == trace->depth
	}
}

void DumperFile::write_stack_line_in_file(StackLine *line, StackTrace *trace, const char *suffix)
{
	// thread id, time, file, stack depth, type, ns, function, symbol
	write_string_in_file(
	"%lld\t" // file id
	"%lld\t" // line number
	"%lld\t" // class id
	"%lld\t" // function id
	"\n%s", // suffix
	line->sample_local_file_path_id,
	line->line_number,
	line->sample_local_class_id,
	line->sample_local_function_id,
	suffix
	);
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
