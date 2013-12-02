#include <syscfg.h> // Path is project specific.
// Created: April 17 2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiActivate.h"
#include "teul/teulKit.h"
#include "wwii/wwiiErrorTypes.h"
#include "wwii/wwiiErrorPackers.h"
#include "wwii/wwiiHostMessages.h"
#include "server/serverInterface.h"
#include "wwii/wwiiTypes.h"

#if defined(IS_MAP_HOST)
#include "teul/teulServer.h"
#include "map/mapDB.h"
#include "chat/chatConstants.h"
#include "mm/mmMain.h"
#endif

static TEUL_CALLBACK*	_SEND_ACTIVATE;

extern void mapUserAbortedApp(Player* player);
extern void hostUserAbortedApp(Player* player);

static SINT32
_sendWWII_ACTIVATE(ACTION_OBJECT* const a)
{
	WWII_ACTIVATE* activate = (WWII_ACTIVATE*)(a->localRawData);
	return activate->packer(TEUL_PACK, a);
}

static SINT32 _recvWWII_ERROR_INFO(ACTION_OBJECT* const a)
{
	WWII_ERROR_INFO info;

	a->offset = wwiiPackERROR_INFO(TEUL_UNPACK, a->packed, a->offset, &info, 0);

#if defined(IS_MAP_HOST)
	{
		Player* player = (Player*)ServerInterface::LookupByTeulIndex(a->teulIndex);
		if ( player )
			mapSaveErrorInfo(*player, info);
	}
#endif
	return a->offset;
}

static SINT32 _recvAbortedApp(ACTION_OBJECT* const a)
{
	Player* player = (Player*)ServerInterface::LookupByTeulIndex(a->teulIndex);
	if ( player == NULL )
	{
		dbugLog(_FLW, "t:%d got abortedApp message from unregistered teulIndex", a->teulIndex);
		teulClose(a->teulIndex, Teul::Closing::AbortedApp);
		return -EINVAL;
	}

#if defined(IS_MAP_HOST)
	mapUserAbortedApp(player);
#elif defined(IS_CELL_HOST)
	hostUserAbortedApp(player);
#else
	dbugLog(_FLW, "t:%d sent us a USER_ABORTED_APP that we weren't expecting");
#endif

	return a->offset;
}

SINT32
wwiiHostInstallCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(WWII_X)
		REGISTER_SENDER(WWII_ACTIVATE_Y, _sendWWII_ACTIVATE, _SEND_ACTIVATE);
		REGISTER_RECEIVER(WWII_ERROR_INFO_Y, _recvWWII_ERROR_INFO);
		REGISTER_RECEIVER(WWII_USER_ABORTED_APP_Y, _recvAbortedApp);
	END_CALLBACK_REGISTRATION
}

SINT32 wwiiSendActivate(SINT32 connection, WWII_ACTIVATE* const activate)
{
	return teulSend(connection, _SEND_ACTIVATE, activate);
}

