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

//////////////////////////////////////////////////////////////////////
// Allocate me a thread-safe buffer

char*
threadsAllocText()
{
	char* p = nullptr;

	p = reinterpret_cast<char *>(::calloc(1, TEXT_POOL_SIZE + 1));
	return p;
}

//////////////////////////////////////////////////////////////////////
// Allocate me a thread-safe buffer and write a string into it

char*
threadsPrintf(const char* fmt, ...)
{
    assert(fmt);

    char *p = nullptr;
    va_list args;

    va_start(args, fmt);
    vasprintf(&p, fmt, args);
    va_end(args);
    return p;
}

//////////////////////////////////////////////////////////////////////
// Release text

void
threadsFreeText(char* text)
{
	::free(text);
}

void
ThreadTextQuery::addEntry(const char* const entry)
{
	assert(entry != nullptr);

	// Are we about to fill the buffer?
	size_t len = strlen(entry);
	if (noRoomFor(len + 1)) { // Account for separator
		// No space in the buffer for this many bytes,
		// so commit immediately.
		commit(0LL);
	}

	if (m_length == 0) {
		// No current buffer, allocate one and copy the prefix into it.
		m_length = strlcpy(m_statement, m_prefix, sizeof(m_statement));
		m_commitFrom = teulClock64;
	} else {
		// We have a buffer, add a separator.
		m_length = strlcat(m_statement, ",", sizeof(m_statement));
	}

	if (noRoomFor(len)) {
		dbugLog(_FLF, "Data too long for ThreadTextQuery - need %zu bytes to add entry %s, but buffer length %zu has only %zu available (current string is %s)",
		        len, entry, sizeof(m_statement), roomLeft(), m_statement);
		throw std::domain_error("Data too long for ThreadTextQuery");
    }

    // Add the new entry.
	m_length = strlcat(m_statement, entry, sizeof(m_statement));
}

