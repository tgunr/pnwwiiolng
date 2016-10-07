#include <syscfg.h> // Path is project specific.
// Date: Fri Feb 25 16:10:31 CST 2005
//
// Client Side API for chat communications
//
// Copyright (c) 2005 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBroadcast.h"
#include "chat/chatClientMessage.h"
#include "chat/chatClientMessages.h"
#include "chat/chatConstants.h"
#include "chat/chatDotCommandList.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatHostMessage.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatServerInfo.h"
#include "teul/teulKit.h"
#include "teul/teulNetworkArray.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPack.h"

extern void rosterMessage(Chat::LocalizedMessage& lmsg) ;

static TEUL_CALLBACK* CHAT_SEND_CLIENT_MESSAGE;

static SINT32 _chatConnection=-1;
static LOGICAL _chatClientConnected = FALSE;
//static LOGICAL _chatConnectionError = FALSE;

static UINT32 _chatIPAddress=0;
static UINT16 _chatPort=0;
static UINT32 _chatOTP=0;
static UINT32 _chatReconects = 0;

static void _chatClientConnectFunction(ENDPOINT&);
void _chatClientErrorFunction (SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error);

void chatCloseConnection()
{
	teulClose(_chatConnection);
	_chatClientConnected = FALSE;
}

//This function sends a message to the server to be distributed on the
//	channel referenced by radioIndex
SINT32
chatSendMessage(SINT32 teulIndex, UINT16 radioIndex, const char* message)
{
	Chat::ClientMessage outGoingMessage ;

	//Setup the outgoing message
	outGoingMessage.m_radios = radioIndex;		//set to the radio Index
												//	of the transmitting Index
	outGoingMessage.m_handle = 0LL;
	outGoingMessage.m_country = 0;

	outGoingMessage.m_length = (UINT32)strlen(message);

	if (outGoingMessage.m_length >= MAX_MESSAGE_LENGTH)
		outGoingMessage.m_length = MAX_MESSAGE_LENGTH-1;

	strncpy(outGoingMessage.m_message, message, outGoingMessage.m_length + 1);
	outGoingMessage.m_uiTarget = nullObjID ;

	return teulSend(teulIndex, CHAT_SEND_CLIENT_MESSAGE, &outGoingMessage);
}

SINT32 chatSetChatInfo(SINT32 chatIndex, UINT32 chatIPAddress, UINT16 chatPort)
{
	_chatConnection=chatIndex;
	_chatClientConnected = TRUE;
	_chatIPAddress=chatIPAddress;
	_chatPort=chatPort;

	return 0;
}

void _chatRecvError(SINT32 /*teulIndex*/, UCHAR* const /*data*/, SINT32 /*size*/, TEUL_CALLBACK* /*object*/, SINT32 /*error*/)
{
	// Do nothing.
}

void _chatSendError(SINT32 /*teulIndex*/, UCHAR* const /*data*/, SINT32 /*size*/, TEUL_CALLBACK* /*object*/, SINT32 /*error*/)
{
	// Do nothing.
}

//The actual send function for Messages
static SINT32 _sendMessage(ACTION_OBJECT* const a)
{
	Chat::ClientMessage* msg = (Chat::ClientMessage*)(a->localRawData) ;
	return msg->packer(TEUL_PACK, a) ;
}

//The actual recv function for Messages
static SINT32 _recvMessage(ACTION_OBJECT* const a)
{
	Chat::HostMessage message;
	char callsign[MAX_HANDLE_SIZE];
	char msg[MAX_MESSAGE_LENGTH*2 + 1];

	message.msg = msg;

	a->offset = message.packer(TEUL_UNPACK, a);

	chatGetCallsign(callsign, message.handle);

	printf("%s: %s\n", callsign, msg) ;

	return a->offset;
}

// Initiate a connection to the chat host
static void
_connectToChatHost(void)
{
	_chatClientConnected = FALSE;

	//Connect to the server and send the otp up to it
	_chatConnection = teulOpenSend(_chatIPAddress,
								  (UINT16)_chatPort,
								  _chatClientConnectFunction,
								  300000,
								  PROTOCOL_TCP,
								  _chatClientErrorFunction);
}

static SINT32 _recvServerInfo(ACTION_OBJECT* const a)
{
	ChatServerInfo serverInfo;

	a->offset = serverInfo.packer(TEUL_UNPACK, a) ;

	_chatIPAddress = serverInfo.m_ipAddress;
	_chatPort = serverInfo.m_port;
	_chatOTP = serverInfo.m_otp;

	_connectToChatHost();

	return a->offset;
}

#define MAX_DISP_DATA (MAX_MESSAGE_LENGTH + 64)

static SINT32 _dispChatError(UINT32 /*msgNumber*/, SINT32 /*errCode*/)
{
	char handle[] = "ERROR: ";
	char displayData[MAX_DISP_DATA+1];
	UINT32 i, handleLength;

	handleLength = strlen(handle);

	////////////////////////////////////////
	// Set channels
	// System messages will be highlighted.
	// Set display data to -9 to indicate
	// a sytem message.
	// displayData is translated as a signed
	// char in canvasChat.
	////////////////////////////////////////
	for (i=0; i<5; i++)
		displayData[i] = -9;

	strncpy(displayData + 5, handle, handleLength);

	displayData[MAX_DISP_DATA-1] = '\0';

	for (i=0; i< (strlen(displayData)); i++)
	{
		if (isalnum(displayData[i]) || ispunct(displayData[i]) || isspace(displayData[i]))
			printf("%c",displayData[i]);
		else
			printf("%02x",displayData[i]);
	}
	printf("\n");

	return 0;
}

void
_reconnectChat(SINT32 error)
{
	if ( _chatReconects++ > 3 )
		_dispChatError(822,error);
	else
	{
		_dispChatError(821,error);
		_connectToChatHost();
		if ( _chatConnection < 0 )
			_dispChatError(822,_chatConnection);
	}
}

void _chatClientErrorFunction(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*object*/, SINT32 error)
{
	//If we get disconnected, attempt to reconnect
	if(error == ENOTCONN || error == ECONNRESET)
		_reconnectChat(error);
}

void _chatClientConnectFunction(ENDPOINT& /*endpoint*/)
{
}

static SINT32
_recvLocalisedMessage(ACTION_OBJECT* const a)
{
	Chat::LocalizedMessage lmsg ;
	lmsg.packer(TEUL_UNPACK, a) ;

	rosterMessage(lmsg) ;

	return a->offset ;
}

static SINT32
_recvGCIMessage(ACTION_OBJECT* const a)
{
	if ( a->canRead<Chat::GCIMessage>() == false )
		return -EINVAL ;

	Chat::GCIMessage& gci = a->read<Chat::GCIMessage>() ;

	printf("HQ: %s %d %d\n", gci.name, gci.gci, gci.side);

	return a->offset;
}

static SINT32
_recvCommandList(ACTION_OBJECT* const a)
{
	auto commands = Chat::GetHostCommandsFromStream(a->packed) ;
	printf("Host Comand List received: %" PRSizeT " commands\n", commands.size()) ;
	for ( size_t i = 0 ; i < commands.size() ; ++i )
	{
		printf("%03" PRSizeT "%c %s\n", i, commands[i].playnetOnly ? '*' : ' ', commands[i].command.c_str()) ;
	}
	return a->offset ;
}

//Installs the client callbacks
SINT32
chatInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)
	{

		REGISTER_SENDER(Chat::CLIENT_MESSAGE_Y, _sendMessage, CHAT_SEND_CLIENT_MESSAGE) ;

		REGISTER_RECEIVER(Chat::SERVER_MESSAGE_Y, _recvMessage) ;
		REGISTER_RECEIVER(Chat::SERVER_INFO_Y, _recvServerInfo) ;
		REGISTER_RECEIVER(Chat::CLIENT_GCI_MESSAGE_Y, _recvGCIMessage) ;
		REGISTER_RECEIVER(Chat::LOCALIZED_MESSAGE_Y, _recvLocalisedMessage) ;
		REGISTER_RECEIVER(Chat::CMD_LIST_Y, _recvCommandList) ;
	}
	END_CALLBACK_REGISTRATION(Chat::CallbackX)
}

