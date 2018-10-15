#include <syscfg.h> // Path is project specific.
// Created: Mon Dec 12 16:02:07 CST 2005
//
// Thread text-pool handling
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <deque>
#include <stdexcept>
#include <mutex>

#include "teul/teulTime.h"					// For teulClock64
#include "str/stringLib.h"
#include "wwii/wwiiThreadTextQuery.h"

static std::mutex textMutex;

using TEXT_POOL = std::deque<char*>;
static TEXT_POOL _pool;

static int inUse = 0;
static int allocations = 0;

//////////////////////////////////////////////////////////////////////
// Allocate me a thread-safe buffer

char*
threadsAllocText()
{
	char* p = nullptr;

#if !defined(XMEMORY_DEBUG)
	{
		std::lock_guard<std::mutex> mutexLock(textMutex);

		if ( !_pool.empty() )
		{
			p = _pool.back();
			_pool.pop_back();
		}

		++inUse;
		if ( p == nullptr )
			++allocations;
	}

	if ( p == nullptr )
	{
		p = new char[TEXT_POOL_SIZE+1]; // (char*)memCalloc(1, TEXT_POOL_SIZE + 1);
		p[TEXT_POOL_SIZE] = 0;
	}
#else
	p = (char*)memCalloc(1, TEXT_POOL_SIZE + 1);
	++inUse;
	++allocations;
#endif

	return p;
}

//////////////////////////////////////////////////////////////////////
// Allocate me a thread-safe buffer and write a string into it

char*
threadsPrintf(const char* fmt, ...)
{
	if ( fmt == nullptr )
		LogAbort("%s called with NULL fmt", __FUNCTION__);

	char* p = threadsAllocText();
	if ( p == nullptr )
		LogAbort("%s out of memory", __FUNCTION__);
	else
	{
		va_list args;
		va_start(args, fmt);
		int bytes = vsnprintf(p, TEXT_POOL_SIZE - 1, fmt, args);
		p[bytes] = 0;
		va_end(args);
	}

	return p;
}

//////////////////////////////////////////////////////////////////////
// Release text

void
threadsFreeText(char* text)
{
#if !defined(XMEMORY_DEBUG)
	std::lock_guard<std::mutex> mutexLock(textMutex);
	_pool.push_back(text);
#else
	memFree(text);
#endif

	--inUse;
}

void
ThreadTextQuery::addEntry(const char* const entry)
{
	assert( entry != nullptr );

	// Are we about to fill the buffer?
	size_t len = strlen(entry);
	if ( noRoomFor(len + 1) ) // Account for separator
	{
		// No space in the buffer for this many bytes,
		// so commit immediately.
		commit(0LL);
	}

	if ( m_length == 0 )
	{
		// No current buffer, allocate one and copy the prefix into it.
		m_length = strlcpy(m_statement, m_prefix, sizeof(m_statement));
		m_commitFrom = teulClock64;
	}
	else
	{
		// We have a buffer, add a separator.
		m_statement[m_length++] = separator();
	}

	if ( noRoomFor(len) ) {
		dbugLog(_FLF, "Data too long for ThreadTextQuery - need %zu bytes to add entry %s, but buffer length %zu has only %zu available (current string is %s)", len, entry,
sizeof(m_statement), roomLeft(), m_statement);
		throw std::domain_error("Data too long for ThreadTextQuery");
    }

	// Copy the data over, no need to worry about buffer size because
	// we've already validated that it fits.
	strlcpy(m_statement + m_length, entry, sizeof(m_statement)); // And yet, we do the safe thing anyway, because, you know, safety?

	m_length += len;
}

