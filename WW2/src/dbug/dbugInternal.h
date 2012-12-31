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

#if platOS == OS_WIN32
#define vsnprintf _vsnprintf
#endif

static const UINT32 DBUG_FLAG_MASK = 0xfffffff0 ;	// flags mask
static const UINT32 DBUG_MAX_STRING = 4224 ;
static const UINT32 DBUG_MAX_PATH = 128;

class DebugLogger
{
public:
	DebugLogger()
		:	initialized(false)
		,	dbugfile(0)
		,	logType(0)
		,	isLog(0)
		,	writeLevel(0)
		,	logLevel(0)
		,	writeFlags(0)
		,	logFlags(0)
		,	flags(0)
		,	errType(0)
		,	err(0)
		,	lineno(0)
		,	m_program(NULL), m_file(NULL)
		{
			m_text[0] = m_text[sizeof(m_text)-1] = 0 ;
		}

public:
	const char* file() const { return m_file ; }
	const char* program() const { return m_program ; }
	const char* text() const { return m_text ; }

	void setFileName(const char* src) { m_file = src ; }
	void setProgram(const char* src) { m_program = src ; }
	void setText(char* src) { if ( src ) strncpy(m_text, src, DBUG_MAX_STRING) ; else m_text[0] = 0 ; m_text[DBUG_MAX_STRING-1] = 0 ; }

	int vformatText(const char* fmt, va_list args)
	{
		const int result = std::min(vsnprintf(m_text, DBUG_MAX_STRING, fmt, args), (int)DBUG_MAX_STRING - 1) ;
		if ( result >= 0 )
		{
			m_text[result] = 0 ;
		}
		return result ;
	}

	int formatText(const char* fmt, ...)
	{
		int len = 0 ;
		va_list args ;
		va_start(args, fmt) ;
		len = vsnprintf(m_text, DBUG_MAX_STRING, fmt, args) ;
		va_end(args) ;
		m_text[DBUG_MAX_STRING-1] = 0 ;
		return len ;
	}

	void initialize(SINT32 logType_, Debug::ERROR_TYPE level_, SINT32 file_, const char* program_);
	bool isInitialized() const { return initialized; }

	void close();

public:
	bool		initialized;
	SINT32		dbugfile;
	SINT32		logType;
	LOGICAL		isLog;
	UINT32		writeLevel;
	UINT32		logLevel;
	SINT32		writeFlags;
	SINT32		logFlags;
	SINT32		flags;
	UINT32		errType;
	SINT32		err;
	SINT32		lineno;
	const char*	m_program;
	const char*	m_file;
	char		m_text[DBUG_MAX_STRING + 1] ;
} ;
