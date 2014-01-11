#include <syscfg.h> // Path is project dependent.
// Created: Sun Nov 20 07:57:56 CST 2011
// Originating Programmer: Oliver "kfsone" Smith
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.

#include "teul/teulMicroWebServer.h"

extern TeulTimeMS teulClock64;

struct Header
{
	static Header* FromPointer(void* const ptr)
	{
		return ((Header*)ptr) - 1;
	}

	size_t				m_allocSize;
	struct Activity*	m_activity;
};

struct Activity
{
	Activity() : m_allocs(0), m_frees(0), m_unfreed(0), m_maxAllocated(0), m_curAllocated(0), m_lastAccess(0) {}

	unsigned long long	m_allocs;
	unsigned long long	m_frees;
	unsigned long long	m_unfreed;
	unsigned long long	m_maxAllocated;
	unsigned long long	m_curAllocated;
	TeulTimeMS			m_lastAccess;

	void describe(unsigned long long fileline, char* const buffer, const size_t bufferSize)
	{
		const char* const file = ((const char*)(fileline >> 32));
		const size_t line = (size_t)(fileline);
		snprintf(buffer, bufferSize - 1
				, "%s:%" PRSizeT " allocs:%llu frees:%llu unfreed:%llu maxAllocated:%llu curAllocated:%llu la:%llu"
					, file, line
					, m_allocs
					, m_frees
					, m_unfreed
					, m_maxAllocated
					, m_curAllocated
					, teulClock64 - m_lastAccess
				);
	}

	size_t describeJSON(unsigned long long fileline, char* const buffer, const size_t bufferSize, const size_t offset)
	{
		const char* const file = ((const char*)(fileline >> 32));
		const size_t line = (size_t)(fileline);

		size_t printed = snprintf(buffer + offset, bufferSize - (1 + offset)
									, "\"%s:%" PRSizeT "\":{\"allocs\":%llu,\"frees\":%llu,\"unfreed\":%llu,\"max\":%llu,\"cur\":%llu,\"la\":%llu}"
										, file, line
										, m_allocs
										, m_frees
										, m_unfreed
										, m_maxAllocated
										, m_curAllocated
										, teulClock64 - m_lastAccess
										);

		return offset + printed;
	}

	void alloc(Header* const ptr_, const size_t amount_)
	{
		m_allocs++;
		m_unfreed++;
		m_curAllocated += amount_;
		if ( m_curAllocated > m_maxAllocated )
			m_maxAllocated = m_curAllocated;
		m_lastAccess = teulClock64;
		ptr_->m_activity = this;
	}

	void free(Header* const ptr_)
	{
		m_frees++;
		m_unfreed--;
		m_curAllocated -= ptr_->m_allocSize;
		m_lastAccess = teulClock64;
	}
};

static unsigned long long _fileLineToLong(const char* const file_, const size_t line_)
{
	return ((unsigned long long)file_) << 32 | (unsigned long long)line_;
}

struct HISTORY
{
	HISTORY()
#ifdef PN_MEM_DEBUG
		: m_inUse(true), m_mutex(), m_data() {}
#else
		: m_inUse(false) {}
#endif
	~HISTORY()
	{
		if (PN_LIKELY(m_inUse)) {
			m_inUse = false;
			std::lock_guard<std::mutex> lock(m_mutex);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	volatile bool m_inUse;
	std::mutex m_mutex;

	std::unordered_map<unsigned long long, Activity> m_data;
} s_history;

void*
memCallocDebug(const size_t units_, const size_t size_, const char* const file_, const size_t line_)
{
	unsigned long long flLong = _fileLineToLong(file_, line_);

	const size_t allocSize = units_ * size_;
	Header* ptr = (Header*)calloc(1, sizeof(Header) + allocSize);
	if ( PN_UNLIKELY(ptr == nullptr) )
		return ptr;

	if ( PN_LIKELY(s_history.m_inUse) )
	{
		std::lock_guard<std::mutex> lock(s_history.m_mutex);
		Activity& act = s_history.m_data[ flLong ];
		act.alloc(ptr, size_);
	}

	ptr->m_allocSize = allocSize;

	return ptr + 1;
}

void*
memReallocDebug(void* const oldPtr_, const size_t newSize_, const char* const file_, const size_t line_)
{
	unsigned long long flLong = _fileLineToLong(file_, line_);

	Header* const oldHeader = Header::FromPointer(oldPtr_);
	const size_t oldSize = oldHeader->m_allocSize;
	ABORT_IF(oldSize == 0);

	Header* newPtr = (Header*)malloc(sizeof(Header) + newSize_);
	memcpy(newPtr, oldHeader, sizeof(Header) + std::min(newSize_, oldSize));
	Activity* const oldAct = newPtr->m_activity;

	if ( PN_LIKELY(s_history.m_inUse) )
	{
		std::lock_guard<std::mutex> lock(s_history.m_mutex);
		Activity& newAct = s_history.m_data[ flLong ];
		oldAct->free(newPtr);
		newAct.alloc(newPtr, newSize_);
	}

	newPtr->m_allocSize = newSize_;

#ifdef DEV_BUILD
	memset(oldHeader, 0xbb, oldSize);
#endif

	free(oldHeader);

	return newPtr + 1;
}

void
memFreeDebug(void* const oldPtr_, const char* const file_, const size_t line_)
{
	Header* header = Header::FromPointer(oldPtr_);
	ABORT_IF(header->m_allocSize == 0);
	if ( PN_LIKELY(s_history.m_inUse) )
	{
		std::lock_guard<std::mutex> lock(s_history.m_mutex);
		header->m_activity->free(header);
	}

#ifdef DEV_BUILD
	const size_t size = header->m_allocSize;
	memset(header, 0xcc, size);
#endif
	free(header);
}

#if defined(MEMORY_DEBUG)
void
memReportJSON(unsigned int fd, const char* const /*path*/, const char* const /*args*/)
{
	char buffer[132 * 200];
	size_t offset = 0;

	const TeulTimeMS cutoff = teulClock64 - (5 * 60 * 1000);

	if ( PN_LIKELY(s_history.m_inUse) )
	{
		std::lock_guard<std::mutex> lock(s_history.m_mutex);
		const auto end = s_history.m_data.end();
		for ( auto it = s_history.m_data.begin(); it != end; ++it )
		{
			if ( it->second.m_lastAccess < cutoff && it->second.m_unfreed == 0 )
				continue;
			offset += sprintf(buffer + offset, "%s", offset ? "," : "{");
			offset = it->second.describeJSON(it->first, buffer, sizeof(buffer), offset);
		}
	}
	else
		return;

	offset += sprintf(buffer + offset, "\r\n}\r\n");

	send(fd, buffer, offset, 0);
}
#endif

void memReport()
{
	fprintf(stderr, "Memory Report:\n");
	char buffer[128];
	for ( auto it = s_history.m_data.begin(), end = s_history.m_data.end(); it != end; ++it )
	{
		it->second.describe(it->first, buffer, sizeof(buffer));
		fprintf(stderr, "%s\n", buffer);
	}
}

void memInit()
{
#if defined(MEMORY_DEBUG)
	atexit(memReport);

	if ( Teul::uWeb::s_debugServer != nullptr )
		Teul::uWeb::s_debugServer->addVirtualFile("Debug Memory Usage Statistics", "/mem.json", memReportJSON);
#endif
}
