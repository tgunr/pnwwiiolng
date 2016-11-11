#include <syscfg.h> // Path is project specific.
// Created: Mon May 21 16:26:58 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "comm/commConstants.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "server/serverInterface.h"
#include "strt/strtConstants.h"
#include "strt/strtData.h"
#include "strt/strtPack.h"
#include "strt/strtServerTypes.h"
#include "strt/strtTypes.h"		// For CP_HIT_HEADER
#include "strt/strtUpdater.h"
#include "strt/strtUpdaterHostMessages.h"
#include "strt/strtUpdaterPack.h"
#include "teul/teulServer.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiNetPorts.h"

//////////////////////////////////////////////////////////////////////
// Given a teulIndex, process an incoming list of strat damage and
// associate it with the relevant player/sortie/etc

static void
_handleHitList(SINT32 teulIndex, CP_HIT_HEADER& hitHeader)
{
	// 1. Make sure there's something to do
	if ( hitHeader.numHits <= 0 )
		return;

	// 2. Look up the player telling us they've damaged something;
	//   teul provides a hashed player index with a void* data structure
	//   for host-specific data. In the strat updata, this structure is
	//   a StratPersona

	Player* player = static_cast<Player*>(ServerInterface::LookupByTeulIndex(teulIndex));
	if ( player == nullptr )
	{
		dbugLog(_FLW, "t:%d not associated with player for damage list"
						, teulIndex
						);
		return;
	}

	// 3. Make sure the player has essential data (playerID, sortieID)
	try
	{
		if ( player->teulIndex() < 0 ) throw std::logic_error("Invalid teulIndex");
		if ( player->playerID() <= 0 ) throw std::logic_error("Invalid playerID");
		if ( player->vehicle == nullptr ) throw std::logic_error("Not joined to any vehicle");
		if ( player->instance().isCampaign() == false ) throw std::logic_error("Non-campaign instance"); ///TODO:INST Have to handle this
	}
	catch (std::logic_error& e)
	{
		player->log(_FLM, "rejecting hitlist: %s", e.what());
		return;
	}

	// 4. Accumulate the damage packets to reduce traffic
	for ( int i = 0; i < hitHeader.numHits; ++i )
	{
		dbugLog(_FLD, "t:%d p:%u v:%u damaged " PRF_FULL_OBJ_ID " with %" PR64BitT " joules"
				, teulIndex
				, player->playerID()
				, player->vehicle->index
				, PRA_FULL_OBJ_ID(hitHeader.list[i].object)
				, hitHeader.list[i].damage
				);

		strtAccumulateDamageMessage(hitHeader.list[i].object, hitHeader.list[i].damage, player->playerID(), player->vehicle->serial());
	}
}

static SINT32
_recvHitList(ACTION_OBJECT* const a)
{
	CP_HIT_HEADER hitHeader;

	// 1. Unpack the packet and check that it was good.
	a->offset = strtPackHitList(TEUL_UNPACK, a->packed, a->offset, &hitHeader, 0);
	if ( a->offset < 0 )
	{
		dbugLog(_FLW, "failed to unpack hit list");
		return a->offset;
	}

	// 2. Call the function that deals solely with the update packet
	_handleHitList(a->teulIndex, hitHeader);

	// 3. Release any memory allocated for the list
	if ( hitHeader.list != nullptr )
		memFree(hitHeader.list);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Callback registration functions.

static SINT32
_registerClientXCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Comm::CallbackX)
	{
		REGISTER_RECEIVER(DAMAGE_LIST_Y, _recvHitList);
	}
	END_CALLBACK_REGISTRATION(Comm::CallbackX)
}

static SINT32
_registerStrtUpdaterXCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
	{
		REGISTER_RECEIVER(STRT_DAMAGE_Y, _recvHitList);
	}
	END_CALLBACK_REGISTRATION(Strat::CallbackX)
}

SINT32
strtUpdateHostInstallCallbacks()
{
	if ( _registerClientXCallbacks() < 0 )
		return -EINVAL;
	if ( _registerStrtUpdaterXCallbacks() < 0 )
		return -EINVAL;
	return 0;
}
