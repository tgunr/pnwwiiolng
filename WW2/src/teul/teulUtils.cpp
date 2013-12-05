#include <syscfg.h> // Path is project specific.
// This module provides support utilities used in
// the teul API. The end-user developer
// should not need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin
// Supporting Programmer: Russell Mirabelli

#if platOS == OS_WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#endif

#if platOS == OS_LINUX
#include <sys/time.h>
#include <sys/times.h>
#endif

#if platOS == OS_MACOS
#include <Carbon/Carbon.h>
#include <unistd.h>
#endif

#if platOS == OS_WIN32
#include <mmsystem.h>
#include <sys/timeb.h>
#endif

#define CLOCK_MULTIPLIER 1000/CLOCKS_PER_SEC

static UINT32	tclock;
TeulTimeMS teulClock64;
UINT32 teulClockTime;

// returns the current clock value truncated to x bits
static inline SINT32 _clockBits(UINT32 bits)
{
	if (bits > 31)
		return -EINVAL;

	// Maximum of 31 bits
	return (tclock & (0xffffffff >> (32-bits)));
}

// Adds  an unsigned x bit time values with a signed 20bit delta .
// Returns an unsigned 20 bit timeval, - val indicates error.
// Truncates overflows (e.g. 0x0fffff + 1 = 0).
static inline SINT32 _addClockBits( UINT32 bits, UINT32 time, SINT32 delta )
{
	SINT32 result;
	UINT32 tMask, cMask, vMask;

	if (bits > 31 || bits < 2)
		return -EINVAL;

	tMask = (0xffffffff << bits);
	cMask = (0xffffffff >> (33 - bits));
	vMask = (0xffffffff >> (32 - bits));

	time &= vMask;
	delta &= vMask;

	if ((time & tMask) != 0  || (delta > (SINT32)cMask) || (delta < (0- (SINT32)cMask)) )
		return -EINVAL;

	result = time + delta;

	// fix overflows
	result &= ~tMask;

	return result;
}

// Subtracts unsigned 31 bit time values only.
// Returns an unsigned 31 bit timeval, - val indicates error.
// Truncates overflows (e.g.  0 - 1 = 0x0fffff0).
static inline SINT32 _subClockBits( UINT32 bits, UINT32 newTime, UINT32 oldTime )
{
	SINT32 result;
	UINT32 tMask, cMask, vMask;

	if (bits > 31 || bits < 2)
		return -EINVAL;
	else
	{
		tMask =  (0xffffffff << bits);
		cMask = (0xffffffff >> (33 - bits));
		vMask = (0xffffffff >> (32 - bits));
	}

	newTime &= vMask;
	oldTime &= vMask;

	if ((newTime & tMask) != 0 || (oldTime &  cMask) != 0)
		return -EINVAL;

	result = newTime - oldTime;

	// fix overflows
	result &=  ~tMask;

	return result;
}

// Returns a signed x bit delta taking into account clock rollover.
// Assumes no real negative time diff larger than 1/2 of the max time of 0xfffff.
// Adjusts for clock overflow (e.g. 0 - 0x0fffff = 1 or 0x0fffff - 0 = -1)
static inline SINT32 _deltaClockBits( UINT32 bits, UINT32 newTime, UINT32 oldTime )
{
	SINT32 result;
	UINT32 tMask, sMask, vMask;

	if (bits > 31  || bits < 2)
		return -EINVAL;
	else
	{
		tMask =  (0xffffffff >> (32-bits+1));
		vMask =  (0xffffffff >> (32-bits));
		sMask = (0x00000001 << (bits));
	}

	newTime &= vMask;
	oldTime &= vMask;

	result = newTime - oldTime;

	// fix overflows
	if (result < (0 - (SINT32)tMask))
	{
//		fprintf(stderr,"Up:%x:%x:%x\n",tMask,0-tMask,result);
		result = newTime - (oldTime - sMask);
//		fprintf(stderr,"U:%x:%x:%x\n",oldTime,newTime,result);
	}

	else if (result > (SINT32)tMask)
	{
//		fprintf(stderr,"Op:%x:%x:%x\n",newTime,oldTime,result);
		result = newTime - (oldTime + sMask);
//		fprintf(stderr,"O:%x:%x:%x\n",newTime,oldTime,result);
	}


	return result;
}

SINT32 teulClockBits(UINT32 bits)
{
	return _clockBits(bits);
}

SINT32 teulDeltaClockBits(UINT32 bits, UINT32 newTime, UINT32 oldTime )
{
	return _deltaClockBits(bits,newTime,oldTime);
}

static inline UINT32
_teulClock(void)
{
	return (tclock & 0x000fffff);				// 20 bit limit
}

// 	----- teulClock

//////////////////////////////////////////////////////////////////////
// Flush the time cache and update teulClock64 and teulTime (since
// teulTime is just teulClock64 / 1000)

SINT32
teulSyncClock(void)
{
#if platOS == OS_LINUX
	{
		struct timeval tv;
		gettimeofday(&tv,0);
		const TeulTimeMS newTimeMS = ((TeulTimeMS)tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL);
		if ( newTimeMS < teulClock64 )
		{
			teulClock64++;
			teulClockTime = teulClock64 / 1000ULL;
		}
		else
		{
			teulClock64 = newTimeMS;
			teulClockTime = (UINT32)tv.tv_sec;
		}
		tclock = (UINT32)teulClock64;
	}
#elif platOS == OS_WIN32
	static const DWORD  BaseClockTime = timeGetTime();
	static const TeulTimeMS BaseClock64 = time(NULL) * 1000ULL + BaseClockTime % 1000ULL;

	teulClock64 = BaseClock64 + (timeGetTime() - BaseClockTime);
	teulClockTime = (UINT32)(teulClock64 / 1000LL);
	tclock = (UINT32)(teulClock64);
#elif platOS == OS_MACOS
	UnsignedWide microTickCount;
	Microseconds(&microTickCount);
	teulClock64 = UnsignedWideToUInt64(microTickCount) / 1000;
	teulClockTime = (UINT32)(teulClock64 / 1000LL);
	tclock = (UINT32)(teulClock64);
#else
#pragma error("No teulSyncClock implementation for this platform")
#endif
	return _teulClock();
}

// returns the current clock value truncated to 20 bits
UINT32 teulClock(void)
{
//	teulSyncClock();
	return _teulClock();
}

// Adds  an unsigned 20 bit time values with a signed 20bit delta .
// Returns an unsigned 20 bit timeval, - val indicates error.
// Truncates overflows (e.g. 0x0fffff + 1 = 0).
SINT32 teulAddClock(UINT32 time, SINT32 delta)
{
	SINT32 result;

	if ((time & 0xfff00000) != 0  || (delta > 0x7ffff) || (delta < (0-0x7ffff)) )
		return -EINVAL;

	result = time + delta;

	// fix overflows
	result &=  0x000fffff;

	return result;
}

// Returns a signed 20 bit delta taking into account clock rollover.
// Assumes no real negative time diff larger than 1/2 of the max time of 0xfffff.
// Adjusts for clock overflow (e.g. 0 - 0x0fffff = 1 or 0x0fffff - 0 = -1)
SINT32 teulDeltaClock( UINT32 newTime, UINT32 oldTime )
{
	SINT32 result;

	result = newTime - oldTime;

	//. fix overflows
	if (result < (0 - 0x7ffff))
		result = newTime - (oldTime - 0x0100000);

	else if (result > 0x7ffff)
		result = newTime - (oldTime + 0x0100000);

	return result;
}

// same as teulDeltaClock but newTime = teulClock()
SINT32 teulElapsedClock(UINT32 oldTime)
{
	return teulDeltaClock(teulClock(),oldTime);
}

// wrapper for inet_ntoa.
// converts UINT32. to doted quad string.
// stores in a static array of char
// that is in turn returned.
// this allows up to 16 addresses to be returned
// before we get an overwrite.
const SCHAR*
teulGetAddress(const UINT32 hostOrderIP)
{
	const UINT32 netOrderIP = htonl(hostOrderIP);
	in_addr thost;
	thost.s_addr = netOrderIP;
	char* addr = inet_ntoa(thost);
	if  ( addr == NULL)
		return NULL;

	static SCHAR ntoa[16][16];
	static SINT32 count = 0; 	// NO THREAD SAFE
	SINT32 slot = (count++) & 0x0f;

	strncpy(ntoa[slot], addr ,16);

	return ntoa[slot];
}

#if platOS != OS_WIN32
const char*
teulGetAddress(const UINT32 host, char* buffer, const size_t bufferSize)
{
	in_addr addr;
	addr.s_addr = htonl(host);
	return inet_ntop(AF_INET, &addr, buffer, bufferSize);
}
#else
const char*
teulGetAddress(const UINT32 host, char* buffer, const size_t bufferSize)
{
	const char* p = teulGetAddress(host);
	strncpy(buffer, p, bufferSize);
	return buffer;
}
#endif

namespace Teul
{

//////////////////////////////////////////////////////////////////////
// Convert an IP address and port to strings.

int
HostToAddr(const UINT32 host_, const UINT16 port_, char* const hostBuf_, const size_t hostBufLen_, char* const portBuf_, const size_t portBufLen_)
{
	sockaddr_in sa;
	sa.sin_addr.s_addr = host_;
	sa.sin_family = AF_INET;
	sa.sin_port = port_;
	return getnameinfo((sockaddr*)&sa, sizeof(sa), hostBuf_, hostBufLen_, portBuf_, portBufLen_, NI_NUMERICHOST | NI_NUMERICSERV);
}

//////////////////////////////////////////////////////////////////////
// Convert an IP address and port to strings.

int
HostToAddr(const struct sockaddr_in* const sa, char* const hostBuf_, const size_t hostBufLen_, char* const portBuf_, const size_t portBufLen_)
{
	return getnameinfo((sockaddr*)&sa, sizeof(sa), hostBuf_, hostBufLen_, portBuf_, portBufLen_, NI_NUMERICHOST | NI_NUMERICSERV);
}

#if platOS != OS_WIN32

//////////////////////////////////////////////////////////////////////
// Specify handler function for a given signal.

int
SetSignalHandler(int signum, void (*handler)(int))
{
	struct sigaction action;
	// Get the pre-existing setup.
	const int err = sigaction(signum, NULL, &action);
	if ( err < 0 )
		return err;

	action.sa_handler = handler;
	sigfillset(&action.sa_mask);
	action.sa_flags = SA_RESETHAND;

	return sigaction(signum, &action, NULL);
}
#endif
}
