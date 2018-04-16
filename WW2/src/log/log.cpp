#include "syscfg.h" // Path is project specific.
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
#ifdef DEBUG_WW2
static UINT32 logMask = LOG_ALL;
static SINT32 logLevel = LOG_INFO;
#else
static UINT32 logMask = LOG_ERROR;
static SINT32 logLevel = LOG_FATAL;
#endif

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker);

static const char* LOG_SYSTEM_NAMES[32] =
{
	"old",
	"terrain",
	"weap",
	"fps",
	"render",
	"sound",
	"ui",
	"vehicle",
	"physics",
	"comm",
	"fx",
	"timer",
    "texture",
	"console",
    "ERROR",
	// NULLS the rest out
};

/************************************************************************************************
	Write_Log
*************************************************************************************************/

void Write_Log(UINT32 system, SINT32 level, const char* str, ...)
{
	va_list marker;

	va_start(marker, str);
	logv(system,level,str, marker);
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

#include "include/vars.h"

static FILE *ofp = nullptr;

/************************************************************************************************
	Open_Debug
*************************************************************************************************/

static LOGICAL Open_Debug_Unlocked(const char* filename)
{
    if (filename == nullptr) {
        return false;
    }

    if (ofp != nullptr) {
        return true;
    }
    ofp = fioUserFile((std::string(filename) + ".log").c_str(), "w");
    if (ofp == nullptr) {
        return false;
    }
    return true;
}

LOGICAL Open_Debug(const char* filename)
{
    {
        std::lock_guard<std::mutex> guard(logLock);
        if (filename == nullptr) {
            return false;
        }
        if (!Open_Debug_Unlocked(filename)) {
            return false;
        }
    }
    Write_Debug("Debug Message System Open.\n");
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
static void _log_outv(const char * const format, va_list args)
{
#ifdef DEBUG_WW2
    vfprintf(stderr, format, args);
    fflush(stderr);
#endif

    if (ofp != nullptr) {
        vfprintf(ofp, format, args);
        fflush(ofp);
    }
}

static void _log_outf(const char * const format, ...)
{
    va_list args;
    va_start(args, format);
    _log_outv(format, args);
    va_end(args);
}

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker)
{
    if (system != LOG_ERROR) { // "error" system is never ignored
        std::lock_guard<std::mutex> guard(logLock);

        if (logLevel < level || (system & logMask) == 0 || str == nullptr) {
            return;
        }
    }

    const UINT64 trueTime = GetMillisecondTick();
    const time_t now = (time_t)(trueTime / 1000);
    const UINT32 milis = (UINT32)(trueTime % 1000);
    char myTime[49];
    char myYear[10];
    struct tm myTm;
#if platOS == OS_WIN32
    localtime_s(&myTm, &now);
#else
    localtime_r(&now, &myTm);
#endif
    strftime(myTime, sizeof(myTime) - 1, "%a %b %d %X", &myTm);
    strftime(myYear, sizeof(myYear) - 1, "%Y", &myTm);

    char *cbuf = nullptr;
    vasprintf(&cbuf, str, marker);
    std::unique_ptr<char[], std::function<void(void*)>> buf(cbuf, free);

    {
        std::lock_guard<std::mutex> guard(logLock);

        // For errors, we don't care whether it's a debug/beta/alpha client or
        // whether logs weren't requested; we need to log these always.
        if (system == LOG_ERROR && ofp == nullptr) {
            Open_Debug_Unlocked(SYS_TITLE); // TODO: The name should not be hardcoded in this specific location
        }
        _log_outf("[%s.%03u %s] %s\n", myTime, milis, myYear, buf.get());
    }
}

#ifdef USE_LOG_TIMER
/************************************************************************************************
	logTimer
*************************************************************************************************/

#include "pi/pi.h"

#define MAX_TIMERS 1024

struct logTimer
{
	char funcname[128];
	int callcount; // to handle recursion
	UINT64 starttime;
	UINT64 tottime[10]; // last 10 totaltimes
};

struct logTimerStat
{
	const char* funcname;
	UINT64 tottime;
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
		memmove(&timer->tottime[1], &timer->tottime[0], 9 * sizeof(SINT64));
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
			timer->starttime = piReadTimeStamp64();

		timer->callcount++;
	}else
	{
		piExitProgram(_FL, "Ran out of logTimers!");
	}
}

/************************************************************************************************
	logTimerEnd
*************************************************************************************************/

UINT64 logTimerEnd(int timerid)
{
	UINT64 dt = 0;

	if(timerid > -1 && timerid < MAX_TIMERS)
	{
		logTimer* timer = &logTimers[timerid];

		timer->callcount--;

		if(timer->callcount == 0)
		{
			dt =  piReadTimeStamp64() - timer->starttime;
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
	size_t i, numTimers;
	UINT64 totaltime = 0;
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
			Write_Log(LOG_TIMER, LOG_INFO, "%s: %.2f ms %.2f%%", sortTime->funcname, logTimerToMs(sortTime->tottime),
						(sortTime->tottime / (FLOAT64)totaltime) * 100.0f);

		}
	}
}

/************************************************************************************************
	logTimerToMs
*************************************************************************************************/

float logTimerToMs(UINT64 dt)
{
	return dt / 1000.0f;
}
#endif //#ifdef USE_LOG_TIMER
#endif // EVENT_LOG
