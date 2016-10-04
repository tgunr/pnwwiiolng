#include <syscfg.h> // Path is project specific.
// Created: Tue Jun 13 13:32:25 2000
//
// Playgate's teulKit API
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Maintaining Programmer:  Oliver "kfs1" Smith

#include "comm/commPlaygateMessages.h"
#include "launchApp.h"
#include "include/infoMessages.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "teul/teulEndpoint.h"

static bool		_isConnected = false;
static SINT32	_connectionError = 0;
static SINT32	_teulIndex = -1;

//////////////////////////////////////////////////////////////////////
// Test if we have a valid connection open

static bool
_haveOpenConnection()
{
	return ( _teulIndex >= 0 && _isConnected == true && _connectionError != ENOTCONN );
}

//////////////////////////////////////////////////////////////////////
// Callback invoked when teulOpenSend connects.

static void
_connectionOpened(ENDPOINT& endpoint)
{
	_teulIndex = endpoint.teulIndex();
	_isConnected = true;
	_connectionError = 0;
}

//////////////////////////////////////////////////////////////////////
// Callback invoked when there is a transport-level error on a
// connection; if the error is ENOTCONN then the connection has
// been closed.

static void
_networkError(
	SINT32 /*teulIndex*/
	, const UCHAR* const /*data*/
	, SINT32 /*size*/
	, const TEUL_CALLBACK* /*object*/
	, SINT32 error
	)
{
	if ( _haveOpenConnection() )
	{
		_isConnected = false;
		_teulIndex = -1;
	}
	_connectionError = error;
}

//////////////////////////////////////////////////////////////////////
// Callback invoked by low-level, internal teulKit errors (such as
// unregistered callbacks, etc).

static void
_teulKitError(
	SINT32 /*teulIndex*/
	, const UCHAR* const /*data*/
	, SINT32 /*size*/
	, const TEUL_CALLBACK* /*object*/
	, SINT32 error
	)
{
	playgate.AlertOK(GetLoc(ERROR_INIT, error));
}

//////////////////////////////////////////////////////////////////////
// Callback invoked when a callback encounters an error.

static void
_callbackError(
	const SINT32 teulIndex
	, const UCHAR* const /*data*/
	, const SINT32 /*size*/
	, const TEUL_CALLBACK* /*object*/
	, const SINT32 error
	)
{
	const std::pair<UINT32, UINT16>& dest = Teul::GetRemote(teulIndex);
	playgate.AlertOK(GetLoc(ERROR_DEBUG
								, teulIndex
								, teulGetAddress(dest.first)
								, dest.second
								, error));
}

//////////////////////////////////////////////////////////////////////
// Initialize Playgate's comm functions (network transport).

bool
commInit(void)
{
	if (teulInitTransport(LOG_TO_FILE, "PlayGATE", _teulKitError, nullptr))
		return false;

#ifdef DEBUG
	dbugSetLogLevel(Debug::ERR_DBUG);
	dbugSetWriteLevel(Debug::ERR_DBUG);
#else
	dbugSetLogLevel(Debug::ERR_INFO);
	dbugSetWriteLevel(Debug::ERR_INFO);
#endif

	launchInstallCallbacks();
	infoInstallCallbacks();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Shutdown the network transport.

void
commKill(void)
{
	// If we have an active connection, shut it down.
	commCloseConnection();
	// Close out all the teulKit transport stuff.
	teulCloseTransport();
}

//////////////////////////////////////////////////////////////////////
// Open a connection to the authHost and return the teulIndex.

bool
commOpenConnection(UINT32 HostNumber, UINT16 Port)
{
	// If we already have a connection open, shut it down.
	commCloseConnection();

	teulUpdateTransport(25);

	// Open a socket; it's not yet a connection though,
	// because teulKit has stuff to do before it is ready
	// to hand off to us.
	const SINT32 ConnectionNumber = teulOpenSend(HostNumber, Port, _connectionOpened, 3000, PROTOCOL_TCP, _networkError);
	if (ConnectionNumber < 0)
		return false;

	// Allow teulKit 45 seconds to connect to the remote
	// host, do its handshakes and hand over to us.
	const TeulTimeMS cutoff = teulClock64 + 45000;
	while ( teulClock64 < cutoff && !_isConnected && _connectionError == 0 )
	{
		teulUpdateTransport(25);
	}

	if ( _connectionError != 0 )
	{
		if ( _connectionError != ENOTCONN )
			teulClose(ConnectionNumber);

		dbugSetErrno(_connectionError);

		return false;
	}

	if ( _isConnected == false )
	{
		teulClose(ConnectionNumber);
		return false;
	}

	_teulIndex = ConnectionNumber;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Close the teulConnection.

void
commCloseConnection()
{
	// Check if we actually think we're connected at all.
	if ( _haveOpenConnection() )
	{
		// Flag ourselves as disconnected so that we don't
		// see the ENOTCONN as a genuine error.
		_isConnected = false;
		// NOW close the connection.
		teulClose(_teulIndex);
		// Give teulKit a moment to consume that.
		teulUpdateTransport(5);
	}

	_teulIndex = -1;
	_isConnected = false;
	_connectionError = 0;
	dbugSetErrno(0);
}

//////////////////////////////////////////////////////////////////////
// Process network updates. Returns false if there are no connections.

bool
commUpdate(void)
{
	teulUpdateTransport(5);

	return ( _haveOpenConnection() );
}

//////////////////////////////////////////////////////////////////////
// Register a callback.

TEUL_CALLBACK*
commInstallMessage(
	UCHAR MessageGroup
	, UCHAR MessageNumber
	, ACTION_REF SendFunc
	, ACTION_REF ReceiveFunc
	)
{
	Teul::CallbackInit ci;

	ci.callX         = MessageGroup;
	ci.callY         = MessageNumber;
	ci.sendAction    = SendFunc;
	ci.recvAction    = ReceiveFunc;
	ci.error         = _callbackError;

	return (teulAllocCallback(&ci));
}

//////////////////////////////////////////////////////////////////////
// Send data via a callback.
SINT32
commSend(TEUL_CALLBACK* object, void*  data)
{
	if ( !_haveOpenConnection() )
		return -ENOTCONN;
	return teulSend(_teulIndex, object, data);
}

//////////////////////////////////////////////////////////////////////
// Satisfy teulKit requirements
#include "teul/teulConnectionInstruction.h"

int
Teul::ConnectionInstruction::receive(SINT32 teulIndex)
{
	return -EINVAL;
}
