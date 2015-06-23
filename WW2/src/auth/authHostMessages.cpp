#include <syscfg.h> // Path is project specific.
// Created: Mon Jul 31 13:32:26 2000
//
// The code behind the messages the host will be sending to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "teul/teulBasic.h"
#include "auth/authTypes.h"
#include "auth/authPack.h"
#include "auth/authClientData.h"
#include "auth/authConstants.h"
#include "auth/authGame.h"
#include "auth/authHost.h"
#include "auth/authDB.h"
#include "auth/authPlaygateTypes.h"
#include "auth/authQueuePosition.h"
#include "wwii/wwiiCallbacks.h"

TEUL_CALLBACK*	authAPP_LAUNCH;
TEUL_CALLBACK*	authAPP_VERIFY;

TEUL_CALLBACK*	authDISPATCH;
TEUL_CALLBACK*	authGAME_ACK;

static TEUL_CALLBACK*	_APP_PLAYGATE_OK;
static TEUL_CALLBACK*	_XCHG_GAME_LIST;
static TEUL_CALLBACK*	_XCHG_GAME_LIST2;
static TEUL_CALLBACK*	_XCHG_AUTH_LOGIN;
static TEUL_CALLBACK*	_SEND_QUEUE_POSITION;

//Send Login Reply function
static SINT32 _sendAPP_PLAYGATE_OK(ACTION_OBJECT* const a)
{
	return a->offset;
}

//Send Login Reply function
static SINT32 _sendAPP_VERIFY(ACTION_OBJECT* const a)
{
	return authPackAPP_VERIFY(TEUL_PACK, a->packed, a->offset, (APP_VERIFY*) a->localRawData, 0);
}

//Send Login Reply function
static SINT32 _sendAPP_LAUNCH(ACTION_OBJECT* const a)
{
	return authPackAPP_LAUNCH(TEUL_PACK, a->packed, a->offset, (APP_LAUNCH*) a->localRawData, 0);
}

//Receive Login Request function
static SINT32 _recvARENA_REQ(ACTION_OBJECT* const a)
{
	ARENA_REQ request;

	a->offset = authPackARENA_REQ(TEUL_UNPACK, a->packed, a->offset, &request, 0);

	authProcessArenaRequest(a->teulIndex, request);

	return a->offset;
}

//Receive Login Request function
static SINT32 _recvARENA_REQ2(ACTION_OBJECT* const a)
{
	ARENA_REQ request;

	a->offset = authPackARENA_REQ2(TEUL_UNPACK, a->packed, a->offset, &request, 0);

	authProcessArenaRequest(a->teulIndex, request);

	return a->offset;
}

static SINT32 _recvAuthLoginRequest(ACTION_OBJECT* const a)
{
	Auth::LoginRequest request(a);
	request.process(a->teulIndex);
	return a->offset;
}

static SINT32 _sendAuthLoginReply(ACTION_OBJECT* const a)
{
	Auth::LoginReply* reply = (Auth::LoginReply*)(a->localRawData);
	return reply->packer(TEUL_PACK, a);
}

//Receive Login Request function
static SINT32 _recvAPP_LAUNCH_REQ(ACTION_OBJECT* const a)
{
	APP_LAUNCH_REQ request;

	a->offset = authPackAPP_LAUNCH_REQ(TEUL_UNPACK, a->packed, a->offset, &request, 0);

	authLaunchRequest(a->teulIndex, request);

	return a->offset;
}

//Receive a hello from the game host
static SINT32 _recvGAME_INIT(ACTION_OBJECT* const a)
{
	AUTH_GAME_INIT helloMessage;

	a->offset = authPackGAME_INIT(TEUL_UNPACK, a->packed, a->offset, &helloMessage, sizeof(AUTH_GAME_INIT));

	authEstablishServer(a->teulIndex, helloMessage);

	return a->offset;
}

//Send Game Cookie function
static SINT32 _sendAUTH_DISPATCH(ACTION_OBJECT* const a)
{
	((Auth::Dispatch*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

//Send an ack of the hello
static SINT32 _sendGAME_ACK(ACTION_OBJECT* const a)
{
	return authPackGAME_ACK(TEUL_PACK, a->packed, a->offset, (AUTH_GAME_ACK*) a->localRawData, sizeof(AUTH_GAME_ACK));
}

MANAGEMENT_MSG::MANAGEMENT_MSG(ACTION_OBJECT* const a)
{
	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, this, sizeof(*this));
}

//Receive a managementMsg
static SINT32
_recvManagementMsg(ACTION_OBJECT* const a)
{
	SERVER* serverEntry = (SERVER*)teulGetPersonaData(a->teulIndex);
	if ( serverEntry == nullptr )
	{
		dbugLog(_FLA,"no server info avalable on %d:",a->teulIndex);
		return 0;
	}

	MANAGEMENT_MSG message(a);
	switch (message.m_action)
	{
		// Login
		case CLIENT_ENTER:
			serverEntry->game->currentUsers++;
			if ( serverEntry->currentUsers < serverEntry->game->currentUsers )
				serverEntry->currentUsers++;
		break;

		// Logout
		case CLIENT_LEAVE:
			if (serverEntry->currentUsers > 0)
				serverEntry->currentUsers--;
			if (serverEntry->game->currentUsers > 0)
				serverEntry->game->currentUsers--;
			serverEntry->game->CheckLoginQueue();
		break;

		// Compiler will complain if we do not cover all enumerations.
	}

	message.process();

	dbugLog(_FLN, "p:%u a:%u ip:%s status:%s server:%s server-users:%u game-users:%u"
					, message.m_customerID
					, serverEntry->game->arenaID
					, teulGetAddress(message.m_ip)
					, (message.m_action == CLIENT_ENTER ? "enter" : (message.m_action == CLIENT_LEAVE ? "leave" : "unk"))
					, teulGetAddress(serverEntry->ipAddress)
					, serverEntry->currentUsers
					, serverEntry->game->currentUsers
					);

	return a->offset;
}

static SINT32
_recvClearLoginTable(ACTION_OBJECT* const a)
{
	CLEAR_LOGIN_TABLE clearLoginTable;

	a->offset = authPackClearLoginTable(TEUL_UNPACK, a->packed, a->offset, &clearLoginTable, sizeof(CLEAR_LOGIN_TABLE));

	if (clearLoginTable.init == 1)
	{
		dbugLog(_FLN,"t:%d received login initialization message", a->teulIndex);
		authResetConnectInfo(1);
	}

	return a->offset;
}

static SINT32
_recvAPP_FILE_CHECK(ACTION_OBJECT* const a)
{
	APP_FILE_CHECK afc;
	UINT32 platform;

	a->offset = authPackAPP_FILE_CHECK(TEUL_UNPACK, a->packed, a->offset, &afc, sizeof(afc));

	platform = afc.presenterVersion >> 24;

	dbugLog(_FLD, "t:%d Received AFC for %d.%d:=%s"
					, a->teulIndex
					, afc.appID
					, platform
					, afc.fileKey
					);

	authVerifyChecksum(a->teulIndex, afc.appID, platform, (char*)afc.fileKey);

	return a->offset;
}

static SINT32
_recvSERVER_LOAD(ACTION_OBJECT* const a)
{
	static AUTH_SERVER_LOAD message;
	a->offset = authPackSERVER_LOAD(TEUL_UNPACK, a->packed, a->offset, &message, sizeof(AUTH_SERVER_LOAD));
	handleServerLoadUpdate(message);

	return a->offset;
}

static SINT32
_recvGAME_CAPACITY(ACTION_OBJECT* const a)
{
	AUTH_GAME_CAPACITY message;
	memset(&message, 0, sizeof(message));
	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &message, sizeof(AUTH_GAME_CAPACITY));
	handleGameCapacityUpdate(message);

	return a->offset;
}

// Deprecated GameList
static SINT32
_recvAppGameListRequest(ACTION_OBJECT* const a)
{
	AppGameList list;
	list.process();
	teulSend(a->teulIndex, _XCHG_GAME_LIST, &list);
	return a->offset;
}

static SINT32
_sendAppGameList(ACTION_OBJECT* const a)
{
	AppGameList* list = (AppGameList*)(a->localRawData);
	return list->packer(TEUL_PACK, a);
}

// New Game List
static SINT32
_recvGameListRequest(ACTION_OBJECT* const a)
{
	Auth::GameList* list = authGetGameList(a->teulIndex);
	if ( list == nullptr )
		return -EINVAL;
	if ( teulSend(a->teulIndex, _XCHG_GAME_LIST2, list) < 0 )
		return -EINVAL;
	return a->offset;
}

static SINT32
_sendGameList(ACTION_OBJECT* const a)
{
	Auth::GameList* list = (Auth::GameList*)(a->localRawData);
	return list->packer(TEUL_PACK, a);
}

static SINT32
_sendAuthQueuePosition(ACTION_OBJECT* const a)
{
	return ((Auth::QueuePosition*)(a->localRawData))->packer(TEUL_PACK, a);
}

SINT32
authSendAppPlaygateOk(SINT32 teulIndex)
{
	return teulSend(teulIndex, _APP_PLAYGATE_OK, nullptr);
}

void
Auth::LoginReply::send(SINT32 teulIndex)
{
	teulSend(teulIndex, _XCHG_AUTH_LOGIN, this);
}

void
authSendPlayerInQueue(Auth::ClientData* cdata, UINT32 position, UINT32 waitTime)
{
	Auth::QueuePosition update(position, waitTime);
	teulSend(cdata->teulIndex, _SEND_QUEUE_POSITION, &update);
}

//This function installs our two callbacks
SINT32 authHostInstallCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Auth::CallbackX)

		REGISTER_RECEIVER(ARENA_REQ_Y, _recvARENA_REQ);
		REGISTER_RECEIVER(APP_LAUNCH_REQ_Y, _recvAPP_LAUNCH_REQ);
		REGISTER_RECEIVER(AUTH_GAME_INIT_Y, _recvGAME_INIT); //	ci->error =		_hostRecvAuthHelloError;

		REGISTER_RECEIVER(ARENA_REQ2_Y, _recvARENA_REQ2);

		REGISTER_SENDER(APP_VERIFY_Y, _sendAPP_VERIFY, authAPP_VERIFY);
		REGISTER_SENDER(APP_LAUNCH_Y, _sendAPP_LAUNCH, authAPP_LAUNCH);
		REGISTER_SENDER(AUTH_DISPATCH_Y, _sendAUTH_DISPATCH, authDISPATCH); //	ci->error =		_hostSendGameCookieError;
		REGISTER_SENDER(AUTH_GAME_ACK_Y, _sendGAME_ACK, authGAME_ACK); // 	ci->error =		_hostSendHelloAckError;

		REGISTER_RECEIVER(GAME_MANAGEMENT_Y, _recvManagementMsg); //	ci->error =		_hostRecvManagementMsgError;
		REGISTER_RECEIVER(GAME_CLEAR_LOGIN_TABLE_Y, _recvClearLoginTable); // _hostRecvManagementMsgError
		REGISTER_RECEIVER(SERVER_LOAD_Y, _recvSERVER_LOAD);
		REGISTER_RECEIVER(GAME_CAPACITY_Y, _recvGAME_CAPACITY);

		REGISTER_RECEIVER(APP_FILE_CHECK_Y, _recvAPP_FILE_CHECK);

		REGISTER_SENDER(APP_PLAYGATE_OK_Y, _sendAPP_PLAYGATE_OK, _APP_PLAYGATE_OK);
		REGISTER_SENDRECV(APP_GET_GAME_LIST_Y, _sendAppGameList, _recvAppGameListRequest, _XCHG_GAME_LIST);
		REGISTER_SENDRECV(APP_XCHG_GAME_LIST_Y, _sendGameList, _recvGameListRequest, _XCHG_GAME_LIST2);
		REGISTER_SENDRECV(AUTH_XCHG_LOGIN_Y, _sendAuthLoginReply, _recvAuthLoginRequest, _XCHG_AUTH_LOGIN);
		REGISTER_SENDER(AUTH_QUEUE_POSITION_Y, _sendAuthQueuePosition, _SEND_QUEUE_POSITION);

	END_CALLBACK_REGISTRATION
}

