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

DumperFile::DumperFile(const char *filename)
{
	_file = NULL;
	_filename = sanspleur_copy_string(filename);
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
	_file = fopen(_filename, "w");
	if (_file) {
		this->write_header(info);
		_usleep_value = usleep_value;
	}
}

void DumperFile::close_file()
{
	if (_file) {
		fclose(_file);
	}
}

void DumperFile::write_header(const char *info)
{
	if (_file) {
        fprintf(_file, "version: %s\n", RUBY_SANSPLEUR_VERSION);
        fprintf(_file, "%s\n", info);
        fprintf(_file, "--\n");
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
		fprintf(_file, "\n");
	}
}

void DumperFile::write_stack_line_in_file(struct stack_line *line, struct stack_trace *trace)
{
	// thread id, time, file, stack depth, type, ns, function, symbol
	fprintf(_file, "1"); // 0: thread id
	fprintf(_file, "\t");
	fprintf(_file, "%d", trace->thread_ticks * _usleep_value);  // 1: time
	fprintf(_file, "\t");
	if (line->file_name) {
		fprintf(_file, "%s", line->file_name);  // 2: file
	}
	fprintf(_file, "\t");
	fprintf(_file, "%d", line->line_number);  // 3: line number
	fprintf(_file, "\t");
	fprintf(_file, "%p", trace->ruby_event);  // 4: type
	fprintf(_file, "\t");
	if (line->function_id) {
		fprintf(_file, "%p", (void *)line->function_id);  // 5: function id
	}
	fprintf(_file, "\t");
	if (line->function_name) {
		fprintf(_file, "%s", line->function_name);  // 6: function name
	}
	fprintf(_file, "\t");
	fprintf(_file, "%s", trace->call_method); // 7: call method
	fprintf(_file, "\n");
}
