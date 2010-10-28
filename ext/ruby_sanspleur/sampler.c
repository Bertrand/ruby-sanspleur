/*
 *  sampler.c
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 15/10/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */

#include "sampler.h"
#include <env.h>
#include <node.h>
#include "ruby_sanspleur.h"
#include <stdio.h>
#include <stdarg.h>

#define COPY_RUBY_STRING 0
#define STRING_BUFFER_SIZE 1024

//#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTF(...)

extern struct FRAME *ruby_frame;
extern rb_thread_t rb_curr_thread;
extern rb_thread_t rb_main_thread;
static rb_thread_t thread_to_sample = NULL;

struct string_buffer {
	char *cstring;
	int total_size;
	int used_size;
};

struct stack_line;
struct stack_line {
	int line_number;
	const char *file_name;
	const char *function_name;
	struct stack_line *next_stack_line;
	ID function_id;
};

struct stack_trace;
struct stack_trace {
	struct stack_line *stack_line;
	struct stack_trace *next_stack_trace;
    int thread_ticks;
};

struct stack_trace_sample {
	struct stack_trace *first_stack_trace;
	struct stack_trace *last_stack_trace;
	int thread_called_count;
	int stack_trace_record_count;
	char *stact_trace_info;
    int usleep_value;
};

struct stack_trace_sample sample = { NULL, NULL, 0, 0, NULL };

struct stack_node;
struct stack_node {
	struct stack_line *line;
	int count;
	struct stack_node *deeper_stack_node;
	struct stack_node *sibling_stack_node;
};

static struct string_buffer *string_buffer_create(int default_size)
{
	struct string_buffer *result;
	
	if (default_size <= 0) {
		default_size = STRING_BUFFER_SIZE;
	}
	result = malloc(sizeof(*result));
	result->total_size = default_size;
	result->cstring = malloc(result->total_size);
	result->cstring[0] = 0;
	result->used_size = 1;
	return result;
}

static void string_buffer_free(struct string_buffer *buffer)
{
	free(buffer->cstring);
	free(buffer);
}

static const char *string_buffer_cstring(struct string_buffer *buffer)
{
	return buffer->cstring;
}

static int string_buffer_cstring_length(struct string_buffer *buffer)
{
	return buffer->used_size - 1;
}

static void string_buffer_increase_size(struct string_buffer *buffer, int diff_minimum)
{
	if (diff_minimum < buffer->total_size) {
		diff_minimum = buffer->total_size;
	}
	buffer->total_size += diff_minimum;
	buffer->cstring = realloc(buffer->cstring, buffer->total_size);
}

static void string_buffer_append_string(struct string_buffer *buffer, char *cstring)
{
	int cstring_length;
	
	cstring_length = strlen(cstring);
	if (buffer->total_size < buffer->used_size + cstring_length) {
		string_buffer_increase_size(buffer, cstring_length);
	}
	strncpy(buffer->cstring + buffer->used_size - 1, cstring, buffer->total_size - buffer->used_size - 1);
	buffer->used_size = buffer->used_size + cstring_length;
	buffer->cstring[buffer->used_size] = 0;
}

static void string_buffer_append_format_va(struct string_buffer *buffer, char *cstring, va_list mylist)
{
	int size_to_increase;
	va_list my_copy;
	
	va_copy(my_copy, mylist);
	size_to_increase = vsnprintf(NULL, 0, cstring, my_copy);
	va_end(my_copy);
	if (buffer->total_size - buffer->used_size < size_to_increase) {
		string_buffer_increase_size(buffer, size_to_increase);
	}
	size_to_increase = vsnprintf(buffer->cstring + buffer->used_size - 1, buffer->total_size - buffer->used_size - 1, cstring, mylist);
	buffer->used_size += size_to_increase;
}

static void string_buffer_append_format(struct string_buffer *buffer, char *cstring, ...)
{
	va_list mylist;
	
	va_start(mylist, cstring);
	string_buffer_append_format_va(buffer, cstring, mylist);
	va_end(mylist);
}

static char *copy_string(const char *string)
{
	int length;
	char *result = NULL;
	
	if (string) {
		length = strlen(string);
		result = malloc(length + 1);
		strncpy(result, string, length);
        result[length] = 0;
	}
	return result;
}

#ifdef RUBY_VM
static void sanspleur_sampler_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
static void sanspleur_sampler_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
	int ticks_count;
    
	sample.thread_called_count++;
	if (thread_to_sample == rb_curr_thread && (ticks_count = sanspleur_did_thread_tick()) != 0) {
		struct FRAME *frame = ruby_frame;
    	NODE *n;
		struct stack_trace *current_stack_trace;
		
		sample.stack_trace_record_count++;
		current_stack_trace = calloc(1, sizeof(*current_stack_trace));
        current_stack_trace->thread_ticks = ticks_count;
		if (!sample.first_stack_trace) {
			sample.first_stack_trace = current_stack_trace;
			sample.last_stack_trace = current_stack_trace;
		} else {
			sample.last_stack_trace->next_stack_trace = current_stack_trace;
			sample.last_stack_trace = current_stack_trace;
		}
		for (; frame && (n = frame->node); frame = frame->prev) {
			const char *function_name = NULL;
			struct stack_line *new_stack_line;
			ID function_id = 0;
			
			if (frame->prev && frame->prev->last_func) {
				if (frame->prev->node == n) {
					if (frame->prev->last_func == frame->last_func) continue;
				}
				
				function_id = frame->prev->last_func;
				function_name = rb_id2name(frame->prev->last_func);
			}
			
			new_stack_line = calloc(1, sizeof(*new_stack_line));
			new_stack_line->next_stack_line = current_stack_trace->stack_line;
			current_stack_trace->stack_line = new_stack_line;
			new_stack_line->line_number = nd_line(n);
			new_stack_line->function_id = function_id;
#if COPY_RUBY_STRING
			new_stack_line->file_name = copy_string(n->nd_file);
			new_stack_line->function_name = copy_string(function_name);
#else
			new_stack_line->file_name = n->nd_file;
			new_stack_line->function_name = function_name;
#endif
		}
	}
}

static void sanspleur_install_sampler_hook()
{
#ifdef RUBY_VM
    rb_add_event_hook(sanspleur_sampler_event_hook,
          RUBY_EVENT_CALL | RUBY_EVENT_RETURN |
          RUBY_EVENT_C_CALL | RUBY_EVENT_C_RETURN
            | RUBY_EVENT_LINE, Qnil); // RUBY_EVENT_SWITCH
#else
    rb_add_event_hook(sanspleur_sampler_event_hook,
          RUBY_EVENT_CALL | RUBY_EVENT_RETURN |
          RUBY_EVENT_C_CALL | RUBY_EVENT_C_RETURN
          | RUBY_EVENT_LINE);
#endif

#if defined(TOGGLE_GC_STATS)
    rb_gc_enable_stats();
#endif
}

static void sanspleur_remove_sampler_hook()
{
#if defined(TOGGLE_GC_STATS)
    rb_gc_disable_stats();
#endif

    /* Now unregister from event   */
    rb_remove_event_hook(sanspleur_sampler_event_hook);
}

VALUE sanspleur_set_current_thread_to_sample(VALUE self)
{
	thread_to_sample = rb_curr_thread;
	return Qnil;
}

static void empty_stack_line(struct stack_line *line)
{
	while (line) {
		struct stack_line *tmp;
		
#if COPY_RUBY_STRING
		if (line->function_name) {
			free(line->function_name);
		}
		if (line->file_name) {
			free(line->file_name);
		}
#endif
		tmp = line->next_stack_line;
		free(line);
		line = tmp;
	}
}

static void empty_stack_trace_sample(struct stack_trace_sample *sample)
{
	while (sample->first_stack_trace) {
		struct stack_trace *tmp;
        
		empty_stack_line(sample->first_stack_trace->stack_line);
		tmp = sample->first_stack_trace->next_stack_trace;
		free(sample->first_stack_trace);
		sample->first_stack_trace = tmp;
	}
    sample->last_stack_trace = NULL;
    if (sample->stact_trace_info) {
    	free(sample->stact_trace_info);
        sample->stact_trace_info = NULL;
    }
}

static void empty_stack_node(struct stack_node *node)
{
	if (node) {
		empty_stack_node(node->sibling_stack_node);
		empty_stack_node(node->deeper_stack_node);
		free(node);
	}
}

static void print_stack_line(struct stack_line *line, char *line_prefix, int depth)
{
	for (; depth > 0; depth--) {
		DEBUG_PRINTF("\t");
	}
	if (line_prefix) {
		DEBUG_PRINTF("%s ", line_prefix);
	}
	DEBUG_PRINTF("%s %s:%d\n", line->function_name, line->file_name, line->line_number);
}

static void print_stack_trace(struct stack_trace *trace)
{
	while (trace) {
		struct stack_line *line;
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

static void write_stack_line_in_file(FILE *file, struct stack_line *line, int time_spent)
{
	// thread id, time, file, stack depth, type, ns, function, symbol
	fprintf(file, "1"); // thread id
	fprintf(file, "\t");
	fprintf(file, "%d", time_spent);  // time
	fprintf(file, "\t");
	fprintf(file, "%s", line->file_name);  // file
	fprintf(file, "\t");
	fprintf(file, "%d", line->line_number);  // line number
	fprintf(file, "\t");
	fprintf(file, "type");  // type
	fprintf(file, "\t");
	fprintf(file, "%p", (void *)line->function_id);  // function id
	fprintf(file, "\t");
	fprintf(file, "%s", line->function_name);  // ns
	fprintf(file, "\n");
}

static void write_stack_trace_sample_in_file(struct stack_trace_sample *sample, const char *filename)
{
	FILE *file;

	file = fopen(filename, "w");
	if (file) {
    	struct stack_trace *trace;
        
        fprintf(file, "version: %s\n", RUBY_SANSPLEUR_VERSION);
        fprintf(file, "%s\n", sample->stact_trace_info);
        fprintf(file, "--\n");
        trace = sample->first_stack_trace;
		while (trace) {
			struct stack_line *line;
			int depth = 0;

			line = trace->stack_line;
			while (line) {
				write_stack_line_in_file(file, line, trace->thread_ticks * sample->usleep_value);
				line = line->next_stack_line;
				depth++;
			}
			fprintf(file, "\n");
			trace = trace->next_stack_trace;
		}
		fclose(file);
	}
}

static int is_stack_line_equal(struct stack_line *line1, struct stack_line *line2)
{
	int length1, length2;
	
	if (line1->line_number != line2->line_number) {
		return 0;
	}
	length1 = strlen(line1->file_name);
	length2 = strlen(line2->file_name);
	if (length1 != length2) {
		return 0;
	}
	return strncmp(line1->file_name, line2->file_name, length1) == 0;
}

static struct stack_node *create_top_down_node(struct stack_trace *trace)
{
	struct stack_node *node;
	
	node = calloc(1, sizeof(*node));
	while (trace) {
		struct stack_node *current_node;
		struct stack_line *current_line;
		
		current_node = node;
		current_line = trace->stack_line;
		while (current_line) {
			struct stack_node *test_node;
			
			test_node = current_node;
			if (!test_node->line) {
				test_node->line = current_line;
			} else {
				while (test_node) {
					if (is_stack_line_equal(test_node->line, current_line)) {
						break;
					}
					test_node = test_node->sibling_stack_node;
				}
				if (!test_node) {
					struct stack_node *last_current_node;
					
					test_node = calloc(1, sizeof(*test_node));
					test_node->line = current_line;
					last_current_node = current_node;
					while (last_current_node->sibling_stack_node) {
						last_current_node = last_current_node->sibling_stack_node;
					}
					last_current_node->sibling_stack_node = test_node;
				}
			}
			test_node->count++;
			current_node = test_node;
			current_line = current_line->next_stack_line;
			if (current_line && !current_node->deeper_stack_node) {
				current_node->deeper_stack_node = calloc(1, sizeof(*current_node->deeper_stack_node));
			}
			current_node = current_node->deeper_stack_node;
		}
		trace = trace->next_stack_trace;
	}
	return node;
}

static void print_stack_node(struct stack_node *node, int depth)
{
	char buffer[255];
	
	if (node) {
		snprintf(buffer, sizeof(*buffer), "%d", node->count);
		print_stack_line(node->line, buffer, depth);
		print_stack_node(node->deeper_stack_node, depth + 1);
		print_stack_node(node->sibling_stack_node, depth);
	}
}

VALUE sanspleur_start_sample(VALUE self, VALUE usleep_value, VALUE info)
{
    struct FRAME *test = ruby_frame;
	int count = 0;
	
	sample.thread_called_count = 0;
	sample.stack_trace_record_count = 0;
	if (!thread_to_sample) {
		thread_to_sample = rb_curr_thread;
	}
	empty_stack_trace_sample(&sample);
    sample.usleep_value = NUM2INT(usleep_value);
    if (info != Qnil) {
	    sample.stact_trace_info = copy_string(StringValueCStr(info));
    }
	sanspleur_start_thread(sample.usleep_value);
	sanspleur_install_sampler_hook();
    return Qnil;
}

VALUE sanspleur_stop_sample(VALUE self, VALUE file_name)
{
    struct FRAME *test = ruby_frame;
	int count = 0;
	
	sanspleur_remove_sampler_hook();
	sanspleur_stop_thread();
	
	DEBUG_PRINTF("thread called %d, stack trace %d\n", sample.thread_called_count, sample.stack_trace_record_count);
	if (file_name != Qnil) {
		const char *cstring_file_name;
		
		cstring_file_name = StringValueCStr(file_name);
		if (cstring_file_name) {
			write_stack_trace_sample_in_file(&sample, cstring_file_name);
		}
	}
	
    return Qnil;
}

VALUE sanspleur_sample(VALUE self, VALUE usleep_value, VALUE info, VALUE file_name)
{
    int result;

    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "A block must be provided to the profile method.");
    }

    sanspleur_start_sample(self, usleep_value, info);
    rb_protect(rb_yield, self, &result);
    return sanspleur_stop_sample(self, file_name);
}

VALUE sanspleur_dump_last_sample(VALUE self)
{
	VALUE result;
	FILE *file;
	char *buffer = NULL;
	long buffer_size = 0;
	
	write_stack_trace_sample_in_file(&sample, "/tmp/dumpstack");
	file = fopen("/tmp/dumpstack", "r");
	if (file) {
		fseek(file, 0, SEEK_END);
		buffer_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		buffer = malloc(buffer_size + 1);
		if (fread(buffer, buffer_size, 1, file) == 1) {
			buffer[buffer_size] = 0;
		} else {
			free(buffer);
			buffer = NULL;
			buffer_size = 0;
		}
		fclose(file);
	}
	if (buffer) {
		result = rb_str_new(buffer, buffer_size);
	} else {
		result = Qnil;
	}
	return result;
}
