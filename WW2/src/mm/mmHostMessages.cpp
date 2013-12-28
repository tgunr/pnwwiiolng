#include <syscfg.h> // Path is project specific.
// Date: Wed Apr 18 14:46:35 2001
//
// Host side message handling for the mmhost.  Specifically this module
// contains all the spawn message handling
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "comm/commConstants.h"
#include "host/hostVehicleRequest.h"
#include "inst/instTypes.h"
#include "mm/mmConstants.h"
#include "mm/mmDirectSpawn.h"
#include "mm/mmHostMessages.h"
#include "mm/mmMain.h"
#include "mm/mmPack.h"
#include "mm/mmSpawn.h"
#include "mm/mmSpawnTypes.h"
#include "server/serverIndexes.h"
#include "server/serverServerInfo.h"
#include "strt/strtEvents.h"
#include "teul/teulBasic.h"
#include "teul/teulServer.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPack.h"

#include "score/scorePacket.h"
#include "score/scoreConstants.h"
namespace Scoring { extern SINT32 InstallCallbacks(); }

#include "include/logging.h"

static TEUL_CALLBACK* MM_SEND_FE_POST_ACCEPT;
static TEUL_CALLBACK* MM_SEND_FE_POST_DENY;
static TEUL_CALLBACK* MM_SEND_FE_POST_EXPIRED;
static TEUL_CALLBACK* MM_SEND_FE_POST_LIST_RESPONSE;
static TEUL_CALLBACK* MM_SEND_FE_JOIN_ACCEPT;
static TEUL_CALLBACK* MM_SEND_FE_JOIN_DENY;
static TEUL_CALLBACK* MM_SEND_FE_JOIN_PERM_REQUEST;
static TEUL_CALLBACK* MM_SEND_FE_SPAWN_NOTIFY;
static TEUL_CALLBACK* MM_SEND_FE_CREW_BAILED;
TEUL_CALLBACK* Scoring::xchgEventCallback;
TEUL_CALLBACK* Host::VehicleRequest::s_callback;

//Sends a post Accept the front end on the connection
SINT32
mmSendPostAccept(SINT32 connection, WEAPON_SERIAL weaponSerial, LOGICAL newVehicle, SINT16 portal, bool mobileSpawnAvailable)
{
	FE_POST_ACCEPT postAccept;

	postAccept.weaponSerial = weaponSerial;
	postAccept.portal = portal;
	postAccept.newVehicle = newVehicle;
	postAccept.mobileSpawnAvailable = mobileSpawnAvailable;

	return teulSend(connection, MM_SEND_FE_POST_ACCEPT, &postAccept);
}

//sends a post deny to the front end on the connection
SINT32 mmSendPostDeny(SINT32 connection, WEAPON_SPEC weaponSpec, UINT32 reasonCode, bool loseWeapon)
{
	FE_POST_DENY postDeny;

	postDeny.weapon = weaponSpec;
	postDeny.reasonCode = reasonCode;
	postDeny.loseWeapon = loseWeapon;

	return teulSend(connection, MM_SEND_FE_POST_DENY, &postDeny);
}

//Sends a post Expired the front end on the connection
SINT32
mmSendPostExpired(SINT32 connection, WEAPON_SERIAL weaponSerial)
{
	FE_POST_EXPIRED postExpired;

	postExpired.weaponSerial = weaponSerial;

	return teulSend(connection, MM_SEND_FE_POST_EXPIRED, &postExpired);
}

//Sends a post Expired the front end on the connection
SINT32
mmSendPostListResponse(SINT32 connection, FULL_OBJ_ID facility, SINT16 numWeapons, COMMANDED_WEAPON* commandedWeapon)
{
	FE_POST_LIST_RESPONSE postListResponse;
	postListResponse.facility = facility;
	postListResponse.numWeapons = numWeapons;
	postListResponse.commandedWeapon = commandedWeapon;

	return teulSend(connection, MM_SEND_FE_POST_LIST_RESPONSE, &postListResponse);
}

//Sends a Join accept to a front end
SINT32
mmSendJoinAccept(SINT32 connection, MM_POSTED_WEAPON postedWeapon)
{
	FE_JOIN_ACCEPT joinAccept;

	joinAccept.postedWeapon = postedWeapon;

	return teulSend(connection, MM_SEND_FE_JOIN_ACCEPT, &joinAccept);
}

//Sends a Join Deny to a front end
SINT32
mmSendJoinDeny(SINT32 connection, UINT32 reasonCode)
{
FE_JOIN_DENY joinDeny;

	joinDeny.reasonCode = reasonCode;

	return teulSend(connection, MM_SEND_FE_JOIN_DENY, &joinDeny);
}

//Sends a Join Permission Request to an FE
SINT32
mmSendJoinPermRequest(SINT32 connection, UINT32 joinRequestID, const SCHAR* const callsign)
{
	FE_JOIN_PERM_REQUEST joinPermRequest;

	joinPermRequest.joinRequestID = joinRequestID;
	joinPermRequest.chatHandle = chatGetHandle(callsign);

	return teulSend(connection, MM_SEND_FE_JOIN_PERM_REQUEST, &joinPermRequest);
}

//Sends an FE Spawn Notify
extern SINT32
mmSendSpawnNotify(
		SINT32 connection
		, const WEAPON_SERIAL& weaponSerial
		, sortieid_t spawnVehicleID, FLOAT64 x, FLOAT64 y, FLOAT64 z, UINT8 decal
		, bool isHighCommander, bool isBuilder, bool isMissionLeader			// Important to get these the right way around.
)
{
	FESpawnNotify message(weaponSerial, spawnVehicleID, x, y, z, decal, isHighCommander, isBuilder, isMissionLeader);

	dbugLog(_FLD, "t:%d sending spawn notify ms:%d@%.03g.%.03g.%.03g decal:%d build:%d hc:%d ml:%d"
					, connection
					, spawnVehicleID, x, y, z
					, decal
					, isBuilder
					, isHighCommander
					, isMissionLeader
					);

	return teulSend(connection, MM_SEND_FE_SPAWN_NOTIFY, &message);
}

//Sends an FE Crew Bailed
SINT32
mmSendCrewBailed(SINT32 connection, UINT32 joinRequestID, UINT32 reasonCode)
{
	FE_CREW_BAILED crewBailed;

	crewBailed.reasonCode = reasonCode;
	crewBailed. joinRequestID = joinRequestID;

	return teulSend(connection, MM_SEND_FE_CREW_BAILED, &crewBailed);
}

void _mmRecvError(SINT32 teulIndex, UCHAR* const data, SINT32 size, TEUL_CALLBACK* object, SINT32 error)
{
	dbugLog(_FLA,"recv error %d on teulIndex %d",error, teulIndex);
}

void _mmSendError(SINT32 teulIndex, UCHAR* const data, SINT32 size, TEUL_CALLBACK* object, SINT32 error)
{
	dbugLog(_FLA,"send error %d on teulIndex %d",error, teulIndex);
}

static SINT32 _sendFEPostAccept(ACTION_OBJECT* const a)
{
	return mmPackFEPostAccept(TEUL_PACK, a->packed, a->offset, (FE_POST_ACCEPT*)a->localRawData, 0);
}

static SINT32 _sendFEPostDeny(ACTION_OBJECT* const a)
{
	return mmPackFEPostDeny(TEUL_PACK, a->packed, a->offset, (FE_POST_DENY*)a->localRawData, 0);
}

static SINT32 _sendFEPostExpired(ACTION_OBJECT* const a)
{
	return mmPackFEPostExpired(TEUL_PACK, a->packed, a->offset, (FE_POST_EXPIRED*)a->localRawData, 0);
}

static SINT32 _sendFEPostListResponse(ACTION_OBJECT* const a)
{
	return mmPackFEPostListResponse(TEUL_PACK, a->packed, a->offset, (FE_POST_LIST_RESPONSE*)a->localRawData, 0);
}

static SINT32 _sendFEJoinAccept(ACTION_OBJECT* const a)
{
	return mmPackFEJoinAccept(TEUL_PACK, a->packed, a->offset, (FE_JOIN_ACCEPT*)a->localRawData, 0);
}

static SINT32 _sendFEJoinDeny(ACTION_OBJECT* const a)
{
	return mmPackFEJoinDeny(TEUL_PACK, a->packed, a->offset, (FE_JOIN_DENY*)a->localRawData, 0);
}

static SINT32 _sendFEJoinPermRequest(ACTION_OBJECT* const a)
{
	return mmPackFEJoinPermRequest(TEUL_PACK, a->packed, a->offset, (FE_JOIN_PERM_REQUEST*)a->localRawData, 0);
}

static SINT32 _sendFESpawnNotify(ACTION_OBJECT* const a)
{
	FESpawnNotify* spawnNotify = (FESpawnNotify*)a->localRawData;
	return spawnNotify->packer(TEUL_PACK, a);
}

static SINT32 _sendFECrewBailed(ACTION_OBJECT* const a)
{
	return mmPackFECrewBailed(TEUL_PACK, a->packed, a->offset, (FE_CREW_BAILED*)a->localRawData, 0);
}

static SINT32 _recvFEPostRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_POST_REQUEST postRequest;

	a->offset = mmPackFEPostRequest(TEUL_UNPACK, a->packed, a->offset, &postRequest, 0);

#ifdef DEBUG
	dbugLog(_FLD,"postRequest: %d:%d:%d:%d:%d %d:%d:%d:%d", postRequest.facility.cell.ox, postRequest.facility.cell.oy, postRequest.facility.id, postRequest.facility.objClass, postRequest.facility.objType, postRequest.weapon.wCountry, postRequest.weapon.wCategory, postRequest.weapon.wClass, postRequest.weapon.wType);
#endif

	mmProcessFEPostRequest(player, postRequest);

	return a->offset;
}

static SINT32
_recvFEPostListRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_POST_LIST_REQUEST postListRequest;

	a->offset = mmPackFEPostListRequest(TEUL_UNPACK, a->packed, a->offset, &postListRequest, 0);

#ifdef DEBUG
	dbugLog(_FLD,"postListRequest: %d:%d:%d:%d:%d", postListRequest.facility.cell.ox, postListRequest.facility.cell.oy, postListRequest.facility.id, postListRequest.facility.objClass, postListRequest.facility.objType);
#endif

	mmProcessFEPostListRequest(player, postListRequest);

	return a->offset;
}

static SINT32 _recvFEJoinRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_JOIN_REQUEST joinRequest;

	a->offset = mmPackFEJoinRequest(TEUL_UNPACK, a->packed, a->offset, &joinRequest, 0);

	mmProcessFEJoinRequest(player, joinRequest);

	return a->offset;
}

static SINT32 _recvFECrewAccept(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_CREW_ACCEPT joinPermResponse;

	a->offset = mmPackFECrewAccept(TEUL_UNPACK, a->packed, a->offset, &joinPermResponse, 0);

	mmProcessFECrewAccept(player, joinPermResponse);

	return a->offset;
}

static SINT32 _recvFECrewRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_CREW_REQUEST crewRequest;

	a->offset = mmPackFECrewRequest(TEUL_UNPACK, a->packed, a->offset, &crewRequest, 0);

	mmProcessFECrewRequest(player, crewRequest);

	return a->offset;
}


static SINT32 _recvFEJoinPermKill(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_JOIN_PERM_KILL joinPermKill;

	a->offset = mmPackFEJoinPermKill(TEUL_UNPACK, a->packed, a->offset, &joinPermKill, 0);

	mmProcessFEJoinPermKill(player, joinPermKill);

	return a->offset;
}

static SINT32 _recvFEPostCancel(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	BETAInfo(a->teulIndex);
	FE_POST_CANCEL postCancel;

	a->offset = mmPackFEPostCancel(TEUL_UNPACK, a->packed, a->offset, &postCancel, 0);

	mmProcessFEPostCancel(player, postCancel);

	return a->offset;
}

static SINT32
_recvFEClientSpawn(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	FE_CLIENT_SPAWN clientSpawn;

	a->offset = mmPackFEClientSpawn(TEUL_UNPACK, a->packed, a->offset, &clientSpawn, 0);

	BETAInfo(a->teulIndex);

	mmProcessFEClientSpawn(player, clientSpawn);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a direct spawn request

static SINT32
_recvDirectSpawn(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	Map::DirectSpawn request;
	a->offset = request.packer(TEUL_UNPACK, a);

	if ( request.m_weapon.wCountry == 0 || request.m_weapon.wType == 0 )
	{
		player.log("direct spawn request with invalid looking weapon spec");
		return -EINVAL;
	}

#ifdef PUBLIC_RELEASE
	if ( !player.isPlaynet() && player.instance().isCampaign() )
	{
		player.log(_FLA, "non-playnet persona requesting direct spawn in campaign instance");
		return false;
	}
#endif

	mmDirectSpawn(player, request.m_weapon, true, request.m_useLocation, request.m_location);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a message indicating a player is returning.

static SINT32
_recvDoDespawn(struct ACTION_OBJECT* const a)
{
	Player* const player = static_cast<Player*>(a->m_client);
	if ( player == NULL )
	{
		dbugLog(_FLW, "t:%d login return on index with no player associated", a->teulIndex);
		return -Teul::Errors::LoginFail_DespawnNoPlayer;
	}

	const auto cellIndex = serverGetPeerIndex(player->m_lastSpawnedTo, CELL_HOST);
	if ( cellIndex >= 0 )
	{
		Teul::Packet outgoing(0, Comm::CallbackX, DESPAWNED_Y);
		outgoing.setUnencrypted();
		outgoing.append(player->playerID());
		outgoing.send(cellIndex);
	}

	if ( !player->onDespawn(a->teulIndex) )
	{
		return -Teul::Errors::LoginFail_DespawnError;
	}

	return a->offset;
}

static SINT32
_registerStratCallback(void)
{
	BEGIN_CALLBACK_REGISTRATION(Comm::CallbackX)
		REGISTER_SENDER(STRAT_EVENTS_Y, Strat::Events::SendFunction, Strat::Events::Callback);
	END_CALLBACK_REGISTRATION
}

//////////////////////////////////////////////////////////////////////
// Send a vehicle request packet.

void
Host::VehicleRequest::send(const Server::ServerInfo* server)
{
	const SINT32 cellIndex = serverGetPeerIndex(server->stats.address, CELL_HOST);
	if ( cellIndex < 0 )
	{
		dbugLog(_FLD, "unable to send vehicle request: cell %s not peering", teulGetAddress(server->stats.address));
		return;
	}

	Teul::Packet outgoing(sizeof(*this), MMHost::CallbackX, MMHost::WEAPONS_HANDOFF_Y);
	outgoing.setUnencrypted();
	outgoing.append(this, sizeof(*this));
	const SINT32 result = outgoing.send(cellIndex);
	dbugLog(_FLD, "sending vehicle request to t:%d a:%u returned:%d", cellIndex, server->stats.address & DIRECT_MASK, result);
}

//Installs the host callbacks
SINT32
mmHostInstallCallbacks(void)
{
	_registerStratCallback();

	Scoring::InstallCallbacks();

	BEGIN_CALLBACK_REGISTRATION(MMHost::CallbackX)

		REGISTER_RECEIVER(MMHost::FE_POST_REQ_Y, _recvFEPostRequest);
		REGISTER_SENDER(MMHost::FE_POST_ACCEPT_Y, _sendFEPostAccept, MM_SEND_FE_POST_ACCEPT);
		REGISTER_SENDER(MMHost::FE_POST_DENY_Y, _sendFEPostDeny, MM_SEND_FE_POST_DENY);
		REGISTER_SENDER(MMHost::FE_POST_EXPIRED_Y, _sendFEPostExpired, MM_SEND_FE_POST_EXPIRED);
		REGISTER_RECEIVER(MMHost::FE_POST_LIST_REQ_Y, _recvFEPostListRequest);
		REGISTER_SENDER(MMHost::FE_POST_LIST_RESPONSE_Y, _sendFEPostListResponse, MM_SEND_FE_POST_LIST_RESPONSE);
		REGISTER_RECEIVER(MMHost::FE_JOIN_REQ_Y, _recvFEJoinRequest);
		REGISTER_SENDER(MMHost::FE_JOIN_ACCEPT_Y, _sendFEJoinAccept, MM_SEND_FE_JOIN_ACCEPT);
		REGISTER_SENDER(MMHost::FE_JOIN_DENY_Y, _sendFEJoinDeny, MM_SEND_FE_JOIN_DENY);
		REGISTER_SENDER(MMHost::FE_JOIN_PERM_REQUEST_Y, _sendFEJoinPermRequest, MM_SEND_FE_JOIN_PERM_REQUEST);
		REGISTER_RECEIVER(MMHost::FE_CREW_ACCEPT_Y, _recvFECrewAccept);
		REGISTER_RECEIVER(MMHost::FE_CREW_REQUEST_Y, _recvFECrewRequest);
		REGISTER_RECEIVER(MMHost::FE_JOIN_PERM_KILL_Y, _recvFEJoinPermKill);
		REGISTER_RECEIVER(MMHost::FE_POST_CANCEL_Y, _recvFEPostCancel);
		REGISTER_RECEIVER(MMHost::FE_CLIENT_SPAWN_Y, _recvFEClientSpawn);
		REGISTER_SENDER(MMHost::FE_SPAWN_NOTIFY_Y, _sendFESpawnNotify, MM_SEND_FE_SPAWN_NOTIFY);
		REGISTER_SENDER(MMHost::FE_CREW_BAILED_Y, _sendFECrewBailed, MM_SEND_FE_CREW_BAILED);
		REGISTER_RECEIVER(MMHost::DIRECT_SPAWN_Y, _recvDirectSpawn);
		REGISTER_RECEIVER(MMHost::DO_DESPAWN_Y, _recvDoDespawn);

	END_CALLBACK_REGISTRATION
}

