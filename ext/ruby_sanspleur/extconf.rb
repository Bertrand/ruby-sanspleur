require "mkmf"

if RUBY_VERSION >= "1.9.3"
  # OK
elsif RUBY_VERSION >= "1.8.7"
  # OK 
else
  STDERR.print("Ruby version is too old. Supported versions : 1.8.7 or above, 1.9.3 or above\n")
  exit(1)
end

def add_define(name, value = nil)
  if value
    $defs.push("-D#{name}=#{value}")
  else
    $defs.push("-D#{name}")
  end
end

# force mkmf to detect absence a of a symbol at link time. Without this, gcc emits a warning 
# which is kindly ignored by mkmf. This fixes detection of rb_thread_add_event_hook.
saved_cflags = $CFLAGS 
if RUBY_VERSION < "2.0.0" then
  $CFLAGS = ($CFLAGS || "") + " -Wimplicit -Werror"
end

have_header("sys/times.h")

if RUBY_VERSION < "2.0.0" then
  have_func("rb_thread_add_event_hook", "ruby.h")
else
  have_func("rb_thread_add_event_hook", "ruby/debug.h")
end

have_func("timer_settime", "time.h")
have_library("stdc++")

add_define("RUBY_VERSION", RUBY_VERSION.gsub('.', ''))
cc_command("g++")

$CFLAGS = saved_cflags

# Use this to debug 
# $CPPFLAGS = "-fPIC -ggdb3 -O0"

create_makefile("ruby_sanspleur")

