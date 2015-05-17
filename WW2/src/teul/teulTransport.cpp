#include <syscfg.h> // Path is project specific.
// This module provides high-level access to the transport
// functions used in the teul API. The end-user developer
// should need only those functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include <event2/event.h>
#include <signal.h>

#include "mem/mem.h"
#include "server/serverInterfaceDescription.h"
#include "teul/teulBasic.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulModule.h"
#include "teul/teulTime.h"
#include "teul/teulVersion.h"

#include "host/hostTiming.h"

#if platOS != OS_WIN32
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#if platOS == OS_WIN32
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

extern SINT32 teulModStartup(ERROR_FUNCTION*);
extern SINT32 teulOpenConnection(const SINT32 teulIndex, const bool listening, const SINT32 timeout, const PROTOCOL protocol, const UINT32 host, const UINT16 port, CONNECT_FUNCTION* connectFn, ERROR_FUNCTION* errorFn);

SCHAR* teulModuleName;
SCHAR* teulAppName;

Teul::LoggingFn Teul::g_logFnPtr = [](const char* const, ...) {};

static bool _transportInited, _transportClosing;

// For client network selection
UINT32 _hostAddressMask = 0;

namespace Teul
{
	UINT32 GetSingleIP(const char* const name)
	{
		struct addrinfo hints;	
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;

		struct addrinfo* result = nullptr;
		int res = getaddrinfo(name, nullptr, &hints, &result);
		if ( res < 0 || result == nullptr )
			return 0;

		UINT32 retval = ntohl(((struct sockaddr_in*) result->ai_addr)->sin_addr.s_addr);

		freeaddrinfo(result);

		return retval;
	}

	bool GetIPs(const char* const name, std::vector<UINT32>& ips)
	{
		struct addrinfo hints;	
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;

		struct addrinfo* result = nullptr;
		int res = getaddrinfo(name, nullptr, &hints, &result);
		if ( res < 0 )
			return false;

		for ( auto rp = result; rp != nullptr; rp = rp->ai_next )
		{
			ips.push_back(ntohl(((struct sockaddr_in*) rp->ai_addr)->sin_addr.s_addr));
		}

		freeaddrinfo(result);

		return !ips.empty();
	}
}

// wrapper for inet_aton.
// converts a doted quad string to a UINT32.
// returns 0 if address is invalid.
bool
teulGetHost(const char* address_, UINT32* host_)
{
	if (strcmp(address_, "localhost") == 0)
	{
		*host_ = INADDR_LOOPBACK;
		return true;
	}

	UINT32 host = (*host_) = 0;
	auto ptResult = inet_pton(AF_INET, address_, (struct in_addr*)(&host));
	if (ptResult < 1)
		return false;
	*host_ = ntohl(host);
	return true;
}

// When the connection is open and connected this function
// will execute a user function.
void
ENDPOINT::doConnect()
{
	if (PN_LIKELY(this->connectFn != nullptr))
	{
		dbugLog(_FLN, "t:%d connected calling %p", this->teulIndex(), this->connectFn);
		this->connectFn(*this);
	}
	else if ( protocol != PROTOCOL_UDB && protocol != PROTOCOL_CTRL )	// UDB doesn't connect.
	{
		dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " state:%u prot:%u connected NO CONNECT FN", this->teulIndex(), PRA_SOCKET(this->socket), this->state(), this->protocol);
	}
}

// any new connections accepted on an ENPOINT owned will
// use this function to call the user function notifying
// the user app of the new connection.
void
ENDPOINT::doAccept(ENDPOINT& newEndpoint)
{
	if (PN_LIKELY(this->connectFn))
	{
		dbugLog(_FLD, "t:%d doing handshaking calling doAccept(t:%d)", this->teulIndex(), newEndpoint.teulIndex());
		this->connectFn(newEndpoint);
	}
	else
	{
		dbugLog(_FLD, "t:%d done handshaking t:%d", this->teulIndex(), newEndpoint.teulIndex());
	}
}

void
teulSetPersonaData(const SINT32 teulIndex, void* personaData)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if (PN_LIKELY(endpoint != nullptr))
		endpoint->appConnectionData = personaData;
}

void*
teulGetPersonaData(const SINT32 teulIndex)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if (PN_LIKELY(endpoint != nullptr))
		return endpoint->appConnectionData;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Code common to all of the listen functions.

SINT32
_openListen(const UINT32 ipAddress
			, const UINT16 portNumber
			, CONNECT_FUNCTION* acceptFnPtr
			, const SINT32 timeout
			, const PROTOCOL protocol
			, ERROR_FUNCTION* errorFnPtr
			)
{
	SINT32 retval;

	teulSyncClock();

	const SINT32 teulIndex = teulQueryNextFreeIndex();
	if (PN_UNLIKELY(teulIndex < 0))
	{
		dbugSetErrno(ENOSPC);
		return -ENOSPC;
	}

	retval = teulOpenConnection(
				teulIndex, true, timeout
				, protocol, ipAddress, portNumber
				, acceptFnPtr, errorFnPtr
				);
	if (PN_UNLIKELY(retval < 0))
	{
		dbugLog(_FLA, "t:%d teulOpenConn failed %d", teulIndex, retval);
		return retval;
	}

	return teulIndex;
}


// opens a port for listening. Assumes that all callbacks have been properly
// installed.  returns the number of the teulIndex associated with this
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
SINT32 teulOpenListen(const UINT16 portNumber
					, CONNECT_FUNCTION* acceptFnPtr
					, const SINT32 timeout
					, const PROTOCOL protocol
					, ERROR_FUNCTION* errorFnPtr
					)
{
	if (PN_UNLIKELY(_transportClosing))
		return -EDEADLK;

	dbugLog(_FLD,"opening listen port on address ? port %d",portNumber);

	return _openListen(0
					,  portNumber
					,  acceptFnPtr
					,  timeout
					,  protocol
					,  errorFnPtr
					);
}

#if platOS == OS_LINUX || platBuild == TEUL_SERVER
// opens a port for listening on all address associated with the specified interface. "eth0" only!
// Assumes that all callbacks have been properly installed.  returns -error_number or 0 on success.
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
bool teulOpenListenInterface(const UINT16 portNumber
					, CONNECT_FUNCTION* acceptFnPtr
					, const SINT32 timeout
					, ERROR_FUNCTION* errorFnPtr
					)
{
	if (PN_UNLIKELY(_transportClosing))
		return false;

	SINT32 retval;
	for (UINT32 i = 0; i < Server::myInterface.numIP; ++i)
	{
		dbugLog(_FLD,"opening listen port on address %s port %d", teulGetAddress(Server::myInterface.addr[i].ip), portNumber);

		retval = _openListen( Server::myInterface.addr[i].ip
						,  portNumber
						,  acceptFnPtr
						,  timeout
						,  PROTOCOL_TCP
						,  errorFnPtr
						);
		if (retval < 0)
			return false;
	}

	return true;
}


// opens a port for listening on ipAddress. Assumes that all callbacks have been properly
// installed.  returns the number of the teulIndex associated with this
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
SINT32 teulOpenListenHost(const UINT32 ipAddress
					, const UINT16 portNumber
					, CONNECT_FUNCTION* acceptFnPtr
					, const SINT32 timeout
					, const PROTOCOL protocol
					, ERROR_FUNCTION* errorFnPtr
					)
{
	if (PN_UNLIKELY(_transportClosing))
		return -EDEADLK;

	dbugLog(_FLD,"opening listen port on  address %s port %d", teulGetAddress(ipAddress), portNumber);

	return _openListen(	ipAddress
					,  portNumber
					,  acceptFnPtr
					,  timeout
					,  protocol
					,  errorFnPtr
					);

}
#endif

//////////////////////////////////////////////////////////////////////
// During login the client can select a network to use from multiple
// possible network connections. Subsequently all connection attempts
// are subject to this mask.

void
teulSetNetworkMask(UINT32 address)
{
	// Only keep the low 8 bits
	_hostAddressMask = address & ~0xff;
}

UINT32
teulMaskNetworkAddress(UINT32 address)
{
	if ( !_hostAddressMask )
		return address;
	return _hostAddressMask | (address & 0xff);
}

//////////////////////////////////////////////////////////////////////
// opens a connection for sending. Assumes that all callbacks have been properly installed.
// Host is a dotted quad combined into a single long using the formula:
// [A].[B].[C].[D] = (A<<24) + (B<<16) + (C<<8) + D;
// returns the number of the teulIndex associated with this connection.
// connectFnPtr is an optional ptr to callback function which is called when
// a connection is established to the destination socket, and hence a new connection
// is created.  pass nullptr if you do not want this notification
SINT32
teulOpenSend(UINT32 host
			, const UINT16 portNumber
			, CONNECT_FUNCTION* connectFnPtr
			, const SINT32 timeout
			, const PROTOCOL protocol
			, ERROR_FUNCTION* errorFnPtr)
{
	if (PN_UNLIKELY(_transportClosing))
		return -EDEADLK;

	// Have we selected a particular network?
	host = teulMaskNetworkAddress(host);

	teulSyncClock();

	const SINT32 teulIndex = teulQueryNextFreeIndex();
	if (PN_UNLIKELY(teulIndex < 0))
	{
		dbugLog(_FLA, "no more connections available");
		dbugSetErrno(ENOSPC);
		return -ENOSPC;
	}

	const SINT32 retval = teulOpenConnection(teulIndex, false, timeout, protocol, host, portNumber, connectFnPtr, errorFnPtr);
	if (PN_UNLIKELY(retval < 0))
	{
		dbugLog(_FLA, "t:%d teulOpenConn failed %d", teulIndex, retval);
		return retval;
	}

	return teulIndex;
}

static UINT32 rawTeulLibVersion;

UINT32 teulGetVersion()
{
	if (_transportInited)
	{
		teulGetHost(Teul::VersionString, &rawTeulLibVersion);
		return rawTeulLibVersion;
	}
	else
		return (Teul::VersionNumber << 16);
}

namespace Teul
{
	event_base* eventBase;
}

// For libevent logging.
static void
_logLibEvent(int eventLevel, const char* msg)
{
	int dbugLevel;
	switch ( eventLevel )
	{
	case _EVENT_LOG_ERR: dbugLevel = Debug::ERR_ABORT; break;
	case _EVENT_LOG_WARN: dbugLevel = Debug::ERR_WARN; break;
	case _EVENT_LOG_MSG: dbugLevel = Debug::ERR_INFO; break;
	default: dbugLevel = Debug::ERR_DBUG; break;
	}
	dbugLog("libevent", 0, dbugLevel, "%s", msg);
}

static void
_initializePlatform()
{
#if platOS == OS_WIN32
	WSADATA wsaData;
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD( 2, 2 );

	const SINT32 err = WSAStartup(wVersionRequested, &wsaData);
	if ( err != 0 )
	{
		LogAbort(_FLA, "WSAStartup failed: %d",err);
	}

	// Confirm that the WinSock DLL supports 2.2.
	// Note that if the DLL supports versions greater
	// than 2.2 in addition to 2.2, it will still return
	// 2.2 in wVersion since that is the version we
	// requested.
	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
		LogAbort("Wrong version of winsock library.");
	// The WinSock DLL is acceptable. Proceed.
#endif

	event_set_log_callback(_logLibEvent);

	// Set up the configuration.
	event_config* const cfg = event_config_new();
	event_config_set_flag(cfg, EVENT_BASE_FLAG_NOLOCK | EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
	Teul::eventBase = event_base_new_with_config(cfg);
	event_config_free(cfg);

	ABORT_IF( Teul::eventBase == nullptr );
}

// We all have to start somewhere and this one function is it for teul.
// this function opens the debugger,  on the client initializes the teul module
// and on the server acquires and interface to communicate to the teulModule.
// it also records the users default error and kill functions.
SINT32
teulInitTransport(SINT32 logType
					, const char* name
					, ERROR_FUNCTION* errorFnPtr
					, void (*killFnPtr)(int)
					, Teul::LoggingFn const logFnPtr)
{
	if ( _transportInited )
	{
		dbugWrite(_FLAS, "can't initialize teul transport twice, must close first");
		return -EINVAL;
	}

	if ( _transportClosing )
	{
		dbugWrite(_FLAS, "can't initialize teul transport during shutdown");
		return -EDEADLK;
	}

	dbugInit(logType, name);

	dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
#ifdef CLOSED_BETA
	dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)STD_ERR));
#else
	dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
#endif

	teulSyncClock();

	_transportInited = true;

	dbugLog(_FLI, "Initializing TeulKit transport [compiled %s %s]", __DATE__, __TIME__);

	_transportClosing = false;

	if ( logFnPtr )
		Teul::g_logFnPtr = logFnPtr ;

	_initializePlatform();

	teulFreeAllCallbacks();

	const SINT32 retval = teulModStartup(errorFnPtr);
	if ( retval < 0 )
	{
		dbugLog(_FLA,"module wont Init");
		return retval;
	}

	teulGetVersion();

	if (killFnPtr)
		signal(SIGINT, killFnPtr);

	return 0;
}

// Since we hae to ensure that callbacks happen in step with whatever
// application is using teul, we have to make a call for that all to happen.
// should be called once per game loop
void
teulUpdateTransport(const TeulTimeMS& duration_)
{
	if (PN_UNLIKELY(_transportClosing))
		return;

#if platBuild == TEUL_SERVER
	ABORT_UNLESS_EX(_transportInited, "transport can't update without being initialized.");
#else
	if (PN_UNLIKELY(_transportInited == false))
		return;
#endif

	teulModIdle(duration_);

#if platOS == OS_LINUX || platBuild == TEUL_SERVER
	extern void serverUpdate(void);
	serverUpdate();
#endif
}

// close all connections and the debugger. (teul go bye bye)
void teulCloseTransport(void)
{
	if(! _transportInited || _transportClosing)
		return;

	_transportClosing = TRUE;

#if platOS == OS_LINUX
	extern void serverFree(void);
	serverFree();
#endif

	teulModShutdown();

	dbugClose();

	teulFreeAllCallbacks();

	_transportInited = false;
}

// after you have opened your connection installed your CALLBACK,
// point the CALLBACK->outData to your data structure and call this
// function to send it.

extern SINT32 teulModSendCALLBACKe(ENDPOINT& endpoint, TEUL_CALLBACK& callback);
extern SINT32 teulModSendCALLBACK(SINT32 teulIndex, TEUL_CALLBACK& callback);

SINT32
teulSend(ENDPOINT& endpoint, TEUL_CALLBACK* const object, void* const data)
{
	if (PN_UNLIKELY(object == nullptr))
	{
		dbugLog(_FLA, "teulSend called with null callback");
		return -EINVAL;
	}

	object->outData = data;

	return teulModSendCALLBACKe(endpoint, *object);
}

SINT32
teulSend(SINT32 teulIndex, TEUL_CALLBACK* const object, void* const data)
{
	if (PN_UNLIKELY(object == nullptr))
	{
		dbugLog(_FLA, "teulSend called with null callback");
		return -EINVAL;
	}

	if (PN_UNLIKELY(teulIndex < 0))
	{
		dbugLog(_FLW, "t:%d illegal", teulIndex);
		return teulIndex;
	}

	object->outData = data;

	return teulModSendCALLBACK(teulIndex, *object);
}

SINT32
teulSendTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK* callback, void* data)
{
	if (PN_UNLIKELY(callback == nullptr))
	{
		dbugLog(_FLA,"illegal callback");
		return -EINVAL;
	}

	if (PN_UNLIKELY(teulIndex < 0))
	{
		dbugLog(_FLA,"illegal index %d", teulIndex);
		return teulIndex;
	}

	callback->outData = data;

	return teulModSendCallbackTo(teulIndex, address, port, *callback);
}


static char _appName[64] = "undefined";

namespace Teul
{

void SetAppName(const char* const name)
{
	memset(_appName, 0, sizeof(_appName));
	strncpy(_appName, name, sizeof(_appName) - 1);
}

const char*
GetAppName()
{
	return _appName;
}

bool IsTransportInited() { return _transportInited; }
bool IsTransportClosing() { return _transportClosing; }

} // namespace Teul

