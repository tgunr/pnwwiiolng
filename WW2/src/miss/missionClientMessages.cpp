#include "syscfg.h" // Path is project specific.
// Created: Tue Jan 23 21:04:48 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "client/client.h"
#include "miss/missionClientMessages.h"
#include "miss/missionConstants.h"
#include "miss/missionDetail.h"
#include "miss/missionListUpdate.h"
#include "miss/missionPack.h"
#include "miss/missionResults.h"
#include "miss/missionUtils.h"
#include "mm/mmSpawnClientMessages.h"
#include "strt/strtFacilityWeaponList.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdaterPack.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiRoomLists.h"

TEUL_CALLBACK*	missSEND_INFO_REQ;
TEUL_CALLBACK*	missSEND_CREATION_PRIV_REQ;
TEUL_CALLBACK*	missSEND_POST_REQ;
TEUL_CALLBACK*	missSEND_EXIT;
TEUL_CALLBACK*	missSEND_WEP_LIST_REQ;

#ifdef LOAD_TEST
#include "client/client.h"
#endif
#ifdef LOAD_TEST
extern UINT32 getRand( int lower, int upper );
#endif

SINT32 missionSendPostRequest(SINT32 hostIndex, MISSION_POST_REQ const* request)
{
	return teulSend(hostIndex, missSEND_POST_REQ, (void*)request);
}

SINT32 missionSendExit(SINT32 hostIndex, UINT32 cancelMissionID)
{
	return teulSend(hostIndex, missSEND_EXIT, &cancelMissionID);
}

void
missionSendDetailRequest(SINT32 hostIndex, UINT32 missionID, bool joinRequest)
{
	MISSION_DETAIL packet;

	// Tell the host whether we want to join or view this mission
	packet.m_flags = joinRequest;

	packet.request(hostIndex, missionID);
}

// missionSendWeaponListRequest()
SINT32 missionSendWeaponListRequest(SINT32 hostIndex, FULL_OBJ_ID facility, LOGICAL allWeapons, LOGICAL isRefresh)
{
WEP_LIST_REQ request;

	request.facility = facility;
	request.allWeapons = allWeapons;
	request.isRefresh = isRefresh;
	return teulSend(hostIndex, missSEND_WEP_LIST_REQ, &request);
}

static SINT32 _sendMISSION_POST_REQ(ACTION_OBJECT* const a)
{
#ifdef DEBUG
	dbugWrite(_FLW, "sending MISSION_POST_REQ");
#endif
	return missPackMISSION_POST_REQ(TEUL_PACK, a->packed, a->offset, (MISSION_POST_REQ*) a->localRawData, 0);

}

static SINT32 _sendMISSION_EXIT(ACTION_OBJECT* const a)
{
#ifdef DEBUG
	dbugWrite(_FLW, "sending MISSION_EXIT");
#endif

	// Send the UINT32 we were called with
	return teulPackUINT32(TEUL_PACK, a->packed, a->offset, (UINT32*)a->localRawData, 0);
}

static SINT32 _sendWEP_LIST_REQ(ACTION_OBJECT* const a)
{
#ifdef DEBUG
	dbugWrite(_FLW, "sending WEP_LIST_REQ");
#endif
	return missPackWEP_LIST_REQ(TEUL_PACK, a->packed, a->offset, (WEP_LIST_REQ*) a->localRawData, 0);
}

#if(0)
static SINT32 _sendMISSION_RESULT_REQ(ACTION_OBJECT* const a)
{
#ifdef DEBUG
	dbugWrite(_FLW, "Requesting Mission Summary");
#endif
	return a->offset;
}
#endif

static SINT32
_recvMISSION_POST_REPLY(ACTION_OBJECT* const a)
{
	MISSION_POST_REPLY result;

	//unpack
	a->offset = missPackMISSION_POST_REPLY(TEUL_UNPACK, a->packed, a->offset, &result, 0);

	dbugWrite(_FLW, "postreply is %d reasoncode is %d missionID is %d ", result.posted, result.reasonCode, result.missionID);

	clientMissionPostReply(a->teulIndex, &result);

	return a->offset;
}

// Incoming room list - optimization to avoid reallocations
static RoomList _inRoomList;

static SINT32
_recvRoomList(ACTION_OBJECT* const a)
{
	a->offset = _inRoomList.packer(TEUL_UNPACK, a);

	clientMissionRoomListReply(a->teulIndex, &_inRoomList);

	return a->offset;
}

static SINT32
_recvMISS_LAUNCH_UPDATE(ACTION_OBJECT* const a)
{
	MISS_LAUNCH_UPDATE launchupdate;

	a->offset = missPackMISS_LAUNCH_UPDATE(TEUL_UNPACK, a->packed, a->offset
								, &launchupdate, sizeof(launchupdate));

	clientMissionLaunchUpdate(a->teulIndex, &launchupdate);

	return a->offset;

}

// Receive after action results
static SINT32 _recvMISSION_RESULT_REPLY(ACTION_OBJECT* const a)
{
	MISSION_RESULTS results;

	a->offset = missPackMISSION_RESULTS(TEUL_UNPACK, a->packed, a->offset, &results, sizeof(results));

	clientMissionResultsReply(a->teulIndex, &results);

	return a->offset;
}


//Receive CLIENT_WEP_LIST
//
//to get this, you send a MISSION_INFO_REQ with condensed field == TRUE
static SINT32
_recvCLIENT_WEP_LIST(ACTION_OBJECT* const a)
{
	clientProcessWeapList(a->teulIndex, a);
	return a->offset;
}

template <> SINT32
MISSION_DETAIL::receive(SINT32 teulIndex, MISSION_DETAIL& incoming)
{
	if ( incoming.action() == FAILED )
	{
		// Do one thing
		///TODO: Handle me!
		// clientMissionDetailFailed(); -> so the request knows it failed?
		// Say "barf" to the host
		return -EINVAL;
	}

	// Otherwise...
	// MissionDetail is derived from mission summary, see missionDetail.h
	clientMissionDetailReply(teulIndex, incoming.data());	// Replaces MissionCheckReply()

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Ask the host to re-send us mission results.

SINT32
missRequestMissionResults(const SINT32 teulIndex, const UINT32 sortieID)
{
	// Prevent the client from spamming the host.
	static TeulTimeMS nextReq = 0ULL;
	if ( teulClock64 < nextReq )
		return 0;

	Teul::Packet outgoing(Teul::PacketId(MISSION_X, FE_MISSION_RESULT_REPLY_Y), sortieID);
	const SINT32 retval = outgoing.send(teulIndex);

	// No more than once every 2.5 seconds.
	nextReq = teulClock64 + 2500;

	return retval;
}

SINT32
missionInstallCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(MISSION_X)
	{
		REGISTER_SENDER(FE_MISSION_POST_REQ_Y, _sendMISSION_POST_REQ, missSEND_POST_REQ);
		REGISTER_SENDER(FE_MISSION_EXIT_Y, _sendMISSION_EXIT, missSEND_EXIT);
		REGISTER_SENDER(FE_MISSION_WEP_LIST_REQ_Y, _sendWEP_LIST_REQ, missSEND_WEP_LIST_REQ);

		REGISTER_RECEIVER(FE_MISSION_POST_REPLY_Y, _recvMISSION_POST_REPLY);
		REGISTER_RECEIVER(FE_MISSION_CLIENT_WEP_LIST_Y, _recvCLIENT_WEP_LIST);
		REGISTER_RECEIVER(FE_MISSION_RESULT_REPLY_Y, _recvMISSION_RESULT_REPLY);
		REGISTER_RECEIVER(FE_MISSION_ROOM_LIST_Y, _recvRoomList);
		REGISTER_RECEIVER(FE_MISSION_LAUNCH_UPDATE_Y, _recvMISS_LAUNCH_UPDATE);

		// Register this callback (_Y) to that teul_network_msg
		REGISTER_MSG(FE_MISSION_DETAIL_Y, MISSION_DETAIL);
		REGISTER_MSG_RECEIVER(FE_MISSION_LIST_UPDATE_Y, MISSION_LIST_UPDATE);
	}
	END_CALLBACK_REGISTRATION(MISSION_X)
}
