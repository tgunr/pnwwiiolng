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

static TEUL_CALLBACK* CHAT_SEND_MANAGEMENT_LOGIN3 ;

// Use the new style login to request a username.
bool
chatSendManagementLogin3(SINT32 chatIndex, const char* usernameRequest, UINT16 countryRequest)
{
	Chat::ManagementLogin3 login ;
	login.m_wantHandle = chatGetHandle(usernameRequest) ;
	login.m_country = countryRequest;

	if ( teulSend(chatIndex, CHAT_SEND_MANAGEMENT_LOGIN3, &login) < 0 )
		return false ;

	return true ;
}

bool
chatSendManagementLogin2(SINT32 chatIndex, const char* usernameRequest)
{
	return chatSendManagementLogin3(chatIndex, usernameRequest, 0);
}

static SINT32 _sendManagementLogin3(ACTION_OBJECT* const a)
{
	return a->packClass<Chat::ManagementLogin3>(TEUL_PACK) ;
}

SINT32 chatInstallClientManagementCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::Management::CallbackX)

		REGISTER_SENDER(Chat::Management::LOGIN3_Y, _sendManagementLogin3, CHAT_SEND_MANAGEMENT_LOGIN3) ;

	END_CALLBACK_REGISTRATION
}
