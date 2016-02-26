//
// Date: Mon Jul 31 13:32:26 2000
//
// The code behind the messages the host will be sending to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a 
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include <stdio.h>
#include <stdlib.h>

#include "auth/authPack.h"
#include "auth/authClientMessages.h"
#include "teul/teulAuth.h"
#include "auth/authConstants.h"
#include "auth/authTypes.h"

#include "wwii/wwiiCallbacks.h"

TEUL_CALLBACK*	authARENA_REQ;
TEUL_CALLBACK*	authAPP_LAUNCH_REQ;

static TEUL_CALLBACK*	_APP_VERIFY;
static TEUL_CALLBACK*	_APP_LAUNCH;

static SINT32 _sendARENA_REQ(ACTION_OBJECT* const a)
{
    ARENA_REQ* ar = (ARENA_REQ*) a->localRawData;
	return authPackARENA_REQ(TEUL_PACK, a->packed, a->offset, ar, 0);
}

static SINT32 _sendAPP_LAUNCH_REQ(ACTION_OBJECT* const a)
{
    APP_LAUNCH_REQ* al = (APP_LAUNCH_REQ*) a->localRawData;
	return authPackAPP_LAUNCH_REQ2(TEUL_PACK, a->packed, a->offset, al, 0);
}

static SINT32
_recvAPP_VERIFY(ACTION_OBJECT* const a)
{
	APP_VERIFY reply;

	a->offset = authPackAPP_VERIFY(TEUL_UNPACK, a->packed, a->offset, &reply, 0);

	authVerifyApp(a->teulIndex, &reply);

	return a->offset;
}

static SINT32 _recvAPP_LAUNCH(ACTION_OBJECT* const a)
{
APP_LAUNCH app;

	a->offset = authPackAPP_LAUNCH(TEUL_UNPACK, a->packed, a->offset, &app, 0);

	authLaunchApp(&app);

	return a->offset;
}

/*
void _clientSendError(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* object, SINT32 error)
{
	printf("error %d sending on index %d!\n", error, teulIndex);
	exit(1);
};

void _clientRecvError(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* object, SINT32 error)
{
	printf("error %d receiving on index %d!\n", error,teulIndex);
	exit(1);
};
*/

//This function installs our two callbacks
SINT32 authClientInstallCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(AUTHENTICATION_X)

		BEGIN_PAIRED_CALLBACKS()
			// Request permission to join an arena with this version.
			REGISTER_SENDER(ARENA_REQ_Y, _sendARENA_REQ, authARENA_REQ) ;

		// Request permission to log in with this version (duplication ftw).
		REGISTER_SENDER(APP_LAUNCH_REQ_Y, _sendAPP_LAUNCH_REQ, authAPP_LAUNCH_REQ) ;

    END_PAIRED_CALLBACKS()
		// Receive verification of app version.
		REGISTER_RECEIVER(APP_VERIFY_Y, _recvAPP_VERIFY, _APP_VERIFY) ;

		// Receive launch permission.
		REGISTER_RECEIVER(APP_LAUNCH_Y, _recvAPP_LAUNCH, _APP_LAUNCH) ;

	END_CALLBACK_REGISTRATION
}

