#include <syscfg.h> // Path is project specific.
#include "log/log.h"

#include <string.h>

#include <stdarg.h>
#include "fio/fio.h"
#include "ui/uiUTF.h"

#if platOS == OS_WIN32
#include "Psapi.h"
#endif // #ifdef OS_WIN32

#ifdef EVENT_LOG

static std::mutex logLock;
static SINT32 gLogLevelOnly = LOG_NONE;
static UINT32 glogMaskOnly = LOG_NONE;

#ifdef DEBUG_WW2
static UINT32 logMask = LOG_WEAPON | LOG_VEHICLE | LOG_1 ;
static SINT32 logLevel = LOG_DEBUG;
#else
static UINT32 logMask = LOG_NONE;
static SINT32 logLevel = 0;
#endif

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker);
static void logw(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, va_list marker);
static void logwstr(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* pstr);

struct {
    const char* prevFile = "";
    int prevLine = 0;
    uint count = 0;
} prevLog ;

/************************************************************************************************
	Write_Where
*************************************************************************************************/
void Write_Where(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, ...) {
    const char* pstr;
    if ( (prevLog.prevFile == file) && (prevLog.prevLine == line) ) {
        switch (prevLog.count) {
            case 0:
                pstr = str;
                prevLog.count += 1;
                va_list marker;
                va_start(marker, str);
                logw(file, line, function, system, level, str, marker);
                va_end(marker);
                break;
            case 1:
                pstr = "REPEATED";
                prevLog.count += 1;
                logwstr(file, line, function, system, level, pstr);
                break;
            default:
                prevLog.count += 1;
                return;
        }
    } else {
        prevLog.count = 1;
        va_list marker;
        va_start(marker, str);
        logw(file, line, function, system, level, str, marker);
        va_end(marker);
    }
    prevLog.prevFile = file;
    prevLog.prevLine = line;
}

/************************************************************************************************
	Write_Log
 *************************************************************************************************/
void Write_Log(UINT32 system, SINT32 level, const char* str, ...)
{
	va_list marker;

	va_start(marker, str);
	logv(system, level, str, marker);
	va_end(marker);
}

/************************************************************************************************
	Enable_Log
*************************************************************************************************/

void Enable_Log(UINT32 system, LOGICAL enable)
{
	std::lock_guard<std::mutex> guard(logLock);

	if (enable) {
		logMask |= system;
	} else {
		logMask &= ~system;
	}
}

/************************************************************************************************
	Set_Log_Level
*************************************************************************************************/

void Set_Log_Level(SINT32 level)
{
	std::lock_guard<std::mutex> guard(logLock);

	logLevel = level;
}

/************************************************************************************************
	Set_Log_Mask
 *************************************************************************************************/

void Set_Log_Mask(const char* system)
{
    int i;
    
    if(!strcmp("all", system)) {
        logMask = LOG_ALL;
        return;
    }
    else if(!strcmp("none", system)) {
        logMask = LOG_NONE;
        return;
    }
    
    for(i = 0; i < 31; i++)
    {
        if(LOG_SYSTEM_NAMES[i] == nullptr)
            break;
        
        if(!strcmp(LOG_SYSTEM_NAMES[i], system))
            logMask = (1 << i);
    }
}

/************************************************************************************************
	Write_Debug
*************************************************************************************************/

void Write_Debug(const char* str, ...)
{
	va_list marker;

	va_start(marker, str);
	logv(LOG_OLD, LOG_INFO, str, marker);
	va_end(marker);
}

/************************************************************************************************
	Log_System_Mask
*************************************************************************************************/

UINT32 Log_System_Mask(const char* system)
{
	int i;

	if(!strcmp("all", system))
		return 0xffffffff;
	else if(!strcmp("none", system))
		return 0x0;

	for(i = 0; i < 31; i++)
	{
		if(LOG_SYSTEM_NAMES[i] == nullptr)
			break;

		if(!strcmp(LOG_SYSTEM_NAMES[i], system))
			return (1 << i);
	}

	return 0;
}

/************************************************************************************************
	Log_System_Name
*************************************************************************************************/

const char* Log_System_Name(UINT32 mask)
{
	int i;

	for(i = 0; i < 31; i++)
	{
		if(mask & 0x1)
			return LOG_SYSTEM_NAMES[i];

		mask >>= 1;
	}

	return nullptr;
}

/************************************************************************************************
	Get_Log_Mask
*************************************************************************************************/

UINT32 Get_Log_Mask()
{
	return logMask;
}

/************************************************************************************************
	logw
 *************************************************************************************************/
static void logwI(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, va_list marker)
{
    int wheresize = snprintf(NULL, 0, "%s:%d: %s: %s", file, line, function, str);
    char buf[wheresize + 1]; // note +1 for terminating null byte
    snprintf(buf, sizeof buf, "%s:%d: %s: %s", file, line, function, str);
    
    vprintf(buf, marker);
    printf("\n");
}


static void logww(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, va_list marker)
{
    if(gLogLevelOnly && glogMaskOnly) {
        if (gLogLevelOnly == level && (system & glogMaskOnly))
            logwI(file, line, function, system, level, str, marker);
    } else {
        if (logLevel < level || !(system & logMask))
            return;
        else
            logwI(file, line, function, system, level, str, marker);
    }
}

static void lognodup(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, va_list marker)
{
    char buf[1024];
    //    char buf[wheresize + 1]; // note +1 for terminating null byte
    if ( (prevLog.prevFile == file) && (prevLog.prevLine == line) ) {
        switch (prevLog.count) {
            case 0:
                snprintf(buf, sizeof buf, "%s:%d: %s: %s", file, line, function, str);
                prevLog.count += 1;
                goto printLog;
            case 1:
                snprintf(buf, sizeof buf, "%s:%d: %s: ...", file, line, function);
                prevLog.count += 1;
                goto printLog;
            default:
                prevLog.count += 1;
                return;
        }
    } else {
        prevLog.count = 1;
    }
printLog:
    vprintf(buf, marker);
    printf("\n");
    prevLog.prevFile = file;
    prevLog.prevLine = line;
}

//static void logwstr(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* pstr)
//{
//    if(gLogLevelOnly && glogMaskOnly) {
//        if ((gLogLevelOnly == level) && (system & glogMaskOnly))
//            goto logwhere;
//    } else {
//        if (logLevel < level || !(system & logMask))
//            return;
//        else
//            goto logwhere;
//    }
//
//logwhere:
//    int wheresize = snprintf(NULL, 0, "%s:%d: %s: %s", file, line, function, pstr);
//    char buf[wheresize + 1]; // note +1 for terminating null byte
//    snprintf(buf, sizeof buf, "%s:%d: %s: %s", file, line, function, pstr);
//    printf("%s\n", buf);
//}

static void logw(const char *file, int line, const char *function, UINT32 system, SINT32 level, const char* str, va_list marker)
{
    if(gLogLevelOnly && glogMaskOnly) {
        if ((gLogLevelOnly == level) && (system & glogMaskOnly))
            goto logwhere;
    } else {
        if (logLevel < level || !(system & logMask))
            return;
        else
            goto logwhere;
    }
logwhere:
    logwI(file, line, function, system, level, str, marker);
}

#if platOS == OS_MACOS

/************************************************************************************************
	logv
*************************************************************************************************/

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker)
{
	if(logLevel < level || !(system & logMask)) return;

	vprintf(str, marker);
	printf("\n");
}

LOGICAL Open_Debug(const char* filename)
{
	return TRUE;
}

void Close_Debug()
{
}

#elif platOS == OS_WIN32

#include "include/vars.h"

static FILE *ofp = nullptr;

/************************************************************************************************
	Open_Debug
*************************************************************************************************/

LOGICAL Open_Debug(const char* filename)
{
	if (filename == nullptr) {
		return false;
	}

	{
		std::lock_guard<std::mutex> guard(logLock);

		if (ofp != nullptr) {
			return true;
		}
		ofp = fioUserFile((std::string(filename) + ".log").c_str(), "w");
		if (ofp == nullptr) {
			return false;
		}
	}

	WriteDebug("\r\nDebug Message System Open.\r\n");
	return true;
}

/************************************************************************************************
	Close_Debug
*************************************************************************************************/

void Close_Debug()
{
	std::lock_guard<std::mutex> guard(logLock);

	if (ofp != nullptr) {
		fclose(ofp);
		ofp = nullptr;
	}
}

/************************************************************************************************
	logv
*************************************************************************************************/

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker)
{
	std::lock_guard<std::mutex> guard(logLock);

	if (logLevel < level || (system & logMask) == 0 || str == nullptr) {
		return;
	}

	// There's no OutputDebugFormat(), so we fake vasprintf()
	size_t len = static_cast<size_t>(_vscprintf(str, marker));
	if (len >= INT_MAX) { return; }
	std::unique_ptr<char[]> buf = std::make_unique<char[]>(len + 1);
	vsnprintf_s(buf.get(), len + 1, _TRUNCATE, str, marker);

#ifdef DEBUG_WW2
	// Output to debugger always
	OutputDebugStringA(buf.get());
	OutputDebugStringA("\r\n");

	// Output to stderr always
	fprintf_s(stderr, "%s\n", buf.get());
	fflush(stderr);
#endif

	// Output to file, if any
	if (ofp != nullptr) {
		fprintf_s(ofp, "%s\n", buf.get());
		fflush(ofp);
	}
}

#endif // OS_WIN32

#ifdef USE_LOG_TIMER
/************************************************************************************************
	logTimer
*************************************************************************************************/

#include "pi/pi.h"

#define MAX_TIMERS 1024

#if platOS == OS_WIN32
extern FLOAT32 GetCPUSpeed();
#endif

struct logTimer
{
	char funcname[128];
	int callcount; // to handle recursion
	SINT32 starttime;
	SINT32 tottime[10]; // last 10 totaltimes
};

struct logTimerStat
{
	const char* funcname;
	SINT32 tottime;
};

static logTimer logTimers[MAX_TIMERS];
static logTimerStat sortedTimes[MAX_TIMERS];

/************************************************************************************************
	logTimerReset
*************************************************************************************************/

void logTimerReset()
{
	int i;
	for(i = 0; i < MAX_TIMERS; i++)
	{
		logTimer* timer = &logTimers[i];

		if(timer->funcname[0] == '\0')
			break;

		if(timer->callcount < 0)
		{
			char s[32];
			snprintf(s, 31, "callcount for %s is %d!", timer->funcname, timer->callcount);
			piExitProgram(_FL, s);
		}

		timer->callcount = 0;
		timer->starttime = 0;
		memmove(&timer->tottime[1], &timer->tottime[0], 9 * sizeof(SINT32));
		timer->tottime[0] = 0;
	}
}

/************************************************************************************************
	logTimerBegin
*************************************************************************************************/

void logTimerBegin(const char* parent, const char* child, int* timerid)
{
	logTimer* timer = nullptr;

	if(*timerid < 0)
	{
		int i;
		for(i = 0; i < MAX_TIMERS; i++)
		{
			if(logTimers[i].funcname[0] == '\0')
			{
				*timerid = i;
				timer = &logTimers[*timerid];

				if(child && *child)
					snprintf(timer->funcname, 127, "%s:%s", parent, child);
				else
					snprintf(timer->funcname, 127, "%s", parent);

				break;
			}
		}
	}else
	{
		if(*timerid > -1 && *timerid < MAX_TIMERS)
			timer = &logTimers[*timerid];
		else
			piExitProgram(_FL, "Invalid timerid!");
	}

	if(timer)
	{
		if(timer->callcount == 0)
			timer->starttime = piReadTimeStamp32();

		timer->callcount++;
	}else
	{
		piExitProgram(_FL, "Ran out of logTimers!");
	}
}

/************************************************************************************************
	logTimerEnd
*************************************************************************************************/

int logTimerEnd(int timerid)
{
	int dt = 0;

	if(timerid > -1 && timerid < MAX_TIMERS)
	{
		logTimer* timer = &logTimers[timerid];

		timer->callcount--;

		if(timer->callcount == 0)
		{
			dt =  piReadTimeStamp32() - timer->starttime;
			timer->tottime[0] += dt;
		}
	}

	return dt;
}

/************************************************************************************************
	logTimerStatCompar
*************************************************************************************************/

static int logTimerStatCompar(const void* v1, const void* v2)
{
	logTimerStat* s1 = (logTimerStat*)v1;
	logTimerStat* s2 = (logTimerStat*)v2;

	return (s2->tottime > s1->tottime) ? 1 : ( (s2->tottime < s1->tottime) ? -1 : 0 );
}

/************************************************************************************************
	logTimerPrint
*************************************************************************************************/

void logTimerPrint(int maintimerid, int frameidx)
{
	int i, numTimers;
	SINT32 totaltime = 0;
	logTimer* timer;
	logTimerStat* sortTime;

	if(frameidx < 0 || frameidx > 9) return;

	if(maintimerid > -1 && maintimerid < MAX_TIMERS)
	{
		timer = &logTimers[maintimerid];
		totaltime = timer->tottime[frameidx];
	}

	memset(sortedTimes, 0x0, sizeof(sortedTimes));

	for(i = 0; i < MAX_TIMERS; i++)
	{
		timer = &logTimers[i];
		sortTime = &sortedTimes[i];

		if(timer->funcname[0] == '\0')
			break;

		sortTime->funcname = timer->funcname;
		sortTime->tottime = timer->tottime[frameidx];

	}
	numTimers = i;

	qsort(sortedTimes, numTimers, sizeof(logTimerStat), logTimerStatCompar);

	for(i = 0; i < MAX_TIMERS; i++)
	{
		sortTime = &sortedTimes[i];

		if(sortTime->funcname == nullptr)
			break;

		if(sortTime->tottime > 0)
		{
			WriteLog(LOG_TIMER, LOG_INFO, "%s: %.2f ms %.2f%%", sortTime->funcname, logTimerToMs(sortTime->tottime),
						(sortTime->tottime / (FLOAT32)totaltime) * 100.0f);

		}
	}
}

/************************************************************************************************
	logTimerToMs
*************************************************************************************************/

float logTimerToMs(int dt)
{
#if platOS == OS_WIN32
	static FLOAT32 cpuSpeed = -1;
	if(cpuSpeed < 0)
		cpuSpeed = GetCPUSpeed() * 1000000.0f;

	return dt / cpuSpeed * 1000.f;
#elif platOS == OS_MACOS
	return dt / 1000.0f;
#endif
}
#endif //#ifdef USE_LOG_TIMER
#endif // EVENT_LOG
