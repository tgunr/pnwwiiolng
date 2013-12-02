#include <syscfg.h> // Path is project specific.
// Created: Sun Jan 21 23:06:04 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "wwii/wwiiCallbacks.h"

#include "chat/chatSupportMessages.h"
#include "chat/chatTypes.h"
#include "include/logging.h"
#include "map/mapBrigades.h"
#include "map/mapDB.h"
#include "map/mapPlayer.h"
#include "miss/missHitArrayNode.h"
#include "miss/missionConstants.h"
#include "miss/missionDetail.h"
#include "miss/mission.h"
#include "miss/missionHostMessages.h"
#include "miss/missionListUpdate.h"
#include "miss/missionMission.h"
#include "miss/missionPack.h"
#include "miss/missionResults.h"
#include "miss/missionTypes.h"
#include "miss/missionUtils.h"
#include "mm/mmMain.h"
#include "mm/mmSpawn.h"
#include "score/scoreSortie.h"
#include "str/stringLib.h"
#include "strt/strtConstants.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFacilityWeaponList.h"
#include "strt/strtMapMessages.h"
#include "strt/strtMapTypes.h"
#include "strt/strtPack.h"
#include "strt/strtUpdaterPack.h"
#include "strt/strtWeaponListRequest.h"
#include "teul/teulAuth.h"
#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "teul/teulUtils.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiString.h"

#define STICK_DEBUG

//static TEUL_CALLBACK*	missFBSTATUS;
static TEUL_CALLBACK*	missSEND_POST_REPLY;
static TEUL_CALLBACK*	missSEND_WEP_LIST;  //to the client
#ifdef MULTIPLE_MISSION_HOSTS
static TEUL_CALLBACK*	missBCAST_NEW;
static TEUL_CALLBACK*	missBCAST_EXISTING;
#endif

static TEUL_CALLBACK*	missSEND_RESULTS_REPLY;
static TEUL_CALLBACK*	missSEND_ROOM_LIST;
static TEUL_CALLBACK*	missSEND_LAUNCH_UPDATE;

//externs found in mmMain.c
extern Server::ServerInfo _myServerInfo;  //yes, why is this called _my... when it's a global?

static UINT32
_getInteractionCount(const std::map<UINT32, UINT32>& counters, const UINT32 specUINT32)
{
	auto it = counters.find(specUINT32);
	if ( it == counters.end() )
		return 0;
	return it->second;
}

static bool
_createMissionResults(const Player& player, MISSION_RESULTS& results)
{
	WWII_SIDE side = MAX_SIDES;

	const MissionPacket& packet = player.missionPacket();

	results.sortieID = packet.sortie().sortieID();

	const FULL_OBJ_ID& origin = packet.sortie().originFac();
	FacilityStruct* facility = strtGetFacility(origin);
	if ( facility != NULL )
		side = facility->side;
	else
	{
		const CPStruct* const cp = strtGetCP(origin);
		if (cp == NULL)
		{
			dbugLog(_FLW, "t:%d p:%u c:%s Mission packet has invalid origin: %d.%d.%u"
							, player.teulIndex(), player.playerID(), player.callsign()
							, origin.cell.ox, origin.cell.oy, origin.id
							);
			return false;
		}

		side = cp->controllingSide;
	}

	const auto& scoring = packet.scoring();

	results.handleID = player.chatHandle();
	results.missionID = player.missionID();
	results.missionSuccess = packet.wasSuccessful;
	results.promoted = player.promoted();
	results.elapsedTime = packet.duration_s();
	results.kills = scoring.kills();
	results.numHits = scoring.hits();
	results.captureEvents = scoring.captures();
	results.captureAssists = 0;
	results.m_side = side;
	results.rank = player.effectiveRank();
	results.m_realRank = player.realRank();
	results.m_maxRank = player.maxRank();
	results.rtb = (CODE_RTB)packet.vehicle().rtbState();
	results.rtbFacility = packet.vehicle().rtbFacility();
	if ( packet.rtbReservedWeapon.serial > 0 )
		results.rtbReservedWeapon = packet.rtbReservedWeapon;
	else
		results.rtbReservedWeapon = NULL_WEAP_SERIAL;
	results.mobileSpawnees = scoring.spawnees();
	results.aisHit = scoring.aiKills();
	results.bridgesHit = scoring.bridgeKills();
	results.bridgesRepaired = scoring.bridgeRepairs();
	results.factorysHit = scoring.factoryKills();
	results.firebasesHit = scoring.firebaseKills();
	results.m_resupplies = scoring.resupplies();
	results.m_resuppliesScored = scoring.resuppliesScored();
	results.m_guardingKills = scoring.guardingKills();
	results.m_capperKills = scoring.cappingKills();
	results.m_killedBy = packet.vehicle().killedBy();
	results.m_guardingPoints = scoring.guardingPoints();
	results.m_assists = scoring.assists();

	results.pointsScored = scoring.points();
	results.pointsAccumulated = player.points();
	results.rankIsKOTH = player.useRank().kingOfTheHill();
	bool maxedRank = player.effectiveRank() >= player.maxRank();
	if ( !maxedRank && !results.rankIsKOTH )
		results.pointsNextRank = player.useRank().points();
	else
		results.pointsNextRank = 0;

	const Rank* nextRank = player.useRank().next();
	if ( nextRank != NULL )
		results.nextRank = std::min(nextRank->level(), player.maxRank());
	else
		results.nextRank = 0;

	const UINT32 sortieID = player.missionPacket().sortieID();
	const Scoring::Sortie* sortie = player.sortie();
	if ( sortie == NULL || sortie->sortieID() == sortieID )
	{
		sortie = Scoring::Sortie::Lookup(sortieID);
		results.numHits = 0;
	}
	if ( sortie != NULL )
	{
		auto dr = sortie->m_typesInteractedWith;
		const size_t types = std::min((size_t)dr.size(), (size_t)MapMiss::MAX_HIT_ARRAY);
		results.numHits = types;
		UINT32 i = 0;
		for ( auto it = dr.begin(); it != dr.end() && i < types; ++it, ++i )
		{
			WEAPON_SPEC spec(*it);
			results.hitArray[i].vehType = spec;
			results.hitArray[i].numVehiclesHit = _getInteractionCount(sortie->m_typesHit, *it);
			results.hitArray[i].numVehiclesKilled = _getInteractionCount(sortie->m_typesKilled, *it);
		}
	}

	results.chatPermissions = *player.chatPermissions();
	results.accountGoodUntil = player.accountGoodUntil();
	results.m_hitArraySize = results.numHits;

	return true;
}

static SINT32
_sendMISS_ROOM_LIST(ACTION_OBJECT* const a)
{
	RoomList* roomList = (RoomList*)(a->localRawData);
	return roomList->packer(TEUL_PACK, a);
}

static SINT32
_sendMISS_LAUNCH_UPDATE(ACTION_OBJECT* const a)
{
	return missPackMISS_LAUNCH_UPDATE(TEUL_PACK, a->packed, a->offset, (MISS_LAUNCH_UPDATE*) a->localRawData, 0);
}

static SINT32
_sendMISSION_POST_REPLY(ACTION_OBJECT* const a)
{
	return missPackMISSION_POST_REPLY(TEUL_PACK, a->packed, a->offset, (MISSION_POST_REPLY*) a->localRawData, 0);
}

static SINT32
_sendWEP_LIST(ACTION_OBJECT* const a)
{
	FacilityWeaponList* outgoing = (FacilityWeaponList*)(a->localRawData);
	return outgoing->packer(TEUL_PACK, a);
}

static SINT32
_sendMISSION_RESULTS(ACTION_OBJECT* const a)
{
	return missPackMISSION_RESULTS(TEUL_PACK, a->packed, a->offset, (MISSION_RESULTS*) a->localRawData, 0);
}

// weap list request
// if not on a mission create a dummy missionPacket
static SINT32
_recvWEP_LIST_REQ(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	WEP_LIST_REQ request;
	FacilityStruct* facility;

	a->offset = missPackWEP_LIST_REQ(TEUL_UNPACK, a->packed, a->offset, &request, 0);

	const SINT32 stratIndex = mapGetStratIndex();
	if ( stratIndex < 0 )
	{
		///TODO: Notify client.
		return -EINVAL;
	}

	BETAInfo(a->teulIndex);

	facility = strtGetFacility(request.facility);
	if (facility == NULL)
	{
		dbugLog(_FLA,"couldnt find facility x:%d y:%d id:%d for index %d"
					, request.facility.cell.ox
					, request.facility.cell.oy
					, request.facility.id
					, a->teulIndex);

		///TODO: Notify client.
		return -EINVAL;
	}

	// Check ownership before bothering the strat host.
	if ( player.side() != facility->side )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s side mismatch persona %d, facility %d", a->teulIndex, player.playerID(), player.callsign(), player.side(), facility->side);

		// Notify the player why the weapon list can't be sent.
		FacilityWeaponList reply(player.playerID(), request.facility);
		reply.m_status = FacilityWeaponList::_ENEMY_OWNS;
		reply.m_numWeapons = 0;
		reply.m_refreshSeconds = 30;
		reply.send(player.teulIndex());

		return a->offset;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s weapons request at %s, all: %d, refresh: %d, perm: %llx"
					, a->teulIndex, player.playerID(), player.callsign()
					, facility->name, request.allWeapons, request.isRefresh, player.permissionMask()
					);

	Teul::Packet outgoing(0, Strat::CallbackX, STRT_FACILITY_WEAP_LIST_Y);
	CONSTRUCT_IN_PACKET(outgoing, WeaponListRequest, player.playerID(), player.brigadeID(), request.facility
													, player.permissionMask()
													, false, request.isRefresh != FALSE);
	outgoing.send(mapGetStratIndex());

	return a->offset;
}

void
MissionDetail::populate(const Mission* mission, bool isCurrent, bool isLeader, UINT32 withLaunchDelay)
{
	MissionSynopsis::populate(mission, isCurrent, isLeader, mission->numTakers());
	strlcpy(m_desc, mission->m_desc, sizeof(m_desc));
//	strncpy(m_desc, mission->m_desc, MISSION_DESC_LENGTH);
//	m_desc[MISSION_DESC_LENGTH] = 0;
	m_timeLaunch = std::max(mission->summary().timeLaunch, withLaunchDelay);
	m_brigadeID = mission->brigade()->brigadeID();
}

// Player requesting to view or take a particular mission
template<>
SINT32
MISSION_DETAIL::receive(SINT32 teulIndex, MISSION_DETAIL& packet)
{
	UINT32 missionID = (UINT32)packet.key();
	bool joinRequest = (bool)packet.flags();
	MissionDetail& data = packet.data();
	Mission* mission;

	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == NULL )
	{
		dbugLog(_FLW, "t:%d mission check on unpopulated connection", teulIndex);
		return -EINVAL;
	}

	packet.m_action = EXCHANGE;
	data.m_result = MC_ERROR;

	//see if the mission has expired

	mission = mapFindMission(missionID);
	if ( mission == NULL || !mission->ongoing() )
		data.m_result = MC_EXPIRED;
	else if ( joinRequest )
	{
		player->ExitMission();
		data.m_result = player->selectMission(mission, true);
		if ( data.m_result < MC_ERROR )
		{
			// Populate the mission synopsis
			data.populate(mission, true, player->isMissionLeader(), player->m_nextLaunch);
		}
	}
	else
	{
		data.m_result = MC_DETAIL;
		bool onMission = player->missionID() == missionID;
		bool isLeader = (onMission) ? player->isMissionLeader() : false;
		data.populate(mission, onMission, isLeader, 0);
	}

	return packet.send(teulIndex);
}

static SINT32
_recvMISSION_EXIT(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);

	const UINT32 missionID = player.missionID() ;
	player.ExitMission();

	// Last one out turn off the lights
	Mission* const mission = mapFindMission(missionID);
	if ( mission != NULL && mission->numTakers() <= 0 )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s m:%u abandoned, cancelling"
						, player.teulIndex(), player.playerID(), player.callsign(), missionID);
		mission->cancel();
	}

	return 0;
}

//Receive MISSION_POST_REQ
static SINT32
_recvMISSION_POST_REQ(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	static MISSION_POST_REQ request;

	//unpack stuff
	a->offset = missPackMISSION_POST_REQ(TEUL_UNPACK, a->packed, a->offset, &request,0);

	return missionSendMissionPostReply(request, &player, a->teulIndex);
}

static SINT32
_recvMISSION_RESULTS_REQ(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	if ( a->sizeInBytes() != sizeof(decltype(MISSION_RESULTS::sortieID)) )
		return -EINVAL;

	const UINT32 sortieID = *(UINT32*)(a->packed);
	if ( sortieID == 0 )
	{
		dbugLog(_FLA, "t:%d missresreq: invalid sortieID %u", a->teulIndex, sortieID);
		return -EINVAL;
	}

	if ( player.m_missionResults == NULL )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s v:%u req for mission results, but player has no results", a->teulIndex, player.playerID(), player.callsign(), sortieID);
		return -1;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s v:%u sending requested mission results", a->teulIndex, player.playerID(), player.callsign(), sortieID);

	return teulSend(player.teulIndex(), missSEND_RESULTS_REPLY, player.m_missionResults);
}

SINT32
mmSendMissionResults(Player& player)
{
	FacilityStruct* facility;
	MM_POST_ACCEPT fakeAccept;

	if ( player.m_missionResults != nullptr )
		delete player.m_missionResults;

	player.m_missionResults = new MISSION_RESULTS();
	MISSION_RESULTS& results = *player.m_missionResults;

	dbugLog(_FLD, "t:%d p:%u c:%s m:%d mp:%p amp:%p", player.teulIndex(), player.playerID(), player.callsign(), player.missionID(), player.mission(), mapFindMission(player.missionID()));

	if ( !_createMissionResults(player, results) )
	{
		dbugLog(_FLA,"%s() Unable to create mission results for %s"
				, __FUNCTION__
				, player.callsign()
				);
		return -EINVAL;
	}

	//by now missionCalcResults should have been called, and we got the info
	//we needed just now take them off the mission
	facility = strtGetFacility(player.missionPacket().sortie().originFac());
	if ( facility == NULL && results.rtbFacility.id != 0 )
	{
		facility = strtGetFacility(results.rtbFacility);
	}

	// Cleanup/reset their mission packet
	player.clearSortieResults(false);

	results.missionInvalid = (bool)(player.mission() == NULL);
	dbugLog(_FLD, "t:%d p:%u c:%s m:%d mission is %s", player.teulIndex(), player.playerID(), player.callsign(), player.missionID(), (results.missionInvalid ? "invalid" : "valid"));

	// If they've got a reserved vehicle, we need to register it to them
	if (
		facility != NULL
		&& facility->side == player.side()
		&& activeCountries[facility->country]
		&& results.rtbReservedWeapon.serial > 0
		&& memcmp(&results.rtbFacility, &nullObjID, sizeof(nullObjID)) != 0
		)
	{
		dbugLog(_FLD, "t:%d p:%u c:%s processing post accept for pre-reserved %s"
						, player.teulIndex(), player.playerID(), player.callsign(), weapPrintName(&results.rtbReservedWeapon.spec));
		memset(&fakeAccept, 0, sizeof(fakeAccept));

		fakeAccept.facility = results.rtbFacility;
		fakeAccept.weaponSerial = results.rtbReservedWeapon;
		fakeAccept.customerID = player.playerID();
		fakeAccept.portal = player.m_portal;
		fakeAccept.multicrew = FALSE;
		fakeAccept.immediateSpawn = FALSE;

		mmProcessPostAccept(player, fakeAccept, MM_ACCEPT_RTB_RESERVED);
	}
	else
	{
		const char* msg;
		if ( facility == NULL )
			msg = "no facility";
		else if ( facility->side != player.side() || activeCountries[facility->country] == false )
			msg = "wrong side";
		else if ( results.rtbReservedWeapon.serial == 0 )
			msg = "no reserved weapon";
		else if ( results.rtbFacility.id == 0 )
			msg = "no rtb facility";
		else msg = "UNKNOWN";
		dbugLog(_FLD, "t:%d p:%u c:%s NO RESERVED VEHICLE: %s", player.teulIndex(), player.playerID(), player.callsign(), msg);
	}

	player.SetState(PERS_AT_MISSION);
	player.updateMissionTaker(false);

	player.updateNextLaunchTime();

	const Mission* mission = player.mission();
	if ( mission )
	{
		sendToClientMissTimeUpdate(
			player.teulIndex()
			, mission->missionID()
			, player.nextLaunchTime()
			, mission->isMobileSpawnAvailable()
			, mission->isMobileSpawnAllowed()
			, mission->active()
			, mission->ongoing()
			);
	}

	dbugLog(_FLI, "t:%d p:%u c:%s sending mission results reply m:%u el:%u k:%u h:%u succ:%u rtb:%u rank:%u p:%u pa:%u"
					, player.teulIndex(), player.playerID(), player.callsign()
					, results.missionID, results.elapsedTime, results.kills, results.numHits, results.missionSuccess, results.rtb, results.rank, results.pointsScored, results.pointsAccumulated
					);
	const SINT32 retval = teulSend(player.teulIndex(), missSEND_RESULTS_REPLY, &results);
#ifdef BETA_RELEASE
	if ( retval >= 0 )
	{
#include "chat/chatLStringIDs.h"
		player.message(LSTR_NOTE, LSTR_GENERIC, "MISSION RESULTS SENT");
	}
#endif

	return retval;
}

static void
_maskFacilityWeaponList(const Player* const player, FacilityWeaponList& fwl)
{
	for ( UINT32 i = 0; i < fwl.m_numWeapons; ++i )
	{
		fwl.m_counters[i].restricted = (player->CheckWeaponMask(fwl.m_weapons[i].asUINT16()) ? 0 : 1);
	}
}

static SINT32
_recvWeapList(ACTION_OBJECT* const a)
{
	if ( a->canRead<FacilityWeaponList>() == false )
		return -EINVAL;
	if ( a->teulIndex != mapGetStratIndex() )
		return -EINVAL;

	auto& fwl = a->read<FacilityWeaponList>();
	// Find the player
	const Player* const player = PlayerInterface::Lookup(fwl.m_playerID);
	if ( player == NULL || player->isAtLobby() == false )
		return a->offset;

	if ( player->permissions(UNMASKED_VEHICLES) == false )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s masking fwl", player->teulIndex(), player->playerID(), player->callsign());
		_maskFacilityWeaponList(player, fwl);
	}

	// Forward the data.
	fwl.send(player->teulIndex());

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Request to lock a mission and make it unjoinable.

static SINT32
_recvLockMission(ACTION_OBJECT* const a)
{
	if ( a->sizeInBytes() != sizeof(UINT32) )
		return -EINVAL;

	Player* const player = (Player*)a->m_client;
	if ( a->m_client != NULL )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s received mission lock from player!", a->teulIndex, player->playerID(), player->callsign());
		return -EINVAL;
	}

	const UINT32& missionID = *(UINT32*)(a->packed);
	Mission* const mission = mapFindMission(missionID);

	if ( mission->active() )
	{
		dbugLog(_FLI, "m:%u locking mission due to chat-host request", missionID);
		mission->deactivate();
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Dispatch a WeaponListRequest to the strat host.

SINT32
FacilityWeaponList::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, missSEND_WEP_LIST, this);
}

SINT32 sendToClientMissionPostReply(MISSION_POST_REPLY reply, SINT32 connection)
{
	return teulSend(connection, missSEND_POST_REPLY, &reply);
}

SINT32 sendToClientRoomList( SINT32 connection, RoomList& roomList)
{
	if ( connection < 0 )
		return 0;
	return teulSend(connection, missSEND_ROOM_LIST, &roomList);
}

SINT32
sendToClientMissTimeUpdate(SINT32 connection
							, UINT32 missionID
							, UINT32 launchTime
							, bool mobileSpawnAvailable
							, bool canMobileSpawn
							, bool isActive
							, bool isOngoing
							)
{
	if ( connection < 0 )
		return 0;

	MISS_LAUNCH_UPDATE update;

	update.missionID = missionID;
	update.timeLaunch = launchTime;
	update.mobileSpawnAvailable = mobileSpawnAvailable;
	update.m_canMobileSpawn = canMobileSpawn;
	if ( isActive )
		update.state = MISS_LAUNCH_UPDATE::ACTIVE;
	else if ( isOngoing )
		update.state = MISS_LAUNCH_UPDATE::INACTIVE;
	else
		update.state = MISS_LAUNCH_UPDATE::CLOSED;

	return teulSend(connection, missSEND_LAUNCH_UPDATE, &update);
}

//////////////////////////////////////////////////////////////////////
// Register teulKit callbacks

static SINT32
_installStratCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
		REGISTER_RECEIVER(STRT_FACILITY_WEAP_LIST_Y, _recvWeapList);
	END_CALLBACK_REGISTRATION
}

SINT32
missionHostInstallCallbacks(void)
{
	_installStratCallbacks();

	BEGIN_CALLBACK_REGISTRATION(MISSION_X)

		REGISTER_RECEIVER(FE_MISSION_POST_REQ_Y, _recvMISSION_POST_REQ);
		REGISTER_RECEIVER(FE_MISSION_EXIT_Y, _recvMISSION_EXIT);
		REGISTER_RECEIVER(FE_MISSION_WEP_LIST_REQ_Y, _recvWEP_LIST_REQ);

		REGISTER_SENDER(FE_MISSION_POST_REPLY_Y, _sendMISSION_POST_REPLY, missSEND_POST_REPLY);
		REGISTER_SENDER(FE_MISSION_CLIENT_WEP_LIST_Y , _sendWEP_LIST, missSEND_WEP_LIST);
		REGISTER_SENDER(FE_MISSION_ROOM_LIST_Y, _sendMISS_ROOM_LIST, missSEND_ROOM_LIST);
		REGISTER_SENDER(FE_MISSION_LAUNCH_UPDATE_Y, _sendMISS_LAUNCH_UPDATE, missSEND_LAUNCH_UPDATE);
		REGISTER_SENDRECV(FE_MISSION_RESULT_REPLY_Y, _sendMISSION_RESULTS, _recvMISSION_RESULTS_REQ, missSEND_RESULTS_REPLY);

		REGISTER_MSG(FE_MISSION_DETAIL_Y, MISSION_DETAIL);
		REGISTER_MSG_SENDER(FE_MISSION_LIST_UPDATE_Y, MISSION_LIST_UPDATE);

#ifdef MULTIPLE_MISSION_HOSTS
		REGISTER_SENDRECV(MISSION_BROADCAST_NEW_Y, _sendBroadcastMission, _recvBroadcastMission, missBCAST_NEW);
		REGISTER_SENDRECV(MISSION_BRIADCAST_EXISTING_Y, _sendExistingMission, _recvExistingMission, missBCAST_EXISTING);
#endif // MULTIPLE_MISSION_HOSTS

		REGISTER_RECEIVER(HOST_LOCK_MISSION_Y, _recvLockMission);

	END_CALLBACK_REGISTRATION
}
