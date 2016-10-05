#pragma once
// Created: Tue Oct 21 17:31:09 CDT 2008
//
// Encapsulation of the concept of a thread text buffer to which the
// application feeds small queries that need to be aggregated
// and either commit automatically when the buffer is filled
// or periodically when no new data has been added.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"

class ThreadTextQuery
{
public:
	ThreadTextQuery(DBThread::DBNo db_, const char* const prefix, const char* const suffix)
		: m_db(db_)
		, m_prefix(prefix)
		, m_suffix(suffix)
		, m_sufLen(strlen(suffix))
		, m_length(0)
		, m_commitFrom(0LL)
	{
		m_statement[0] = 0;
		assert( strlen(m_prefix) + m_sufLen < sizeof(m_statement) - 1 );
	}
	~ThreadTextQuery() { commit(0LL); }

	// Commit and hand-off the existing buffer if this data would fill the buffer;
	// Allocate a new buffer if required;
	// Add this text to the end of the buffer.
	void addEntry(const char* const entry);

	// Commit the buffer if maxAge milliseconds have elapsed since the last
	// commit (thus if maxAge = 0 it will force a commit)
	bool commit(const TeulTimeMS maxAge);

	// Return true if the number of bytes specified would exceed the available
	// space left in the current buffer.
	// current length + separator (,) plus new string length plus suffix string length plus null byte
	bool noRoomFor(size_t lenBytes) { return m_length + 1 + lenBytes + m_sufLen + 1 >= sizeof(m_statement) - 1; }

	size_t roomLeft() const { return TEXT_POOL_SIZE - 1 - m_sufLen - 1 - m_length; }

	// Return the separator character - in case I decide to make it an option later.
	char separator() const { return ','; }

private:
	DBThread::DBNo		m_db;
	const char* const	m_prefix;
	const char* const	m_suffix;
	const size_t		m_sufLen;

	char				m_statement[TEXT_POOL_SIZE + 1];
	size_t				m_length;
	TeulTimeMS			m_commitFrom;	// Timestamp from which we decide whether commit maturity has been reached

	ThreadTextQuery& operator = (const ThreadTextQuery& rhs) = delete;
};

