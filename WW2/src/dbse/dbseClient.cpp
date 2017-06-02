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
#include "wwii/wwiiNetPorts.h"
#include "include/configVars.h"

static SINT32 hostIndex = 0;
static Dbse::ConnectionStatus connected = Dbse::ConnectionStatus::None;
static bool _handshakeSent;
static bool _dbseError = true;

static SINT32 dbseOpenConnection();

SINT32 dbseRecvHandshake(ACTION_OBJECT* a, Dbse::Handshake& handshake)
{
	connected = handshake.m_status;
	switch (connected)
	{
	case Dbse::ConnectionStatus::Bad:
		dbugLog(_FLA, "dbse host version mismatch, rejected version %u", handshake.m_version);
		break;

	case Dbse::ConnectionStatus::Refuse:
		dbugLog(_FLA, "dbse host arena mismatch, rejected arena request for arena %u", handshake.m_arenaID);
		break;

	case Dbse::ConnectionStatus::Accept:
		dbugLog(_FLI, "db connection accepted");
		break;

	default:
		LogAbort("ERROR: Unable to determine db connection status\n Abort!\n");
	}

	return a->offset;
}

SINT32
dbseSend(TEUL_CALLBACK* action, void*object)
{
	if (connected == Dbse::ConnectionStatus::Dropped)
	{
		if (!dbseInit())
			return -EINVAL;
	}

	if (connected == Dbse::ConnectionStatus::Accept)
		return teulSend(hostIndex,action,object);
	return 0;
}

bool
dbseInit()
{
	static bool initialized = false;
	if (initialized == false)
	{
		initialized = true;
		ABORT_IF(cv_dbsePrimaryAddress.GetUint32() == 0);

		dbugLog(_FLI,"Installing dbseCallbacks");
		dbseInstallClientCallbacks();
	}

//	connected = ConnectionStatus::None;
	hostIndex = dbseOpenConnection();
	for ( ; ; )
	{
		teulUpdateTransport(30);
		switch (connected)
		{
			case Dbse::ConnectionStatus::None:
				continue;

			case Dbse::ConnectionStatus::Accept:
				return true;

			case Dbse::ConnectionStatus::Bad:
			case Dbse::ConnectionStatus::Refuse:
				return false;

			case Dbse::ConnectionStatus::Open:
				if (!_handshakeSent)
				{
					dbugLog(_FLI, "t:%d dbd client connected %d", hostIndex, static_cast<int>(connected));
					dbseSendHandshake(hostIndex, cv_arenaID.GetUint32());
					_handshakeSent = true;
				}
				continue;

			default:
				dbugLog(_FLA, "Unhandled connection status in dbseInit!");
				return false;
		}
	}
}

SINT32
dbseClose()
{
	if ( hostIndex >= 0 ) {
        teulClose(hostIndex);
		return Teul::Closing::DBDClose;
    }
	return 0;
}

static void _connectFn(ENDPOINT& endpoint)
{
	dbugLog(_FLI, "t:%d dbd client", endpoint.teulIndex());
	connected = Dbse::ConnectionStatus::Open;
}

static void _errorFn(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	_dbseError = true;
    if (error == ENOTCONN)
    {
		if (connected != Dbse::ConnectionStatus::None)
		{
			dbugLog(_FLW, "dbse connection went away");
			connected = Dbse::ConnectionStatus::Dropped;
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
		connected = Dbse::ConnectionStatus::Dropped;
	}
}

static SINT32
dbseOpenConnection()
{
	const char* dbseAddress = cv_dbsePrimaryAddress.GetString();
	static const UINT16 dbsePort = WWII::Net::Ports::Dbse;
	dbugLog(_FLI ,"Connecting to dbse: %s:%u", dbseAddress, dbsePort);

	_handshakeSent = false;
	_dbseError = false;

	const SINT32 tempIndex = teulOpenSend(cv_dbsePrimaryAddress.GetUint32(), dbsePort, _connectFn, cv_dbseConnectionTimeout_ms.GetUint32(), PROTOCOL_TCP, _errorFn);
	if ( tempIndex < 0)
	{
		dbugLog(_FLA,"couldnt make dbseDaemon connection, err:%d", hostIndex);
		_errorFn(hostIndex, nullptr, 0, nullptr, tempIndex);
		return tempIndex;
	}

	while ( connected != Dbse::ConnectionStatus::Open )
	{
		teulUpdateTransport(25);

		if ( _dbseError )
			LogAbort("Unable to complete dbse connection.");
	}

	return tempIndex;
}
