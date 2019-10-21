#pragma once
// Date: Wed Apr 13 15:45:40 CDT 2005
//
// Stuff for dbug.cpp and friends that not everyone needs to see
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#ifndef platOS
#pragma error "Must have included platform.h first"
#endif

#include "str/stringLib.h"

#if platOS == OS_WIN32
#define vsnprintf _vsnprintf
#endif

constexpr UINT32 DBUG_FLAG_MASK = 0xfffffff0;	// flags mask
constexpr UINT32 DBUG_MAX_STRING = 4224;
constexpr UINT32 DBUG_MAX_PATH = 128;

struct DebugLogger
{
	DebugLogger() noexcept {}

	const char* program() const noexcept { return m_program; }
	const char* text() const noexcept { return m_text; }

	void setProgram(const char* src) noexcept { m_program = src; }

	UINT32 vformatText(const char* fmt, va_list args) noexcept
	{
		UINT32 len = static_cast<UINT32>(vsnprintf(m_text, DBUG_MAX_STRING, fmt, args));
		if (len < DBUG_MAX_STRING)
			m_text[len++] = '\n';
		else
			m_text[DBUG_MAX_STRING] = '\n';
		return len;
	}

	template<typename... Args>
	UINT32 formatText(const char* fmt, Args&&... args) noexcept
	{
		return strprintf(m_text, fmt, std::forward<Args>(args)...);
	}

	void initialize(LogMode logMode, Debug::ERROR_TYPE level, SINT32 file, const char* program) noexcept;
	bool isInitialized() const noexcept { return m_initialized; }

	void close() noexcept;

public:
	bool			m_initialized;
	int				m_dbugfile;
	LogMode			m_logMode;
	unsigned int	m_writeLevel;
	unsigned int	m_logLevel;
	unsigned int	m_writeFlags;
	unsigned int	m_logFlags;
	//unsigned int	m_flags;
	//unsigned int	m_errType;
	//int				m_lineno;  // can be -1
	const char*		m_program;
	//const char*		m_file;
	static thread_local char m_text[DBUG_MAX_STRING + 1];
};
