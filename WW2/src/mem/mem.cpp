#include <syscfg.h> // Path is project specific.
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: John Lundy
// Supporting Programmer: Martin Poulin

#ifdef FRONT_END
#include "pi/pi.h"
#endif

#if !defined(__STDC__)
#include <malloc.h>
#endif

#include "mem/mem.h"
#include "dbug/dbug.h"

#ifdef MEMORY_DEBUG
#include "str/strpool.h"
#include "systhread.h"
#endif

#ifdef MEMORY_DEBUG
#pragma pack(push, 4)
// Size must be evenly divisible by 16
struct MEMORYDEBUG
{
	UINT32		guardstart;
	MEMORYDEBUG*	prev;
	MEMORYDEBUG*	next;
	UINT32		allocsize;
	void*		trueptr;
	UINT32*		lastdword;
	SINT8		File[32];
	SINT32		Line;
	UINT32		guardend;
	UINT32		retaddress;
} ;
#pragma pack(pop)
#endif

#ifndef MEMORY_DEBUG

static size_t maxAllocation = 0;
static const char* maxAllocFile;
static int maxAllocLine;

// trap to see if there are large wild-ass allocation requests
// so far I see 32,179,745 as the biggest
void* memAllocReleaseX(size_t allocSize, const char* f, int n )
{
	if ( allocSize > maxAllocation )
	{
		maxAllocation = allocSize;
		maxAllocFile = f;
		maxAllocLine = n;
#if platOS != OS_LINUX
		if ( allocSize> 50L*1024*1024 )
		{
			char s[256];
			sprintf(s,"Bigass memory request: %lu @ %s[%d]\n",maxAllocation,maxAllocFile,maxAllocLine);
			OutputDebugStringA(s);
		}
#endif
	}
	return calloc(1, allocSize) ;
}
#endif

// Statics
#ifdef MEMORY_DEBUG
static UINT32		numallocations;

static UINT32		peakallocated;
static UINT32		amountallocated;
systhread_mutex memlistMutex;
#pragma pack(push, 4)

// Size must be evenly divisible by 16

#pragma pack(pop)

MEMORYDEBUG		memorylist;


// Add memory to list
static LOGICAL AddToList(MEMORYDEBUG* lplist, MEMORYDEBUG* lpmem)
{
	MEMORYDEBUG*	lplast;

	if (lplist == NULL || lpmem == NULL)
	{
		return(FALSE);
	}

	systhreadMutexLock(memlistMutex);

	lplast = lplist->next;
	lpmem->next = lplast;
	lpmem->prev = lplist;
	lplist->next = lpmem;
	if (lplast != NULL)
	{
		lplast->prev = lpmem;
	}

	systhreadMutexUnlock(memlistMutex);

	return(TRUE);
}

// Remove memory from list
static LOGICAL RemoveFromList(MEMORYDEBUG* lpmem)
{
	if (lpmem == NULL)
	{
		return(FALSE);
	}

	systhreadMutexLock(memlistMutex);

	if (lpmem->prev != NULL)
	{
		lpmem->prev->next = lpmem->next;
	}

	if (lpmem->next != NULL)
	{
		lpmem->next->prev = lpmem->prev;
	}

	systhreadMutexUnlock(memlistMutex);

	return(TRUE);
}

// Check memory list
static LOGICAL CheckList(MEMORYDEBUG* lpmem)
{
	if (lpmem == NULL)
	{
		return(FALSE);
	}

	systhreadMutexLock(memlistMutex);

	if (lpmem->lastdword == NULL)
	{
		lpmem = lpmem->next;
	}

	while (lpmem != NULL)
	{
		if (*(lpmem->lastdword) != 0xfedc1234)
		{
			char buf[256];
			snprintf(buf, 255, "Memory overwrite detected: 0x%08x in %s at line %d", lpmem, lpmem->File, lpmem->Line);
			piExitProgram(__FILE__,__LINE__,buf);
		}

		lpmem = lpmem->next;
	}

	systhreadMutexUnlock(memlistMutex);

	return(TRUE);
}


LOGICAL memInit(void)
{
	numallocations = 0;
	amountallocated = 0;

	memset(&memorylist, 0, sizeof(MEMORYDEBUG));

	memlistMutex = systhreadMutexCreate();

	return(TRUE);
}

void memKill(void)
{
#if platOS == OS_WIN32
    MEMORYDEBUG* mem;

	Write_Debug("Peak memory allocation: %d", peakallocated);

    int count = 0;
    int size = 0;

    mem = memorylist.next;

    char buff[256];

	while(mem)
	{
        count++;
        size += mem->allocsize;

		sprintf(buff, "Memory leak: size = %d, file = %s, line = %d\n", mem->allocsize, mem->File, mem->Line);
        OutputDebugStringA(buff);

		if (*(mem->lastdword) != 0xfedc1234 || mem->guardstart != 0xfedc1234 || mem->guardend != 0xfedc1234)
		{
			char buf[256];
			snprintf(buf, 255, "Memory overwrite detected: 0x%08x in %s at line %d", mem, mem->File, mem->Line);
			piExitProgram(__FILE__, __LINE__, buf);
		}

		mem = mem->next;
	}

    sprintf(buff, "[memKill] %d (%d bytes) objects not freed\n", count, size);
    OutputDebugStringA(buff);

	if(memlistMutex)
		systhreadMutexKill(memlistMutex);

	return;
#else
    MEMORYDEBUG* mem;

	Write_Debug("Peak memory allocation: %d", peakallocated);

    int count = 0;
    int size = 0;

    mem = memorylist.next;

    char buff[256];

	while(mem)
	{
        count++;
        size += mem->allocsize;

		sprintf(buff, "Memory leak: size = %d, file = %s, line = %d\n", mem->allocsize, mem->File, mem->Line);
        OutputDebugStringA(buff);
            
		if (*(mem->lastdword) != 0xfedc1234 || mem->guardstart != 0xfedc1234 || mem->guardend != 0xfedc1234)
		{
			char buf[256];
			snprintf(buf, 255, "Memory overwrite detected: 0x%08x in %s at line %d", mem, mem->File, mem->Line);
			piExitProgram(__FILE__, __LINE__, buf);			
		}

		mem = mem->next;
	}

    sprintf(buff, "[memKill] %d (%d bytes) objects not freed\n", count, size);
    OutputDebugStringA(buff);

	if(memlistMutex)
		systhreadMutexKill(memlistMutex);

	return;
#endif
}

void memVerifyMemory(void)
{
	CheckList(&memorylist);
}

void* memAllocDebug(SINT32 allocsize, SINT8* File, SINT32 Line)
{
SINT8*	pfile;
MEMORYDEBUG*	ptr;

#ifdef MEMORY_DEBUG_EXTREME
	CheckList(&memorylist);
#endif

	ptr = (MEMORYDEBUG*)calloc(1, allocsize + sizeof(MEMORYDEBUG));
	if (ptr == NULL)
	{
		Write_Debug("No memory available for allocation; %d", allocsize);
		return(NULL);
	}
	ptr->guardstart = ptr->guardend = 0xfedc1234;
	ptr->allocsize = allocsize;
	ptr->trueptr = ptr;
	ptr->lastdword = (UINT32*)((BYTE*)ptr + allocsize + sizeof(MEMORYDEBUG) - sizeof(UINT32));
	*(ptr->lastdword) = 0xfedc1234;
	pfile = strrchr(File, '\\');
	if (pfile == NULL)
	{
		pfile = File;
	}
	else
	{
		pfile += 1;
	}

	strncpy(ptr->File, pfile,31);
	ptr->Line = Line;
	AddToList(&memorylist, ptr);

	numallocations += 1;
	amountallocated += allocsize;
	if (amountallocated > peakallocated)
	{
		peakallocated = amountallocated;
	}

	return(&ptr->retaddress);
}

void memFreeDebug(void* allocptr, SINT8* File, SINT32 Line)
{
MEMORYDEBUG*	ptr;

#ifdef MEMORY_DEBUG_EXTREME
	CheckList(&memorylist);
#endif

	ptr = (MEMORYDEBUG*)allocptr;
	if (allocptr == NULL)
	{
		Write_Debug("Attempting to free a null memory pointer");
		return;
	}

	ptr = (MEMORYDEBUG*)((BYTE*)ptr - sizeof(MEMORYDEBUG) + sizeof(UINT32));

	if (*(ptr->lastdword) != 0xfedc1234 || ptr->guardstart != 0xfedc1234 || ptr->guardend != 0xfedc1234)
	{
			char buf[256];
			snprintf(buf, 255, "Memory overwrite detected: 0x%08x in %s at line %d", ptr, ptr->File, ptr->Line);
			piExitProgram(__FILE__,__LINE__,buf);
	}else
	{
		RemoveFromList(ptr);
		numallocations -= 1;
		amountallocated -= ptr->allocsize;
		free(ptr->trueptr);
	}
}

#else	//	MEMORY_DEBUG

#endif	//	MEMORY_DEBUG
