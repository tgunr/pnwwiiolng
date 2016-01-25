#include <syscfg.h> // Path is project specific.
// Date: Wed Apr 18 13:44:11 2001
//
// These are the messages the front end sends and receives in congress with
// the map server to negotiate spawning
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "mem/mem.h"
#include "mm/mmConstants.h"
#include "mm/mmSpawnTypes.h"
#include "mm/mmPack.h"
#include "mm/mmSpawnClientMessages.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPack.h"
#include "mm/mmDirectSpawn.h"
#include "ui/uiChat.h"
#include "client/clientConnections.h"	// For MapHostConnection.

#include "client/client.h"

static TEUL_CALLBACK* MM_SEND_FE_POST_REQ;
static TEUL_CALLBACK* MM_SEND_FE_POST_LIST_REQ;
static TEUL_CALLBACK* MM_SEND_FE_JOIN_REQ;
static TEUL_CALLBACK* MM_SEND_FE_CREW_REQ;
static TEUL_CALLBACK* MM_SEND_FE_CREW_ACCEPT;
static TEUL_CALLBACK* MM_SEND_FE_POST_CANCEL;
static TEUL_CALLBACK* MM_SEND_FE_JOIN_PERM_KILL;
static TEUL_CALLBACK* MM_SEND_FE_CLIENT_SPAWN;
static TEUL_CALLBACK* _SEND_DIRECT_SPAWN;
static TEUL_CALLBACK* _SEND_DO_DESPAWN;

static SINT32 _sendFEPostRequest(ACTION_OBJECT* const a)
{
	return mmPackFEPostRequest(TEUL_PACK, a->packed, a->offset, (FE_POST_REQUEST*)a->localRawData, 0);
}

static SINT32 _sendFEPostListRequest(ACTION_OBJECT* const a)
{
	return mmPackFEPostListRequest(TEUL_PACK, a->packed, a->offset, (FE_POST_LIST_REQUEST*)a->localRawData, 0);
}

static SINT32 _sendFEJoinRequest(ACTION_OBJECT* const a)
{
	return mmPackFEJoinRequest(TEUL_PACK, a->packed, a->offset, (FE_JOIN_REQUEST*)a->localRawData, 0);
}

static SINT32 _sendFECrewRequest(ACTION_OBJECT* const a)
{
	return mmPackFECrewRequest(TEUL_PACK, a->packed, a->offset, (FE_CREW_REQUEST*)a->localRawData, 0);
}

static SINT32 _sendFECrewAccept(ACTION_OBJECT* const a)
{
	return mmPackFECrewAccept(TEUL_PACK, a->packed, a->offset, (FE_CREW_ACCEPT*)a->localRawData, 0);
}

static SINT32 _sendFEJoinPermKill(ACTION_OBJECT* const a)
{
	return mmPackFEJoinPermKill(TEUL_PACK, a->packed, a->offset, (FE_JOIN_PERM_KILL*)a->localRawData, 0);
}

static SINT32 _sendFEPostCancel(ACTION_OBJECT* const a)
{
	return mmPackFEPostCancel(TEUL_PACK, a->packed, a->offset, (FE_POST_CANCEL*)a->localRawData, 0);
}

static SINT32 _sendFEClientSpawn(ACTION_OBJECT* const a)
{
	return mmPackFEClientSpawn(TEUL_PACK, a->packed, a->offset, (FE_CLIENT_SPAWN*)a->localRawData, 0);
}

static SINT32 _sendDirectSpawn(ACTION_OBJECT* const a)
{
	return ((Map::DirectSpawn*)(a->localRawData))->packer(TEUL_PACK, a);
}


static SINT32 _recvFEPostAccept(ACTION_OBJECT* const a)
{
FE_POST_ACCEPT postAccept;

	a->offset = mmPackFEPostAccept(TEUL_UNPACK, a->packed, a->offset, &postAccept, 0);

	dbugWrite(_FLN,"post Accept: %d:%d:%d:%d:%d %d:%d:%d:%d %d %d\n", postAccept.weaponSerial.source.cell.ox, postAccept.weaponSerial.source.cell.oy, postAccept.weaponSerial.source.id, postAccept.weaponSerial.source.objClass, postAccept.weaponSerial.source.objType, postAccept.weaponSerial.spec.wCountry, postAccept.weaponSerial.spec.wCategory, postAccept.weaponSerial.spec.wClass, postAccept.weaponSerial.spec.wType, postAccept.weaponSerial.serial, postAccept.portal);
	clientProcessPostAccept(a->teulIndex,&postAccept);

	return a->offset;
}

static SINT32 _recvFEPostDeny(ACTION_OBJECT* const a)
{
FE_POST_DENY postDeny;

	a->offset = mmPackFEPostDeny(TEUL_UNPACK, a->packed, a->offset, &postDeny, 0);

	clientProcessPostDeny(a->teulIndex, &postDeny);
	dbugWrite(_FLN,"postDeny: %d:%d:%d:%d %d\n", postDeny.weapon.wCountry, postDeny.weapon.wCategory, postDeny.weapon.wClass, postDeny.weapon.wType, postDeny.reasonCode);

	return a->offset;
}

static SINT32 _recvFEPostExpired(ACTION_OBJECT* const a)
{
FE_POST_EXPIRED postExpired;

	a->offset = mmPackFEPostExpired(TEUL_UNPACK, a->packed, a->offset, &postExpired, 0);

	clientProcessPostExpired(a->teulIndex, &postExpired);
	dbugWrite(_FLN,"post Expired: %d:%d:%d:%d:%d %d:%d:%d:%d %d\n", postExpired.weaponSerial.source.cell.ox, postExpired.weaponSerial.source.cell.oy, postExpired.weaponSerial.source.id, postExpired.weaponSerial.source.objClass, postExpired.weaponSerial.source.objType, postExpired.weaponSerial.spec.wCountry, postExpired.weaponSerial.spec.wCategory, postExpired.weaponSerial.spec.wClass, postExpired.weaponSerial.spec.wType, postExpired.weaponSerial.serial);

	return a->offset;
}

static SINT32 _recvFEPostListResponse(ACTION_OBJECT* const a)
{
FE_POST_LIST_RESPONSE postListResponse;

#ifdef MM_CLIENT
UINT32 i;
#endif

	postListResponse.numWeapons = 0;
	postListResponse.commandedWeapon = nullptr;

	a->offset = mmPackFEPostListResponse(TEUL_UNPACK, a->packed, a->offset, &(postListResponse), 0);

#ifdef MM_CLIENT
	//Debug stuff
	dbugWrite(_FLN,"%d:%d:%d:%d:%d %d\n", postListResponse.facility.cell.ox, postListResponse.facility.cell.oy, postListResponse.facility.id, postListResponse.facility.objClass, postListResponse.facility.objType,
postListResponse.numWeapons);

	for (i=0; i< postListResponse.numWeapons; i++)
	{
		dbugWrite(_FLN,"\t%d:%d:%d:%d:%d %d:%d:%d:%d %d %s\n",
			postListResponse.commandedWeapon[i].weaponSerial.source.cell.ox,
			postListResponse.commandedWeapon[i].weaponSerial.source.cell.oy,
			postListResponse.commandedWeapon[i].weaponSerial.source.id,
			postListResponse.commandedWeapon[i].weaponSerial.source.objClass,
			postListResponse.commandedWeapon[i].weaponSerial.source.objType,
			postListResponse.commandedWeapon[i].weaponSerial.spec.wCountry,
			postListResponse.commandedWeapon[i].weaponSerial.spec.wCategory,
			postListResponse.commandedWeapon[i].weaponSerial.spec.wClass,
			postListResponse.commandedWeapon[i].weaponSerial.spec.wType,
			postListResponse.commandedWeapon[i].weaponSerial.serial,
			postListResponse.commandedWeapon[i].handle);
	}
#endif

	clientProcessPostList(a->teulIndex, &postListResponse);

	if (postListResponse.commandedWeapon!= nullptr)
		memFree(postListResponse.commandedWeapon);

	return a->offset;
}

static SINT32 _recvFEJoinAccept(ACTION_OBJECT* const a)
{
FE_JOIN_ACCEPT joinAccept;

	a->offset = mmPackFEJoinAccept(TEUL_UNPACK, a->packed, a->offset, &joinAccept, 0);

	clientProcessJoinAccept(a->teulIndex, &joinAccept);
	dbugWrite(_FLN,"join Accept: %d:%d:%d:%d %d %d:%d\n", joinAccept.postedWeapon.weaponSerial.spec.wCountry, joinAccept.postedWeapon.weaponSerial.spec.wCategory, joinAccept.postedWeapon.weaponSerial.spec.wClass, joinAccept.postedWeapon.weaponSerial.spec.wType, joinAccept.postedWeapon.weaponSerial.serial, joinAccept.postedWeapon.commander, joinAccept.postedWeapon.crew);

	return a->offset;
}

static SINT32 _recvFEJoinDeny(ACTION_OBJECT* const a)
{
FE_JOIN_DENY joinDeny;

	a->offset = mmPackFEJoinDeny(TEUL_UNPACK, a->packed, a->offset, &joinDeny, 0);

	clientProcessJoinDeny(a->teulIndex, &joinDeny);
	dbugWrite(_FLN,"joinDeny: %d\n", joinDeny.reasonCode);

	return a->offset;
}

static SINT32 _recvFEJoinPermRequest(ACTION_OBJECT* const a)
{
	FE_JOIN_PERM_REQUEST joinPermRequest;

	a->offset = mmPackFEJoinPermRequest(TEUL_UNPACK, a->packed, a->offset, &joinPermRequest, 0);

	clientProcessJoinPerm(a->teulIndex, &joinPermRequest);

	dbugWrite(_FLN,"join vehicle requested by %u:%s\n"
					,joinPermRequest.joinRequestID, chatGetCallsign(joinPermRequest.chatHandle));

#ifndef FRONT_END
#warning "move"
	mmSendAcceptCrew(a->teulIndex, joinPermRequest.joinRequestID);
#endif

	return a->offset;
}

static SINT32 _recvFESpawnNotify(ACTION_OBJECT* const a)
{
	FESpawnNotify spawnNotify(a);

	clientProcessSpawnNotify(a->teulIndex, spawnNotify);

	return a->offset;
}

static SINT32 _recvFECrewBailed(ACTION_OBJECT* const a)
{
FE_CREW_BAILED crewBailed;

	a->offset = mmPackFECrewBailed(TEUL_UNPACK, a->packed, a->offset, &crewBailed, 0);

	clientProcessCrewBailed(a->teulIndex, &crewBailed);
	dbugWrite(_FLN,"crewBailed %d\n",crewBailed.reasonCode);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Request a vehicle "post" (reservation to us humans).
// CAVEATS:
//  - Facility is redundant since 1.19
//  - Weapon must be a valid weapon spec
//  - If immediateSpawn is true and the post is granted, the host will
//    proceed as though this was also a SpawnGrantRequest

SINT32
mmSendPostRequest(SINT32 connection, const FULL_OBJ_ID& facility, const WEAPON_SPEC& weapon, LOGICAL immediateSpawn)
{
	FE_POST_REQUEST postRequest;

  postRequest.facility = facility;
	postRequest.weapon = weapon;
	postRequest.immediateSpawn = immediateSpawn;

	return teulSend(connection, MM_SEND_FE_POST_REQ, &postRequest);
}

//////////////////////////////////////////////////////////////////////
// Request a "post list" - list of commanders with vehicles reserved
// for multicrewing
// CAVEATS:
//  - Facility is redundant since 1.19
SINT32
mmSendPostListRequest(SINT32 connection, const FULL_OBJ_ID& facility)
{
	FE_POST_LIST_REQUEST postListRequest;

	postListRequest.facility = facility;

	return teulSend(connection, MM_SEND_FE_POST_LIST_REQ, &postListRequest);
}

//this function sends the join request to the mmhost
SINT32
mmSendJoinRequest(SINT32 connection, const WEAPON_SERIAL& weaponSerial)
{
	FE_JOIN_REQUEST joinRequest;

	joinRequest.weaponSerial = weaponSerial;

	return teulSend(connection, MM_SEND_FE_JOIN_REQ, &joinRequest);
}

//this function sends the request for crew to the mmhost
SINT32
mmSendRequestCrew(SINT32 connection, const WEAPON_SERIAL& weaponSerial, UINT32 numCrewNeeded)
{
	FE_CREW_REQUEST crewRequest;

	crewRequest.weaponSerial = weaponSerial;
	crewRequest.numCrewNeeded = numCrewNeeded;

	return teulSend(connection, MM_SEND_FE_CREW_REQ, &crewRequest);
}

//this function sends the join request to the mmhost
SINT32
mmSendAcceptCrew(SINT32 connection, UINT32 joinRequestID)
{
	FE_CREW_ACCEPT joinPermResponse;

	joinPermResponse.joinRequestID = joinRequestID;

	return teulSend(connection, MM_SEND_FE_CREW_ACCEPT, &joinPermResponse);
}

//this function sends a kill for a join request
SINT32
mmSendJoinPermKill(SINT32 connection, UINT32 joinRequestID)
{
	FE_JOIN_PERM_KILL killJoin;

	killJoin.joinRequestID = joinRequestID;
	return teulSend(connection, MM_SEND_FE_JOIN_PERM_KILL, &killJoin);
}

//this function sends a cancel for a post, join etc
SINT32
mmSendVehicleCancel(SINT32 connection, const WEAPON_SERIAL& weaponSerial)
{
	FE_POST_CANCEL cancelPost;

	cancelPost.weaponSerial = weaponSerial;
	return teulSend(connection, MM_SEND_FE_POST_CANCEL, &cancelPost);
}

//this function sends a client spawn
SINT32
mmSendClientSpawn(SINT32 connection, const WEAPON_SERIAL& weaponSerial, LOGICAL wantMobileSpawn)
{
	FE_CLIENT_SPAWN clientSpawn;

	clientSpawn.weaponSerial = weaponSerial;
	clientSpawn.wantMobileSpawn = wantMobileSpawn;

	return teulSend(connection, MM_SEND_FE_CLIENT_SPAWN, &clientSpawn);
}

//send a request for a direct spawn of a system weapon for command line client
SINT32
mmSendDirectSpawn(SINT32 connection, const WEAPON_SPEC& weapon, const FLOAT64VECTOR* location)
{
	Map::DirectSpawn request;
	request.m_weapon = weapon;
	request.m_useLocation = (location) ? true: false;
	request.m_location = (location) ? *location: FLOAT64VECTOR();
	return teulSend(connection, _SEND_DIRECT_SPAWN, &request);
}

/*
void
MapHostConnection::doDespawned()
{
	teulSend(m_teulIndex, _SEND_DO_DESPAWN, this);
}
*/

void mmSendDespawn(Teul::Connection* conn)
{
	//uiChatPrintLineColor(colorORANGEALPHA, L"Sending client mm despawn!");
	teulSend(conn->m_teulIndex, _SEND_DO_DESPAWN, conn);
}

SINT32
mmClientInstallCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(MM::CallbackX)

		REGISTER_SENDER(MM::FE_POST_REQ_Y, _sendFEPostRequest, MM_SEND_FE_POST_REQ);
		REGISTER_RECEIVER(MM::FE_POST_ACCEPT_Y, _recvFEPostAccept);
		REGISTER_RECEIVER(MM::FE_POST_DENY_Y, _recvFEPostDeny);
		REGISTER_RECEIVER(MM::FE_POST_EXPIRED_Y, _recvFEPostExpired);
		REGISTER_SENDER(MM::FE_POST_LIST_REQ_Y, _sendFEPostListRequest, MM_SEND_FE_POST_LIST_REQ);
		REGISTER_RECEIVER(MM::FE_POST_LIST_RESPONSE_Y, _recvFEPostListResponse);
		REGISTER_SENDER(MM::FE_JOIN_REQ_Y, _sendFEJoinRequest, MM_SEND_FE_JOIN_REQ);
		REGISTER_RECEIVER(MM::FE_JOIN_ACCEPT_Y, _recvFEJoinAccept);
		REGISTER_RECEIVER(MM::FE_JOIN_DENY_Y, _recvFEJoinDeny);
		REGISTER_RECEIVER(MM::FE_JOIN_PERM_REQUEST_Y, _recvFEJoinPermRequest);
		REGISTER_SENDER(MM::FE_CREW_ACCEPT_Y, _sendFECrewAccept, MM_SEND_FE_CREW_ACCEPT);
		REGISTER_SENDER(MM::FE_CREW_REQUEST_Y, _sendFECrewRequest, MM_SEND_FE_CREW_REQ);
		REGISTER_SENDER(MM::FE_JOIN_PERM_KILL_Y, _sendFEJoinPermKill, MM_SEND_FE_JOIN_PERM_KILL);
		REGISTER_SENDER(MM::FE_POST_CANCEL_Y, _sendFEPostCancel, MM_SEND_FE_POST_CANCEL);
		REGISTER_SENDER(MM::FE_CLIENT_SPAWN_Y, _sendFEClientSpawn, MM_SEND_FE_CLIENT_SPAWN);
		REGISTER_RECEIVER(MM::FE_SPAWN_NOTIFY_Y, _recvFESpawnNotify);
		REGISTER_RECEIVER(MM::FE_CREW_BAILED_Y, _recvFECrewBailed);
		REGISTER_SENDER(MM::DIRECT_SPAWN_Y, _sendDirectSpawn, _SEND_DIRECT_SPAWN);
		REGISTER_SENDER_NODATA(MM::DO_DESPAWN_Y, _SEND_DO_DESPAWN);

	END_CALLBACK_REGISTRATION
}

