#include <syscfg.h> // Path is project specific.
// Date: Tue Jan 23 13:04:08 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "chat/chatSupportMessages.h"
#include "chat/chatTypes.h"
#include "comm/commConstants.h"
#include "comm/commPackers.h"
#include "host/hostPlayers.h"
#include "updt/loginTypes.h"
#include "map/mapArenaDescription.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapConstants.h"
#include "map/mapData.h"
#include "map/mapDB.h"
#include "map/mapHCUnitLocations.h"
#include "map/mapHostMessages.h"
#include "map/mapMissionList.h"
#include "map/mapPersonaSummary.h"
#include "map/mapPersonaTypes.h"
#include "map/mapPlayer.h"
#include "map/mapSwingMeter.h"
#include "miss/mission.h"
#include "miss/missionHostMessages.h"
#include "mm/mmdbClientMessages.h"
#include "mm/mmMain.h"
#include "mm/mmSpawn.h"
#include "mm/mmSpawnTypes.h"
#include "teul/teulAuth.h"
#include "teul/teulServer.h"
#include "teul/teulTransmission.h"
#include "tut/tutGroupProgress.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiOpinionPoll.h"

#define SPAWN_AT_DEPOTS

extern SwingMeter g_swingMeter ;

// RECV = receive only callback, SEND = send only, XCHG = bi-dir
static	TEUL_CALLBACK*	_XCHG_PERSONA_LIST ;
static	TEUL_CALLBACK*	_XCHG_PERSONA_SELECT ;
static	TEUL_CALLBACK*	_XCHG_BRIGADE_LIST ;
static	TEUL_CALLBACK*	_XCHG_BRIGADE_SELECT ;
static	TEUL_CALLBACK*	_XCHG_MISSION_LIST ;
static	TEUL_CALLBACK*	_SEND_HCUNIT_LIST ;
static	TEUL_CALLBACK*	_SEND_ARENA_DESCRIPTION ;
static	TEUL_CALLBACK*	_SEND_TUT_PROGRESS ;

SINT32
HCUnitList::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, _SEND_HCUNIT_LIST, this) ;
}

static SINT32
_sendHCUnitList(ACTION_OBJECT* const a)
{
	HCUnitList* hcul = (HCUnitList*)(a->localRawData) ;
	return hcul->packer(TEUL_PACK, a) ;
}

static SINT32
_recvExitWWIIOL(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player) ;
	dbugLog(_FLN, "t:%d p:%u c:%s received exitWWIIOL", a->teulIndex, player.playerID(), player.callsign());

	player.selectNullPersona() ;

	return 0 ;
}

// Handling of persona list requests etc

static PersonaSummaries _inPersonaSummaries ;
SINT32
_recvPersonaListRequest(ACTION_OBJECT* const a)
{
	_inPersonaSummaries.packer(TEUL_UNPACK, a) ;
	_inPersonaSummaries.receive(a->teulIndex) ;
	return a->offset ;
}

SINT32
_sendPersonaListResponse(ACTION_OBJECT* const a)
{
	PersonaSummaries* ps = (PersonaSummaries*)(a->localRawData) ;
	return ps->packer(TEUL_PACK, a) ;
}

SINT32
PersonaSummaries::send(SINT32 teulIndex)
{
	dbugLog(_FLD, "t:%d sending %d persona summaries", teulIndex, m_count) ;
	return teulSend(teulIndex, _XCHG_PERSONA_LIST, this) ;
}

// Handling of persona selection

SINT32
_recvPersonaSelectRequest(ACTION_OBJECT* const a)
{
	PersonaSelection request(a) ;
	request.receive(a->teulIndex) ;
	return a->offset ;
}

SINT32
_sendPersonaSelectResponse(ACTION_OBJECT* const a)
{
	PersonaSelection* ps = (PersonaSelection*)(a->localRawData) ;
	return ps->packer(TEUL_PACK, a) ;
}

SINT32
PersonaSelection::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, _XCHG_PERSONA_SELECT, this) ;
}

// Handling of brigade list requests etc

static BrigadeSummaries _inBrigadeSummaries ;
SINT32
_recvBrigadeListRequest(ACTION_OBJECT* const a)
{
	_inBrigadeSummaries.packer(TEUL_UNPACK, a) ;
	_inBrigadeSummaries.receive(a->teulIndex) ;
	return a->offset ;
}

SINT32
_sendBrigadeListResponse(ACTION_OBJECT* const a)
{
	BrigadeSummaries* bs = (BrigadeSummaries*)(a->localRawData) ;
	return bs->packer(TEUL_PACK, a) ;
}

SINT32
BrigadeSummaries::send(SINT32 teulIndex)
{
	betaLog(_FLD, "t:%d sending %u brigade summaries", teulIndex, m_count) ;
	return teulSend(teulIndex, _XCHG_BRIGADE_LIST, this) ;
}

// Handling of brigade selection/query

SINT32
_recvBrigadeSelectRequest(ACTION_OBJECT* const a)
{
	BrigadeSelection request(a) ;
	request.receive(a->teulIndex) ;
	return a->offset ;
}

SINT32
_sendBrigadeSelectResponse(ACTION_OBJECT* const a)
{
	BrigadeSelection* bs = (BrigadeSelection*)(a->localRawData) ;
	return bs->packer(TEUL_PACK, a) ;
}

SINT32
BrigadeSelection::send(SINT32 teulIndex)
{
	dbugLog(_FLD, "t:%d selected brigade %d result:%d", teulIndex, m_detail.brigadeID(), m_status) ;
	return teulSend(teulIndex, _XCHG_BRIGADE_SELECT, this) ;
}

// Handling of mission lists

static MissionList _inMissionList ;
SINT32
_recvMissionListRequest(ACTION_OBJECT* const a)
{
	_inMissionList.packer(TEUL_UNPACK, a) ;
	_inMissionList.receive(a->teulIndex) ;
	return a->offset ;
}

SINT32
_sendMissionListResponse(ACTION_OBJECT* const a)
{
	MissionList* ml = (MissionList*)(a->localRawData) ;
	return ml->packer(TEUL_PACK, a) ;
}

SINT32
MissionList::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, _XCHG_MISSION_LIST, this) ;
}

SINT32
_sendArenaDescription(ACTION_OBJECT* const a)
{
	ArenaDescription* arena = (ArenaDescription*)(a->localRawData) ;
	return arena->packer(TEUL_PACK, a) ;
}

SINT32
ArenaDescription::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, _SEND_ARENA_DESCRIPTION, this) ;
}

namespace Map
{

//////////////////////////////////////////////////////////////////////
// Notify all players of the current bias meter

class ReadyForSwing : public FilterFn
{
public:
	bool operator()(wwiiPlayer* const& player) const { return (((Player*)player)->isConnected() && ((Player*)player)->hasCompletedSpam()) ; }
} ;

void
BroadcastSwingMeter()
{
	if ( g_players.empty() == true )
		return ;

	static const UINT32 _BROADCAST_INTERVAL = 90 ;	// Every 90 seconds
	static UINT32 nextBroadcast = teulTime() + _BROADCAST_INTERVAL ;

	if ( nextBroadcast > teulTime() )
		return ;

	static TeulTransmission broadcast(SWING_METER_MSG::callback()) ;

	SWING_METER_MSG outgoing(g_swingMeter) ;
	broadcast.populate(&outgoing) ;
	broadcast.send(g_players.begin(), g_players.end(), ReadyForSwing()) ;

	nextBroadcast = teulTime() + _BROADCAST_INTERVAL ;
}

//////////////////////////////////////////////////////////////////////
// Notify one player about the current side meter

void
SendSwingMeter(SINT32 teulIndex)
{
	SWING_METER_MSG outgoing(g_swingMeter) ;
	outgoing.send(teulIndex) ;
}

//////////////////////////////////////////////////////////////////////
// Send a (null terminated) array of GroupProgress to a client

static SINT32
_sendTutProgress(ACTION_OBJECT* const a)
{
	Tutorial::GroupProgressBatch& batch = *(Tutorial::GroupProgressBatch*)(a->localRawData) ;
	for ( UINT32 i = 0 ; i < batch.size() ; ++i )
	{
		batch[i].packer(TEUL_PACK, a) ;
	}
	return a->offset ;
}

void
SendTutProgressBatch(SINT32 teulIndex, Tutorial::GroupProgressBatch& batch)
{
	teulSend(teulIndex, _SEND_TUT_PROGRESS, &batch) ;
}

//////////////////////////////////////////////////////////////////////
// Installs our callbacks

SINT32
HostInstallCallbacks(void)
{
	wwiiRegisterPlayerMessageCallback() ;

	BEGIN_CALLBACK_REGISTRATION(Map::CallbackX)

		REGISTER_RECEIVER(Map::EXIT_WWIIOL_Y, _recvExitWWIIOL) ;

		REGISTER_SENDER(Map::XCHG_HCUNIT_LIST_Y, _sendHCUnitList, _SEND_HCUNIT_LIST) ;
		REGISTER_SENDER(Map::ARENA_DESCRIPTION_Y, _sendArenaDescription, _SEND_ARENA_DESCRIPTION) ;

		REGISTER_RECVSEND(Map::XCHG_PERSONA_LIST_Y, _recvPersonaListRequest, _sendPersonaListResponse, _XCHG_PERSONA_LIST) ;
		REGISTER_RECVSEND(Map::XCHG_PERSONA_SELECT_Y, _recvPersonaSelectRequest, _sendPersonaSelectResponse, _XCHG_PERSONA_SELECT) ;
		REGISTER_RECVSEND(Map::XCHG_BRIGADE_LIST_Y, _recvBrigadeListRequest, _sendBrigadeListResponse, _XCHG_BRIGADE_LIST) ;
		REGISTER_RECVSEND(Map::XCHG_BRIGADE_SELECT_Y, _recvBrigadeSelectRequest, _sendBrigadeSelectResponse, _XCHG_BRIGADE_SELECT) ;
		REGISTER_RECVSEND(Map::XCHG_MISSION_LIST_Y, _recvMissionListRequest, _sendMissionListResponse, _XCHG_MISSION_LIST) ;

		REGISTER_MSG_SENDER(Map::HC_UNIT_LOCATIONS_Y, HC_UNIT_LOCATIONS) ;

		REGISTER_MSG_SENDER(Map::SWING_METER_Y, SWING_METER_MSG) ;

		REGISTER_MSG_SENDER(Map::OPINION_POLL_Y, OPINION_POLL_MSG) ;
		REGISTER_MSG_RECEIVER(Map::POLL_RESULT_Y, OPINION_CHOICE_MSG) ;

		REGISTER_SENDER(Map::TUT_PROGRESS_Y, _sendTutProgress, _SEND_TUT_PROGRESS) ;

	END_CALLBACK_REGISTRATION
}

}
