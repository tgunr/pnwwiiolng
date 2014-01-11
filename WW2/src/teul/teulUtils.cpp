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

#include <atomic>

TeulTimeMS teulClock64;
UINT32 teulClockTime;

// 	----- teulClock

//////////////////////////////////////////////////////////////////////
// Flush the time cache and update teulClock64 and teulTime (since
// teulTime is just teulClock64 / 1000)

void
teulSyncClock(void)
{
#if platOS == OS_LINUX
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	const TeulTimeMS newTimeMS = ((TeulTimeMS)tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL);
	if ( PN_LIKELY(newTimeMS > teulClock64) )
	{
		teulClock64 = newTimeMS;
		teulClockTime = (UINT32)tv.tv_sec;
	}
	else if (PN_UNLIKELY(newTimeMS < teulClock64))
	{
		teulClock64++;
		teulClockTime = teulClock64 / 1000ULL;
	}
#elif platOS == OS_WIN32
	static const DWORD  BaseClockTime = timeGetTime();
	static const TeulTimeMS BaseClock64 = time(NULL) * 1000ULL + BaseClockTime % 1000ULL;

	teulClock64 = BaseClock64 + (timeGetTime() - BaseClockTime);
	teulClockTime = (UINT32)(teulClock64 / 1000LL);
#elif platOS == OS_MACOS
	UnsignedWide microTickCount;
	Microseconds(&microTickCount);
	teulClock64 = UnsignedWideToUInt64(microTickCount) / 1000;
	teulClockTime = (UINT32)(teulClock64 / 1000LL);
#else
#pragma error("No teulSyncClock implementation for this platform")
#endif
}

#if platOS == OS_LINUX
void testMonotonicClock()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	static uint32_t clockBase = 0;
	static uint64_t clockBase64 = 0;
	if (PN_UNLIKELY(clockBase == 0)) {
		struct timeval tv;
		gettimeofday(&tv, nullptr);
		clockBase = tv.tv_sec - ts.tv_sec;
		clockBase64 = uint64_t(clockBase) * 1000ULL;
	}
	teulClockTime = uint32_t(ts.tv_sec) + clockBase;
	teulClock64 = clockBase64 + uint64_t(ts.tv_sec) * 1000ULL + uint64_t(ts.tv_nsec / 1000000ULL);
}
#endif

// wrapper for inet_ntoa.
// converts UINT32. to doted quad string.
// stores in a static array of char
// that is in turn returned.
// this allows up to 16 addresses to be returned
// before we get an overwrite.
const char*
teulGetAddress(const UINT32 hostOrderIP)
{
	static const size_t NumInstances = 16;
	static char ntoa[NumInstances][(4 * 3) + (3 * 1) + 1];	// 4 3-digit numbers plus 3 dots plus nul
	static std::atomic<size_t> s_instance { 0 };
	const size_t instance = (s_instance++) & (NumInstances - 1);

	const UCHAR* quads = (const UCHAR*)&hostOrderIP;
	// naive implementation: on a little-endian (intel), the octets are stored in reverse order.
	strprintf(ntoa[instance], "%u.%u.%u.%u", quads[3], quads[2], quads[1], quads[0]);

	return ntoa[instance];
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
