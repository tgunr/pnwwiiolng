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
#include "teul/teulEndpoint.h"
#include "auth/authConstants.h"
#include "auth/authTypes.h"
#include "comm/commPlaygateMessages.h"

#include "wwii/wwiiCallbacks.h"

static SINT32 _errorFunc(ACTION_OBJECT *Action);

TEUL_CALLBACK*  authARENA_REQ;
TEUL_CALLBACK*  authAPP_LAUNCH_REQ;
static TEUL_CALLBACK*  _LoginXchg;

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

//////////////////////////////////////////////////////////////////////
// Callback invoked when a callback encounters an error.

static void
_callbackError(
               const SINT32 teulIndex
               , const UCHAR* const data
               , const SINT32 size
               , const TEUL_CALLBACK* object
               , const SINT32 error
               )
{
    ENDPOINT * const ep = teulGetEndpoint(teulIndex);

    dbugLog(_FLA, "ERROR: TeulIndex-%d, Address-%s, Dest-%u, Error-%d",
        teulIndex, teulGetAddress(ep ? ep->destAddress() : Teul::INVALID_INDEX), ep ? ep->destPort() : -1, error
    );
}

//////////////////////////////////////////////////////////////////////
// Register a callback.

TEUL_CALLBACK*
commInstallMessage(
                   UCHAR MessageGroup
                   , UCHAR MessageNumber
                   , ACTION SendFunc
                   , ACTION ReceiveFunc
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

//This function installs our two callbacks
SINT32 authClientInstallCallbacks()
{
    try
    {
        authARENA_REQ = commInstallMessage(Auth::CallbackX, ARENA_REQ_Y, _sendARENA_REQ, nullptr);
        if ( authARENA_REQ == nullptr )
            throw 1;
        authAPP_LAUNCH_REQ = commInstallMessage(Auth::CallbackX, APP_LAUNCH_REQ_Y, _sendAPP_LAUNCH_REQ, _errorFunc);
        if ( authAPP_LAUNCH_REQ == nullptr )
            throw 3;
        if ( commInstallMessage(Auth::CallbackX, APP_LAUNCH_Y, nullptr, _recvAPP_LAUNCH) == nullptr )
            throw 4;
        if ( commInstallMessage(Auth::CallbackX, APP_VERIFY_Y, nullptr, _recvAPP_VERIFY) == nullptr)
            throw 5;
        if ( nullptr == _LoginXchg )
            throw 8;
    }
    catch ( int n )
    {
        return n;
    }

    return 0;
}

static SINT32 _errorFunc(ACTION_OBJECT *Action)
{
    return 1;
}
