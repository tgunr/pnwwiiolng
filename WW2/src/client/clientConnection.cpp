#include <syscfg.h> // Path is project specific.
// Date: Fri Mar 31 2006
//
// "Find Best Connection" implementation
//
// Establish a connection to the specified "pinghost" to obtain a list
// of the addresses it advertises and determine which is the fastest
// for this client.
//  - Use the network block (X.Y.Z in X.Y.Z.*) as a mask for all
//    subsequent game connections (teulSetNetworkMask)
//  - Do not perform testing if only one address advertised
//  - Log which address is being used to ww2net.log
//  - Manual selection can be forced by setting g_useNetworkRoute
//    to the 1-based index (1 = addresses[0])
//  - When manual selection is enabled, testing is still performed
//    to provide comparison feedback in the logs (user selects #2
//    and then network conditions change and it becomes unfavorable)
//
// Copyright (c) 2006 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/vars.h"

#include <vector>

#include "teul/teulKit.h"
#include "teul/teulTime.h"
#include "wwii/wwiiTypes.h"
#include "wwii/wwiiCallbacks.h"
#include "client/clientNetError.h"
#include "wwii/wwiiNetPorts.h"
#include "fio/fio.h"
#include "rn/rnlogo.h"
#include <OpenThreads/Thread>

static TeulTimeMS _pingSent = 0LL, _pingRxed = 0LL ;
//static TeulTimeMS _connectTime = 0LL ;
static bool _stop = false, _connected = false, _error = false, _timedOut = false ;
static bool _haveAddresses = false ;
static std::vector<UINT32> _addresses ;

extern SINT32 g_useNetworkRoute ;

//////////////////////////////////////////////////////////////////////
// Exchange callbacks

static TEUL_CALLBACK* _RECV_ADDRESS_LIST ;
static TEUL_CALLBACK* _XCHG_PING ;
static UINT16 _testPort = 0 ;
static UINT16 _testProtocol = 0 ;

static SINT32
_recvAddressList(ACTION_OBJECT* const a)
{
	_haveAddresses = true ;
	UINT32 count = 0 ;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &count, 16) ;
	_addresses.resize(count) ;
	for ( UINT32 i = 0 ; i < count ; ++i )
	{
		a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &_addresses[i], 0) ;
	}

	// Now have the host tell us where to do our tests
	a->offset = teulPackUINT16(TEUL_UNPACK, a->packed, a->offset, &_testPort, 0) ;
	a->offset = teulPackUINT16(TEUL_UNPACK, a->packed, a->offset, &_testProtocol, 0) ;

	if ( _testPort == 0 )
	{
		_testPort = WWII::Net::Ports::ConnectionTest ;
		_testProtocol = PROTOCOL_TCP ;
	}

	_stop = true ;

	return a->offset ;
}

static SINT32
_sendPing(ACTION_OBJECT* const a)
{
	if ( !_stop )
	{
		_pingSent = teulClock64 ;
		_pingRxed = 0LL ;
	}
	return a->offset ;
}

static SINT32
_recvPing(ACTION_OBJECT* const a)
{
	_stop = true ;
	_pingRxed = teulClock64 ;
	return a->offset ;
}

static SINT32
_registerCallbacks(void)
{
	static bool haveRegistered = false ;
	if ( haveRegistered )
		return 0 ;

	BEGIN_CALLBACK_REGISTRATION(WWII_X)
		REGISTER_RECEIVER(WWII_ADDRESS_LIST_Y, _recvAddressList) ;
		REGISTER_SENDRECV(WWII_PING_Y, _sendPing, _recvPing, _XCHG_PING) ;
	END_CALLBACK_REGISTRATION
}

//////////////////////////////////////////////////////////////////////
// Connection established handler

static void
_connectFn(ENDPOINT& /*endpoint*/)
{
	_connected = _stop = true ;
}

//////////////////////////////////////////////////////////////////////
// Connection error handler

static void
_errorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	_error = _stop = true ;
}

static void
_quietErrorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	bool stop = true;
}

//////////////////////////////////////////////////////////////////////
// Wait for a given period of time in teulUpdateTransport

static void
_waitFor(UINT32 maxTime_ms)
{
	_stop = _timedOut = false ;
	teulSyncClock() ;
	for ( TeulTimeMS cutoff = teulClock64 + maxTime_ms ; !_stop ; )
	{
		teulUpdateTransport(10) ;
		if ( teulClock64 > cutoff )
			_stop = _timedOut = true ;
	}
}

static void
_closeIndex(SINT32 connection)
{
	teulClose(connection) ;
	_error = _stop = false ;
}

void
_report(const bool timePrefix_, const char* const fmt_, ...)
{
	FILE* const fp = Fio::OpenLogFile() ;
	if ( fp == NULL )
		return ;

	if ( timePrefix_ )
	{
		char myTime[80] ;
#if platOS == OS_WIN32
		time_t now = (time_t)teulTime() ;
		strftime(myTime, sizeof(myTime) - 1, "%a %b %d %X %Y", localtime_r(&now, NULL)) ;
#else
	 	struct tm tmStruct ;
		time_t now = time(NULL) ;
		strftime(myTime, sizeof(myTime) - 1, "%a %b %d %X %Y", localtime_r(&now, &tmStruct)) ;
#endif
		if ( myTime[0] == 0 )
			strcpy(myTime, "[unknown]") ;
		fprintf(fp, "%s ", myTime) ;
	}

	va_list args ;
	va_start(args, fmt_) ;
	vfprintf(fp, fmt_, args) ;
	va_end(args) ;

	fclose(fp) ;
}

//////////////////////////////////////////////////////////////////////
// Obtain a list of available game IPs and if no manual selection
// specified, find the fastest to connect to and use as the network
// mask for future teulkit connections.
// (See file description for more thorough explanation)

void
clientFindBestConnection(UINT32 host)
{
	// Stop any teul masking
	teulSetNetworkMask(0) ;

	_registerCallbacks() ;

	// Open connection
	teulSyncClock() ;	// Mac may need this.
	_report(true, "starting network selection\n") ;
	SINT32 connection = teulOpenSend(host, WWII::Net::Ports::ConnectionList, _connectFn, 60000, PROTOCOL_TCP, _errorFn) ;
	if ( connection < 0 )
	{
		_report(true, "failed creating pingd connection\n") ;
		throw ClientNetError("connect.find.nonefound") ;
	}

	// Wait for connection to be established
	if ( !_connected && !_error )
	{
		_waitFor(15*1000) ;
	}
	if ( _error )
	{
		_report(true, "failed opening pingd connection: %d\n", dbugGetErrno()) ;
		throw ClientNetError("connect.find.cantopen") ;
	}
	if ( !_connected )
	{
		_report(true, "timed out opening pingd connection\n") ;
		throw ClientNetError("connect.find.timeout.network") ;
	}

	// Wait for the address list to arrive
	if ( !_haveAddresses )
	{
		_waitFor(30*1000) ;
	}
	if ( !_haveAddresses )
	{
		if ( _error )
		{
			_report(true, "error waiting for pingd response: %d\n", dbugGetErrno()) ;
			throw ClientNetError("connect.find.nodata") ;
		}
		if ( _timedOut )
		{
			_report(true, "timed out waiting for pingd response\n") ;
			throw ClientNetError("connect.find.timeout.data") ;
		}
		_report(true, "no data from pingd\n") ;
		throw ClientNetError("connect.find.nodata") ;
	}

	// Don't need the connection now
	_closeIndex(connection) ;

	if ( _addresses.empty() )
	{
		_report(true, "no pingd, using the network selection we have: %s\n", teulGetAddress(host)) ;
		teulSetNetworkMask(host) ;
		return ;
	}

	// No point doing the tests if there is only one address
	if ( _addresses.size() == 1 )
	{
		_report(true, "pingd only offered one network, using that: %s\n", teulGetAddress(host)) ;
		teulSetNetworkMask(_addresses[0]) ;
		return ;
	}

	rnlogoUpdateStatus("loading.netperf") ;

	// Try the addresses multiple times and find which gives us
	// the fastest connection
	TeulTimeMS winningTime = 30 * 1000 ;
	UINT32 winningAddress = 0 ;
	TeulTimeMS startTime ;

	std::vector<bool> successes ;
	std::vector<TeulTimeMS> times ;

	successes.resize(_addresses.size()) ;
	times.resize(_addresses.size()) ;

	for ( UINT32 i = 0 ; i < _addresses.size() ; ++i )
	{
		successes[i] = false ;
		times[i] = 0ULL ;

		_waitFor(1000) ;

		_connected = _error = _stop = false ;

		connection = teulOpenSend(_addresses[i], _testPort, _connectFn, 30000, (PROTOCOL)_testProtocol, _quietErrorFn) ;
		if ( connection < 0 )
		{
			_report(true, "option %u (%s:%u) failed %d\n", i, teulGetAddress(_addresses[i]), (UINT32)_testPort, connection) ;
			continue ;
		}

		_report(true, "t:%d %u %s:%u... ", connection, i, teulGetAddress(_addresses[i]), (UINT32)_testPort) ;

		teulSyncClock() ;
		startTime = teulClock64 ;

		if ( !_connected )
		{
			_waitFor(30*1000) ;
		}
		if ( !_connected )
		{
			_report(true, "conn failed: %d\n", i, teulGetAddress(_addresses[i]), (UINT32)_testPort, connection) ;
			_closeIndex(connection) ;
			continue ;
		}

		_stop = false ;
		_pingSent = _pingRxed = 0LL ;

		const SINT32 pingResult = teulSend(connection, _XCHG_PING, NULL) ;
		if ( pingResult < 0 )
		{
			_report(true, "ping failed: %d\n", pingResult) ;
			_closeIndex(connection) ;
			continue ;
		}

		if ( _pingRxed == 0LL )
		{
			_waitFor(30*1000) ;
		}

		if ( _pingRxed )
		{
			teulSyncClock() ;

			successes[i] = true ;
			times[i] = _pingRxed - startTime ;

			_report(true, "ping rxed: %ums\n", times[i]) ;

			if ( times[i] < winningTime || (i > 0 && times[i] <= winningTime + 5 && (rand() % 3) == 1) )
			{
				winningTime = times[i] ;
				winningAddress = _addresses[i] ;
			}
		}
		else
		{
			_report(true, "no ping\n") ;
		}

		_closeIndex(connection) ;
	}

	if ( winningAddress == 0 )
	{
		_report(true, "*** ELECTION FAILED - Don't seem to have a network, using default.\n");
		winningAddress = host ;
	}
	else if ( g_useNetworkRoute > 0 && g_useNetworkRoute <= (SINT32)_addresses.size() )
	{
		// Player has manually forced an address
		_report(true, "Recommended: %s, ", teulGetAddress(winningAddress)) ;
		// Only a n00b would forget to subtract one from the manual selection...
		winningAddress = _addresses[g_useNetworkRoute - 1] ;
		_report(false, " Overriding with user's manual selection: %s\n", teulGetAddress(winningAddress)) ;
	}
	else
		_report(true, "Winner: %s\n", teulGetAddress(winningAddress)) ;

	// Now use this address for all subsequent teulOpenSend operations
	teulSetNetworkMask(winningAddress) ;
}
