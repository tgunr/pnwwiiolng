#include <syscfg.h> // Path is project specific.
// Created: Wed Oct  1 23:44:02 CDT 2008
//
// Test harness for connection to auth
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testSuite.h"

#include "teul/teulKit.h"
#include "teul/teulEndpoint.h"
#include "chat/chatHandle.h"
#include "wwii/wwiiCallbacks.h"
#include "teul/teulNetworkObject.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiUTF.h"

using namespace std ;

bool connected = false ;
bool error = false ;

static void
_connectFn(ENDPOINT& /*endpoint*/)
{
	connected = true ;
}

static void
_errorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 errorNo)
{
	if ( errorNo == ENOTCONN )
	{
		return ;
	}
	fprintf(stderr, "%llu got error %d attempting connection\n", teulClock64, errorNo) ;
	error = true ;
}

//
static int
_runTest(UINT32 address)
{
	connected = error = false ;

	SINT32 teulIndex = teulOpenSend(address, WWII::Net::Ports::AuthHost, _connectFn, 60000, PROTOCOL_TCP, _errorFn) ;
	if ( teulIndex < 0 )
	{
		fprintf(stderr, "connection refused\n") ;
		return -1 ;
	}

	teulSyncClock() ;
	TeulTimeMS cutoff = teulClock64 + 30000 ;
	printf("%llu starting\n", teulClock64) ;
	while ( !connected && !error && teulClock64 < cutoff )
	{
		teulUpdateTransport(5) ;
	}

	teulClose(teulIndex) ;

	if ( error )
		return -1 ;

	if ( !connected )
	{
		assert( teulClock64 >= cutoff ) ;
		fprintf(stderr, "connection attempt timed out\n") ;
		return -3 ;
	}

	teulSyncClock() ;
	printf("%llu success.\n", teulClock64) ;

	return 0 ;
}


int
main(int argc, char* argv[])
{
	Teul::ConfigVar::InitializeConfigVars();

	teulInitTransport(LOG_TO_FILE, "./testAuthConnect", NULL) ;

	teulSyncClock() ;

	dbugSetLogLevel(Debug::ERR_DBUG) ;

	UINT32* const address = teulGetHostByName("auth.playnet.com") ;
	if ( address == NULL )
	{
		fprintf(stderr, "failed to resolve auth.playnet.com\n") ;
		return -2 ;
	}

	for ( UINT32 i = 0 ; i < 10 ; ++i )
	{
		int result = _runTest(*address) ;
		if ( result < 0 )
			return result ;
	}

	return 0 ;
}

