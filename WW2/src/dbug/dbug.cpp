// This module provides support utilities used in
// all APIs to support debugging and error handling.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "syscfg.h"
#include "dbug/dbugInternal.h"
#include "build_info.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include "include/defs.h"
#include "teul/teulTime.h"
#include "include/configVars.h"
#include <locale.h>
#include <mutex>

#if platBuild == BUILD_SERVER
#include "wwii/wwiiThreads.h"
static const char _OPEN_MSG[] = "Playnet Server Log File.";
static const char _CLOSE_MSG[] = "\nPlayer Server Log File Closed.\n";
#else
static const char _OPEN_MSG[] = "Debug Message System Open.";
static const char _CLOSE_MSG[] = "\nDebug Message System Closed.\n";
#endif

#if platOS != OS_WIN32
# include <unistd.h>
# include <signal.h>
# include <sys/time.h>
# define _O_CREAT O_CREAT
# define _O_WRONLY O_WRONLY
# define _O_TRUNC O_TRUNC
# define PMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#else
# include <io.h>
# include <winsock2.h>
# pragma comment(lib, "ws2_32.lib")
# define PMODE (_S_IREAD | _S_IWRITE)
# define STDERR_FILENO 2
#endif

static const size_t MAX_FILE_NUM = 5;

static DebugLogger _logger;

const char* errorTypes[] = {
	"F",		// fatal error detected, exit program
	"A",		// error detected, operation aborted
	"W",		// possible error detected, operation continues
    "M",        // error detected, routine recovered, operation continues
	"N",		// note, operation continues
	"I",		// informational message
	"D",		// debug level message
};

// Global boolean that controls whether or not 'D'ebug level messages will be logged.
// Exposed globally so it can be tested before prior to performing otherwise expensive
// message constructions.
//bool g_logDebug = true;

static std::mutex s_dbugInitMutex;
static bool s_dbugInitialized = false;
static std::mutex s_dbugOutputMutex;

thread_local char DebugLogger::m_text[DBUG_MAX_STRING + 1];

struct fake_thread_id
{
    size_t m_id;
    
    static std::atomic<size_t> s_id;
    
    fake_thread_id() : m_id(s_id++) {}
};
std::atomic<size_t> fake_thread_id::s_id { 0 };

static thread_local fake_thread_id current_fake_thread_id {};

//////////////////////////////////////////////////////////////////////
// Set errno.

static void
_dbugSetErr(SINT32 err)
{
#if platOS == OS_WIN32
	WSASetLastError(err);
#else
	errno = err;
#endif
}

//////////////////////////////////////////////////////////////////////
// Rename a file 'xxx.N.log' to 'xxx.M.log'

#if platBuild == BUILD_SERVER
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
        return;

	// Already qualified.
#if platOS == OS_WIN32
    if ( filename[0] == '\\' || (isalpha(filename[0]) && filename[1] == ':' && filename[2] == '\\')  ) // yes, this is short-string-safe
#else
	if ( (filename[0] == '.' && filename[1] == '/') || filename[0] == '/' )
#endif
		strsafecpy(output, filename, outputSize);
	else
	{
#if platBuild == BUILD_SERVER
		const char* const defaultPath = cv_defaultLogPath.GetString();
#else
		const char* const defaultPath = getenv("DEFAULT_LOGDIR");
#endif
		if ( defaultPath && defaultPath[0] != 0 )
		{
			OS_MKDIR(defaultPath, 0770);
			snprintf(output, outputSize, "%s%c%s", defaultPath, OS_PATHSEP_C, filename);
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

	if (PN_LIKELY(srcFile_ != nullptr))
	{
#if platOS == OS_WIN32
		int oflags = _O_CREAT | _O_WRONLY | _O_TRUNC | _O_SEQUENTIAL | _O_NOINHERIT;
#else
		int oflags = _O_CREAT | _O_WRONLY | _O_TRUNC;
#endif

#if platBuild == BUILD_SERVER
		// bump existing .%u.log files up a number and then delete the Nth
		for ( int i = MAX_FILE_NUM; i >= 0; --i ) {
			_roll(filename, i);
		}
		strprintf(ofn, "%s.%" PRSizeT ".log", filename, MAX_FILE_NUM + 1);
		unlink(ofn);

		strprintf(ofn, "%s.0.log", filename);
#else
		// client always writes to .0.log, and since we removed the limit
		// on per-log-file sizes, the client *always* writes to .0.log.
		static UINT32 _fileNumber = 0;
		strprintf(ofn, "%s.%u.log", filename, _fileNumber++);

		if ( _fileNumber > MAX_FILE_NUM)
			_fileNumber = 0;
#endif

		OS_OPEN(&file, ofn, oflags, PMODE);
	} else {
		file = STDERR_FILENO;
    }

	return file;
}

//////////////////////////////////////////////////////////////////////
// Write data to the current log file. If the data written results in
// MAX_FILE_SIZE being exceeded, then perform file roll-over.

static void
_dbugOut(bool logToConsole
			, const LogMode	logMode
			, const char*	outString
			, const size_t	length
			, const	bool	rollover) noexcept
{
	if ( PN_UNLIKELY(logToConsole) ) {
		OS_WRITE(STDERR_FILENO, outString, (unsigned long)length);
		if ( PN_UNLIKELY(_logger.m_dbugfile == STDERR_FILENO) )
			return;
	}

	if ( logMode != LogMode::TO_FILE || _logger.m_dbugfile < 0 )
		return;

	/*size_t retval = */OS_WRITE(_logger.m_dbugfile, outString, (unsigned long)length);
	///TODO: handle negative retval???
	if ( _logger.m_dbugfile == STDERR_FILENO )
		return;
}

void DebugLogger::initialize(LogMode logMode, Debug::ERROR_TYPE level, SINT32 fd, const char* programName) noexcept
{
	m_logMode = logMode;
	m_dbugfile = fd;
	if ( isInitialized() == false )
	{
		m_logLevel = static_cast<Debug::ERROR_TYPE>(level & ~DBUG_FLAG_MASK);
		m_writeLevel = m_logLevel;
		m_logFlags = 0;
		m_writeFlags = 0;
	}

	setProgram(programName);
	m_text[0] = 0;

	m_initialized = true;
}

#if platBuild == BUILD_SERVER && platOS != OS_WIN32
static void
_handleSIGUSR2(SINT32)
{
	static Debug::ERROR_TYPE oldLevel = Debug::ERR_DBUG;

	//Ignore any incoming signals
	signal(SIGUSR2, SIG_IGN);

	// safe as long as we dont use dbugSetLogLevel at same time elsewhere
	if ( !g_logDebug )
	{
		oldLevel = _logger.m_logLevel;
		_logger.m_logLevel = Debug::ERR_DBUG;
		g_logDebug = true;
	}
	else if ( oldLevel != Debug::ERR_DBUG )
	{
		_logger.m_logLevel = oldLevel;
		g_logDebug = false;
	}

	signal(SIGUSR2, _handleSIGUSR2);
}
#endif

#if platBuild == BUILD_SERVER && platOS != OS_WIN32

#include <execinfo.h>

//////////////////////////////////////////////////////////////////////
// Linux: On a fatal signal, write a backtrace to the log file.

static const size_t MaxBacktraceAddrs = 32;
static void *backtraceAddrs[MaxBacktraceAddrs];

static void _dbugDumpBacktrace(int signum)
{
    int fd = STDERR_FILENO;
    
	if ( _logger.isInitialized() && _logger.m_logMode == LogMode::TO_FILE && _logger.m_dbugfile >= 0) {
        fd = _logger.m_dbugfile;
    }
    
	if ( signum > 0 ) {
		dprintf(fd, "\n\n");
		dprintf(fd, "*** Process terminating due to fatal signal #%d:\n", signum);
	}

	int numAddrs = backtrace(backtraceAddrs, MaxBacktraceAddrs);
	
	if ( numAddrs < 0 ) {
		dprintf(fd, "Backtrace error: %d\n", numAddrs);
		return;
	}
	
    dprintf(fd, "BACKTRACE [Rev %s (code %s/data %s), %u functions]:\n", PN_product_version_string, PN_code_git_commitish_string, PN_data_git_commitish_string, numAddrs);
    backtrace_symbols_fd(backtraceAddrs, numAddrs, fd);
}

static int _setSignal(int signum, void (*handler)(int), bool resetHandler = true)
{
	struct sigaction action;
	
    action.sa_handler = handler;
	sigfillset(&action.sa_mask);
	action.sa_flags = resetHandler ? SA_RESETHAND : 0;
	return sigaction(signum, &action, nullptr);
}
#endif

//////////////////////////////////////////////////////////////////////////
// dbugInit: sets the current error level to ERR_INFO, opens the debug
// file, and initializes the error object.

void dbugInit(LogMode logMode, const char* const filename)
{
	std::lock_guard<std::mutex> mutexLock(s_dbugInitMutex);
	if ( s_dbugInitialized )
		return;
	s_dbugInitialized = true;

	setlocale(LC_CTYPE, "");

	Debug::ERROR_TYPE level = Debug::ERR_WARN;
//    g_logDebug = true;

    int file = -1;
    if (logMode == LogMode::TO_FILE) {
        file = _openDbugFile(filename);
        if (file == -1) {
#if platBuild == BUILD_SERVER
            LogAbort("Failed to open %s log file, can not continue!", filename);
#else
            fprintf(stderr, "ERROR: Unable to open %s log file.", filename);
#endif
        }
    }
	_logger.initialize(logMode, level, file, filename);

#if platBuild == BUILD_SERVER && platOS != OS_WIN32
	if ( _setSignal(SIGUSR2, _handleSIGUSR2, false) < 0
        || _setSignal(SIGPIPE, SIG_IGN, false) < 0
        || _setSignal(SIGABRT, _dbugDumpBacktrace) < 0
		|| _setSignal(SIGILL, _dbugDumpBacktrace) < 0
		|| _setSignal(SIGFPE, _dbugDumpBacktrace) < 0
		|| _setSignal(SIGSEGV, _dbugDumpBacktrace) < 0
		|| _setSignal(SIGTERM, _dbugDumpBacktrace) < 0
		|| _setSignal(SIGBUS, _dbugDumpBacktrace) < 0 )
	{
		LogAbort("Unable to install signal handlers.");
	}
#endif

    if ( file >= 0 ) {
        dbugLog(_FLM, "%s", _OPEN_MSG);
        dbugLog(_FLM, "Version: %s (%s%s, %s)", PN_product_version_string, PN_code_git_commitish_string, PN_data_git_commitish_string, PN_build_platform_string);
    }
}

// dbugClose: closes the debug file and cleans up.
void dbugClose()
{
//    g_logDebug = false;
	if ( _logger.isInitialized() == false )
		return;

	_logger.close();
}

void DebugLogger::close() noexcept
{
	if ( m_logMode != LogMode::TO_FILE || m_dbugfile < 0 )
		return;
	std::lock_guard<std::mutex> lock(s_dbugOutputMutex);
	_dbugOut(true, _logger.m_logMode, _CLOSE_MSG, sizeof(_CLOSE_MSG)-1, false);

	if ( m_dbugfile !=  STDERR_FILENO )
		OS_CLOSE(_logger.m_dbugfile);
	_logger.m_dbugfile = -1;
}

// dbugSetErrLevel: set the current debug level for display of error messages
// and interface to the microsoft debuger through the SetLastErrorEx and
// SetDebugErrorLevel functions.
void dbugSetWriteLevel(Debug::ERROR_TYPE level)
{
	SINT32 flags;
	SINT32 temp;

	flags = level & DBUG_FLAG_MASK;
	temp = level;
	temp &= ~DBUG_FLAG_MASK;
	level = (Debug::ERROR_TYPE)temp;

	if ( _logger.isInitialized() == false )
		return;

	_logger.m_writeFlags = flags;

	switch(level)
	{
	case Debug::ERR_FATAL:
	case Debug::ERR_ABORT:
	case Debug::ERR_MINOR:
	case Debug::ERR_WARN:
	case Debug::ERR_NOTE:
	case Debug::ERR_INFO:
	case Debug::ERR_DBUG:
		_logger.m_writeLevel = level;
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

//    if ( level == Debug::ERR_DBUG )
//        g_logDebug = true;
//    else
//        g_logDebug = false;

	_logger.m_logFlags = flags;

	switch(level)
	{
	case Debug::ERR_FATAL:
	case Debug::ERR_ABORT:
	case Debug::ERR_MINOR:
	case Debug::ERR_WARN:
	case Debug::ERR_NOTE:
	case Debug::ERR_INFO:
	case Debug::ERR_DBUG:
		_logger.m_logLevel = level;
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
void dbugSetErrno(int err)
{
    _dbugSetErr(err);
}

// dbugGetErrno: returns the current error number.  Error numbers adhear to
// errno.h. Under win32 "dbugWErrno.h" may also be included to map windows
// socket errors to unix errno.  Additional errors beyond those covered in
// errno.h and dbugWErrno.h must be added to debugExtErrno.h and must adhear
// to windows error code conventions to eliminate problems with compataibility.
SINT32	dbugGetErrno()
{
#if platOS == OS_WIN32
	return WSAGetLastError();
#else
    return errno;
#endif
}

// OS-provided basename() on POSIX systems is bad.
const char* sane_basename(const char* const filepath) noexcept
{
	const char* slash = strrchr(filepath, '\\');
	if (PN_UNLIKELY(!slash))
		slash = strrchr(filepath, '/');
	return PN_LIKELY(slash) ? slash + 1 : filepath;
}

// Retrieve the current time to millisecond resolution in portable fashion. The
// extra math is needed thanks to design flaws in C++'s chrono library.
static void _getCurrentTime(struct tm &tm, uint32_t &millis)
{
    auto now = std::chrono::system_clock::now();
    time_t nowTime = std::chrono::system_clock::to_time_t(now);
    auto nowRounded = std::chrono::system_clock::from_time_t(nowTime);
    
    // If the time value was rounded upward instead of truncated or rounded down,
    // adjust for the rounding.
    if (nowRounded > now) {
        --nowTime;
        nowRounded -= std::chrono::seconds(1);
    }
    
    localtime_r(&nowTime, &tm);
    millis = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(now - nowRounded).count();
}

// _dbugLogPrivate: if the current filename, lineno or string is nullptr then it
// sets these members.  The parameters are output as a string to the debug
// file depending on the debug level selected and the current error type.
static inline SINT32
_dbugLogPrivate(bool isLog, const char* const filename, const SINT32 lineno, const UINT32 errType, const char* const fmt, va_list args)
{
	if ( PN_UNLIKELY(_logger.isInitialized() == false) )
		_logger.initialize(LogMode::TO_FILE,  Debug::ERR_INFO, STDERR_FILENO, "not_initialized");

	if ( PN_UNLIKELY(fmt == nullptr) )
		return 0;

	// split incoming 'errType' into flags bits and type bits
	UINT32 flags = errType & DBUG_FLAG_MASK, error = errType & ~DBUG_FLAG_MASK;
	flags |= isLog ? _logger.m_logFlags : _logger.m_writeFlags;

	const UINT32 minLevel = isLog ? _logger.m_logLevel : _logger.m_writeLevel;
	if ( PN_LIKELY((error != Debug::ERR_FATAL) && (error > minLevel)) )
		return 0;


	struct tm tm;
	uint32_t ms;
	
	_getCurrentTime(tm, ms);
	
	char prefix[512];
	size_t pfxLen = strprintf(prefix, "%s [%04u-%02u-%02u %02u:%02u.%02u.%03u %s:%u] %" PRSizeT "# "
							, errorTypes[error]
							, tm.tm_year + 1900
							, tm.tm_mon + 1
							, tm.tm_mday
							, tm.tm_hour
							, tm.tm_min
							, tm.tm_sec
							, ms
							, sane_basename(filename)
							, lineno
							, current_fake_thread_id.m_id);
	const char* text = nullptr;
	size_t len = _logger.vformatText(fmt, args);
	
    if ( len == (size_t)~0 ) {
		text = "[ERROR]\n";
		len = strlen(text);
	} else {
		text = _logger.text();
	}

    bool logToConsole =
            (error < Debug::ERR_MINOR) ||
            (error < Debug::ERR_INFO && (flags & STD_ERR)) ||
            (flags & LOG_ALL_TO_STDERR);
            
    try {
        std::lock_guard<std::mutex> lock(s_dbugOutputMutex);
        // re-check: could have changed while we got the lock
        if (g_logDebug) {
            if ( logToConsole || _logger.m_dbugfile >= 0 ) {
                _dbugOut(logToConsole, _logger.m_logMode, prefix, pfxLen, false);
                _dbugOut(logToConsole, _logger.m_logMode, _logger.text(), len, true);
            }
        }
    } catch (std::system_error &e) {
        // This probably means we tried to grab the mutex during exit(),
        // when it's not valid anymore. Silently ignore.
    }
    
	if ( PN_UNLIKELY(error == Debug::ERR_FATAL) ) {
		abort();
	}

	return (SINT32)len;
}

// dbugLog: if the current filename, lineno or string is nullptr then it
// sets these members.  The parameters are output as a string to the debug
// file depending on the debug level selected and the current error type.
SINT32 _dbugLog(const char* const filename, const SINT32 lineno, const UINT32 errType, const char* const fmt, ...)
{
	SINT32 result = 0;
	if (PN_UNLIKELY(_logger.isInitialized() == false))
	{
		if (PN_UNLIKELY(errType == Debug::ERR_FATAL))
		{
			va_list args;
			va_start(args, fmt);
			vfprintf(stderr, fmt, args);
			va_end(args);
			abort();
		}
	}
	else
	{
        if (g_logDebug) {
            if (!(errType != Debug::ERR_FATAL && (errType & ~DBUG_FLAG_MASK) > _logger.m_logLevel))
            {
                va_list args;
                va_start(args, fmt);
                result = _dbugLogPrivate(TRUE, filename, lineno, errType, fmt, args);
                va_end(args);
            }
        }
    }

	return result;
}

//_dbugWrite: same as above for dbugWrite
SINT32 _dbugWrite(const char* filename, SINT32 lineno, UINT32 errType, const char* fmt, ...)
{
	va_list args;

//	theError = dbugGetErrno();

	va_start(args, fmt);
	const SINT32 retval = _dbugLogPrivate(FALSE, filename,lineno,errType,fmt,args);
	va_end(args);

	return retval;
}

//////////////////////////////////////////////////////////////////////
// "Pretty" logging routine.

NORETURN_FN void
_LogAbort(const char* file_, unsigned int line_, unsigned int error_, const char* assertion_, const char* fmt_, ...)
{
	va_list args;
	va_start(args, fmt_);

#if platOS == OS_WIN32
	if ( assertion_ == nullptr || *assertion_ == 0 )
		assertion_ = "";

	const size_t roughSize = vsnprintf(_logger.m_text, sizeof(_logger.m_text), fmt_, args);
	va_end(args);
	const size_t maxSize = roughSize + 256 + strlen(assertion_);
	char* message = (char*)calloc(maxSize, sizeof(char));
	if ( PN_UNLIKELY(message == nullptr) )
	{
		MessageBoxA(0, "OUT OF MEMORY", "OUT OF MEMORY", MB_ICONERROR | MB_OK | MB_APPLMODAL);
		abort();
	}

	const char* caption;
#if defined(DEBUG)
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
#else
	caption = "Critical Error";
	sprintf_s(message, maxSize, "%s: %s\n", caption, _logger.m_text);
#endif
	MessageBoxA(0, message, caption, MB_OK | MB_ICONERROR | MB_APPLMODAL);
	free(message);

#if defined(DEBUG)
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
