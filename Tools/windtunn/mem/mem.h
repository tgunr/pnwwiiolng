#pragma once
// mem.h
//
// Date: Mon Feb 02 14:36:43 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a 
// compiled library.
//
// Originating Programmer: John "Kango" Lundy
// Maintaining Programmer: John "Kango" Lundy
// Supporting Programmer:  Marty "Vandal" Poulin

//	------------------------------	Includes

// ---> project headers
#include "syscfg.h"
#include "platform.h"
#include "systypes.h"

// ---> system headers
#include <stdlib.h>
#include <memory.h>

#if platOS == OS_MACOS
#define memAlloc(a)   memAllocRelease(a)
#define memFree(a)   memFreeRelease(a)
void* memAllocRelease(SINT32 allocSize);
void memFreeRelease(void* pointer);
static void* memCalloc(size_t members, size_t size) { return calloc(members, size) ; }
extern LOGICAL memInit(void);
extern void memKill(void);
#else
// ---> other headers

//	------------------------------	Public Definitions

#if platOS == OS_LINUX
	#ifdef MEMORY_DEBUG
		#define memAlloc(a) calloc(1,a)
	#else
		#define memAlloc(a) malloc(a)
	#endif
	
	#define memFree(a) free(a)
	#define memCalloc(a,b) calloc(a,b)
#else
								
	#ifdef MEMORY_DEBUG
		#define memAlloc(a)   memAllocDebug(a, __FILE__, __LINE__)
		#define memFree(a)   memFreeDebug(a, __FILE__, __LINE__)
	#else
		#define memAlloc(a)   memAllocRelease(a)
		#define memFree(a)   memFreeRelease(a)
	#endif

	#ifdef MEMORY_DEBUG
		void memVerifyMemory(void);
		void *memAllocDebug(SINT32 Size, SINT8 *file, SINT32 line);
		void memFreeDebug(void *Pointer, SINT8 *file, SINT32 line);
		#define memCalloc(a,b) memAlloc((a)*(b))
	#else
		static _inline void* memAllocRelease(size_t allocSize) { return calloc(1, allocSize) ; }
		static _inline void memFreeRelease(void* pointer) { free(pointer) ; }
		static _inline void* memCalloc(size_t members, size_t size) { return calloc(members, size) ; }
	#endif
	

#endif

//	------------------------------	Public Types
//	------------------------------	Public Variables
//	------------------------------	Public Functions

#ifdef MEMORY_DEBUG
	extern LOGICAL memInit(void);
	extern void memKill(void);
#else
	#define memInit()
	#define memKill()
#endif
#endif
#ifdef __cplusplus
#define memRelease(ptr) { if(ptr != NULL) { memFree(ptr) ; ptr = NULL ; } }
#endif
