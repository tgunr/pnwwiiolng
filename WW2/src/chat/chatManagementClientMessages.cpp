#include <syscfg.h> // Path is project specific.
// Date: Thu Jul  5 14:07:25 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "wwii/wwiiCallbacks.h"
#include "teul/teulBasic.h"

#include "chat/chatManagementConstants.h"
#include "chat/chatManagementTypes.h"

static TEUL_CALLBACK* CHAT_SEND_MANAGEMENT_LOGIN2 ;

// Use the new style login to request a username.
bool
chatSendManagementLogin2(SINT32 chatIndex, const char* usernameRequest)
{
	Chat::ManagementLogin2 login ;
	login.m_wantHandle = chatGetHandle(usernameRequest) ;

	if ( teulSend(chatIndex, CHAT_SEND_MANAGEMENT_LOGIN2, &login) < 0 )
		return false ;

	return true ;
}

static SINT32 _sendManagementLogin2(ACTION_OBJECT* const a)
{
	return a->packClass<Chat::ManagementLogin2>(TEUL_PACK) ;
}

SINT32 chatInstallClientManagementCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::Management::CallbackX)
	{

		REGISTER_SENDER(Chat::Management::LOGIN2_Y, _sendManagementLogin2, CHAT_SEND_MANAGEMENT_LOGIN2) ;
	}
	END_CALLBACK_REGISTRATION(Chat::Management::CallbackX)
}
