#pragma once
// This module provides the debug print utility.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Marty Poulin

#include <cstdarg>
#include <errno.h>
#include "log/log.h"

#if platBuild == BUILD_SERVER || defined(DEBUG) || defined(ALPHA_RELEASE)
#define  _FL __FILE__, __LINE__
#else
#define  _FL "", 0
#endif

namespace Debug
{
	enum
	{
		ERR_FATAL = 0,		// fatal error detected, exit program
		ERR_ABORT,			// error detected, operation aborted
		ERR_WARN,			// error detected, operation continues
		ERR_MINOR,			// possible error detected, routine recovered, operation continues
		ERR_NOTE,			// normal but significant
		ERR_INFO,			// informational message
		ERR_DBUG,			// debug level message
	};
	using ERROR_TYPE = UINT32;
	using ERROR_HANDLER = void(*)(void);
}

#define STD_ERR				0x0010	// mirror significant output to stderr
#define LOG_ALL_TO_STDERR	0x0100	// ALL logging to stderr

// always use these macros in place of Full functions so filename and number and errType are set.
// basic parameter macros for dbugLog
#define _FLF _FL, Debug::ERR_FATAL
#define _FLA _FL, Debug::ERR_ABORT
#define _FLM _FL, Debug::ERR_MINOR
#define _FLW _FL, Debug::ERR_WARN
#define _FLN _FL, Debug::ERR_NOTE
#define _FLI _FL, Debug::ERR_INFO
#define _FLD _FL, Debug::ERR_DBUG

// Whether debugging is enabled or not.
enum class LogMode { OFF, TO_FILE };

// Each ERROR_TYPE is associated with a syslog error level on linux
//
//	ERR_FATAL       = LOG_EMRG	-fatal error, system is unusable
//	ERR_ERRALERT	= LOG_ALERT	-action should be taken immediately
//	ERR_ABORT       = LOG_CRIT	-error condition, operation aborted
//	ERR_MINOR       = LOG_ERR	-error detected, routine recovered, operation continues
//	ERR_WARN        = LOG_WARN	-possible error detected, operation continues
//	ERR_NOTE		= LOG_NOTE	-normal but significant
//	ERR_INFO		= LOG_INFO	-informational message
//	ERR_DBUG		= LOG_DEBUG	-debug level message

namespace Debug
{
	using ERROR_HANDLER = void(*)();
}

// dbugInit: sets the current error level to ERR_INFO, opens the debug file, and initializes
// the error object.
void dbugInit(LogMode logMode, const char* filename);

// dbugClose: closes the debug file and cleans up.
void dbugClose();

// dbugSetLogLevel: set the current debug level for display of log messages.
void dbugSetLogLevel(Debug::ERROR_TYPE level);

// dbugSetWriteLevel: set the current debug level for display of dbug messages and interface to
// the Microsoft debugger through the SetDebugErrorLevel function.
void dbugSetWriteLevel(Debug::ERROR_TYPE level);

// dbugSetErrno and dbugGetErrno: Pointless wrappers around errno
void dbugSetErrno(int err);
int dbugGetErrno();

// dbugLog: if the current filename, lineno or string is nullptr then it sets these members. The
// parameters are output as a string to the debug file depending on the debug level selected and
// the current error type.  calls handlers.
int _dbugLog(const char* filename, int lineno, UINT32 errType, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(4,5);

// dbugWrite: Same as dbugLog but has its own logging level. Don't know why this exists.
SINT32 _dbugWrite(const char* filename, SINT32 lineno, UINT32 errType, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(4,5);

// LogAbort: Fire a ERR_FATAL and abort()
NORETURN_FN void _LogAbort(const char* filename, unsigned int lineno, UINT32 errType, const char* assertion, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(5,6);

// We used to provide the ability to disable the logging functions globally. We no longer do so.
#define dbugLog(m, fmt, ...) _dbugLog(m, fmt, ## __VA_ARGS__)
#define dbugWrite(m, fmt, ...) _dbugWrite(m, fmt, ## __VA_ARGS__)

#define LogAbort(fmt, ...) _LogAbort(_FLF, nullptr, fmt, ## __VA_ARGS__)

// These macros assume that the condition is unlikely.
#define ABORT_UNLESS_EX(assertion_, fmt, ...) do { if (PN_UNLIKELY(!(assertion_))) { _LogAbort(_FLF, #assertion_, fmt, ## __VA_ARGS__); } } while (0)
#define ABORT_IF_EX(assertion_, fmt, ...)     do { if (PN_UNLIKELY((assertion_)))  { _LogAbort(_FLF, #assertion_, fmt, ## __VA_ARGS__); } } while (0)
#define ABORT_UNLESS(assertion_) ABORT_UNLESS_EX(assertion_, "Failed !(%s)", #assertion_)
#define ABORT_IF(assertion_)     ABORT_IF_EX(assertion_, "Failed %s", #assertion_)

#if platBuild == BUILD_SERVER
    #define DEBUG_ERR(fmt, ...) dbugLog(_FLA, fmt, ## __VA_ARGS__)
    #define DEBUG_FATAL(fmt, ...) LogAbort(fmt, ## __VA_ARGS__)
#else
    #define DEBUG_ERR(fmt, ...) WriteLog(LOG_ALL, LOG_WARN, fmt, ## __VA_ARGS__)
    #define DEBUG_FATAL(fmt, ...) WriteLog(LOG_ALL, LOG_FATAL, fmt, ## __VA_ARGS__)
    //#define DEBUG_HERE Write_Where(__FILE__, __LINE__, __FUNCTION__, LOG_ALL, LOG_DEBUG, "Entered", ## __VA_ARGS__)
#endif

extern const char* sane_basename(const char* const filepath) noexcept;
