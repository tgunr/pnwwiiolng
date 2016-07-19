#pragma once
// This module provides the debug print utility.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Marty Poulin

#include <cstdarg>

#ifndef EWOULDBLOCK
	#include	<errno.h>
#endif

#if platOS == OS_WIN32
	#include "dbug/dbugWErrno.h"
#endif

#if defined(DEBUG) || defined(ALPHA_RELEASE)
#define  _FL __FILE__, __LINE__
#else
#define  _FL "", 0
#endif

namespace Debug
{
	enum
	{
		ERR_FATAL	= 	0,		// fatal error detected, exit program
		ERR_ABORT,				// error detected, operation aborted
		ERR_MINOR,				// error detected, routine recovered, operation continues
		ERR_WARN,				// possible error detected, operation continues
		ERR_NOTE,				// normal but significant
		ERR_INFO,					// informational message
		ERR_DBUG					// debug level message
	,	ERROR_TYPE_SIZE
	};
	using ERROR_TYPE = UINT32;
}

#define STD_ERR				0x0010	// mirror significant output to stderr
#define BARE_DEBUG			0x0080	// no extra formating, basically printf
#define LOG_ALL_TO_STDERR	0x0100	// ALL logging to stderr

// always use these macro's in place of Full functions so filename and number and errType are set.
// basic parameter macros for dbugLog
#define _FLF _FL, Debug::ERR_FATAL
#define _FLA _FL, Debug::ERR_ABORT
#define _FLM _FL, Debug::ERR_MINOR
#define _FLW _FL, Debug::ERR_WARN
#define _FLN _FL, Debug::ERR_NOTE
#define _FLI _FL, Debug::ERR_INFO
#define _FLD _FL, Debug::ERR_DBUG

// same as above but no extra formating
#define _FLFB _FL, (Debug::ERR_FATAL | BARE_DEBUG)
#define _FLAB _FL, (Debug::ERR_ABORT | BARE_DEBUG)
#define _FLMB _FL, (Debug::ERR_MINOR | BARE_DEBUG)
#define _FLWB _FL, (Debug::ERR_WARN | BARE_DEBUG)
#define _FLNB _FL, (Debug::ERR_NOTE | BARE_DEBUG)
#define _FLIB _FL, (Debug::ERR_INFO | BARE_DEBUG)
#define _FLDB _FL, (Debug::ERR_DBUG | BARE_DEBUG)

// make go to stderr
#define _FLFS _FL, (Debug::ERR_FATAL | STD_ERR)
#define _FLAS _FL, (Debug::ERR_ABORT | STD_ERR)
#define _FLMS _FL, (Debug::ERR_MINOR | STD_ERR)
#define _FLWS _FL, (Debug::ERR_WARN | STD_ERR)
#define _FLNS _FL, (Debug::ERR_NOTE | STD_ERR)
#define _FLIS _FL, (Debug::ERR_INFO | STD_ERR)
#define _FLDS _FL, (Debug::ERR_DBUG | STD_ERR)

// same as above but no extra formating
#define _FLFSB _FL, (Debug::ERR_FATAL | STD_ERR | BARE_DEBUG)
#define _FLASB _FL, (Debug::ERR_ABORT | STD_ERR | BARE_DEBUG)
#define _FLMSB _FL, (Debug::ERR_MINOR | STD_ERR | BARE_DEBUG)
#define _FLWSB _FL, (Debug::ERR_WARN | STD_ERR | BARE_DEBUG)
#define _FLNSB _FL, (Debug::ERR_NOTE | STD_ERR | BARE_DEBUG)
#define _FLISB _FL, (Debug::ERR_INFO | STD_ERR | BARE_DEBUG)
#define _FLDSB _FL, (Debug::ERR_DBUG | STD_ERR | BARE_DEBUG)

// the log type parameter of dbugInit can take any of the log facilities from
// syslog including LOG_LOCAL0-7 or be set to LOG_TO_FILE to log
// directly to a file only.
#define LOG_TO_FILE 1
#define LOG_OFF	-1

// Each ERROR_TYPE is associated with a syslog error level on linux
//
//	ERR_FATAL	= LOG_EMRG	-fatal error, system is unusable
//	ERR_ERRALERT	= LOG_ALERT	-action should be taken immediately
//	ERR_ABORT	= LOG_CRIT	-error condition, operation aborted
//	ERR_MINOR	= LOG_ERR	-error detected, routine recovered, operation continues
//	ERR_WARN	= LOG_WARN	-possible error detected, operation continues
//	ERR_NOTE		= LOG_NOTE	-normal but significant
//	ERR_INFO		= LOG_INFO	-informational message
//	ERR_DBUG		= LOG_DEBUG	-debug level message

namespace Debug
{
	using ERROR_HANDLER = void(*)(void);
}

// dbugInit: sets the current error level to ERR_INFO, opens the debug file, and initializes
// the error object. The log type parameter of dbugInit can take any of the log facilities from
// syslog including LOG_LOCAL0-7 or be set to LOG_TO_FILE to log
// directly to a file only.
void dbugInit(int logType, const char* filename);

// dbugClose: closes the debug file and cleans up.
void dbugClose(void);

// dbugSetLogLevel: set the current debug level for display of log messages.
void dbugSetLogLevel(Debug::ERROR_TYPE level);

// dbugSetWriteLevel: set the current debug level for display of dbug messages and interface to
// the Microsoft debugger through the SetDebugErrorLevel function.
void dbugSetWriteLevel(Debug::ERROR_TYPE level);

// dbugSetErrno: sets Errno. Error numbers adhere to errno.h. Under win32 "dbugWErrno.h"
// may also be included to map windows socket errors to Unix errno.  Additional errors beyond
// those covered in errno.h and dbugWErrno.h must be added to debugExtErrno.h and must adhere to
// windows error code conventions to eliminate problems with compatibility.
void dbugSetErrno( int err);

// dbugGetErrno: returns the current error number.  Error numbers adhere to errno.h.  Under
// win32 "dbugWErrno.h" may also be included to map windows socket errors to Unix errno.
// Additional errors beyond those covered in errno.h and dbugWErrno.h must be added to
// debugExtErrno.h and must adhere to windows error code conventions to eliminate problems
// with compatibility.
int dbugGetErrno(void);

// dbugLog: if the current filename, lineno or string is nullptr then it sets these members. The
// parameters are output as a string to the debug file depending on the debug level selected and
// the current error type.  calls handlers.
int _dbugLog(const char* filename, int lineno, UINT32 errType, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(4,5);

// dbugWrite: Same as dbugLog but has its own logging level. Don't know why this exists.
SINT32 _dbugWrite(const char* filename, SINT32 lineno, UINT32 errType, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(4,5);

// LogAbort: Fire a ERR_FATAL and abort()
[[noreturn]] void _LogAbort(const char* filename, unsigned int lineno, UINT32 errType, const char* assertion, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(5,6);

// Disable dbugLog() and dbugWrite() if respectively requested.
#ifdef NO_DEBUG_LOG
    #define dbugLog(m, fmt, ...)
#else
    #define dbugLog(m, fmt, ...) _dbugLog(m, fmt, ## __VA_ARGS__)
#endif

#ifdef NO_DEBUG_WRITE
    #define dbugWrite(m, fmt, ...)
#else
	#define dbugWrite(m, fmt, ...) _dbugWrite(m, fmt, ## __VA_ARGS__)
#endif

#define LogAbort(fmt, ...) _LogAbort(_FLF, nullptr, fmt, ## __VA_ARGS__)

// These macros assume that the condition is unlikely.
#define ABORT_UNLESS_EX(assertion_, fmt, ...) do { if (PN_UNLIKELY(!(assertion_))) { _LogAbort(_FLF, #assertion_, fmt, ## __VA_ARGS__); } } while (0)
#define ABORT_IF_EX(assertion_, fmt, ...)     do { if (PN_UNLIKELY((assertion_)))  { _LogAbort(_FLF, #assertion_, fmt, ## __VA_ARGS__); } } while (0)
#define ABORT_UNLESS(assertion_) ABORT_UNLESS_EX(assertion_, "Failed !(%s)", #assertion_)
#define ABORT_IF(assertion_)     ABORT_IF_EX(assertion_, "Failed %s", #assertion_)

// Define betaLog()
#if defined(BETA_RELEASE)
    #define betaLog(m, fmt, ...) dbugLog(m, fmt, ## __VA_ARGS__)
#else
    #define betaLog(m, fmt, ...)
#endif

// This is set to true when the dbug library is logging _FLD messages
extern bool g_logDebug;

#ifndef FRONT_END
    #define DEBUG_ERR(fmt, ...) dbugLog(_FLA, fmt, ## __VA_ARGS__)
    #define DEBUG_FATAL(fmt, ...) LogAbort(fmt, ## __VA_ARGS__)
#else
    #define DEBUG_ERR(fmt, ...) Write_Debug(fmt, ## __VA_ARGS__)
    #define DEBUG_FATAL(fmt, ...) Write_Log(LOG_ALL, LOG_FATAL, fmt, ## __VA_ARGS__)
#endif
