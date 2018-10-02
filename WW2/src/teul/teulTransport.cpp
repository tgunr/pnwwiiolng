#include "syscfg.h" // Path is project specific.
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

#include "mem/mem.h"
#include "server/serverInterfaceDescription.h"
#include "teul/teulBasic.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulModule.h"
#include "teul/teulTime.h"
#include "teul/teulVersion.h"

static bool _transportInited, _transportClosing;

namespace Teul
{
	UINT32 GetSingleIP(const char* const name)
	{
		struct addrinfo hints{};
		hints.ai_family = AF_INET;

		struct addrinfo* result = nullptr;
		int res = getaddrinfo(name, nullptr, &hints, &result);

        if (res != 0) {
            dbugLog(_FLD, "getaddrinfo() error: %d (%s)", res, gai_strerror(res)); // gai_strerror() is not threadsafe, but neither is the rest of Teul
            return 0;
        }
        assert(result);

		UINT32 retval = ntohl(reinterpret_cast<struct sockaddr_in *>(result->ai_addr)->sin_addr.s_addr);

		freeaddrinfo(result);

		return retval;
	}
}

// wrapper for inet_aton - converts a doted quad string to a UINT32
// returns 0 if address is invalid
bool teulGetHost(const char *address_, UINT32 *host_)
{
	if (strcmp(address_, "localhost") == 0) {
		*host_ = INADDR_LOOPBACK;
		return true;
	}

	auto ptResult = inet_pton(AF_INET, address_, reinterpret_cast<struct in_addr *>(host_));
    
    if (ptResult < 1) {
		return false;
    }
	*host_ = ntohl(*host_);
	return true;
}

// When the connection is open and connected this function will execute a user function.
void ENDPOINT::doConnect()
{
    assert(protocol == PROTOCOL_TCP || protocol == PROTOCOL_UDP);
	if (this->connectFn) {
		dbugLog(_FLN, "t:%d connected calling callback", this->teulIndex());
		this->connectFn(*this);
	}
}

// any new connections accepted on an ENPOINT will call the user function to notify of the new connection
void ENDPOINT::doAccept(ENDPOINT& newEndpoint)
{
    dbugLog(_FLD, "t:%d done handshaking calling doAccept(t:%d)", this->teulIndex(), newEndpoint.teulIndex());
	if (this->connectFn) {
		this->connectFn(newEndpoint);
	}
}

void teulSetPersonaData(const SINT32 teulIndex, void *personaData)
{
	if (ENDPOINT * const endpoint = teulGetEndpoint(teulIndex)) {
		endpoint->appConnectionData = personaData;
    }
}

void *teulGetPersonaData(const SINT32 teulIndex)
{
	if (ENDPOINT* const endpoint = teulGetEndpoint(teulIndex)) {
		return endpoint->appConnectionData;
    }
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Code common to all of the listen functions.

// opens a port for listening. Assumes that all callbacks have been properly
// installed.  returns the number of the teulIndex associated with this
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
SINT32
teulOpenListen(const UINT16 portNumber
					, CONNECT_FUNCTION acceptFnPtr
					, const SINT32 timeout
					, const PROTOCOL protocol
					, ERROR_FUNCTION errorFnPtr
					)
{
	dbugLog(_FLD,"opening listen port on address 0.0.0.0 port %hu", portNumber);
    return teulOpenConnection(true, timeout, protocol, 0, portNumber, acceptFnPtr, errorFnPtr, false);
}

#if platBuild == BUILD_SERVER
// opens a port for listening on all address associated with the specified interface.
// Assumes that all callbacks have been properly installed.  returns -error_number or 0 on success.
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
bool teulOpenListenInterface(const UINT16 portNumber
					         , CONNECT_FUNCTION acceptFnPtr
                             , const SINT32 timeout
                             , ERROR_FUNCTION errorFnPtr
					         , const bool initBackChannel
)
{
	if (PN_UNLIKELY(Teul::IsTransportClosing())) {
		return false;
    }

	for (UINT32 i = 0; i < Server::myInterface.numIP; ++i) {
		///TODO: See how this handles actual multiple ips: do the callers appreciate
		///TODO: the fact that they might be getting these calls with different
		///TODO: teulIndexes? Do they cache them?
		dbugLog(_FLD,"opening listen port on address %s port %d", teulGetAddress(Server::myInterface.addr[i].ip), portNumber);
        
        SINT32 retval = teulOpenConnection(true, timeout, PROTOCOL_TCP, Server::myInterface.addr[i].ip, portNumber, acceptFnPtr, errorFnPtr, initBackChannel);
        
		if (retval < 0) {
			return false;
        }
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
					      , CONNECT_FUNCTION acceptFnPtr
					      , const SINT32 timeout
					      , const PROTOCOL protocol
					      , ERROR_FUNCTION errorFnPtr
)
{
	dbugLog(_FLD,"opening listen port on address %s port %d", teulGetAddress(ipAddress), portNumber);
    return teulOpenConnection(true, timeout, protocol, ipAddress, portNumber, acceptFnPtr, errorFnPtr, false);
}
#endif

//////////////////////////////////////////////////////////////////////
// opens a connection for sending. Assumes that all callbacks have been properly installed.
// Host is a dotted quad combined into a single long using the formula:
// [A].[B].[C].[D] = (A<<24) + (B<<16) + (C<<8) + D;
// returns the number of the teulIndex associated with this connection.
// connectFnPtr is an optional ptr to callback function which is called when
// a connection is established to the destination socket, and hence a new connection
// is created.  pass nullptr if you do not want this notification
SINT32 teulOpenSend(UINT32 host, UINT16 portNumber, CONNECT_FUNCTION connectFnPtr, SINT32 timeout, PROTOCOL protocol, ERROR_FUNCTION errorFnPtr, bool initBackChannel)
{
	return teulOpenConnection(false, timeout, protocol, host, portNumber, connectFnPtr, errorFnPtr, initBackChannel);
}

//////////////////////////////////////////////////////////////////////
// libevent junk
namespace Teul
{
	event_base* eventBase;
}

// For libevent logging.
static void _logLibEvent(int eventLevel, const char* msg)
{
	int dbugLevel = Debug::ERR_NOTE;
	
    switch (eventLevel) {
        case _EVENT_LOG_ERR: dbugLevel = Debug::ERR_ABORT; break;
        case _EVENT_LOG_WARN: dbugLevel = Debug::ERR_WARN; break;
        case _EVENT_LOG_MSG: dbugLevel = Debug::ERR_INFO; break;
        default: dbugLevel = Debug::ERR_DBUG; break;
	}
	dbugLog("libevent", 0, dbugLevel, "%s", msg);
}

#if platOS == OS_WIN32
void Teul::InitializeWinsock()
{
    static std::once_flag winsockInitialized;
	
    std::call_once(winsockInitialized, []() {
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
        SINT32 err = WSAStartup(wVersionRequested, &wsaData);
        
		if (err != 0) {
			LogAbort(_FLA, "WSAStartup failed: %d", err);
        }

		// Confirm that the WinSock DLL supports 2.2.
		// If the DLL supports versions greater than 2.2 in addition to 2.2, it
        // will still return 2.2 in wVersion since that is the version we requested.
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
			LogAbort("Wrong version of winsock library.");
        }
	});
}
#endif

static void _initializeLibEvent()
{
	event_set_log_callback(_logLibEvent);

	// Set up the configuration.
	event_config* const cfg = event_config_new();
	event_config_set_flag(cfg, EVENT_BASE_FLAG_NOLOCK | EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
    
	Teul::eventBase = event_base_new_with_config(cfg);
	event_config_free(cfg);

	ABORT_IF(!Teul::eventBase);
}

// We all have to start somewhere and this one function is it for teul.
// This function opens the debugger, on the client initializes the teul module,
// and on the server acquires and interface to communicate to the teulModule.
// It also records the default error and kill functions.
SINT32 teulInitTransport(LogMode logType, const char* name, ERROR_FUNCTION errorFnPtr, void (*killFnPtr)(int))
{
    // This shouldn't be here. Get it out of here when we can.
    dbugInit(logType, name);
    
#ifndef NDEBUG
    dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)LOG_ALL_TO_STDERR));
    dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)LOG_ALL_TO_STDERR));
#else
    dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
#if defined(ALPHA_RELEASE)
    dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)STD_ERR));
#else
    dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
#endif
#endif


	if (_transportInited) {
		dbugLog(_FLA, "can't initialize teul transport twice, must close first");
		return -EINVAL;
	}
    assert(!_transportClosing);

#if platOS == OS_WIN32
    // In host builds, this was already done by InitializeConfigVars(), but oh well
    Teul::InitializeWinsock();
#endif
    _initializeLibEvent();

	teulInitClock();

	_transportInited = true;
    _transportClosing = false;
    teulModStartup(errorFnPtr);
	dbugLog(_FLI, "Initializing TeulKit transport [compiled %s %s]", __DATE__, __TIME__);

	if (killFnPtr) {
		signal(SIGINT, killFnPtr);
    }
	return 0;
}

// Since we have to ensure that callbacks happen in step with whatever application is using teul,
// we have to make a call for that all to happen. Should be called once per game loop.
// This is not actually honored and reentrancy rampantly makes a mess of a lot of things.
void teulUpdateTransport(const TeulTimeMS duration_)
{
#if platBuild == BUILD_SERVER
    ABORT_UNLESS_EX(_transportInited, "transport can't update without being initialized.");
#else
    if (!_transportInited) {
        return;
    }
#endif

	if (_transportClosing) {
		return;
    }

	teulModIdle(duration_);

#if platBuild == BUILD_SERVER
	extern void serverUpdate();
	serverUpdate();
#endif

	// update any backchannels
	teulEndpointBackchannels();
}

// close all connections and the debugger. (teul go bye bye)
void teulCloseTransport()
{
	if (!_transportInited || _transportClosing) {
		return;
    }

	_transportClosing = true;

#if platBuild == BUILD_SERVER
	extern void serverFree();
	serverFree();
#endif
	teulModShutdown();
	dbugClose();
	teulFreeAllCallbacks();
}

// after you have opened your connection installed your CALLBACK,
// point the CALLBACK->outData to your data structure and call this
// function to send it.
extern SINT32 teulModSendCALLBACKe(ENDPOINT& endpoint, TEUL_CALLBACK& callback, const UCHAR protocol);
extern SINT32 teulModSendCALLBACK(SINT32 teulIndex, TEUL_CALLBACK& callback, const UCHAR protocol);

SINT32 teulSend(ENDPOINT& endpoint, TEUL_CALLBACK *const object, void* const data, const UCHAR protocol)
{
	if (!object) {
		dbugLog(_FLA, "teulSend called with null callback");
		return -EINVAL;
	}

	object->outData = data;
	return teulModSendCALLBACKe(endpoint, *object, protocol);
}

SINT32 teulSend(SINT32 teulIndex, TEUL_CALLBACK* const object, void* const data, const UCHAR protocol)
{
	if (!object) {
		dbugLog(_FLA, "teulSend called with null callback");
		return -EINVAL;
	}

	if (teulIndex < 0) {
		dbugLog(_FLW, "t:%d illegal index", teulIndex);
		return -EINVAL;
	}
    
	object->outData = data;
	return teulModSendCALLBACK(teulIndex, *object, protocol);
}

SINT32 teulSendTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK* callback, void* data)
{
	if (!callback) {
		dbugLog(_FLA,"illegal callback");
		return -EINVAL;
	}

	if (teulIndex < 0) {
        dbugLog(_FLW, "t:%d illegal index", teulIndex);
        return -EINVAL;
	}

	callback->outData = data;
	return teulModSendCallbackTo(teulIndex, address, port, *callback);
}


namespace Teul
{
    static char _appName[64] = "undefined";

    void SetAppName(const char* const name)
    {
        strsafecpy(_appName, name);
    }

    const char *GetAppName()
    {
        return _appName;
    }

    bool IsTransportInited() { return _transportInited; }
    bool IsTransportClosing() { return _transportClosing; }
}
