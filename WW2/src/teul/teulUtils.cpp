#include "syscfg.h" // Path is project specific.
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

#if platOS == OS_WIN32
#include <mmsystem.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#endif

#include <atomic>
#include <chrono>

///TODO: We need these to be atomic or thread local or something.
TeulTimeMS teulClock64;
UINT32 teulClockTime;

// 	----- teulClock

//////////////////////////////////////////////////////////////////////
// Flush the time cache and update teulClock64 and teulTime (since
// teulTime is just teulClock64 / 1000)

void
teulSyncClock()
{
	static std::mutex oneAtATime;
	std::lock_guard<std::mutex> lock(oneAtATime);

	using namespace std::chrono;
	auto now = system_clock::now();
	TeulTimeMS newTimeMS = duration_cast<milliseconds>(now.time_since_epoch()).count();
#if platBuild == BUILD_SERVER
	if ( PN_UNLIKELY(newTimeMS < teulClock64) )
		LogAbort("Time went backwards: %" PR64BitUT " -> %" PR64BitUT "", teulClock64, newTimeMS);
#endif
	// avoid calculations when called many times a millisecond
	if ( newTimeMS > teulClock64 ) {
		teulClock64 = newTimeMS;
		teulClockTime = static_cast<UINT32>(newTimeMS / 1000ULL);
	}
}

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
	strsafecpy(buffer, p, bufferSize);
	return buffer;
}
#endif

namespace Teul
{

//////////////////////////////////////////////////////////////////////
// Convert an IP address and port to strings.

int
HostToAddr(const UINT32 host_, const UINT16 port_, char* const hostBuf_, const size_t hostBufLen_, char* const portBuf_, const UINT32 portBufLen_)
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
HostToAddr(const struct sockaddr_in* const sa, char* const hostBuf_, const size_t hostBufLen_, char* const portBuf_, const UINT32 portBufLen_)
{
	return getnameinfo((const sockaddr*)(&sa), sizeof(sa), hostBuf_, hostBufLen_, portBuf_, portBufLen_, NI_NUMERICHOST | NI_NUMERICSERV);
}

#if platOS != OS_WIN32

//////////////////////////////////////////////////////////////////////
// Specify handler function for a given signal.

int
SetSignalHandler(int signum, void (*handler)(int))
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
}
