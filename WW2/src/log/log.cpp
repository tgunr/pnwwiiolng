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


static UINT32 logMask=0;
static SINT32 logLevel=0;

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
	"console"
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
	if(enable)
		logMask |= system;
	else
		logMask &= ~system;
}

/************************************************************************************************
	Set_Log_Level
*************************************************************************************************/

void Set_Log_Level(SINT32 level)
{
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
		if(LOG_SYSTEM_NAMES[i] == NULL)
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

	return NULL;
}

/************************************************************************************************
	Get_Log_Mask
*************************************************************************************************/

UINT32 Get_Log_Mask()
{
	return logMask;
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

void Close_Debug(void)
{
}

#elif platOS == OS_WIN32

#include "include/vars.h"

static char					ofn[_MAX_PATH];
static FILE*				ofp = NULL;

/************************************************************************************************
	Open_Debug
*************************************************************************************************/

LOGICAL Open_Debug(const char* filename)
{
	if (filename == NULL)
		return(FALSE);

    strcpy(ofn, filename);
#ifdef _DEBUG
	strcat(ofn, "_Debug");
#endif
	strcat(ofn, ".log");

	ofp = fioUserFile(ofn, "w");

	if ( ofp == NULL )
		return FALSE;

	Write_Debug("\r\nDebug Message System Open.\r\n");

	return TRUE;
}

/************************************************************************************************
	Close_Debug
*************************************************************************************************/

void Close_Debug(void)
{
	if ( ofp != NULL )
		fclose(ofp);
	ofp = NULL;
}

/************************************************************************************************
	logv
*************************************************************************************************/

static void logv(UINT32 system, SINT32 level, const char* str, va_list marker)
{
	if(logLevel < level || !(system & logMask) || str == NULL) return;
	if ( ofp == NULL )
	{
		vprintf(str, marker);
		printf("\n");
	}
	else
	{
#ifdef DEBUG_WW2
		char buf[255+1];

		_vsnprintf(buf, 255, str, marker);
		// Since its on the stack:
		// any string longer >= 255 chars will not be null terminated
		buf[255] = 0;

		// log system always uses ascii
		OutputDebugStringA(buf);
		OutputDebugStringA("\r\n");

		fprintf(ofp, "%s\r\n", buf);
		fflush(ofp);

#else
		vfprintf(ofp, str, marker);
		fprintf(ofp, "\r\n");
		fflush(ofp);
#endif
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
extern FLOAT32 GetCPUSpeed(void);
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
			piExitProgram(__FILE__, __LINE__, s);
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
	logTimer* timer = NULL;

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
			piExitProgram(__FILE__, __LINE__, "Invalid timerid!");
	}

	if(timer)
	{
		if(timer->callcount == 0)
			timer->starttime = piReadTimeStamp32();

		timer->callcount++;
	}else
	{
		piExitProgram(__FILE__, __LINE__, "Ran out of logTimers!");
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

		if(sortTime->funcname == NULL)
			break;

		if(sortTime->tottime > 0)
		{
			Write_Log(LOG_TIMER, LOG_INFO, "%s: %.2f ms %.2f%%", sortTime->funcname, logTimerToMs(sortTime->tottime),
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

#ifdef ENABLE_FLOATERS

#include <sstream>
#include <fstream>
#include <iomanip>
#include "rn/rnmain.h"
#include "float.h"
#include "include/vars.h"
#include "chat/chatDefines.h"
#include "client/GameClient.h"
#include "VersionNo.h"
#if platOS == OS_WIN32 || platOS == OS_WIN64
#include "sys/sysreg.h"
#endif

static	FLOAT32				remainingProfilingTime;
static	std::fstream		profilingStream;
static	std::string			CurrentProfileName = "";
static	std::string			CurrentProfileFileName = "";
static	std::ostringstream 	frameTimesString;


void logInitProfiling(const char* const profileName, FLOAT32 profilingDuration)
{
	CurrentProfileName = STRVER;
	CurrentProfileName.append("_");
	CurrentProfileName.append(profileName);

	CurrentProfileFileName = CurrentProfileName;
	CurrentProfileFileName.append("_profile.txt");
	remainingProfilingTime = profilingDuration * 60000;
	profilingStream.open(CurrentProfileFileName.c_str(), std::fstream::out | std::fstream::trunc);
	profilingStream << std::fixed << std::setprecision(3);
}


// Return a string describing characteristics of the user's system
static void _getSystemDetails(char* into, size_t sizeofInto)
{
#if platOS == OS_WIN32 || platOS == OS_WIN64
	// Find the number of CPUs
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	// Query the registry for the CPU type etc
	TCHAR cpuName[256];
	DWORD cpuMhz = 0;
	DWORD cpuFlags = 0;

	static const TCHAR* HKLM_CPU0 = _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
	sysregGetHKeyText(HKEY_LOCAL_MACHINE, HKLM_CPU0, _T("ProcessorNameString"), cpuName);
	sysregGetHKeyDword(HKEY_LOCAL_MACHINE, HKLM_CPU0, _T("~Mhz"), &cpuMhz);
	sysregGetHKeyDword(HKEY_LOCAL_MACHINE, HKLM_CPU0, _T("FeatureSet"), &cpuFlags);

	snprintf(into, sizeofInto - 1, "Win %u %x %u %s", sysInfo.dwNumberOfProcessors, cpuFlags, cpuMhz, UTF8(cpuName));

	///TODO: Would be /really/ nice to add total memory and graphics card data.
#elif platOS == OS_MACOS || platOS == OS_MACOSX

	strncpy(into, "Mac", sizeofInto - 1);

#else

	strncpy(into, "Unknown", sizeofInto - 1);

#endif

	// snprintf/strncpy don't guarantee a null terminator
	into[sizeofInto - 1] = 0;
}


void logUpdateProfiling()
{
			FLOAT32	thisFrameTime;
	static	FLOAT32	passedTimeSinceLastTick	= 0;
			UINT32	passedTicks;
	static	FLOAT32	tickDuration			= 100.0;
	static	FLOAT32	minFrameTime			= FLT_MAX;
	static	FLOAT32	maxFrameTime			= 0;
	static	FLOAT32	totalFramesTime			= 0;
	static	UINT32	totalFrameCnt			= 0;
	static	FLOAT32	lastFramesTime			= 0;
	static	UINT32	lastFrameCount			= 0;
	static	UINT32	FramesOver100ms			= 0;
	static	UINT32	FramesOver300ms			= 0;
	static	UINT32	FramesOver500ms			= 0;
	static	UINT32	FramesOver1000ms		= 0;
			size_t	processPeakMemoryUsage	= 0;


	if(!remainingProfilingTime) return;

	thisFrameTime	= 1000 / gLastFrameFPS;
	totalFramesTime += thisFrameTime;
	totalFrameCnt++;
	lastFramesTime += thisFrameTime;
	lastFrameCount++;

	if(thisFrameTime > 1000)	FramesOver1000ms++;
	if(thisFrameTime > 500)		FramesOver500ms++;
	if(thisFrameTime > 300)		FramesOver300ms++;
	if(thisFrameTime > 100)		FramesOver100ms++;

	passedTimeSinceLastTick += thisFrameTime;
	passedTicks = (UINT32)(passedTimeSinceLastTick  / tickDuration);

	if(passedTicks)
	{
		FLOAT32	thisFrameTimeAvg = lastFramesTime / lastFrameCount;

		for(UINT32 i=0; i < passedTicks && remainingProfilingTime > 0; i++)
		{
			frameTimesString  << std::endl << thisFrameTimeAvg;
			remainingProfilingTime -= tickDuration;
		}
		if(thisFrameTimeAvg < minFrameTime) minFrameTime = thisFrameTimeAvg;
		if(thisFrameTimeAvg > maxFrameTime) maxFrameTime = thisFrameTimeAvg;

		passedTimeSinceLastTick -= passedTicks * tickDuration;
		lastFramesTime	= 0;
		lastFrameCount	= 0;
	}

	if(remainingProfilingTime <= 0)
	{
		profilingStream << "Profile name: " << CurrentProfileName.c_str() << std::endl << std::endl;
		profilingStream << "Frame time maximum: " << maxFrameTime << " ms" << std::endl;
		profilingStream << "Frame time average: " << totalFramesTime / totalFrameCnt << " ms" << std::endl;
		profilingStream << "Frame time minimum: " << minFrameTime << " ms" << std::endl << std::endl;

		profilingStream << "Frames over 100 ms: " << FramesOver100ms << std::endl;
		profilingStream << "Frames over 300 ms: " << FramesOver300ms << std::endl;
		profilingStream << "Frames over 500 ms: " << FramesOver500ms << std::endl;
		profilingStream << "Frames over 1000 ms: " << FramesOver1000ms << std::endl;

#if platOS == OS_WIN32
		PROCESS_MEMORY_COUNTERS	memoryCounters;
		HANDLE						processHandle	= GetCurrentProcess();

		GetProcessMemoryInfo(processHandle, &memoryCounters, sizeof(memoryCounters));
		processPeakMemoryUsage = memoryCounters.PeakPagefileUsage;
#endif // #ifdef OS_WIN32

		profilingStream << std::endl;
#if platOS == OS_WIN32
		profilingStream.imbue(std::locale("US"));
#endif
		profilingStream << "Peak memory usage during profiling: " << processPeakMemoryUsage << " Bytes";
		profilingStream << std::endl;	profilingStream << std::endl;
		profilingStream << frameTimesString.str();

		rnmainListText("... completed profiling.\n\n");
		rnmainListText("Average frame time: %.3f ms\n", (FLOAT32)totalFramesTime / (FLOAT32)totalFrameCnt);
		rnmainListText("Complete profile data can be found in %s.", CurrentProfileFileName.c_str());

		if(Online)
		{
			// Build a chat command version of the results for the host.
			char result[MAX_MESSAGE_LENGTH+1];

			int written = 0;
			written = snprintf(result, MAX_MESSAGE_LENGTH
								, "/profres"
									" %s"				// Version number
									" %u %f %f %f"		// Frame count[minTime/maxTime/ttlTime]
									" %u %u %u %u"		// Frames over 100, 300, 500, 1000 ms
									" %lu"				// peak mem
									" \"%s\""			// Profile name
									" "
								, STRVER
								, totalFrameCnt, minFrameTime, maxFrameTime, totalFramesTime
								, FramesOver100ms, FramesOver300ms, FramesOver500ms, FramesOver1000ms
								, processPeakMemoryUsage
								, CurrentProfileName.c_str()
								);
			if ( written > MAX_MESSAGE_LENGTH )
				written = MAX_MESSAGE_LENGTH;
			// snprintf doesn't guarantee it will write a trailing zero...
			result[MAX_MESSAGE_LENGTH] = 0;

			// Tack on as much of the system specs as we can at the end
			_getSystemDetails(result + written, sizeof(result) - written);

			// Send the command to the chat host
			PlayerClient()->SendChatMessage(0, result);

			// Note it to the profile file just FYI
			profilingStream << std::endl;	profilingStream << std::endl;
			profilingStream << " " << result;
		}


		minFrameTime			= FLT_MAX;
		maxFrameTime			= 0;
		totalFramesTime			= 0;
		totalFrameCnt			= 0;
		passedTimeSinceLastTick	= 0;
		remainingProfilingTime	= 0;
		FramesOver100ms			= 0;
		FramesOver300ms			= 0;
		FramesOver500ms			= 0;
		FramesOver1000ms		= 0;
		CurrentProfileName = "";

		profilingStream.close();
		frameTimesString.str("");
	}
}
#endif	// #ifdef ENABLE_FLOATERS
