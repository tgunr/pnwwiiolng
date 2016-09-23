#include <syscfg.h> // Path is project specific.
// Created: Mon May 21 20:15:08 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "teul/teulBasic.h"
#include "strt/strtServerTypes.h"
#include "strt/strtConstants.h"
#include "strt/strtUpdaterPack.h"
#include "strt/strtPack.h"
#include "wwii/wwiiCallbacks.h"
#include "strt/strtTypes.h"

#include "strt/strtUpdaterSupportMessages.h"
#include "comm/commHostMessages.h"
#include "host/host.h"
#include "wwii/wwiiPlayer.h"
#include "server/serverInterface.h"
#include "teul/teulServer.h"
#include "wwii/wwiiNetPorts.h"

TEUL_CALLBACK*	STRT_SEND_DAMAGE;
TEUL_CALLBACK*	STRT_SEND_HIT_LIST;

static SINT32
_sendHitList(ACTION_OBJECT* const a)
{
	return strtPackHitList(TEUL_PACK, a->packed, a->offset, (CP_HIT_HEADER*) a->localRawData, 0);
}

//////////////////////////////////////////////////////////////////////
// Receive an EWS message and forward to strat

static SINT32
_recvEnemySpotted(ACTION_OBJECT* const a)
{
	wwiiPlayer* player = ServerInterface::LookupByTeulIndex(a->teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLW, "t:%d ignoring EWS from unregistered player", a->teulIndex);
		return -EINVAL;
	}
	if ( player->instance().isCampaign() == false )
	{
		player->log(_FLD, "t:%d ignoring EWS from non-campaign player", a->teulIndex);
		return a->offset;
	}

	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_ENEMY_REPORT_Y));
	auto& report = outgoing.allocate<STRAT_ENEMY_REPORT>();
	a->offset = strtPackEnemySpotted(TEUL_UNPACK, a->packed, a->offset, &report, 0);
	if ( a->offset < 0 )
		return a->offset;

    dbugLog(_FLD,"t:%d received enemy report for vehicle " PRF_WEAPON_SERIAL "", a->teulIndex, PRA_WEAPON_SERIAL(report.enemyReported));

	return serverSendPacketToType(outgoing, STRAT_HOST);
}

SINT32
strtInstallUpdaterSupportCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
		REGISTER_SENDER(STRT_DAMAGE_Y, _sendHitList, STRT_SEND_HIT_LIST);
		REGISTER_RECEIVER(STRT_ENEMY_SPOTTED_Y, _recvEnemySpotted);
	END_CALLBACK_REGISTRATION
}
