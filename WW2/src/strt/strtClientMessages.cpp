#include <syscfg.h> // Path is project specific.
// Created: Fri Nov  3 13:53:33 2000
//
// This file contains the setup and message API that the front-end will use
// to send messages straight to the strat system.  Most messages will* NOT*
// come to the strat system this way.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "wwii/wwiiCallbacks.h"
#include "strt/strtTypes.h"
#include "strt/strtConstants.h"
#include "strt/strtClientMessages.h"
#include "strt/strtUpdaterPack.h"
#include "teul/teulBasic.h"

static TEUL_CALLBACK* _ENEMY_SPOTTED;

SINT32 strtSendEnemyReport(SINT32 teulIndex, STRAT_ENEMY_REPORT* report)
{
	return teulSend(teulIndex, _ENEMY_SPOTTED,report);
}

static SINT32 _sendEnemySpotted(ACTION_OBJECT* const a)
{
	return strtPackEnemySpotted(TEUL_PACK, a->packed, a->offset,(STRAT_ENEMY_REPORT*) a->localRawData, 0);
}

//Installs all the client callbacks
extern SINT32
strtInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)

		REGISTER_SENDER(STRT_ENEMY_SPOTTED_Y, _sendEnemySpotted, _ENEMY_SPOTTED) ;

	END_CALLBACK_REGISTRATION
}
