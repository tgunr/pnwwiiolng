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

#ifdef MEMORY_DEBUG
#include "str/strpool.h"
#include "systhread.h"
#endif

//typedef UINT64 guard_t;
//static const guard_t GUARD_VALUE = 0xaafebbdccc12dd34ULL;
typedef UINT32 guard_t;
static const guard_t GUARD_VALUE = 0xfedc1234;

#ifdef MEMORY_DEBUG
#pragma pack(push, 4)
// Size must be evenly divisible by 16
struct MemAllocation
{
	guard_t			m_startGuard;
	MemAllocation*	m_prev;
	MemAllocation*	m_next;
	size_t			m_allocSize;
	const char*		m_file;	// points to __FILE__ values which are static constants.
	SINT32			m_line;
	guard_t			m_headerGuard;
	union
	{
		guard_t		m_dataStart;
		char		m_data[1];	// overlay pattern
	};

	MemAllocation();

	MemAllocation(size_t allocSize, const char* file, UINT32 line)
	{
		m_startGuard = GUARD_VALUE;
		m_allocSize = allocSize;
		m_prev = m_next = nullptr;
		m_file = file;
		m_line = line;
		m_headerGuard = *(guard_t*)(m_data + m_allocSize) = GUARD_VALUE;
		Clear(0);
	}

	guard_t StartGuard() const { return m_startGuard; }
	guard_t HeaderGuard() const { return m_headerGuard; }
	guard_t EndGuard() const { return *(guard_t*)(m_data + m_allocSize); }

	void Clear(unsigned int value)
	{
		memset(m_data, value, m_allocSize);
	}

	bool CheckGuards() const
	{
		return (StartGuard() == GUARD_VALUE && HeaderGuard() == GUARD_VALUE && EndGuard() == GUARD_VALUE);
	}
};
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
	return calloc(1, allocSize);
}

// trap to see if there are large wild-ass allocation requests
// so far I see 32,179,745 as the biggest
void* memReallocReleaseX(void* ptr, size_t allocSize, const char* f, int n)
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
	return realloc(ptr, allocSize);
}
#endif

// Statics
#ifdef MEMORY_DEBUG
static UINT64		s_totalAllocations;
static UINT64		s_totalFrees;
static UINT64		s_peakAllocated;
static UINT64		s_totalBytesAllocated;
static UINT64		s_totalBytesFreed;

systhread_mutex		s_memoryListMutex;
MemAllocation		memoryList;

void*				s_ballast;

static void s_memInitialization()
{
	static std::once_flag s_onceFlag;
	std::call_once(s_onceFlag, []() {
		s_totalAllocations = 0;
		s_totalBytesAllocated = 0;

		memset(&memoryList, 0, sizeof(MemAllocation));

		s_memoryListMutex = systhreadMutexCreate();

		// allocate a block of memory we can throw away if we do
		// run out of memory, so there'll be memory available for
		// whatever we need to do as the application terminates.
		s_ballast = malloc(1 * 1024 * 1024);
	});
}

MemAllocation::MemAllocation()
{
	s_memInitialization();
}

// Add memory to list
static bool AddToList(MemAllocation* lpmem)
{
	ABORT_IF(lpmem == nullptr);

	s_memInitialization();
	lpmem->m_prev = &memoryList;
	systhreadMutexLock(s_memoryListMutex);
	{
		lpmem->m_next = memoryList.m_next;
		if (lpmem->m_next)
			lpmem->m_next->m_prev = lpmem;
		memoryList.m_next = lpmem;
	}
	systhreadMutexUnlock(s_memoryListMutex);

	return true;
}

// Remove memory from list
static void RemoveFromList(MemAllocation* mem)
{
	systhreadMutexLock(s_memoryListMutex);
	{
		if (mem->m_prev)
			mem->m_prev->m_next = mem->m_next;

		if (mem->m_next)
			mem->m_next->m_prev = mem->m_prev;
	}
	systhreadMutexUnlock(s_memoryListMutex);

	mem->m_prev = mem->m_next = nullptr;
}

// Check memory list
static bool CheckList()
{
	s_memInitialization();
	systhreadMutexLock(s_memoryListMutex);
	{
		for (MemAllocation* mem = memoryList.m_next; mem != nullptr; mem = mem->m_next)
		{
			if (PN_UNLIKELY(!mem->CheckGuards()))
			{
				char buf[256];
				strprintf(buf, "Memory overwrite detected: 0x%08x in %s at line %d", mem, mem->m_file, mem->m_line);
				piExitProgram(__FILE__, __LINE__, buf);
			}
		}
	}
	systhreadMutexUnlock(s_memoryListMutex);

	return true;
}

void memInit(void)
{
	s_memInitialization();
}

#if platOS != OS_WIN32
#define OutputDebugStringA printf
#endif

void memKill(void)
{
	if (s_ballast)
		free(s_ballast);

	Write_Debug("Peak memory allocation: %llu", s_peakAllocated);
	UINT64 count = 0;
	UINT64 size = 0;
	MemAllocation* mem = memoryList.m_next;
	char buf[256];

	while (mem)
	{
		count++;
		size += mem->m_allocSize;

		strprintf(buf, "Memory leak: size = %" PRSizeT ", file = %s, line = %d\n", mem->m_allocSize, mem->m_file, mem->m_line);
		OutputDebugStringA(buf);

		if (!mem->CheckGuards())
		{
			strprintf(buf, "Memory overwrite detected: 0x%08x in %s at line %d", mem, mem->m_file, mem->m_line);
			piExitProgram(__FILE__, __LINE__, buf);
		}

		mem = mem->m_next;
	}
}

void memVerifyMemory(void)
{
	CheckList();
}

void* memAllocDebug(size_t allocSize, const char* File, int Line)
{
#ifdef MEMORY_DEBUG_EXTREME
	CheckList();
#endif

	MemAllocation* ptr = (MemAllocation*)malloc(allocSize + sizeof(MemAllocation));
	if (ptr == nullptr)
	{
		free(s_ballast);
		LogAbort("Out of memory");
	}

	// initialize
	new (ptr) MemAllocation(allocSize, File, Line);

	AddToList(ptr);

	++s_totalAllocations;
	s_totalBytesAllocated += allocSize;
	if (s_totalBytesAllocated > s_peakAllocated)
		s_peakAllocated = s_totalBytesAllocated;

	return &(ptr->m_dataStart);
}

void* memReallocDebug(void* inPtr, size_t allocSize, const char* file, int line)
{
#ifdef MEMORY_DEBUG_EXTREME
	CheckList();
#endif

	if (PN_UNLIKELY(inPtr == nullptr))
		return memAllocDebug(inPtr, file, line);

	MemAllocation* oldBlock = (MemAllocation*)((char*)inPtr - offsetof(struct MemAllocation, m_data));
	if (oldBlock->m_allocSize >= allocSize)
		return inPtr;

	// all references to oldBlock are going to become invalid once realloc is complete,
	// which means realloc is going to invalidate memoryList until we fix it up.
	MemAllocation* newBlock;
	systhreadMutexLock(s_memoryListMutex);
	{
		newBlock = (MemAllocation*)(realloc(oldBlock, allocSize + sizeof(MemAllocation));
		if (newBlock == nullptr)
		{
			free(s_ballast);
			LogAbort("Out of memory");
		}

		// oldBlock is now completely invalid.

		newBlock->m_prev = newBlock;
		if (newBlock->m_next)
			newBlock->m_next->m_prev = newBlock;
	}
	systhreadMutexUnlock(s_memoryListMutex);

	return &(newBlock->m_dataStart);
}

void memFreeDebug(void* allocptr, const char* File, int Line)
{
	if (allocptr == nullptr)
	{
		Write_Debug("Attempting to free a nullptr memory pointer");
		return;
	}

#ifdef MEMORY_DEBUG_EXTREME
	CheckList();
#endif

	MemAllocation* ptr = (MemAllocation*)((char*)allocptr - offsetof(struct MemAllocation, m_data));

	if (!ptr->CheckGuards())
	{
			char buf[256];
			strprintf(buf, "Memory overwrite detected: 0x%08x in %s at line %d", ptr, ptr->m_file, ptr->m_line);
			piExitProgram(__FILE__, __LINE__, buf);
	}

	RemoveFromList(ptr);
	s_totalFrees++;
	s_totalBytesFreed += ptr->m_allocSize;
#ifdef MEMORY_DEBUG_EXTREME
	ptr->Clear(0xbb);
#endif
	free(ptr);
}

#else	//	MEMORY_DEBUG

#endif	//	MEMORY_DEBUG
