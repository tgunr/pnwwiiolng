#include <syscfg.h> // Path is project specific.
// Created: Mon May 21 16:26:58 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "server/serverInterface.h"
#include "strt/strtConstants.h"
#include "strt/strtData.h"
#include "strt/strtPack.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSyncData.h"
#include "strt/strtTypes.h"				// For VISLEVEL_NORMAL, ugh.
#include "strt/strtUpdates.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPlayer.h"

static SINT32
_recvConfigUpdate(ACTION_OBJECT* const a)
{
#ifdef DEV_BUILD
	dbugLog(_FLD, "receiving config update");
#endif
	return teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &host_config, sizeof(WWII_CONFIG));
}

SINT32
strtChatSupportInstallCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
	{
		REGISTER_RECEIVER(STRT_BROADCAST_CONFIG_UPDATE_Y, _recvConfigUpdate);
	}
	END_CALLBACK_REGISTRATION(Strat::CallbackX)
}

