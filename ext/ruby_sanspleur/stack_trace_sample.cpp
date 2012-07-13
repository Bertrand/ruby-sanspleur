/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "stack_trace_sample.h"
#include "info_header.h"
#include <time.h>

static void empty_stack_line(StackLine *line)
{
	while (line) {
		StackLine *tmp = line->next_stack_line;
		free(line);
		line = tmp;
	}
}

static void print_stack_line(StackLine *line, char *line_prefix, int depth)
{
	for (; depth > 0; depth--) {
		DEBUG_PRINTF("\t");
	}
	if (line_prefix) {
		DEBUG_PRINTF("%s ", line_prefix);
	}
	DEBUG_PRINTF("%s %s:%d\n", line->function_name, line->file_name, line->line_number);
}

static void print_stack_trace(StackTrace *trace)
{
	while (trace) {
		StackLine *line;
		int depth = 0;
		
		line = trace->stack_line;
		while (line) {
			print_stack_line(line, NULL, depth);
			line = line->next_stack_line;
			depth++;
		}
		trace = trace->next_stack_trace;
	}
}

const char *StackTraceSample::current_date_string()
{
	time_t date;
	struct tm d;
	char *buffer;
	
	buffer = (char *)malloc(128);
	time(&date);
	localtime_r(&date, &d);
	strftime(buffer, 127, "%c", &d);
	return buffer;
}

StackTraceSample::StackTraceSample(const InfoHeader *info_header)
{
	_info_header = info_header->copy();
	_first_stack_trace = NULL;
	_last_stack_trace = NULL;
	_thread_called_count = 0;
	_stack_trace_count = 0;
	_ending_info = NULL;
}

StackTraceSample::~StackTraceSample()
{
	if (_info_header) {
		delete _info_header;
	}
	while (_first_stack_trace) {
		StackTrace *tmp;
        
		empty_stack_line(_first_stack_trace->stack_line);
		tmp = _first_stack_trace->next_stack_trace;
		free((void *)_first_stack_trace->call_method);
		free(_first_stack_trace);
		_first_stack_trace = tmp;
	}
    _last_stack_trace = NULL;
	if (_ending_info) {
		free((void *)_ending_info);
		_ending_info = NULL;
	}
}

StackTrace *StackTraceSample::get_first_stack_trace()
{
	return _first_stack_trace;
}

void StackTraceSample::set_extra_ending_info(const char *info)
{
	if (_ending_info) {
		free((void *)_ending_info);
	}
	_ending_info = sanspleur_copy_string(info);
}

const char *StackTraceSample::get_extra_ending_info()
{
	return _ending_info;
}

void StackTraceSample::thread_called()
{
	_thread_called_count++;
}

void StackTraceSample::add_new_stack_trace(StackTrace *new_trace)
{
	if (!_first_stack_trace) {
		_first_stack_trace = new_trace;
	} else {
		_last_stack_trace->next_stack_trace = new_trace;
	}
	_last_stack_trace = new_trace;
	_last_stack_trace->next_stack_trace = NULL;
	_stack_trace_count++;
}

void StackTraceSample::set_total_tick_count(long long count)
{
	_total_tick_count = count;
}

long long StackTraceSample::get_total_tick_count()
{
	return _total_tick_count;
}

const InfoHeader *StackTraceSample::get_info_header()
{
	return _info_header;
}
