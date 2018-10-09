#include <syscfg.h> // Path is project specific.
// Created: Thu Jun 15 15:26:30 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "auth/authConstants.h"
#include "auth/authGameMessages.h"
#include "auth/authPack.h"
#include "teul/teulAuth.h"
#include "teul/teulAuthServer.h"
#include "teul/teulBasic.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"

TEUL_CALLBACK*	authGAME_INIT;
TEUL_CALLBACK*	authSERVER_LOAD;
TEUL_CALLBACK*	authGAME_CAPACITY;

static TEUL_CALLBACK*	_AUTH_DISPATCH;
static TEUL_CALLBACK*	_MANAGEMENT_MSG;
static TEUL_CALLBACK*	_AUTH_GAME_ACK;
static TEUL_CALLBACK*	_INIT_LOGIN;
static TEUL_CALLBACK*	_PLAYER_LOGIN;
static TEUL_CALLBACK*	_PLAYER_LOGIN_REPLY;

static ERROR_FUNCTION _externalErrorFn;


//Callback function declaraton
static SINT32 _recvDISPATCH(ACTION_OBJECT* const a);
static SINT32 _sendGAME_INIT(ACTION_OBJECT* const a);
static SINT32 _sendManagementMsg(ACTION_OBJECT* const a);
static SINT32 _sendClearLoginTable(ACTION_OBJECT* const a);
static SINT32 _recvGAME_ACK(ACTION_OBJECT* const a);
static SINT32 _recvPlayerLogin(ACTION_OBJECT* const a);
static SINT32 _sendPlayerLoginReply(ACTION_OBJECT* const a);
static SINT32 _sendServerLoad(ACTION_OBJECT* const a);
static SINT32 _sendGameCapacity(ACTION_OBJECT* const a);

//This function installs our two callbacks
SINT32 authInstallGameCallbacks(ERROR_FUNCTION externErrorFn)
{
	Teul::CallbackInit ci;

	_externalErrorFn = externErrorFn;

	memset(&ci,0,sizeof(ci));

	//Global values
	ci.callX		= Auth::CallbackX;

	//The recv function setup
	ci.callY		= AUTH_DISPATCH_Y;
	ci.sendAction	= nullptr;
	ci.recvAction	= _recvDISPATCH;

	_AUTH_DISPATCH = teulAllocCallback(&ci);

	//The send function setup
	ci.callY		= AUTH_GAME_INIT_Y;
	ci.sendAction	= _sendGAME_INIT;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((authGAME_INIT = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//The send function setup
	ci.callY		= PLAYER_LOGIN_Y;
	ci.sendAction	= nullptr;
	ci.recvAction	= _recvPlayerLogin;

	if ( PN_UNLIKELY((_PLAYER_LOGIN = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//The send function setup
	ci.callY		= PLAYER_LOGIN_REPLY_Y;
	ci.sendAction	= _sendPlayerLoginReply;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((_PLAYER_LOGIN_REPLY = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//The game ack recv callback
	ci.callY		= AUTH_GAME_ACK_Y;
	ci.sendAction	= nullptr;
	ci.recvAction	= _recvGAME_ACK;

	if ( PN_UNLIKELY((_AUTH_GAME_ACK = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//The managementMsg send function setup
	ci.callY		= GAME_MANAGEMENT_Y;
	ci.sendAction	= _sendManagementMsg;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((_MANAGEMENT_MSG = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//server load send msg
	ci.callY		= SERVER_LOAD_Y;
	ci.sendAction	= _sendServerLoad;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((authSERVER_LOAD = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//game capacity send msg
	ci.callY		= GAME_CAPACITY_Y;
	ci.sendAction	= _sendGameCapacity;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((authGAME_CAPACITY = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

	//The setup for the login init message
	ci.callY		= GAME_CLEAR_LOGIN_TABLE_Y;
	ci.sendAction	= _sendClearLoginTable;
	ci.recvAction	= nullptr;

	if ( PN_UNLIKELY((_INIT_LOGIN = teulAllocCallback(&ci)) == nullptr) )
		return (0-dbugGetErrno());

#ifdef DEBUG
	dbugWrite(_FLN,"installed auth game callbacks");
#endif

	return 0;
}

//Send game capacity function
static SINT32 _sendGameCapacity(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(AUTH_GAME_CAPACITY));
}

//Send function
static SINT32 _sendServerLoad(ACTION_OBJECT* const a)
{
	return authPackSERVER_LOAD(TEUL_PACK, a->packed, a->offset, (AUTH_SERVER_LOAD*) a->localRawData, sizeof(AUTH_SERVER_LOAD));
}

//Send function
static SINT32 _sendPlayerLoginReply(ACTION_OBJECT* const a)
{
	PLAYER_LOGIN_REPLY* reply = (PLAYER_LOGIN_REPLY*)(a->localRawData);
	a->packer(TEUL_PACK, reply->attemptID);
	a->packer(TEUL_PACK, reply->resultCode);

	return a->offset;
}

static SINT32 _sendGAME_INIT(ACTION_OBJECT* const a)
{
	return authPackGAME_INIT(TEUL_PACK, a->packed, a->offset, (AUTH_GAME_INIT*) a->localRawData, sizeof(AUTH_GAME_INIT));
}

//This function sends the data describing when someone logs in or out.
static SINT32 _sendManagementMsg(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MANAGEMENT_MSG));
}

//This sends a message telling the server to init the currentonline table
static SINT32 _sendClearLoginTable(ACTION_OBJECT* const a)
{
	return authPackClearLoginTable(TEUL_PACK, a->packed, a->offset, (CLEAR_LOGIN_TABLE*) a->localRawData, sizeof(CLEAR_LOGIN_TABLE));
}

//Receive function

static SINT32
_recvPlayerLogin(ACTION_OBJECT* const a)
{
	PLAYER_LOGIN login;

    SINT32 acceptResult = 0;
    SINT32 sendResult = 0;

	try
	{
		login.packer(TEUL_UNPACK, a);
		if ( PN_UNLIKELY(a->offset < 0) )
			throw std::logic_error("unable to unpack player login");

		char versionNo[4 * 6 + 1];
		teulGetAddress(login.appVersion, versionNo, sizeof(versionNo) - 1);
		dbugLog(_FLI, "t:%d otp:%u app:%s crs:%s", a->teulIndex, login.OTP, versionNo, login.CopyRightString);

		UINT32 destAddress = a->endpoint->destAddress();

		PLAYER_LOGIN_REPLY reply;
		reply.attemptID = login.attemptID;
		reply.resultCode = authAcceptPlayer(a->teulIndex, destAddress, login);
		sendResult = teulSend(a->teulIndex, _PLAYER_LOGIN_REPLY, &reply);

		if ( PN_UNLIKELY(sendResult < 0) )
			throw std::logic_error("send of acceptResult failed");

		if ( PN_UNLIKELY(acceptResult < 0) )
			throw std::logic_error("acceptResult failed closing conn");

		return a->offset;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d %s. off:%d, sr:%d, ar:%d", a->teulIndex, e.what(), a->offset, sendResult, acceptResult);
		teulClose(a->teulIndex);
		return -EINVAL;
	}
	catch ( ... )
	{
		dbugLog(_FLA, "t:%d !!!Unhandled exception!!! off:%d, sr:%d, ar:%d", a->teulIndex, a->offset, sendResult, acceptResult);
		teulClose(a->teulIndex);
		return -EINVAL;
	}
}

static SINT32 _recvDISPATCH(ACTION_OBJECT* const a)
{
	Auth::Dispatch dispatch;

	dispatch.packer(TEUL_UNPACK, a);

	authRecordDispatch(&dispatch, true);

	return a->offset;
}

//Recv hello ack function
static SINT32 _recvGAME_ACK(ACTION_OBJECT* const a)
{
	AUTH_GAME_ACK ack;

	a->offset = authPackGAME_ACK(TEUL_UNPACK, a->packed, a->offset, &ack, 0);

	if ( PN_UNLIKELY(ack.gotcha != AUTH_HELLO_VALID) )
		return -EINVAL;

	return a->offset;
}

// used to report client actions (login, logout etc..)
// <connection> the auth connection
// <costomerID> the customer ID of the client.
// <gameID> the game that is being played.
// <actionType> action (login, logout, etc..)
// <timestamp> when the action occurred.
SINT32
authSendClientAction(SINT32 connection, UINT32 arenaID, UINT32 customerID, CLIENT_ACTIONS action, UINT32 ip)
{
	dbugLog(_FLD, "p:%u a:%u ca:%u ip:%u sending client action to auth", customerID, arenaID, action, ip);
	MANAGEMENT_MSG managementMsg(customerID, arenaID, action, ip);
	SINT32 retval = teulSend(connection, _MANAGEMENT_MSG, &managementMsg);
	if ( PN_UNLIKELY(retval < 0) )
		dbugLog(_FLA, "sending management message returned %d", retval);

	return retval;
}

// used to clear the current client connection table.  Sent at the
//	beginning to whack any stale data from the current connection table.
//	<connection> is the connection the message should go out on
SINT32
authSendGameInit(SINT32 connection)
{
	CLEAR_LOGIN_TABLE clearLoginTable;
	SINT32 retval;

	clearLoginTable.init=1;

	if ( (retval = teulSend(connection, _INIT_LOGIN, &clearLoginTable)) < 0)
	{
		dbugWrite(_FLA, "Something went wrong in sending Login Initialization.\n");
	}
	else
	{
		dbugWrite(_FLN,"Sent Login Initialization.");
	}

	return retval;
}

