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
#include "server/serverConfigVars.h"
#include "wwii/wwiiNetPorts.h"

CVAR_ADDR(dbsePrimaryAddress, NULL, "DBSE_PRIMARY_ADDRESS", true
			, "IP address dbd clients will reach the dbd on.");

static SINT32 hostIndex = 0;
static CONNECTION_STATUS connected = CONNECT_NONE;
static LOGICAL _handshakeSent;
static bool _dbseError = true;

static SINT32 dbseOpenConnection();

extern void dbseSetConnectionStatus(CONNECTION_STATUS status)
{
	connected = status;
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

SINT32
dbseInit( )
{
	SINT32 returnVal = -1;
	static bool initialized = false;
	if (initialized == false)
	{
		initialized = true;
		ABORT_IF(cv_dbsePrimaryAddress.GetUint32() == 0);

		dbugLog(_FLI,"Installing dbseCallbacks");
		dbseInstallClientCallbacks();
	}

//	connected = CONNECT_NONE;
	hostIndex = dbseOpenConnection();
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
					dbugLog(_FLI, "t:%d dbd client connected %d", hostIndex, connected);
					dbseSendHandshake(hostIndex, cv_arenaID.GetUint32());
					_handshakeSent = TRUE;
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
				dbugLog(_FLA, "Unhandled connection status in dbseInit!");
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
	return 0;
}

static void _connectFn(ENDPOINT& endpoint)
{
	dbugLog(_FLI, "t:%d dbd client", endpoint.teulIndex());
	connected = CONNECT_OPEN;
}

static void _errorFn (SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	_dbseError = true;
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
		LogAbort("dbse connection refused.");
	}
    else
    {
		dbugLog(_FLA, "t:%d dbse connection error %d reported.", teulIndex, error);
		teulClose(teulIndex, Teul::Closing::DBDErr);
		connected = CONNECT_DROPPED;
	}
}

static SINT32
dbseOpenConnection()
{
	const char* dbseAddress = cv_dbsePrimaryAddress.GetString();
	static const UINT16 dbsePort = WWII::Net::Ports::Dbse;
	dbugLog(_FLI ,"Connecting to dbse: %s:%u", dbseAddress, dbsePort);

	_handshakeSent=FALSE;
	_dbseError = false;

	const SINT32 tempIndex = teulOpenSend(cv_dbsePrimaryAddress.GetUint32(), dbsePort, _connectFn, 2500, PROTOCOL_TCP, _errorFn);
	if ( tempIndex < 0)
	{
		dbugLog(_FLA,"couldnt make dbseDaemon connection, err:%d", hostIndex);
		_errorFn(hostIndex, NULL, 0, NULL, tempIndex);
		return tempIndex;
	}

	while ( connected != CONNECT_OPEN )
	{
		teulUpdateTransport(25);

		if ( _dbseError )
			LogAbort("Unable to complete dbse connection.");
	}

	return tempIndex;
}
