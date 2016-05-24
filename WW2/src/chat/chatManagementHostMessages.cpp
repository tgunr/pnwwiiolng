#include <syscfg.h> // Path is project specific.
// Date: Thu Jul  5 16:15:25 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatManagementTypes.h"
#include "chat/chatManagementConstants.h"
#include "wwii/wwiiCallbacks.h"

static SINT32 _recvManagementLogin2(ACTION_OBJECT* const a)
{
	Chat::ManagementLogin2 login;
	login.packer(TEUL_UNPACK, a);

	login.Login(a->teulIndex);

	return a->offset;
}

SINT32
chatInstallHostManagementCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::Management::CallbackX)

		REGISTER_RECEIVER(Chat::Management::LOGIN2_Y, _recvManagementLogin2);

	END_CALLBACK_REGISTRATION
}
