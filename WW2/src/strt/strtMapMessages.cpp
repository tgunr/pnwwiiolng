#include <syscfg.h> // Path is project specific.
// Created: Wed Jan 31 15:04:49 2001
//
// Strat support for the map system.  Basically, all updates from the
// strat system and destined for the map system are received in this code
// here.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "wwii/wwiiCallbacks.h"
#include "teul/teulBasic.h"
#include "strt/strtConstants.h"
#include "strt/strtTypes.h"
#include "strt/strtMapMessages.h"
#include "strt/strtPack.h"
#include "miss/missionPack.h"
#include "miss/missionDamageEvent.h"
#include "mm/mmSerialNumberRequest.h"
#include "weap/weapWeapon.h"

#include "strt/strtServerTypes.h"
#include "strt/strtPostListReply.h"
#include "map/mapHostMessages.h"
#include "garrisons/garrisonCountryChangeRequest.h"
#include "mm/mmMain.h"

#ifndef STRAT_CLIENT
#include "strt/strtFacilityWeaponList.h"
#include "miss/mission.h"
#include "mm/mmSpawn.h"
#include "mm/mmSpawnGrantResponse.h"
#endif

#if defined(IS_MAP_HOST)
# include "map/mapPlayer.h"
#endif

#define STICK_DEBUG

static TEUL_CALLBACK* _SEND_MM_POST_LIST;
static TEUL_CALLBACK* _XCHG_MM_SPAWN_GRANT;
static TEUL_CALLBACK* _XCHG_MM_FACILITY_CAPACITY;

static TEUL_CALLBACK* _SEND_MM_POST_REQ;
static TEUL_CALLBACK* _SEND_MM_JOIN_REQ;
static TEUL_CALLBACK* _SEND_MM_POST_INFO_UPDATE;
static TEUL_CALLBACK* _SEND_MM_POST_CANCEL;
static TEUL_CALLBACK* _SEND_MM_SPAWN_CANCEL;

static TEUL_CALLBACK* _XCHG_MM_JOIN_LIST;
static TEUL_CALLBACK* _XCHG_MM_LEAVE_LIST;
static TEUL_CALLBACK* _XCHG_MM_GARRISON_COUNTRY_CHANGE;

//Sends the MM Post request to the strat system
static SINT32 _sendMMPostReq(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_REQUEST));
}

static SINT32 _recvMMPostAccept(ACTION_OBJECT* const a)
{
	MM_POST_ACCEPT postAccept;
	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postAccept, sizeof(MM_POST_ACCEPT));

    dbugLog(_FLD,"c:%u postAccept " PRF_WEAPON_SERIAL " %d %d"
                    , postAccept.customerID
                    , PRA_WEAPON_SERIAL(postAccept.weaponSerial)
                    , postAccept.portal
                    , postAccept.immediateSpawn);
	//if (postAccept.portal == 0)
		//postAccept.portal = 1;

	Player* player = PlayerInterface::Lookup(postAccept.customerID);
	if ( player != nullptr )
	{
		mmProcessPostAccept(*player, postAccept, MM_ACCEPT_NEW);
		return a->offset;
	}


	dbugLog(_FLA, "t:%d unknown persona, ignoring", a->teulIndex);
	return -EINVAL;
}

static SINT32
_recvMMPostDeny(ACTION_OBJECT* const a)
{
	MM_POST_DENY postDeny;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postDeny, sizeof(MM_POST_DENY));


#ifdef DEBUG
	dbugLog(_FLD,"postDeny: %d %d:%d:%d:%d %d", postDeny.customerID, postDeny.weapon.wCountry
				, postDeny.weapon.wCategory, postDeny.weapon.wClass
				, postDeny.weapon.wType, postDeny.reasonCode);
#endif

	mmProcessPostDeny(postDeny);

	return a->offset;
}

static SINT32 _sendMMPostListReq(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_LIST_REQUEST));
}

//Sends the MM Join request to the strat system
static SINT32 _sendMMJoinReq(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_JOIN_REQUEST));
}

static SINT32 _recvMMJoinAccept(ACTION_OBJECT* const a)
{
	MM_JOIN_ACCEPT joinAccept;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &joinAccept, sizeof(MM_JOIN_ACCEPT));

#ifdef DEBUG
	dbugLog(_FLD,"join Accept: %d %d:%d:%d:%d %d %d"
				, joinAccept.customerID
				,joinAccept.postedWeapon.weaponSerial.spec.wCountry
				, joinAccept.postedWeapon.weaponSerial.spec.wCategory
				, joinAccept.postedWeapon.weaponSerial.spec.wClass
				, joinAccept.postedWeapon.weaponSerial.spec.wType
				, joinAccept.postedWeapon.weaponSerial.serial
				, joinAccept.postedWeapon.commander);
#endif

	//dbugLog(_FLD,"handling join accept");

#ifndef STRAT_CLIENT
	mmProcessJoinAccept(joinAccept);
#endif

	return a->offset;
}

static SINT32 _recvMMJoinDeny(ACTION_OBJECT* const a)
{
MM_JOIN_DENY joinDeny;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &joinDeny, sizeof(MM_JOIN_DENY));

#ifdef DEBUG
	dbugLog(_FLD,"joinDeny: %d %d", joinDeny.customerID, joinDeny.reasonCode);
#endif
	mmProcessJoinDeny(joinDeny);

	return a->offset;
}

static SINT32 _recvMMPostExpired(ACTION_OBJECT* const a)
{
MM_POST_EXPIRED postExpired;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postExpired, sizeof(MM_POST_EXPIRED));

#ifdef STRAT_CLIENT
	dbugWrite(_FLD,"postExpired: %d %d\n",postExpired.customerID, postExpired.weaponSerial.serial);
#endif

#ifndef STRAT_CLIENT
	mmProcessPostExpired(postExpired);
#endif

	return a->offset;
}

static SINT32
_sendMMSpawnGrantRequest(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_SPAWN_GRANT_REQUEST));
}

static SINT32
_recvMMSpawnGrantResponse(ACTION_OBJECT* const a)
{
	MM_SPAWN_GRANT_RESPONSE spawnGrantResponse;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &spawnGrantResponse, sizeof(MM_SPAWN_GRANT_RESPONSE));

#ifdef STRAT_CLIENT
	dbugWrite(_FLD,"spawnGrantResponse: %d %d\n", spawnGrantResponse.commander, spawnGrantResponse.crew);
#endif

#ifndef STRAT_CLIENT
	dbugLog(_FLD,"spawnGrantResponse: %d %d", spawnGrantResponse.commander, spawnGrantResponse.crew);
	mmProcessSpawnGrantResponse(spawnGrantResponse);
#endif

	return a->offset;
}

static SINT32 _sendMMJoinListRequest(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_INFO_REQUEST));
}

static SINT32 _sendMMLeaveListRequest(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_INFO_REQUEST));
}

static SINT32 _recvMMJoinListResponse(ACTION_OBJECT* const a)
{
	MM_JOIN_LIST_RESPONSE response;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &response, sizeof(MM_JOIN_LIST_RESPONSE));

	mmProcessJoinListResponse(response);

	return a->offset;
}

static SINT32 _recvMMLeaveListResponse(ACTION_OBJECT* const a)
{
	MM_POST_INFO_RESPONSE postInfoResponse;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postInfoResponse, sizeof(MM_POST_INFO_RESPONSE));

//	mmProcessLeaveListResponse(&postInfoResponse);

	return a->offset;
}

static SINT32 _sendMMPostInfoUpdate(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_INFO_UPDATE));
}

static SINT32 _sendMMPostCancel(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_CANCEL));
}

static SINT32 _sendMMSpawnCancel(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_SPAWN_CANCEL));
}

static SINT32 _recvMMCrewBailed(ACTION_OBJECT* const a)
{
MM_CREW_BAILED crewBailed;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &crewBailed, sizeof(MM_CREW_BAILED));

#ifdef DEBUG
	dbugLog(_FLD, "cmdr:%u crew:%u crew bailed", crewBailed.comanderID, crewBailed.crewID);
#endif

#ifndef STRAT_CLIENT
	mmProcessCrewBailed(crewBailed);
#endif

	return a->offset;
}


static SINT32 _sendMMFacilityCapacityRequest(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_CAPACITY_REQUEST));
}

static SINT32 _recvMMFacilityCapacityResponse(ACTION_OBJECT* const a)
{
	MM_CAPACITY_RESPONSE response;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &response, sizeof(MM_CAPACITY_RESPONSE));

#ifdef STRAT_CLIENT
	dbugLog(_FLD, "capacity: %d totalRPs: %d", response.capacity, response.totalRPs);
#else

	Player* player = PlayerInterface::Lookup(response.customerID);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "%s(%u): null persona", __FUNCTION__, response.customerID);
		return -EINVAL;
	}

#ifdef STICK_DEBUG
	dbugLog(_FLD, "t:%d p:%u c:%s tag %d fac.cap: %d rps: %d"
					, player->teulIndex(), player->playerID(), player->callsign()
					, response.tag, response.capacity, response.totalRPs
					);
#endif

	switch(response.tag)
	{
		case CAPACITY_AFTER_MISSION:
			player->processCapacityResponse(response.rtbFacility, response.rtbReservedWeapon, response.vehicleDelivered);
		break;

		case CAPACITY_BEFORE_MISSION:

			//save the info to the mp
			if ( !player->validMissionPacket() )
			{
				dbugLog(_FLA,"%s() null mission packet", __FUNCTION__);
				return -EINVAL;
			}

			break;
		default:
			dbugLog(_FLW, "unrecognized missCap response.tag %d for %s"
							, response.tag
							, player->callsign());
			break;
	}
#endif

	return a->offset;
}

//This function sends a request to post a weapon at a facility
SINT32
strtSendMMPostRequest(SINT32 connection, UINT32 customerID, const char* callsign
					, UINT32 missionID, UINT32 brigadeID, const PermissionMask permMask
					, const FULL_OBJ_ID facility, WEAPON_SPEC weapon
					, bool multicrew, bool immediateSpawn)
{
	MM_POST_REQUEST postRequest;

	SINT32 retval;

	postRequest.customerID = customerID;
	postRequest.handle = chatGetHandle(callsign);
	postRequest.missionID = missionID;
	postRequest.brigadeID = brigadeID;
	postRequest.permMask = permMask;
	postRequest.facility = facility;
	postRequest.weapon = weapon;
	postRequest.immediateSpawn = immediateSpawn;
	postRequest.multicrew = multicrew;

	retval = teulSend(connection, _SEND_MM_POST_REQ, &postRequest);
	if (retval < 0)
		return retval;

	return 0;
}

//This function sends a request to list all the posted weapons at a facility
SINT32
strtSendMMPostListRequest(SINT32 connection, UINT32 customerID, const char* callsign, UINT32 missionID, UINT32 brigadeID, const FULL_OBJ_ID facility)
{
	SINT32 retval;
	MM_POST_LIST_REQUEST postListRequest;

	postListRequest.crewID = customerID;
	postListRequest.missionID = missionID;
	postListRequest.facility = facility;
	postListRequest.crewHandle = chatGetHandle(callsign);
	postListRequest.brigadeID = brigadeID;

	retval = teulSend(connection, _SEND_MM_POST_LIST, &postListRequest);

	if (retval < 0)
		return retval;

	return 0;
}

//This function sends a join request to the strat system
SINT32 strtSendMMJoinRequest(SINT32 connection, UINT32 customerID, const WEAPON_SERIAL& weaponSerial)
{
	SINT32 retval;
	MM_JOIN_REQUEST joinRequest;

	joinRequest.customerID = customerID;
	joinRequest.weaponSerial = weaponSerial;

	dbugLog(_FLD,"joinRequest: %d %d:%d:%d:%d:%d %d:%d:%d:%d"
				, joinRequest.customerID
				, joinRequest.weaponSerial.source.cell.ox, joinRequest.weaponSerial.source.cell.oy
				, joinRequest.weaponSerial.source.id, joinRequest.weaponSerial.source.objClass
				, joinRequest.weaponSerial.source.objType, joinRequest.weaponSerial.spec.wCountry
				, joinRequest.weaponSerial.spec.wCategory, joinRequest.weaponSerial.spec.wClass
				, joinRequest.weaponSerial.spec.wType);

	retval = teulSend(connection, _SEND_MM_JOIN_REQ, &joinRequest);

	if (retval < 0)
		return retval;

	return 0;
}

SINT32 strtSendMMPostInfoUpdate(SINT32 connection, UINT32 customerID, const WEAPON_SERIAL& weaponSerial, bool multicrew)
{
	SINT32 retval;
	MM_POST_INFO_UPDATE infoUpdate;

	infoUpdate.customerID = customerID;
	infoUpdate.weaponSerial = weaponSerial;
	infoUpdate.multicrew = multicrew;

	retval = teulSend(connection, _SEND_MM_POST_INFO_UPDATE, &infoUpdate);

	if (retval < 0)
		return retval;

	return 0;
}

SINT32
strtSendMMSpawnGrantRequest(SINT32 connection, const WEAPON_SERIAL& weaponSerial, Player& player)
{
	SINT32 retval;
	MM_SPAWN_GRANT_REQUEST spawnGrantRequest;

	spawnGrantRequest.weaponSerial = weaponSerial;
	spawnGrantRequest.playerID = player.playerID();
	spawnGrantRequest.missionID = 0;
	spawnGrantRequest.usedMobileSpawn = player.m_wantMobileSpawn;

	if ( player.validMissionPacket() )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s spawnGrantRequest m:%u br:%u v:%u %s at " PRF_FULL_OBJ_ID ""
						, player.teulIndex()
						, player.playerID()
						, player.callsign()
						, player.missionID()
						, player.brigadeID()
						, spawnGrantRequest.weaponSerial.serial
						, weapPrintName(spawnGrantRequest.weaponSerial.spec)
						, PRA_FULL_OBJ_ID(spawnGrantRequest.weaponSerial.source)
						);

		spawnGrantRequest.missionID = player.missionID();

		if(player.missionPacket().m_missionType == POINT_ATTACK ||
			player.missionPacket().m_missionType == AREA_ATTACK ||
			player.missionPacket().m_missionType == RESUPPLY)
		{
			//get capacity of target
			spawnGrantRequest.facility = player.missionPacket().sortie().target();
		}
		else if(player.missionPacket().m_missionType == POINT_DEFENSE ||
			player.missionPacket().m_missionType == AREA_DEFENSE)
		{
			//get capacity of origin
			spawnGrantRequest.facility = player.missionPacket().sortie().originFac();
		}
		else
		{
			dbugLog(_FLA, "t:%d p:%u c:%s illegal missionType %d"
							, player.teulIndex()
							, player.playerID()
							, player.callsign()
							, player.missionPacket().m_missionType
							);
		}
	}

	retval = teulSend(connection, _XCHG_MM_SPAWN_GRANT, &spawnGrantRequest);

	if (retval < 0)
	{
		dbugLog(_FLI, "t:%d p:%u c:%s failed to send spawn grant request", player.teulIndex(), player.playerID(), player.callsign());
		return retval;
	}

	return 0;
}

SINT32 strtSendMMJoinListRequest(SINT32 connection, UINT32 crewID, const WEAPON_SERIAL& weaponSerial, const ChatHandle handle, bool join)
{
	SINT32 retval;
	MM_POST_INFO_REQUEST postInfoRequest;

	postInfoRequest.customerID = crewID;
	postInfoRequest.weaponSerial = weaponSerial;
	postInfoRequest.handle = handle;
	postInfoRequest.join = join;

	retval = teulSend(connection, _XCHG_MM_JOIN_LIST, &postInfoRequest);

	if (retval < 0)
		return retval;

	return 0;
}

//This function sends a post cancel to the strat system
SINT32
strtSendMMPostCancel(SINT32 connection, const WEAPON_SERIAL& weaponSerial)
{
	SINT32 retval;
	MM_POST_CANCEL postCancel;

	postCancel.weaponSerial = weaponSerial;

	retval = teulSend(connection, _SEND_MM_POST_CANCEL, &postCancel);

	if (retval < 0)
		return retval;

	return 0;
}

//This function sends a spawn cancel to the strat system
SINT32 strtSendMMSpawnCancel(SINT32 connection, UINT32 customerID, const FULL_OBJ_ID facility, const WEAPON_SERIAL& weaponSerial, const FULL_OBJ_ID originFacility)
{
	SINT32 retval;
	MM_SPAWN_CANCEL spawnCancel;

	spawnCancel.customerID = customerID;
	spawnCancel.weaponSerial = weaponSerial;
	spawnCancel.facility = facility;
	spawnCancel.originFacility = originFacility;

	retval = teulSend(connection, _SEND_MM_SPAWN_CANCEL, &spawnCancel);

	if (retval < 0)
		return retval;

	return 0;
}

//This function sends a capacity request to the strat system
SINT32 strtSendMMCapacityRequest(SINT32 connection, UINT32 tag, UINT32 missionID, UINT32 customerID, const FULL_OBJ_ID facility, CODE_RTB rtb)
{
	MM_CAPACITY_REQUEST capacityRequest;
	SINT32 retval;

	capacityRequest.tag = tag;
	capacityRequest.missionID = missionID;
	capacityRequest.customerID = customerID;
	capacityRequest.facility = facility;
	capacityRequest.rtb = rtb;

    dbugLog(_FLD, "capacityRequest: %u:%u:%u @ " PRF_FULL_OBJ_ID " code:%u"
                , capacityRequest.tag, capacityRequest.missionID
                , capacityRequest.customerID
                , PRA_FULL_OBJ_ID(capacityRequest.facility)
                , capacityRequest.rtb
                );

	retval = teulSend(connection, _XCHG_MM_FACILITY_CAPACITY, &capacityRequest);

	if (retval < 0)
		return retval;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a strat damage effect message

static SINT32
_recvMissionDamageEvent(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<MissionDamageEvent>() == false )
		return -EINVAL;

	auto& incoming = a->read<MissionDamageEvent>();

	// Can we find this player?
	Player* player = PlayerInterface::Lookup(incoming.m_playerID);
	if ( player == nullptr )
	{
		dbugLog(_FLI, "t:%d p:%u received mde for non-existant (logged out?) player", a->teulIndex, incoming.m_playerID);
		return -EINVAL;
	}

	// Can we find their sortie?
	if ( player->weapon().serial != incoming.m_sortieID )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s mde has arrived too late", a->teulIndex, player->playerID(), player->callsign());
		return -EINVAL;
	}

	player->addStratHit(incoming);

	return 0;
}

// Send a Garrison country change request to Strat.
static SINT32 _sendMMGarrisonCountryChangeRequest(ACTION_OBJECT* const a)
{
	GarrisonCountryChangeReq* request = (GarrisonCountryChangeReq*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

// Garrison country change response from Strat.
static SINT32 _recvMMGarrisonCountryChangeResponse(ACTION_OBJECT* const a)
{
	dbugLog(_FLD, "GarrisonCountryChangeReq received on map host from strat.");
	return mmhostToClientGarrisonCountryChange(a);
}

// Send an GarrisonCountryChangeReq recieved from a client to the Strat host.
SINT32 strtSendGarrisonCountryChangeRequest(GarrisonCountryChangeReq& garrisonCountryChangeReq)
{
	GarrisonCountryChangeReq gcc = garrisonCountryChangeReq;
	dbugLog(_FLD, "GarrisonCountryChangeRequest is being sent to strat.");
	return gcc.sendToStrat(mapGetStratIndex());
    //return garrisonCountryChangeReq->sendToStrat(mapGetStratIndex());
}

// Request a GarrisonCountryChangeReq from Strat.
SINT32 GarrisonCountryChangeReq::sendToStrat(SINT32 teulIndex) {
	// 
	return teulSend(teulIndex, _XCHG_MM_GARRISON_COUNTRY_CHANGE, this);
}

//////////////////////////////////////////////////////////////////////
//Install all the map callbacks
SINT32
strtInstallMapCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
	{
		REGISTER_SENDRECV(STRT_MM_POST_LIST_Y, _sendMMPostListReq, PostListReply::Process, _SEND_MM_POST_LIST);
		REGISTER_SENDRECV(STRT_MM_SPAWN_GRANT_Y, _sendMMSpawnGrantRequest, _recvMMSpawnGrantResponse, _XCHG_MM_SPAWN_GRANT);
		REGISTER_SENDRECV(STRT_MM_CAPACITY_Y, _sendMMFacilityCapacityRequest, _recvMMFacilityCapacityResponse, _XCHG_MM_FACILITY_CAPACITY);
		REGISTER_SENDRECV(STRT_MM_JOIN_LIST_Y, _sendMMJoinListRequest, _recvMMJoinListResponse, _XCHG_MM_JOIN_LIST);
		REGISTER_SENDRECV(STRT_MM_LEAVE_LIST_Y, _sendMMLeaveListRequest, _recvMMLeaveListResponse, _XCHG_MM_LEAVE_LIST);
		REGISTER_SENDRECV(STRT_MM_GARRISON_COUNTRY_CHANGE_Y, _sendMMGarrisonCountryChangeRequest, _recvMMGarrisonCountryChangeResponse, _XCHG_MM_GARRISON_COUNTRY_CHANGE);

		REGISTER_SENDER(STRT_MM_POST_REQ_Y, _sendMMPostReq, _SEND_MM_POST_REQ);
		REGISTER_SENDER(STRT_MM_POST_CANCEL_Y, _sendMMPostCancel, _SEND_MM_POST_CANCEL);
		REGISTER_SENDER(STRT_MM_SPAWN_CANCEL_Y, _sendMMSpawnCancel, _SEND_MM_SPAWN_CANCEL);
		REGISTER_SENDER(STRT_MM_POST_INFO_UPDATE_Y, _sendMMPostInfoUpdate, _SEND_MM_POST_INFO_UPDATE);
		REGISTER_SENDER(STRT_MM_JOIN_REQ_Y, _sendMMJoinReq, _SEND_MM_JOIN_REQ);

		REGISTER_RECEIVER(STRT_MM_POST_ACCEPT_Y, _recvMMPostAccept);
		REGISTER_RECEIVER(STRT_MM_POST_DENY_Y, _recvMMPostDeny);
		REGISTER_RECEIVER(STRT_MM_JOIN_ACCEPT_Y, _recvMMJoinAccept);
		REGISTER_RECEIVER(STRT_MM_JOIN_DENY_Y, _recvMMJoinDeny);
		REGISTER_RECEIVER(STRT_MM_POST_EXPIRED_Y, _recvMMPostExpired);
		REGISTER_RECEIVER(STRT_MM_CREW_BAILED_Y, _recvMMCrewBailed);

		REGISTER_RECEIVER(STRT_MM_DAMAGE_EVENT_Y, _recvMissionDamageEvent);

		REGISTER_MSG(STRT_MM_SERIAL_REQUEST_Y, SERIAL_NUMBER_REQUEST);
	}
	END_CALLBACK_REGISTRATION(Strat::CallbackX)
}

