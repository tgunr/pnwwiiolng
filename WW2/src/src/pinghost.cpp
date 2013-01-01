#include <syscfg.h> // Path is project specific.
// Created: Thu Jan 18 08:39:26 2001
//
// Pinghost's job is to accept connections so that the
// client can determine which set of IP addresses to the
// game hosts provide the fastest connection.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm
// Supporting Programmer:  Marty "Vandal" Poulin

#include "versionNo.h"

#include "teul/teulEndpoint.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiTypes.h"

typedef std::vector<UINT32> ADDRESS_LIST ;
extern const ADDRESS_LIST& teulGetAddresses() ;

#ifdef BETA_RELEASE
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
	//#define DEF_ERR_LEVEL Debug::ERR_NOTE
#endif

#define DEF_ERR_FLAGS 0

static TEUL_CALLBACK* _SEND_ADDRESSES ;
static TEUL_CALLBACK* _XCHG_PING ;

static ADDRESS_LIST _addresses ;

//////////////////////////////////////////////////////////////////////

static void
_errorFunction(SINT32 /* teulIndex */, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
}

//////////////////////////////////////////////////////////////////////

static void
_kill(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
	exit (0);
}

static void
_acceptAddrConnection(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d addr connection from %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
	endpoint.setNagleEnabled(false) ;
	teulSend(endpoint.teulIndex(), _SEND_ADDRESSES, NULL) ;
}

static void
_acceptPingConnection(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d ping connection from %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
	endpoint.setNagleEnabled(false) ;
}

static SINT32
_sendAddresses(ACTION_OBJECT* const a)
{
	static UINT16 testPort = WWII::Net::Ports::ConnectionTest ;
	static UINT16 testProtocol = TCP ;
	static bool initialized = false ;
	static UCHAR buf[512] ;
	static int bufSize = 0 ;

	if ( !initialized )
	{
		UINT32 count = _addresses.size() ;
		bufSize = teulPackUINT32(TEUL_PACK, buf, bufSize, &count, 16) ;
		for ( UINT32 i = 0 ; i < count ; ++i )
		{
			bufSize = teulPackUINT32(TEUL_PACK, buf, bufSize, &_addresses[i], 0) ;
		}
		bufSize = teulPackUINT16(TEUL_PACK, buf, bufSize, &testPort, 0) ;
		bufSize = teulPackUINT16(TEUL_PACK, buf, bufSize, &testProtocol, 0) ;

		// Must be a whole bit group
		assert( (bufSize & 7) == 0 ) ;

		// Divide by eight
		bufSize >>= 3 ;

		initialized = true ;
	}

	return teulPackVoid(TEUL_PACK, a->packed, a->offset, buf, bufSize) ;
}

static SINT32
_sendPing(ACTION_OBJECT* const a)
{
	static char data[120] = "x" ;

	if ( data[0] == 'x' )
		memset(data, 0, 120) ;

	a->offset = teulPackVoid(TEUL_PACK, a->packed, a->offset, data, 120) ;

	return a->offset ;
}

static SINT32
_recvPing(ACTION_OBJECT* const a)
{
	dbugLog(_FLD, "t:%d ping received, acknowledging", a->teulIndex) ;
	teulSend(a->teulIndex, _XCHG_PING, NULL) ;
	teulClose(a->teulIndex) ;
	return 0 ;
}

static SINT32
_registerCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(WWII_X)
		REGISTER_SENDER(WWII_ADDRESS_LIST_Y, _sendAddresses, _SEND_ADDRESSES) ;
		REGISTER_SENDRECV(WWII_PING_Y, _sendPing, _recvPing, _XCHG_PING) ;
	END_CALLBACK_REGISTRATION
}

//////////////////////////////////////////////////////////////////////
// Main entry point

int
main(int /*argc*/, char** /*argv*/)
{
	teulInitTransport(LOG_TO_FILE, "pingHost", _errorFunction, _kill);

	dbugSetWriteLevel(Debug::ERR_INFO | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	dbugLog(_FLN,"ping Server starting");

	_registerCallbacks() ;

	//set up the client port, where the FE will connect to on all ips for this interface
	SINT32 addrIndex ;
	addrIndex = teulOpenListenInterface(WWII::Net::Ports::ConnectionList, _acceptAddrConnection, 20000, TCP, _errorFunction);
	if ( addrIndex < 0 )
	{
		dbugLog(_FLA, "Unable to open connection listing interface") ;
		exit(EINVAL) ;
	}

	SINT32 tcpIndex ;
	tcpIndex = teulOpenListenInterface(WWII::Net::Ports::ConnectionTest, _acceptPingConnection, 20000, TCP, _errorFunction);
	if ( tcpIndex < 0 )
	{
		dbugLog(_FLA, "Unable to open connection test interface") ;
		exit(EINVAL) ;
	}

	_addresses = teulGetAddresses() ;

	if ( _addresses.size() == 0 )
		LogAbort("No network interfaces found") ;

	for ( UINT32 i = 0 ; i < _addresses.size() ; ++i )
	{
		dbugLog(_FLI, "address #%u: %s", i, teulGetAddress(_addresses[i])) ;
	}

	while (true)
	{
		teulUpdateTransport(100);
	}

	return 0 ;
}

