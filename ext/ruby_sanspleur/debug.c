/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include <signal.h> 
#include <stdio.h> 
#include <string.h> 
#include <sys/time.h> 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>


static int count = 0; 
static  struct sigaction sa; 

static void timer_handler (int signum) 
{
  ++count;
  printf ("timer expired %d %d times\n", ++count, signum); 
}

void test_timer_enabled()
{
    sigset_t myset;
    
    printf("get sigprocmask %d\n", sigprocmask(SIG_UNBLOCK, NULL, &myset));
    printf("sigismember SIGPROF %d\n", sigismember(&myset, SIGPROF));
    printf("sigismember SIGVTALRM %d\n", sigismember(&myset, SIGVTALRM));
}

void enable_timer()
{
    sigset_t myset;
 
    test_timer_enabled();
#if 1
    sigfillset(&myset);
//    sigemptyset(&myset);
#else
    sigemptyset(&myset);
    sigaddset(&myset, SIGPROF);
#endif
    printf("sigismember1 SIGPROF %d\n", sigismember(&myset, SIGPROF));
    printf("sigismember1 SIGVTALRM %d\n", sigismember(&myset, SIGVTALRM));
    printf("sigprocmask1 %d\n", sigprocmask(SIG_SETMASK, &myset, NULL));
    test_timer_enabled();
}

void set_timer (int signal) 
{
  struct itimerval timer, previous_timer;
  
 enable_timer();
  /* Install timer_handler as the signal handler for SIGPROF.  */ 
  memset (&sa, 0, sizeof (sa)); 
  sa.sa_handler = &timer_handler; 
  if (signal == SIGPROF) {
      printf("sigaction SIGPROF %d\n", sigaction (SIGPROF, &sa, NULL)); 
  } else {
      printf("sigaction SIGVTALRM %d\n", sigaction (SIGVTALRM, &sa, NULL)); 
  }
 
  /* Configure the timer to expire after 250 msec...  */ 
  timer.it_value.tv_sec = 0; 
  timer.it_value.tv_usec = 250000; 
  /* ... and every 250 msec after that.  */ 
  timer.it_interval.tv_sec = 0; 
  timer.it_interval.tv_usec = 250000; 
  /* Start a virtual timer. It counts down whenever this process is 
     executing.  */ 
  if (signal == SIGPROF) {
      printf("setitimer %d\n", setitimer (ITIMER_PROF, &timer, &previous_timer));
  } else {
      printf("setitimer %d\n", setitimer (ITIMER_VIRTUAL, &timer, &previous_timer));
  }
  
    printf("previous timer %d %d %d %d\n", (int)previous_timer.it_value.tv_sec, (int)previous_timer.it_value.tv_usec, (int)previous_timer.it_interval.tv_sec, (int)previous_timer.it_interval.tv_usec);
}

void test_timer()
{
    sigset_t myset;
    
    printf("timer %d %d\n", count, sa.sa_flags);
    printf("sigpending %d\n", sigpending(&myset));
    printf("SIGPROF pending %d\n", sigismember(&myset, SIGPROF));
    printf("SIGVTALRM pending %d\n", sigismember(&myset, SIGVTALRM));
}

void debug_log(char *string, ...) 
{ 
    FILE *file; 
    file = fopen("/tmp/log.txt", "a+");
    if (file) {
    	va_list list;
        
        va_start(list, string);
        vfprintf(file, string, list);
        va_end(list);
        fclose(file);
    }
}
