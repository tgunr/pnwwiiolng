#include <syscfg.h> // Path is project specific.
// Created: Thu Sep  7 13:58:57 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "dbse/dbseTypes.h"
#include "dbse/dbseClient.h"
#include "dbse/dbseClientMessages.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpoint.h"

#define		DBSE_PORT		9000

struct DBSEHOST
{
	UINT16	port;
	UINT32	ip;
} ;

static SINT32 hostIndex = 0;
static CONNECTION_STATUS connected = CONNECT_NONE;
static LOGICAL _handshakeSent;
static bool _dbseError = true ;

static SCHAR primaryAddress[128];
static UINT32 arenaID;

static SINT32 dbseOpenConnection(DBSEHOST& host);

static void _getenv();

extern void dbseSetConnectionStatus(CONNECTION_STATUS status)
{
	connected = status ;
}

SINT32
dbseSend(TEUL_CALLBACK* action, void*object)
{
	if (connected == CONNECT_DROPPED)
		dbseInit();

	if (connected == CONNECT_ACCEPT)
		return teulSend(hostIndex,action,object);
	else return 0;
}

static void
_initHostAddresses(DBSEHOST& primary)
{
	primary.port = DBSE_PORT;
	teulGetHost(primaryAddress, &primary.ip) ;
}

SINT32
dbseInit( )
{
	SINT32 returnVal = -1;
	static SINT32 initialized = 0;
	DBSEHOST dbseHost ;

	if (initialized == 0)
	{
		_getenv();

		dbugLog(_FLI,"Installing dbseCallbacks");
		dbseInstallClientCallbacks();
		initialized = 1;
	}

	_initHostAddresses(dbseHost) ;

//	connected = CONNECT_NONE;
	hostIndex = dbseOpenConnection(dbseHost) ;
	for ( ; ; )
	{
		teulUpdateTransport(30);
		switch (connected)
		{
			case CONNECT_NONE:
				continue;
			case CONNECT_BAD:
				returnVal = FAIL_CONNECT;
				break;
			case CONNECT_OPEN:
				if (!_handshakeSent)
				{
					dbugLog(_FLI,"connected %d code %d",hostIndex,connected);
					dbseSendHandshake(hostIndex,arenaID);
					_handshakeSent=TRUE;
				}
				continue;
			case CONNECT_REDIRECTED:
			case CONNECT_REFUSE:
				returnVal = FAIL_VERSION;
				break;
			case CONNECT_ACCEPT:
				returnVal = SUCCESS;
				break;
			default:
				dbugLog(_FLA,"Unhandled connection status in dbseInit!");
		}
		break;
	}

	return (returnVal);
}

SINT32
dbseClose()
{
	if ( hostIndex >= 0 )
		return (teulClose(hostIndex), Teul::Closing::DBDClose);
	return 0 ;
}

static void _connectFn(ENDPOINT& endpoint)
{
	dbugLog(_FLI,"connected %d", endpoint.teulIndex());
	connected = CONNECT_OPEN;
}

static void _errorFn (SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	_dbseError = true ;
    if (error == ENOTCONN)
    {
		if (connected != CONNECT_NONE)
		{
			dbugLog(_FLW, "dbse connection went away");
			connected = CONNECT_DROPPED;
		}
    }
	else if ( error == ECONNREFUSED )
	{
		LogAbort("dbse connection refused.") ;
	}
    else
    {
		dbugLog(_FLA, "t:%d dbse connection error %d reported.", teulIndex, error) ;
		teulClose(teulIndex, Teul::Closing::DBDErr) ;
		connected = CONNECT_DROPPED ;
	}
}

static SINT32
dbseOpenConnection(DBSEHOST& host)
{
	dbugLog(_FLI,"Connecting to dbDaemon on: %s:%d",teulGetAddress(host.ip), host.port);

	_handshakeSent=FALSE;
	_dbseError = false ;

	const SINT32 tempIndex = teulOpenSend(host.ip, host.port, _connectFn, 2500, TCP, _errorFn) ;
	if ( tempIndex < 0)
	{
		dbugLog(_FLA,"couldnt make dbseDaemon connection, err:%d",hostIndex);
		_errorFn(hostIndex, NULL, 0, NULL, tempIndex);
		return tempIndex ;
	}

	while ( connected != CONNECT_OPEN )
	{
		teulUpdateTransport(25);

		if ( _dbseError )
			LogAbort("Unable to complete dbse connection.") ;
	}

	return tempIndex ;
}

static void _getenv()
{
	SCHAR* s;

    arenaID = 0;
    memset(primaryAddress,0,128);

    s = getenv("DBSE_PRIMARY_ADDRESS");

// pointer sanity check
    if(s && *s)
        sprintf(primaryAddress,"%s",s);
    else
	 	LogAbort("Environment not set yet.  Could not fetch DBSE_PRIMARY_ADDRESS");

    s = getenv("ARENA_ID");

// pointer sanity check
    if(s && *s)
        arenaID = atoi(s);
    else
	 	LogAbort("Environment not set yet.  Could not fetch ARENA_ID");
}

