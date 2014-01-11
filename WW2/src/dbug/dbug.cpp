#include <syscfg.h> // Path is project specific.
// This module provides support utilities used in
// all API's to support debugging and error handling's.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "dbug/dbugInternal.h"

#include <fcntl.h>
#include <sys/stat.h>
#include "include/defs.h"
#include "teul/teulTime.h"
#include <locale.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#if platOS == OS_LINUX
#include "hostproj/revision.h"
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if platBuild == TEUL_SERVER
#include "teul/teulConfigVar.h"
extern ConfigPath cv_defaultLogPath;
#endif

#if platOS == OS_WIN32
	#include <io.h>
	#include <winsock2.h>
	#define wErrno WSAGetLastError()
	#define wSetErrno WSASetLastError
#pragma comment(lib, "ws2_32.lib")
#endif

#if !defined(_MSC_VER)
# define _O_CREAT O_CREAT
# define _O_WRONLY O_WRONLY
# define _O_TRUNC O_TRUNC
#endif

#if platOS == OS_MACOS
	#define PMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#elif platOS == OS_WIN32
	#define PMODE (_S_IREAD | _S_IWRITE)
#else
	#define PMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#endif

// set max file size to 50M for roll over to new log
static const size_t MAX_FILE_SIZE = 50 * 1000 * 1024;
// Set a maximum of 5 files
static const size_t MAX_FILE_NUM = 5;

#if	platOS == OS_WIN32
	#define vsnprintf _vsnprintf
	#define snprintf _snprintf
	#define CARRIAGE_RETURN "\r\n"
#else
	#define CARRIAGE_RETURN "\n"
#endif

static const SINT32 _stdErrFileNo = OS_FILENO(stderr);
static DebugLogger _logger;

const char* errorTypes[] = {
	"F",		// fatal error detected, exit program
	"A",		// error detected, operation aborted
	"M",		// error detected, routine recovered, operation continues
	"W",		// possible error detected, operation continues
	"N",		// note, operation continues
	"I",		// informational message
	"D"		// debug level message
};

#if platBuild == TEUL_SERVER
namespace Teul { namespace uWeb { class Server; Server* s_debugServer = nullptr; } }
#endif

//static SINT32	theError = 0;
static size_t _fileSize = 0;
static void _levelHandler(UINT32 errType, const char* const text);

#if defined(REPOSITORY_INFO)
static const char _OPEN_MSG[] = "Playnet Server Log File. Rev: " REPOSITORY_INFO CARRIAGE_RETURN;
#else
static const char _OPEN_MSG[] = "DebugMessage System Open." CARRIAGE_RETURN;
#endif

static const char _CLOSE_MSG[] = CARRIAGE_RETURN "Debug Message System Closed." CARRIAGE_RETURN;

// Global boolean that controls whether or not 'D'ebug level messages will be logged.
// Exposed globally so it can be tested before prior to performing otherwise expensive
// message constructions.
bool g_logDebug = true;

#include <mutex>
static std::mutex s_dbugInitMutex;
static bool s_dbugInitialized = false;

//////////////////////////////////////////////////////////////////////
// Set errno.

static void
_dbugSetErr(SINT32 err)
{
#if platOS == OS_WIN32
	wSetErrno(err);
#else
	errno = err;
#endif
}

//////////////////////////////////////////////////////////////////////
// Rename a file 'xxx.N.log' to 'xxx.M.log'

#if platBuild == TEUL_SERVER
static void
_roll(const char* const filename, const int baseNum)
{
	char oldName[DBUG_MAX_PATH + 1], newName[DBUG_MAX_PATH + 1];

	strprintf(oldName, "%s.%d.log", filename, baseNum);
	strprintf(newName, "%s.%d.log", filename, baseNum + 1);

	rename(oldName, newName);
}
#endif

//////////////////////////////////////////////////////////////////////
// Combine a filename with a default path if needed.

static void
_qualifyFilename(const char* filename, char* const output, const size_t outputSize)
{
	if ( filename == nullptr || filename[0] == 0 )
		filename = "logfile";

	// Already qualified.
	if ( filename[0] == '.' || filename[0] == '/' || filename[0] == '\\' || filename[1] == ':')
		strncpy(output, filename, outputSize);
	else
	{
#if platBuild == TEUL_SERVER
		const char* const defaultPath = cv_defaultLogPath.GetString();
#else
		const char* const defaultPath = getenv("DEFAULT_LOGDIR");
#endif
		if ( defaultPath && defaultPath[0] != 0 )
		{
			OS_MKDIR(defaultPath, 0770);
			snprintf(output, outputSize, SUB_PATH("%s", "%s"), defaultPath, filename);
			return;
		}

#if platOS == OS_WIN32
		// Ask Windows for a default temporary path.
		GetTempPathA(outputSize, output);
		strncat(output, "WW2OL\\", outputSize);
		OS_MKDIR(output);
#else
		strcpy(output, "/tmp/");
#endif
		strcat(output, filename);
	}
}

//////////////////////////////////////////////////////////////////////
// Open the actual debug file.

static SINT32
_openDbugFile(const char* const srcFile_)
{
	SINT32 file;
	char filename[DBUG_MAX_PATH+1];
	char ofn[DBUG_MAX_PATH+1];

	_qualifyFilename(srcFile_, filename, sizeof(filename));

	if (PN_UNLIKELY(filename != nullptr))
	{
		int oflags = _O_CREAT|_O_WRONLY|_O_TRUNC;

#if platBuild == TEUL_SERVER
		for ( int i = MAX_FILE_NUM; i >= 0; --i )
		{
			_roll(filename, i);
		}

		// Delete the last file.
		strprintf(ofn, "%s.%" PRSizeT ".log", filename, MAX_FILE_NUM + 1);
		unlink(ofn);

		strprintf(ofn, "%s.0.log", filename);
#else
		static UINT32 _fileNumber = 0;
		strprintf(ofn, "%s.%u.log", filename, _fileNumber++);

		if ( _fileNumber > MAX_FILE_NUM)
			_fileNumber = 0;
#endif

		file = OS_OPEN(ofn, oflags, PMODE);
		_fileSize = 0;

		if ( file >= 0 )
			_fileSize += OS_WRITE(file, _OPEN_MSG, sizeof(_OPEN_MSG) - 1);
	}
	else
		file = OS_FILENO(stderr);

	return file;
}

//////////////////////////////////////////////////////////////////////
// Write data to the current log file. If the data written results in
// MAX_FILE_SIZE being exceeded, then perform file roll-over.

static void
_dbugOut(const SINT32		file
			, const SINT32		logType
			, const SINT32		// errType
			, const SINT32		// flags
			, const SCHAR*		outString
			, const size_t		length
			, const	bool		rollover)
{
	if (logType != LOG_TO_FILE || file < 0)
		return;

	size_t retval = OS_WRITE(file, (void*)outString, (unsigned long)length);
	if ( file == _stdErrFileNo )
		return;

	_fileSize += retval;
	if ( PN_UNLIKELY(rollover && _fileSize >= MAX_FILE_SIZE) )
	{
		OS_CLOSE(file);
		_logger.dbugfile = _openDbugFile(_logger.program());
	}
}

void DebugLogger::initialize(SINT32 logType_, Debug::ERROR_TYPE level_, SINT32 file_, const char* program_)
{
	logType = logType_;
	dbugfile = file_;
	if ( isInitialized() == false )
	{
		logLevel = static_cast<Debug::ERROR_TYPE>(level_ & ~DBUG_FLAG_MASK);
		writeLevel = logLevel;
		logFlags = 0;
		writeFlags = 0;
	}

	err = 0;

	// transient error flags
	errType = Debug::ERR_ABORT;
	lineno = -1;

	setProgram(program_);
	setFileName(nullptr);		// Don't have an open file.
	setText(nullptr);

	initialized = true;
}

#if platOS == OS_LINUX
static void
_handleSIGUSR2(SINT32)
{
	static Debug::ERROR_TYPE oldLevel = Debug::ERR_DBUG;

	//Ignore any incoming signals
	signal(SIGUSR2, SIG_IGN);

	// safe as long as we dont use dbugSetLogLevel at same time elsewhere
	if ( !g_logDebug )
	{
		oldLevel = _logger.logLevel;
		_logger.logLevel = Debug::ERR_DBUG;
		g_logDebug = true;
	}
	else if ( oldLevel != Debug::ERR_DBUG )
	{
		_logger.logLevel = oldLevel;
		g_logDebug = false;
	}

	signal(SIGUSR2, _handleSIGUSR2);
}
#endif

#if platOS == OS_LINUX

#include <execinfo.h>

// Needs to be static to avoid allocs during signal.
static const size_t MaxBacktraceAddrs = 16;
static void* backtraceAddrs[MaxBacktraceAddrs];
static char  backtraceReport[128];

//////////////////////////////////////////////////////////////////////
// Brute-force write to log file.

static void _report(const int fd, const char* const fmt, const int num)
{
	const int len = strprintf(backtraceReport, fmt, num);
	if ( len > 0 )
		OS_WRITE(fd, (void*)backtraceReport, len);
}


//////////////////////////////////////////////////////////////////////
// Linux: On a fatal signal, write a backtrace to the log file.

void dbugDumpBacktrace(int signum)
{
	if ( _logger.isInitialized() == false || _logger.logType != LOG_TO_FILE )
		return;
	const int fd = _logger.dbugfile;
	if ( fd < 0 )
		return;

	if ( signum > 0 )
	{
		_report(fd, "\n\n", 0);
		_report(fd, "*** Process terminating due to fatal signal #%d:\n", signum);
	}

	const int numAddrs = backtrace(backtraceAddrs, MaxBacktraceAddrs);
	if ( numAddrs < 0 )
	{
		_report(fd, "Backtrace error: %d\n", numAddrs);
		return;
	}
	if ( numAddrs == 0 )
	{
		_report(fd, "backtrace available.\n", 0);
		return;
	}

	_report(fd, "BACKTRACE [Rev " REPOSITORY_INFO ", %u functions]:\n", numAddrs);

	backtrace_symbols_fd(backtraceAddrs, numAddrs, fd);

	return;
}

static int
_setSignal(int signum, void (*handler)(int))
{
	struct sigaction action;
	// Get the pre-existing setup.
	const int err = sigaction(signum, nullptr, &action);
	if ( err < 0 )
		return err;

	action.sa_handler = handler;
	sigfillset(&action.sa_mask);
	action.sa_flags = SA_RESETHAND;

	return sigaction(signum, &action, nullptr);
}
#endif

//////////////////////////////////////////////////////////////////////////
// dbugInit: sets the current error level to ERR_INFO, opens the debug
// file, and initializes the error object.

void dbugInit(SINT32 logType, const char* const filename)
{
	std::lock_guard<std::mutex> mutexLock(s_dbugInitMutex);
	if ( s_dbugInitialized )
		return;
	s_dbugInitialized = true;

	setlocale(LC_CTYPE, "");

	SINT32 file;
	Debug::ERROR_TYPE level;

	level = Debug::ERR_DBUG;
	g_logDebug = true;

#if platOS != OS_LINUX
	if ( logType != LOG_OFF)
		logType = LOG_TO_FILE;
#endif

	switch(logType)
	{
	case LOG_TO_FILE:
		file = _openDbugFile(filename);
		break;
	case LOG_OFF:
		file = -1;
		break;
	default:
		LogAbort("Invalid initial state for logging subsystem.");
	}

	_logger.initialize(logType, level, file, filename);

#if platOS == OS_LINUX
	signal(SIGUSR2, _handleSIGUSR2);
	signal(SIGPIPE, SIG_IGN);

	if ( _setSignal(SIGABRT, dbugDumpBacktrace) < 0
		|| _setSignal(SIGILL, dbugDumpBacktrace) < 0
		|| _setSignal(SIGFPE, dbugDumpBacktrace) < 0
		|| _setSignal(SIGSEGV, dbugDumpBacktrace) < 0
		|| _setSignal(SIGTERM, dbugDumpBacktrace) < 0
		|| _setSignal(SIGBUS, dbugDumpBacktrace) < 0 )
	{
		LogAbort("Unable to install signal handlers.");
	}

	static struct timeval tv;
	gettimeofday(&tv, nullptr);
	const TeulTimeMS newTimeMS = ((TeulTimeMS)tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
	if ( newTimeMS < teulClock64 )
	{
		teulClock64++;
		tv.tv_sec = (teulClock64 / 1000LL);
	}
	else
	{
		teulClock64 = newTimeMS;
		teulClockTime = (UINT32)tv.tv_sec;
	}
#endif
}

// dbugClose: closes the debug file and cleans up.
void dbugClose(void)
{
	g_logDebug = false;
	if ( _logger.isInitialized() == false )
		return;

	_logger.close();
}

void DebugLogger::close()
{
	if ( logType != LOG_TO_FILE )
		return;

	if ( dbugfile < 0 )
		return;

	_dbugOut(_logger.dbugfile, _logger.logType, _logger.errType, 0, _CLOSE_MSG, sizeof(_CLOSE_MSG)-1, false);

	if ( dbugfile > _stdErrFileNo )
		OS_CLOSE(_logger.dbugfile);

	_logger.dbugfile = -1;
}

// dbugSetErrLevel: set the current debug level for display of error messages
// and interface to the microsoft debuger through the SetLastErrorEx and
// SetDebugErrorLevel functions.
void dbugSetWriteLevel(Debug::ERROR_TYPE level)
{
	SINT32 flags;
	SINT32 temp;

	flags = level &DBUG_FLAG_MASK;
	temp = level;
	temp &= ~DBUG_FLAG_MASK;
	level = (Debug::ERROR_TYPE)temp;

	if ( _logger.isInitialized() == false )
		return;

	_logger.writeFlags = flags;

	switch(level)
	{
	case Debug::ERR_FATAL:
	case Debug::ERR_ABORT:
	case Debug::ERR_MINOR:
	case Debug::ERR_WARN:
	case Debug::ERR_NOTE:
	case Debug::ERR_INFO:
	case Debug::ERR_DBUG:
		_logger.writeLevel = level;
		break;
	default:
		return;
	}

#if platOS == OS_WIN32
//	if (IsDebuggerPresent())
		SetDebugErrorLevel(level);
#endif
}

// dbugSetErrLevel: set the current debug level for display of error messages
// and interface to the microsoft debuger through the SetLastErrorEx and
// SetDebugErrorLevel functions.
void dbugSetLogLevel(Debug::ERROR_TYPE level)
{
	if ( _logger.isInitialized() == false )
		return;

	SINT32 flags = level & DBUG_FLAG_MASK;
	level = static_cast<Debug::ERROR_TYPE>(level & ~DBUG_FLAG_MASK);

	if ( level == Debug::ERR_DBUG )
		g_logDebug = true;
	else
		g_logDebug = false;

	if ( _logger.isInitialized() == false )
		return;

	_logger.logFlags = flags;

	switch(level)
	{
	case Debug::ERR_FATAL:
	case Debug::ERR_ABORT:
	case Debug::ERR_MINOR:
	case Debug::ERR_WARN:
	case Debug::ERR_NOTE:
	case Debug::ERR_INFO:
	case Debug::ERR_DBUG:
		_logger.logLevel = level;
		break;
	default:
		return;
	}
}

// dbugSetErrno: sets Errno. Error numbers adhear to errno.h. Under win32
// "dbugWErrno.h" may also be included to map windows socket errors to unix
// errno.  Additional errors beyond those covered in errno.h and dbugWErrno.h
// must be added to debugExtErrno.h and must adhear to windows error code
// conventions to eliminate problems with compataibility.
void dbugSetErrno(SINT32 err)
{
    _dbugSetErr(err);
}

// dbugGetErrno: returns the current error number.  Error numbers adhear to
// errno.h. Under win32 "dbugWErrno.h" may also be included to map windows
// socket errors to unix errno.  Additional errors beyond those covered in
// errno.h and dbugWErrno.h must be added to debugExtErrno.h and must adhear
// to windows error code conventions to eliminate problems with compataibility.
SINT32	dbugGetErrno(void)
{
#if platOS == OS_WIN32
	return wErrno;
#else
    return errno;
#endif
}

// _dbugLogPrivate: if the current filename, lineno or string is NULL then it
// sets these members.  The parameters are output as a string to the debug
// file depending on the debug level selected and the current error type.
static inline SINT32
_dbugLogPrivate(bool isLog, const char* const filename, const SINT32 lineno, const UINT32 errType, const char* const fmt, va_list args)
{
	size_t len = 0;
	SINT32	retval = 0, flags;
	const char* myName;
	UINT32 minLevel;

	if ( PN_UNLIKELY(_logger.isInitialized() == false) )
		_logger.initialize(LOG_TO_FILE,  Debug::ERR_INFO, _stdErrFileNo, "not_initialized");

	_logger.isLog = isLog;

	if (_logger.dbugfile < 0)
		return 0;

	if (isLog)
		minLevel = _logger.logLevel;
	else
		minLevel = _logger.writeLevel;

	flags = (_logger.isLog) ? _logger.logFlags : _logger.writeFlags;
	flags |= (errType & DBUG_FLAG_MASK);
	_logger.flags = flags;

	if (PN_LIKELY(errType != Debug::ERR_FATAL))
		_logger.errType = errType & ~DBUG_FLAG_MASK;
	else
		_logger.errType = errType;

	_logger.err = dbugGetErrno();

	if ( PN_LIKELY((_logger.errType != Debug::ERR_FATAL) && (_logger.errType > minLevel)) )
		return 0;

	if ( (myName = strrchr(filename,'/')) != nullptr)
		myName++;
	else if ( (myName = strrchr(filename,'\\')) != nullptr)
		myName++;
	else
		myName = filename;

	_logger.setFileName(myName);
	_logger.lineno = lineno;

	if ( ! (flags & BARE_DEBUG) )
	{
		time_t now = (time_t)(teulClock64 / 1000);
		struct tm tmStruct;
		struct tm* tm = &tmStruct;
		tm = localtime_r(&now, &tmStruct);
		len = _logger.formatText(CARRIAGE_RETURN "%s [%04u-%02u-%02u %02u:%02u.%02u.%03u %s:%u] "
								, errorTypes[_logger.errType]
								, tm->tm_year + 1900
								, tm->tm_mon + 1
								, tm->tm_mday
								, tm->tm_hour
								, tm->tm_min
								, tm->tm_sec
								, (UINT32)(teulClock64 % 1000)
								, _logger.file()
								, _logger.lineno);
		_dbugOut(_logger.dbugfile, _logger.logType, _logger.errType, flags, _logger.text(), len, false);
	}

	const char* text = nullptr;
	if ( PN_UNLIKELY(fmt == nullptr) )
		len = 0;
	else
	{
		len = _logger.vformatText(fmt, args);
		if ( len == (size_t)~0 )
		{
			text = "[ERROR]";
			len = strlen(text);
		}
		else
			text = _logger.text();
	}

	if ( PN_UNLIKELY(len == 0) )
	{
		text = "[NULL]";
		len = strlen(text);
	}

	_dbugOut(_logger.dbugfile, _logger.logType, _logger.errType, flags, text, len, true);

	if ( (flags & LOG_ALL_TO_STDERR) || ((flags & STD_ERR) && _logger.errType <= Debug::ERR_MINOR) )
	{
		if ( flags & BARE_DEBUG )
			printf("%s", text);
		else
			printf("%s%s", CARRIAGE_RETURN, text);
	}

	_levelHandler(_logger.errType, text);

	if (retval < 0)
		return retval;
	else
		return (SINT32)len;
}

// dbugLogV: Log the (already started) va_list.
SINT32 _dbugLogV(const char* const filename, const SINT32 lineno, const UINT32 errType, const char* const fmt, va_list& args)
{
	SINT32 result = 0;
	if ( PN_UNLIKELY(_logger.isInitialized() == false) )
	{
		if ( PN_UNLIKELY(errType == Debug::ERR_FATAL) )
		{
			char text[DBUG_MAX_STRING + 1];
			vsnprintf(text, sizeof(text), fmt, args);
			_levelHandler(errType, text);
		}
	}
	else
	{
		if ( !(errType != Debug::ERR_FATAL && (errType & ~DBUG_FLAG_MASK) > _logger.logLevel) )
			result = _dbugLogPrivate(TRUE, filename,lineno, errType, fmt, args);
	}

	return result;
}

// dbugLog: if the current filename, lineno or string is NULL then it
// sets these members.  The parameters are output as a string to the debug
// file depending on the debug level selected and the current error type.
SINT32 _dbugLog(const char* const filename, const SINT32 lineno, const UINT32 errType, const char* const fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const SINT32 retval = _dbugLogV(filename, lineno, errType, fmt, args);
	va_end(args);
	return retval;
}

//_dbugWrite: same as above for dbugWrite
SINT32 _dbugWrite(char* filename, SINT32 lineno, UINT32 errType, char* fmt, ...)
{
	va_list args;

//	theError = dbugGetErrno();

	va_start(args, fmt);
	const SINT32 retval = _dbugLogPrivate(FALSE, filename,lineno,errType,fmt,args);
	va_end(args);

	return retval;
}

void _levelHandler(UINT32 errType, const char* const text)
{
	if ( PN_LIKELY(errType != Debug::ERR_FATAL) )
		return;

	fprintf(stderr, "%s\n", text);

	abort();
}

//////////////////////////////////////////////////////////////////////
// "Pretty" logging routine.

void NORETURN_FN
_LogAbort(const char* file_, unsigned int line_, unsigned int error_, const char* assertion_, const char* fmt_, ...)
{
	va_list args;
	va_start(args, fmt_);

#if platOS == OS_WIN32
	if ( assertion_ == NULL || *assertion_ == 0 )
		assertion_ = "";

	const size_t roughSize = vsnprintf(_logger.m_text, sizeof(_logger.m_text), fmt_, args);
	va_end(args);
	const size_t maxSize = roughSize + 256 + strlen(assertion_);
	char* message = (char*)OS_ALLOCA(maxSize * sizeof(char));
	if ( PN_UNLIKELY(message == nullptr) )
	{
		MessageBoxA(0, "OUT OF MEMORY", "OUT OF MEMORY", MB_ICONERROR | MB_OK | MB_APPLMODAL);
		abort();
	}
	const char* caption;
	if ( assertion_[0] )
	{
		caption = "Critical Error (ASSERT)";
		sprintf_s(message, maxSize, "File: %s\nLine: %u\nAssert: %s\nError: %s\n", file_, line_, assertion_, _logger.m_text);
	}
	else
	{
		caption = "Critical Error";
		sprintf_s(message, maxSize, "File: %s\nLine: %u\nError: %s\n", file_, line_, _logger.m_text);
	}
	MessageBoxA(0, message, caption, MB_OK | MB_ICONERROR | MB_APPLMODAL);

#if defined(_DEBUG)
	__debugbreak();
#endif

	exit(error_ != 0 ? error_ : -1);
#else
	if ( PN_LIKELY(_logger.isInitialized()) )
	{
		if ( assertion_ != nullptr && *assertion_ != 0 )
			_dbugLog(file_, line_, error_, "ASSERTION: %s", assertion_);
		_dbugLogPrivate(FALSE, file_, line_, error_, fmt_, args);
	}

	vfprintf(stderr, fmt_, args);
	va_end(args);

	abort();
#endif
}
