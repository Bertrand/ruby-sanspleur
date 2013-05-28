/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "stack_trace_sample.h"
#include "info_header.h"
#include <time.h>
#include "ruby_sanspleur.h"


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


////
//// StackTrace
////

StackTrace::StackTrace()
{
  stack_line = NULL;
  call_method = NULL;
  next_stack_trace = NULL;
}

StackTrace::~StackTrace()
{
  free((void *)call_method);
  empty_stack_line(stack_line);
}

void StackTrace::push_stack_frame(const char* file_path, unsigned long long line_number, const char* class_name, ID function_id)
{
  StackLine *new_line = new StackLine();
  new_line->next_stack_line = stack_line;
  stack_line = new_line;

  new_line->sample_local_function_id = sample->local_function_id_for_id(function_id);
  new_line->sample_local_class_id = sample->local_class_id_for_class_name(class_name);
  new_line->sample_local_file_path_id = sample->local_file_id_for_path(file_path);
}



////
//// StackTraceSample
////

StackTrace* StackTraceSample::new_stack_trace()
{
  StackTrace* trace = new StackTrace();
  trace->sample = this;
  return trace;
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

  _local_function_ids = st_init_numtable();
  _local_class_ids = st_init_strtable();
  _file_paths_ids = st_init_strtable();

  _next_local_function_id = 0;
  _next_local_class_id = 0;
  _next_file_path_id = 0;
}

StackTraceSample::~StackTraceSample()
{
  if (_info_header) {
    delete _info_header;
  }
  while (_first_stack_trace) {
    StackTrace *tmp;
    tmp = _first_stack_trace->next_stack_trace;
    delete _first_stack_trace;
    _first_stack_trace = tmp;
  }
    _last_stack_trace = NULL;
  if (_ending_info) {
    free((void *)_ending_info);
    _ending_info = NULL;
  }

  st_free_table(_local_function_ids);
  st_free_table(_local_class_ids);
  st_free_table(_file_paths_ids);
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

ID StackTraceSample::local_function_id_for_id(ID function_id)
{
  ID local_id = 0; // 0 means 'unattributed'. 
  if (function_id && !st_lookup(_local_function_ids, (st_data_t)function_id, (st_data_t *)&local_id)) {
    local_id = ++_next_local_function_id;     
    st_insert(_local_function_ids, (st_data_t)function_id, (st_data_t)local_id);
  }
  return local_id;
}

ID StackTraceSample::local_class_id_for_class_name(const char* class_name)
{
  ID local_id = 0; 
  if (class_name && !st_lookup(_local_class_ids, (st_data_t)class_name, (st_data_t *)&local_id)) {
    local_id = ++_next_local_class_id;      
    char* copied_class_name = sanspleur_copy_string(class_name);
    st_insert(_local_class_ids, (st_data_t)copied_class_name, (st_data_t)local_id);
  }
  return local_id;
}


ID StackTraceSample::local_file_id_for_path(const char* file_path)
{
  ID local_id = 0; 
  if (file_path && !st_lookup(_file_paths_ids, (st_data_t)file_path, (st_data_t *)&local_id)) {
    local_id = ++_next_file_path_id;      
    st_insert(_file_paths_ids, (st_data_t)file_path, (st_data_t)local_id);
  }
  return local_id;
}


typedef struct {
  SymbolIteratorFunction* iter;
  void* closure;
} _SymbolIteratorInfo;

static int
each_string(st_data_t key, st_data_t value, st_data_t data)
{
  ID local_id = (ID)value; 
  const char* string = (const char*)key; 
  _SymbolIteratorInfo* symbol_iterator_info = (_SymbolIteratorInfo*)data;

  symbol_iterator_info->iter(symbol_iterator_info->closure, local_id, string);
  return ST_CONTINUE;
}

void StackTraceSample::sample_file_paths_each(SymbolIteratorFunction* iter, void* closure)
{
  void* target_info[2];
  target_info[0] = (void*)iter;
  target_info[1] = closure;
  _SymbolIteratorInfo symbol_iterator_info;
  symbol_iterator_info.iter = iter;
  symbol_iterator_info.closure = closure;
  st_foreach(_file_paths_ids, (int (*)(...))each_string, (st_data_t)&symbol_iterator_info);
}

void StackTraceSample::sample_class_each(SymbolIteratorFunction* iter, void* closure)
{
  void* target_info[2];
  target_info[0] = (void*)iter;
  target_info[1] = closure;
  _SymbolIteratorInfo symbol_iterator_info;
  symbol_iterator_info.iter = iter;
  symbol_iterator_info.closure = closure;
  st_foreach(_local_class_ids, (int (*)(...))each_string, (st_data_t)&symbol_iterator_info);
}

