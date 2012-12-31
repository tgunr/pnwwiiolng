#include <syscfg.h> // Path is project specific.
/*
 *  trackmac.c
 *  WW2OL
 *
 *  Created by Andrew Wulf on 11/30/10.
 *  Copyright 2010 Idle Diversions. All rights reserved.
 *
 */
#ifdef TESTMAC

#include "trackmac.h"
#include "testmac.h"
#include <dlfcn.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc/malloc.h>
#include <pthread.h>
#include <string.h>

static void* Array[ARRAY_SIZE];
static int stacktop = 0;

int TrackActive = 0;

void getStack( char* buffer, int maxsize )
{
  Dl_info info;
  int status;
  unsigned long i, n = 0;

  for (i=0; i<stacktop; i++)
  {
    if ( dladdr(Array[i], &info) != 0 )
    {
      char* funcname = (char*)altmalloc(1024);
      size_t length=2048;
      abi::__cxa_demangle(info.dli_sname,funcname,&length,&status);
      if ( status == 0 )
      {
        int l = strlen(funcname);

        if ( maxsize-1 <= l+1 )
        {
          altfree(funcname);
          buffer[n] = 0;
          return;
        }

        strcpy(&buffer[n], funcname);
        buffer[n+l] = ',';
        n += l+1;
      }
      altfree(funcname);
    }
  }
  buffer[n] = 0;
}

static void push( void* s )
{
  if ( stacktop == ARRAY_SIZE-1 )
  {
    malloc_printf("stack overflow\n");
    exit(-1);
  }
  Array[stacktop] = s;
  stacktop++;
}

static void pop()
{
  if ( stacktop == 0 )
  {
    malloc_printf("stack underflow\n");
    exit(-1);
  }
  stacktop--;
}

void __cyg_profile_func_enter (void* this_fn, void* call_site)
{
  if ( pthread_main_np() == 0 || ! TrackActive ) return;

  push(this_fn);
}

void __cyg_profile_func_exit  (void* this_fn, void* call_site)
{
  if ( pthread_main_np() == 0 || ! TrackActive) return;
  pop();
}

#endif
