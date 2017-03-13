#include <syscfg.h> // Path is project specific.
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "include/clustercfg.h"

#include "chat/chatCellHostSupportMessages.h"
#include "chat/chatCellTypes.h"
#include "chat/chatConstants.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatName.h"
#include "chat/chatServerTypes.h"
#include "chat/chatSupportRPCMessages.h"
#include "comm/commConstants.h"
#include "comm/commHostMessages.h"
#include "comm/commPackers.h"
#include "host/hostConstants.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostStats.h"
#include "host/hostVehicle.h"
#include "include/arenaLockEnums.h"
#include "server/serverInterface.h"
#include "server/serverPlayerStatusNode.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulPacket.h"
#include "updt/loginTypes.h"
#include "updt/updtInfDeath.h"
#include "updt/updtPackers.h"
#include "updt/updtVehicleHit.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiThreads.h"

// WWIIOL specific.
#include "host/hostConstants.h"
#include "host/hostResupplyIssue.h"
#include "host/hostTimeUpdate.h"
#include "include/damg.h"
#include "include/wthrTime.h"
#include "score/scoreHit.h"
#include "score/scorePacket.h"
#include "server/serverServerInfo.h"
#include "strt/strtCaptureScore.h"
#include "strt/strtCaptureState.h"
#include "strt/strtEvents.h"
#include "strt/strtUpdaterSupportMessages.h"
#include "teul/teulEndpoint.h"
#include "updt/updtDeathByBuilding.h"
#include "wwii/wwiiFullObjID.h"

#define DEATH_TIMEOUT 30			// A minute

TEUL_CALLBACK*		commHOST_KILL;

static TEUL_CALLBACK*		_GRANT_VEHICLE_JOIN;


//static TEUL_CALLBACK*		_CLIENT_KILL;

static TEUL_CALLBACK*		_CLIENT_HIT;
static TEUL_CALLBACK*		_ACTIVATE_CLIENT;
static TEUL_CALLBACK*		_PLAYER_EXIT;
static TEUL_CALLBACK*		_XFER_RESUPPLY_ISSUE;
static TEUL_CALLBACK*		_SEND_MOVE;

static TEUL_CALLBACK*		_SEND_CAPTURE_STATE;

TEUL_CALLBACK*		Host::TimeUpdate::Callback = nullptr;

UCHAR commCookieString[] = AUTH_SEQ;

static void _expireTurretBits(struct TURRET_STRUCT* turret);
static void _expireFireBits(struct FIRE_STRUCT* fire, UINT16 fireMask);

static PLAYER_EXIT _playerExit;

#ifdef BETA_RELEASE
static bool g_enablePacketSizeRecording = true;
#else
static bool g_enablePacketSizeRecording = false;
#endif

//////////////////////////////////////////////////////////////////////
//

SINT32
commSendPlayerExit(SINT32 teulIndex, PLAYER_DESTINATION destination, PLAYER_EXIT_CODE exitCode, const FULL_OBJ_ID facility)
{
	dbugLog(_FLN, "t:%d sending player exit %d dest %d to " PRF_FULL_OBJ_ID ""
				, teulIndex
				, destination
				, exitCode
				, PRA_FULL_OBJ_ID(facility)
				);

	_playerExit.destination = destination;
	_playerExit.exitCode = exitCode;
	_playerExit.facility = facility;

	return teulSend(teulIndex, _PLAYER_EXIT, &_playerExit);
}

//////////////////////////////////////////////////////////////////////
//

static void
_badPlayer(Player& player, const SCHAR* problem, bool alert)
{
	const SCHAR* callsign = "*unknown*";

	// Find out who we are dealing with
	PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(player.playerID());
	if ( psn != nullptr )
		callsign = psn->dispatch().m_callsign;

	dbugLog(_FLW, "t:%d p:%u c:%s *** Kicking: %s", player.teulIndex(), player.playerID(), callsign, problem);

	if ( alert )
		chatSendKickNotice(player.playerID(), callsign, problem);

	commSendPlayerExit(player.teulIndex(), PDEST_EXIT, PEXIT_DEAD, nullObjID);
}

//////////////////////////////////////////////////////////////////////
//

void
commForcePlayerExit(Player& player, const char* reason, bool alert)
{
	if ( player.m_timeToLive == 0 )
	{
		_badPlayer(player, reason, alert);
		player.m_timeToLive = 100 + (rand() % 100);
	}
}

//////////////////////////////////////////////////////////////////////
//

void
commDropPlayer(Player& player, const bool ignoredPriorKick)
{
	if ( ignoredPriorKick )
		_badPlayer(player, "*ignored prior kick message", true);

	_playerExit.destination = PDEST_EXIT;
	_playerExit.exitCode = PEXIT_DEAD;
	_playerExit.facility = nullObjID;

	hostPlayerLeave(player, _playerExit);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendClientVehicleHit(ACTION_OBJECT* const a)
{
	Vehicle::Hit* const hit = (Vehicle::Hit*)(a->localRawData);
	hit->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendClientActivate(ACTION_OBJECT* const a)
{
	dbugLog(_FLI, "send ClientActivate");
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendGrantVehicleJoin(ACTION_OBJECT* const a)
{
	return  commPackVEHICLE_REPLY(TEUL_PACK, a->packed, a->offset, (VEHICLE_REPLY*) a->localRawData, sizeof(VEHICLE_REPLY));
}

//////////////////////////////////////////////////////////////////////
//

static void
_expireFireBits(FIRE_STRUCT* const fire, UINT16 fireMask)
{
	UINT16 tmpMask, i;
	const TeulTimeMS bestBefore = teulClock64 - 2000;

	for (tmpMask = 1 , i = 0; i < MAX_GUNS; i++, tmpMask <<= 1)
	{
		if (tmpMask & fireMask)
		{
			// if we get a new on state
			fire->masterClock64 = fire->clock64[i] = teulClock64;
		}
		else if ( (fire->valid & tmpMask) && fire->clock64[i] <= bestBefore )
		{
			// if the fire times out clear the valid bit
			fireMask &= ~tmpMask;
			fire->valid &= ~tmpMask;
			fire->masterClock64 = teulClock64;
		}
	}

	// state allways == current rx fire mask
	fire->state = fireMask;
	// any on state is allways reflected as valid
	fire->valid |= fireMask;
}

//////////////////////////////////////////////////////////////////////
//

static void
_expireTurretBits(struct TURRET_STRUCT* turret)
{
	UINT32 mask, i;
	const TeulTimeMS bestBefore = teulClock64 - 3000;

	for ( mask = 1, i = 0; i < MAX_TURRETS; i++, mask <<= 1 )
	{
		if ( (turret->valid & mask) && turret->clock64[i] <= bestBefore )
		{
#ifdef NEVER
			dbugLog(_FLD,"turret invalid: %x mask %x",turret->valid, tmpMask);
			dbugLog(_FLD,"turret invalid: %d, stamp %" PR64BitUT " now %" PR64BitUT "",i, turret->timeStamp[i], teulClock64);
#endif
			turret->valid &= ~mask;
			turret->clock64[i] = teulClock64;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//

SINT32
commRecvVehicleUpdate(UINT32 serverAddress, const UCHAR* const buffer, SINT32 offset)
{
	if ( Server::myServerInfo.stats.status == Server::SERVER_LOGIN )
	{
		dbugLog(_FLD, "ignoring broadcastupdate during SERVER_LOGIN");
		return -1;
	}

	static TeulTimeMS nextBlip = teulClock64 + 10000;
	static UINT32 tallyUpdates = 0;
	++tallyUpdates;
	if ( teulClock64 >= nextBlip )
	{
		if ( tallyUpdates > 0 )
			dbugLog(_FLI, "vehicle broadcasts received: %u", tallyUpdates);
		tallyUpdates = 0;
		nextBlip = teulClock64 + 10000;
	}

	static VEHICLE_HOLDER vh;
	vh.vehicle = nullptr;
	offset = updtPackVehicleBroadcast(TEUL_UNPACK, const_cast<UCHAR*>(buffer), offset, &vh, 0);

	if (!vh.vehicle)
	{
		dbugLog(_FLW, "Received a vehicle broadcast but no vehicles could be decoded");
		return -1;
	}

	if ( offset < 0 )
	{
		dbugLog(_FLA, "Unable to unpack vehicle broadcast retval %d", offset);
		return -1;
	}

	vh.vehicle->serverAddress = serverAddress;

	_expireFireBits( &(vh.vehicle->vehicleData.fire), vh.fireMask );
	_expireTurretBits( &(vh.vehicle->vehicleData.turret) );

	if ( vh.vehicle->isActive() == false )
		hostActivateVehicle(vh.vehicle, false);
	else
		vh.vehicle->updateGrid();

	vh.vehicle->lastRX = teulClock64;

	return offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendPlayerExit(ACTION_OBJECT* const a)
{
	PLAYER_EXIT* const pe = (PLAYER_EXIT*) a->localRawData;

	dbugLog(_FLI, "t:%d sending player exit %d dest %d", a->teulIndex, pe->destination, pe->exitCode);

	return commPackPlayerExit(TEUL_PACK, a->packed, a->offset, pe, 0);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvPlayerExit(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	PLAYER_EXIT pe;
	memset(&pe, 0, sizeof(pe));

	a->offset = commPackPlayerExit(TEUL_UNPACK, a->packed, a->offset, &pe, 0);

	dbugLog(_FLD, "t:%d code:%u received player exit destination %d", a->teulIndex, pe.exitCode, pe.destination);

	hostPlayerLeave(player, pe);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvVehicleJoinRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	if ( player.m_hasExited )
	{
		teulClose(a->teulIndex, Teul::Closing::VJRExited);
		return 0;
	}

	VEHICLE_REQUEST localVehicleRequest;
	a->offset = commPackVEHICLE_REQUEST(TEUL_UNPACK, a->packed, a->offset, &localVehicleRequest, 0);

	dbugLog(_FLD, "t:%d p:%u v:%u st:%d joinReq %s " PRF_WEAPON_SPEC ""
				, a->teulIndex
				, player.playerID()
				, localVehicleRequest.weapon.serial
				, localVehicleRequest.station
				, weapPrintName(localVehicleRequest.weapon.spec)
				, PRA_WEAPON_SPEC(localVehicleRequest.weapon.spec)
				);

	const SINT32 vIndex = hostJoinVehicle(player, localVehicleRequest);
	if ( (vIndex < 0) | (player.vehicle == nullptr) )
	{
		dbugLog(_FLW, "t:%d p:%u v:%i FAILED TO JOIN VEHICLE"
						, a->teulIndex
						, player.playerID()
						, localVehicleRequest.weapon.serial
						);
	}
	else
	{
		HostVehicle* const vehicle = player.vehicle;
		VEHICLE_REPLY vReply;

		vReply.hostIndex = vehicle->index;
		vReply.spec = vehicle->spec();
		vReply.source = vehicle->serial().source;
	  	vReply.station = player.station;

		// index of this vehicle on this persons
	 	vReply.persVehicleIndex = (UCHAR) vIndex;

	 	vReply.rankLevel = player.effectiveRank();

	 	// only send positive replies the FE cant handle it otherwise
		const SINT32 retval = teulSend(a->teulIndex, _GRANT_VEHICLE_JOIN, &vReply);
		if ( retval >= 0 )
		{
			dbugLog(_FLD, "t:%d p:%u v:%i st:%d joined successfully"
							, a->teulIndex
							, player.playerID()
							, vReply.hostIndex
	  						, vReply.station
							);
			return a->offset;
		}

		dbugLog(_FLA, "t:%d p:%u v:%i couldn't send _GRANT_VEHICLE_JOIN"
					, a->teulIndex
					, player.playerID()
					, vReply.hostIndex
					);
	}

	commSendPlayerExit(a->teulIndex, PDEST_MAP, PEXIT_DEAD, nullObjID);

	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
//

static void
_recordPacketSize(UINT32 size)
{
	static UINT32 maxSize = 0;
	static UINT32 minSize = 0;
	using SIZES = std::map<SINT32,UINT32>;
	static SIZES sizes;
	static UINT32 packets = 0;
	static UINT64 totalBits = 0LL;

	if ( size > maxSize )
		maxSize = size;
	if ( minSize == 0 || size < minSize )
		minSize = size;

	totalBits += size;

	++sizes[size];
	++packets;

	static UINT32 nextLog = teulTime() + 5;
	if ( (packets & 4095) != 0 || teulTime() < nextLog )
		return;

	UINT32 mean = 0;
	for ( auto it = sizes.begin(), end = sizes.end(); it != end; ++it )
	{
		if ( mean == 0 || it->second >= sizes[mean] )
			mean = it->first;
	}

	UINT32 avg = (UINT32)(totalBits / packets);

	dbugLog(_FLI, "rps[%u] bits minSize=%u avgSize=%u meanSize=%u maxSize=%u totalBits=%" PR64BitUT ""
				, packets, minSize, avg, mean, maxSize, totalBits);

	nextLog = teulTime() + 30;
}

//////////////////////////////////////////////////////////////////////
// Check to see if I should appear on AWS

void
Player::awsCheck()
{
	if ( m_onAWS == true )
	{
		// If the vehicle stops moving for 5 seconds,
		if ( vehicle->isMoving() == false )
		{
#ifdef BETA_RELEASE
			message(LSTR_BETA, LSTR_GENERIC, "Off AWS");
#endif
			m_onAWS = false;
			m_awsTrigger = 0;
		}
		return;
	}

	// Once you start moving, we mark you for AWS but you are still inside
	// the grace period.
	if ( m_awsTrigger > 0 )
	{
		if ( vehicle->isMoving() == false )
		{
#ifdef BETA_RELEASE
			message(LSTR_BETA, LSTR_GENERIC, "AWS Cancelled");
#endif
			m_awsTrigger = 0;
			return;
		}

		if ( m_awsTrigger <= teulTime() )
		{
#ifdef BETA_RELEASE
			message(LSTR_BETA, LSTR_GENERIC, "On AWS");
#endif
			m_onAWS = true;
			vehicle->m_nextLocUpdate = 1ULL;
		}
		return;
	}

	if ( teulTime() < m_nextVelocityCheck )
		return;

	if ( vehicle->isReallyActive() == false )
		return;

	if ( vehicle->exitState != PEXIT_UNINITIALIZED || exitState != PEXIT_UNINITIALIZED )
		return;

	if ( station != UPDT_LONE && station != UPDT_CONTROL )
		return;

	const FLOAT32 vX = (FLOAT32)vehicle->vehicleData.velocity.velX / 50.0f;
	const FLOAT32 vY = (FLOAT32)vehicle->vehicleData.velocity.velY / 50.0f;
	const FLOAT32 vZ = (FLOAT32)vehicle->vehicleData.velocity.velZ / 50.0f;
	const FLOAT32 vel = (vX * vX) + (vY * vY) + (vZ + vZ);

	static const FLOAT32 MPH_TO_MS = 0.44704f;
	static const FLOAT32 DESIRED_TRIP_SPEED_MS = 75 * MPH_TO_MS;
	static const FLOAT32 VEL_SQUARE_TRIP = DESIRED_TRIP_SPEED_MS * DESIRED_TRIP_SPEED_MS;

	if ( vel < VEL_SQUARE_TRIP )
	{
		m_nextVelocityCheck = teulTime() + 1;
		return;
	}

#ifdef BETA_RELEASE
	char vOver[80];
	vOver[0] = '\0';
	if ( vel >= VEL_SQUARE_TRIP )
		strprintf(vOver, "AWS Candidate (vel=%3.2fms/%3.2fMph)", sqrt(vel), sqrt(vel) / MPH_TO_MS);
	else
		strsafecpy(vOver, "UNEXPECTED AWS Trip!");
	message(LSTR_BETA, LSTR_GENERIC, vOver);
#endif

	m_awsTrigger = teulTime() + host_config.update.awsGracePeriod_s;
}

//////////////////////////////////////////////////////////////////////
// Receive a periodic update from game clients - heartbeat+.

SINT32
commRecvPeriodicUpdate(const SINT32 teulIndex, Player* const player, UCHAR* const message, const SINT32 startOffset, const bool isFromTeul)
{
	PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(player->playerID());
	if ( psn == nullptr)
	{
		dbugLog(_FLA,"t:%d _recvUpdateInfo: invalid player", teulIndex);
		return -EINVAL;
	}

	if ( teulIndex >= 0 && player->teulIndex() != teulIndex )
	{
		dbugLog(_FLA,"t:%d p:%u c:%s _recvUpdateInfo: player index mismatch (%d)"
						, teulIndex, player->playerID(), player->callsign(), player->teulIndex());
		return -EINVAL;
	}

	if ( player->m_hasExited )
	{
		dbugLog(_FLM, "t:%d p:%u c:%s received update from player post-exit (lagged data?)"
						, teulIndex, player->playerID(), player->callsign());
		return -EINVAL;
	}

	// We need someplace to extract the data.
	VEHICLE_UPDATE_HEADER vh;
	SHARED_VEHICLE_INFO	vehInfo;

	memset(&vh, 0, sizeof(vh));
	vh.player = player;
	vh.vehInfo = &vehInfo;

	const SINT32 offset = updtPackSHARED_VEHICLE_INFO(TEUL_UNPACK, message, startOffset, &vh, 0);
	if ( offset < 0 )
	{
		dbugLog(_FLA,"t:%d p:%u c:%s illegal return val from updtPackVehicleInfo %d"
						, player->teulIndex(), player->playerID(), player->callsign(), offset);
		return offset;
	}

	HostVehicle* const vref = hostFindVehicleLocal(vh.vehInfo->index);
	if (vref == nullptr)
	{
		dbugLog(_FLA,"t:%d p:%u c:%s v:%u_recvUpdateInfo: ERROR (3)"
					, player->teulIndex(), player->playerID(), player->callsign(), vehInfo.index);
		return -EINVAL;
	}

	if ( player->vehicle != vref )
	{
		dbugLog(_FLA,"t:%d p:%u c:%s v:%u _recvUpdateInfo: ERROR player=%p"
					, player->teulIndex(), player->playerID(), player->callsign(), vref->index
					, player);
		return offset;
	}
	else if ( player->station == UPDT_OTHER && (vh.present & PR_POSITION) )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s v:%u st:%u ILLEGAL: Positions info not allowed for other than control player"
						, player->teulIndex(), player->playerID(), player->callsign(), vref->index, player->station);
	}

	if ( g_enablePacketSizeRecording )
		_recordPacketSize((UINT32)(offset - startOffset));

	player->m_updatesSinceRX = 0;

	_expireFireBits( &(vh.vehInfo->fire), vh.fireMask);
#ifdef TEST_FIRING_BUG
		dbugLog(_FLI, "t:%d p:%u c:%s v:%u fm:%s fs:%s fv:%s ft:%" PR64BitUT ",%" PR64BitUT ",%" PR64BitUT ",%" PR64BitUT ""
						, player->teulIndex(), player->playerID(), player->dispatch().callsign, vref->index
						, _bits(vh.fireMask), _bits(vh.vehInfo->fire.state), _bits(vh.vehInfo->fire.valid)
						, vh.vehInfo->fire.clock64[0]
						, vh.vehInfo->fire.clock64[1]
						, vh.vehInfo->fire.clock64[2]
						, vh.vehInfo->fire.clock64[3]
						);
#endif
	_expireTurretBits( &(vh.vehInfo->turret) );

	// PR_DEAD is considered as an indication of 'Crew Dead',
	// which sets the vehicle to auto-despawn after a period of time.
	if ( ((vref->vehicleData.globalFlags & VEHG_DEAD) == 0) && (vh.present & PR_DEAD) )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s v:%i received dead from vehicle"
					, player->teulIndex(), player->playerID(), player->callsign(), vref->index);
		vref->setCrewDead();
	}
	else if ( (vref->timeOfDeath) && (vref->state == VEH_DEAD) && (teulTime() - vref->timeOfDeath >= DEATH_TIMEOUT) )
	{
		dbugLog(_FLW,"t:%d p:%u c:%s v:%i forceing exit of dead vehicle"
					, player->teulIndex(), player->playerID(), player->callsign(), vref->index);
		vref->state = VEH_EXIT;
		hostForceVehExit(vref);
	}

	// PR_KILLED is considered as an indication that the vehicle itself has
	// been critically damaged (killed). The first time this happens, we
	// immediately process the scoring for this.
	if ( (vref->vehicleData.globalFlags & VEHG_KILLED) == 0 && (vh.present & PR_KILLED) )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s v:%i received killed from vehicle"
					, player->teulIndex(), player->playerID(), player->callsign(), vref->index);
		vref->setVehicleKilled(0);
	}

	if (player->active == FALSE)
	{
		dbugLog(_FLI,"t:%d p:%u c:%s v:%u activating player"
					, player->teulIndex(), player->playerID(), player->callsign(), vref->index);
		hostActivatePlayer(player);
	}

	if ( vref->isActive() == false )
	{
		hostActivateVehicle(vref, true);
		vh.present = (PRESENT_FLAGS)(vh.present | PRX_FIRST_TIME);
	}
	else
	{
		// We've gone active, but have we been marked as really active yet?
		if ( vref->isReallyActive() == false && vref->shouldBeReallyActive() )
		{
			vref->setReallyActive(true);
			dbugLog(_FLD, "t:%d p:%u c:%s v:%u really activating vehicle"
						, player->teulIndex(), player->playerID(), player->callsign(), vref->index);
		}

		if ( player->station == UPDT_LONE || player->station == UPDT_CONTROL )
			vref->updateGrid();
	}

	vref->lastRX = teulClock64;

	// If the client sends us visBits from a trooper, it indicates that they are
	// trying to resupply someone; use the data and clear it.
	if ( (vh.present & PR_VEH_VIS) && vref->spec().wCategory == _INFANTRY && vref->vehicleData.visBits != 0 )
	{
		vref->issueResupply(player, vref->vehicleData.visBits);
		// Clear it out again
		vref->vehicleData.visBits = 0;
		vh.present &= ~PR_VEH_VIS;
	}

	// If the vehicle is marked "really" active and we're counting packets...
	if ( (host_config.arena.checksums || player->m_flags.canEject) && player->m_flags.checksumsDirty )
	{
		if ( player->m_timeToLive == 0 )
		{
			dbugLog(_FLW, "*** ILLEGAL CHECKSUM EJECTION for %s %d conn %d - KILLING", player->callsign(), player->playerID(), player->m_numConnects);
			commForcePlayerExit(*player, "Invalid chient", true);
		}
	}


	if ( player->station == UPDT_LONE || player->station == UPDT_CONTROL )
	{
		// Tell the despawn timer whether or not they are currently
		// requesting a despawn timer.
		bool wantTimerOn = (bool)(vh.present & PRX_DESPAWN_TIMER);
		vref->setDespawnTimer(wantTimerOn);

		player->changeFPSBuilding(vref->vehicleData.fpsBuildingID, vref->vehicleData.fpsInCaptureBuilding);
		if ( vref->spec().wCategory == _INFANTRY )
			vref->checkCapture();
	}

	if ( player->station == UPDT_OTHER )
		vh.present &= ~PR_FROM_CONTROLLER;
	else
	{
		vh.present |= PR_FROM_CONTROLLER;
	}

	hostAddVehicleToBroadcast(vref, &vh);

	player->processUpdateValues(vh.updateNo, vh.updatePrime);

	player->m_hasSentUpdate = isFromTeul;

	if ( player->instance().isCampaign() && vref->spec().wCategory == _AIRCRAFT )
	{
		player->awsCheck();
	}

	if ( teulPackUINT64(TEUL_UNPACK, message, offset, &player->m_lastTimeRx, 0) > offset )
		player->m_lastTimeRxed = teulClock64;

	return offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvUpdateInfo(ACTION_OBJECT* const a)
{
	Player* const player = (Player*)teulGetPersonaData(a->teulIndex);
	if ( player == nullptr)
	{
		dbugLog(_FLA,"t:%d _recvUpdateInfo: invalid player", a->teulIndex);
		commSendPlayerExit(a->teulIndex, PDEST_MAP, PEXIT_DEAD, nullObjID);
		teulClose(a->teulIndex, Teul::Closing::RUINoPersona);
		return 0;
	}

	return commRecvPeriodicUpdate(a->teulIndex, player, a->packed, a->offset, true);
}

//////////////////////////////////////////////////////////////////////
//

static bool
_forwardHitsToRemoteServer(const HostVehicle& vehicle, const SINT32 bits, const char* const source)
{
	const SINT32 cellIndex = serverGetPeerIndex(vehicle.serverAddress, CELL_HOST);
	if ( cellIndex < 0 )
		return false;

	const size_t bytes = Teul::BitsToBytes(bits);
	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, HOST_HIT_FWD_Y), source, bytes);
	outgoing.setUnencrypted();
	const SINT32 result = outgoing.send(cellIndex);
	if ( result < 0 )
	{
		dbugLog(_FLA, "v:%u failed sending apply hit to t:%d cell %x: %d", vehicle.index, cellIndex, vehicle.serverAddress, result);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Given the details of a hit packet, notify the scoring process.

static void
_sendScoringHitEvent(const HostVehicle& victim, const HostVehicle& shooter, const UINT32 numComponentsHit, const UINT64& joules)
{
	Scoring::Packet<Scoring::Hit> packet;
	Scoring::Actor actTarget(&victim);
	Scoring::Actor actShooter(&shooter);

	dbugLog(_FLD, "sending hit event [p:%u]->[p:%u] to map host", actShooter.playerID(), actTarget.playerID());

	Teul::Packet outgoing(Teul::PacketId(Scoring::CallbackX, Scoring::EVENT_Y));
	outgoing.construct<Scoring::Hit>(actTarget, actShooter, numComponentsHit, joules);
	outgoing.setUnencrypted();
	if ( !serverSendPacketToType(outgoing, MAP_HOST) )
		LogAbort("failed to send hit message");
}

//////////////////////////////////////////////////////////////////////
// If the area is limiting damage, then limit damage to 1 joule.

static void
_applyDamageRestrictions(Vehicle::Hit& hit, unsigned char* const source)
{
	// Check if there's anything to do.
	if ( host_config.arena.limitedDamage == 0 )
		return;

	// Limit to one component.
	hit.m_components.resize(1);

	// And limit that to 1 joule.
	hit.m_components.front().m_damage = 1;

	// Re-pack the damage.
	ACTION_OBJECT a(source, 0);
	hit.packer(TEUL_PACK, &a);
}

//////////////////////////////////////////////////////////////////////
// Apply an incoming damage packet, either from a player or forwarded
// via another cell host.

static bool
_processVehicleHit(Vehicle::Hit& hit, Player* const shooter, const SINT32 fromTeulIndex, unsigned char* const source, const SINT32 bits)
{
	// If KILLs are disabled ... start by disabling damage.
	if ( (host_config.arena.lock & AL_KILL) || hit.m_components.empty() )
	{
		/* IGNORE */
		return true;
	}

	HostVehicle* const vTarget = hostFindVehicleLocal(hit.m_target);
	if ( vTarget == nullptr)
	{
		dbugLog(_FLW, "cant apply hit, unknown m_target %u, may have already despawned",hit.m_target);
		return true;
	}

	//const bool isFromPlayer = (shooter != NULL);
	if ( hit.m_shooter == 0 )
	{
		// actually its always 0 right now but player should be non NULL
		LogAbort("received invalid hit message shooter is 0");
	}

	HostVehicle* const vShooter = hostFindVehicleLocal(hit.m_shooter);
	if ( vShooter == nullptr )
	{
		dbugLog(_FLW, "v:%u cant apply hit, unknown m_shooter %u, may have already despawned", hit.m_target, hit.m_shooter);
		return false;
	}

	if ( hit.m_components.size() > VEH_MAX_OBJ_HIT )
	{
		return false;
	}

	if ( vTarget->isRemote() == true )
	{
		vTarget->m_lastHitTime = teulClock64;

		teulPackUINT32(TEUL_PACK, (UCHAR*)source, 0, &hit.m_shooter, 0);

		return _forwardHitsToRemoteServer(*vTarget, bits, (const char*)source);
	}

	_applyDamageRestrictions(hit, source);

	VEH_HIT vHit;
	const size_t numComponents = hit.m_components.size();
	vHit.numComponents = numComponents;
	vHit.shooter = hit.m_shooter;
	vHit.target = hit.m_target;

	UINT64 totalDamageJ = 0LL;

	for ( size_t i = 0; i < numComponents; i++ )
	{
		const Vehicle::Hit::Component& comp = hit.m_components[i];
		vHit.compHit[i].compID = comp.compID;
		vHit.compHit[i].numHits = comp.NumHits;
		vHit.compHit[i].damage = comp.m_damage;
		vHit.compHit[i].priority = comp.isImportant;
		if ( comp.isImportant )
			totalDamageJ += comp.m_damage;
	}

	_sendScoringHitEvent(*vTarget, *vShooter, vHit.numComponents, totalDamageJ);

	// We're done processing the packet for ourselves, now we have to
	// send it on to the victim vehicle.
	const size_t hitBytes = Teul::BitsToBytes(bits);

	// Tread carefully with the iterators because teulSend might
	// delete the player.
	PLAYER_LIST::iterator it = vTarget->players().begin();
	while ( it != vTarget->players().end () )
	{
		Player* const thePlayer = *it;
		++it;
		if ( thePlayer == nullptr || thePlayer->teulIndex() < 0 )
			continue;

		if ( host_config.kills.reportDamage )
		{
			static char buffer[512];
			strprintf(buffer, "INCOMING: %" PR64BitT " joules of damage %s %s"
							, totalDamageJ
							, (vShooter->isRemote()) ? "from [alt-cell]" : "from [same cell]"
							, vShooter->callsign()
							);
			thePlayer->message(LSTR_BETA, LSTR_GENERIC, buffer);
		}

		Teul::Packet forward(Teul::PacketId(Comm::CallbackX, HOST_HIT_Y), source, hitBytes);
		forward.append(source, hitBytes);
		if ( forward.send(thePlayer->teulIndex()) < 0)
			dbugLog(_FLA, "v:%u couldn't send hit %u(%d)", hit.m_shooter, hit.m_target, thePlayer->teulIndex());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
commProcessApplyHit(ACTION_OBJECT* const a)
{
	Vehicle::Hit hit(a);
	if ( a->offset < 0 )
	{
		dbugLog(_FLA, "t:%d sent invalid VEHICLE_HIT", a->teulIndex);
		return -EINVAL;
	}
	if ( hit.m_shooter == 0 )
	{
		dbugLog(_FLA, "t:%d sent us invalid VEHICLE_HIT (shooter==0)", a->teulIndex);
		return -EINVAL;
	}

	_processVehicleHit(hit, nullptr, a->teulIndex, a->packed, a->sizeInBits());

	return a->offset + a->sizeInBits();
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvClientVehicleHit(ACTION_OBJECT* const a)
{
	Vehicle::Hit hit(a);
	if ( a->offset < 0 )
	{
		dbugLog(_FLA, "t:%d couldnt unpack Vehicle::Hit message", a->teulIndex);
		return a->offset;
	}

	Player* const shooter = (Player*)teulGetPersonaData(a->teulIndex);
	if ( shooter == nullptr || shooter->vehicle == nullptr )
	{
		dbugLog(_FLW, "t:%d suspicious looking vehicleHit - non-local player from non-broadcast source", a->teulIndex);
		commSendPlayerExit(a->teulIndex, PDEST_MAP, PEXIT_DEAD, nullObjID);
		return false;
	}

	///TODO: Make sure this player is actually responsible for this vehicle?

	hit.m_shooter = shooter->vehicle->index;
	if ( hit.m_shooter == 0 )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s no vehicle to process hit from"
						, a->teulIndex, shooter->playerID(), shooter->vehicle->callsign());
		return false;
	}
	// Put the shooter back into the data.
	teulPackUINT32(TEUL_PACK, a->packed, 0, &hit.m_shooter, 0);

	if ( (host_config.arena.checksums || shooter->m_flags.canEject) && shooter->m_flags.checksumsDirty )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s discarded damage messages: dirty checksum"
						, a->teulIndex, shooter->playerID(), shooter->vehicle->callsign());
		return true;
	}

	if ( shooter->m_hasExited )
	{
		dbugLog(_FLM, "t:%d p:%u c:%s discarded damage messages: vehicle is exiting"
						, a->teulIndex, shooter->playerID(), shooter->vehicle->callsign());
		return true;
	}

	if ( shooter->m_updatesSinceRX > 3 * FE_UPDATES_PER_SECOND )
	{
		dbugLog(_FLW, "t:%d p:%u damage message after %u missed updates"
						, shooter->teulIndex()
						, shooter->playerID()
						, shooter->m_updatesSinceRX
						);
	}

	if ( !_processVehicleHit(hit, shooter, a->teulIndex, a->packed, a->offset) )
		return -EINVAL;
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Notify a client that they have resupply

struct ClientResupplyIssue
{
	ClientResupplyIssue(sortieid_t fromSerial, const ChatHandle fromHandle) : m_fromSerial(fromSerial), m_fromHandle(fromHandle) {}
	UINT32		m_fromSerial;
	ChatHandle	m_fromHandle;
};

//////////////////////////////////////////////////////////////////////
//

void
commSendClientResupplyIssue(HostVehicle* vehicle, sortieid_t fromSerial, const ChatHandle fromHandle)
{
	assert( vehicle->isRemote() == false );

	ClientResupplyIssue outgoing(fromSerial, fromHandle);
	PLAYER_LIST::iterator it = vehicle->players().begin();
	while ( it != vehicle->players().end() )
	{
		Player* player = *it;
		++it;
		if ( player->teulIndex() >= 0 )
			teulSend(player->teulIndex(), _XFER_RESUPPLY_ISSUE, &outgoing);
	}
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendResupplyIssue(ACTION_OBJECT* const a)
{
	ClientResupplyIssue* issue = (ClientResupplyIssue*)a->localRawData;
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &issue->m_fromSerial, 0);
	a->offset = wwiiPackHandle(TEUL_PACK, a->packed, a->offset, &issue->m_fromHandle, 0);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Send a client a new position

SINT32
commSendMove(SINT32 teulIndex, const Chat::PlayerMove& move)
{
	if ( teulIndex < 0 )
		return -EINVAL;
	dbugLog(_FLI, "t:%d sending move -> %d/%d/%d", teulIndex, move.m_x, move.m_y, move.m_z);
	return teulSend(teulIndex, _SEND_MOVE, (void*)&move);
}

//////////////////////////////////////////////////////////////////////
// Receive a message from the mmhost telling us a player despawned.

static SINT32
_recvDespawned(ACTION_OBJECT* const a)
{
	const playerid_t playerID = a->read<playerid_t>();
	Player* const player = (Player*)ServerInterface::LookupByPlayerID(playerID);
	if ( player == nullptr )
		return a->offset;

	_playerExit.destination = PDEST_MAP;
	_playerExit.exitCode = PEXIT_DEAD;
	_playerExit.facility = nullObjID;

	dbugLog(_FLM, "t:%d p:%u c:%s v:%u received despawn-player instruction from mmhost"
					, player->teulIndex(), player->playerID(), player->callsign()
					, player->sortieID());

#ifdef BETA_RELEASE
	player->message(LSTR_CAUTION, LSTR_GENERIC, "Sortie being terminated by map-host 'despawned' signal rather than client confirmation.");
#endif

	hostPlayerLeave(*player, _playerExit);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a message from a client telling us who killed us

static SINT32
_recvKillNotice(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	auto& killerID = a->read<sortieid_t>();

	if ( player.vehicle )
	{
#ifdef DEV_BUILD
		dbugLog(_FLD, "t:%d p:%u c:%s receiving kill notice [%u]", a->teulIndex, player.playerID(), player.callsign(), killerID);	///TODO:134:REMOVE
#endif
		player.vehicle->setVehicleKilled(killerID);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

extern UINT32 hostReceiveVehicleUpdates(unsigned char* const input, const UINT32 bytes, const UINT32 fromAddr);

static SINT32
_recvVehicleBcast(ACTION_OBJECT* const a)
{
	hostReceiveVehicleUpdates((unsigned char * const)a->packed, (UINT32)a->sizeInBytes(), a->fromAddress);
	return 0;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendMove(ACTION_OBJECT* const a)
{
	Chat::PlayerMove* pos = (Chat::PlayerMove*)(a->localRawData);
	a->offset = teulPackSINT32(TEUL_PACK, a->packed, a->offset, &(pos->m_x), 0);
	a->offset = teulPackSINT32(TEUL_PACK, a->packed, a->offset, &(pos->m_y), 0);
	a->offset = teulPackSINT32(TEUL_PACK, a->packed, a->offset, &(pos->m_z), 0);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a message telling us to move a player

static SINT32
_recvMovePlayer(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
	{
		dbugLog(_FLW, "t:%d unexpected player move from player", a->teulIndex);
		return -EINVAL;
	}
	auto& incoming = a->read<Chat::PlayerMove>();
	Player* player = static_cast<Player*>(ServerInterface::LookupByPlayerID(incoming.m_playerID));
	if ( player == nullptr || player->teulIndex() < 0 || player->vehicle == nullptr )
	{
		dbugLog(_FLD, "p:%u ignoring move request", incoming.m_playerID);
		return a->offset;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s v:%u moving player to %d/%d/%d"
					, player->teulIndex(), incoming.m_playerID, player->callsign(), player->vehicle->index
					, incoming.m_x, incoming.m_y, incoming.m_z);

	commSendMove(player->teulIndex(), incoming);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a force-despawn message for a given player.

static SINT32
_recvForceDespawn(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
	{
		dbugLog(_FLW, "t:%d received unexpected force despawn from player", a->teulIndex);
		return -EINVAL;
	}
	auto& playerID = a->read<playerid_t>();
	Player* player = static_cast<Player*>(ServerInterface::LookupByPlayerID(playerID));
	if ( player == nullptr )
		return a->offset;
	dbugLog(_FLD, "p:%u c:%s v:%u received force-despawn instruction", player->playerID(), player->callsign(), player->sortieID());

	PLAYER_EXIT pe = { PDEST_MAP, PEXIT_RTB, nullObjID };
	hostPlayerLeave(*player, pe);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendCaptureState(ACTION_OBJECT* const a)
{
	CaptureState* const capState = (CaptureState*)(a->localRawData);
	capState->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
Host::TimeUpdate::SendFunction(ACTION_OBJECT* const a)
{
	Host::TimeUpdate* const update = (Host::TimeUpdate*)(a->localRawData);
	update->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

void
Host::TimeUpdate::Send(SINT32 teulIndex)
{
	Host::TimeUpdate update(wthrGetGameTime(), wthrGetTimeFactor(), wthrGetWeatherPattern(), wthrGetFogPattern());
	teulSend(teulIndex, Host::TimeUpdate::Callback, &update);
}

//////////////////////////////////////////////////////////////////////
//

void
HostVehicle::sendCaptureState(const CaptureState& capState)
{
	if ( isActive() == false )
		return;
	if ( isInCaptureBuilding() == WWII_NO_SIDE )
		return;

	Player* player = activeCommander();
	if ( player == nullptr || player->teulIndex() < 0 )
		return;

	teulSend(player->teulIndex(), _SEND_CAPTURE_STATE, (void*)&capState);
}

//////////////////////////////////////////////////////////////////////
// Strat notifying us to reward everyone in a CP with a capture point

static SINT32
_recvCaptureScore(ACTION_OBJECT* const a)
{
	if ( a->canRead<CaptureScore>() == false )
		return -EINVAL;
	if ( a->m_client != nullptr )
		return -EINVAL;

	auto& incoming = a->read<CaptureScore>();
	dbugLog(_FLI, "capscore: recvd bld:%d,%d:%u side:%u"
					, incoming.m_fullObjID.cell.ox, incoming.m_fullObjID.cell.oy, incoming.m_fullObjID.id
					, incoming.m_side);
	hostGiveCaptureScoreToPlayers(incoming.m_fullObjID, incoming.m_side);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a capture state packet from the strat host, propagte to
// players.

static SINT32
_recvCaptureState(ACTION_OBJECT* const a)
{
	if ( a->canRead<CaptureState>() == false )
	{
		dbugLog(_FLA, "t:%d sent capture state we can't read", a->teulIndex);
		return -EINVAL;
	}
	if ( a->m_client != nullptr )
	{
		dbugLog(_FLA, "t:%d sending us capture state, naughty naughty", a->teulIndex);
		return -EINVAL;
	}

	auto& incoming = a->read<CaptureState>();

	hostSendCaptureStatesToOccupants(incoming);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a resupply issue from another host.

static SINT32
_recvResupplyIssue(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<HostResupplyIssue>() == false )
		return -EINVAL;

	auto& incoming = a->read<HostResupplyIssue>();
	HostVehicle* const vehicle = hostFindVehicleLocal(incoming.m_toSerial);
	if ( vehicle == nullptr )
	{
		dbugLog(_FLI, "rv:%u v:%u received resupply message for NULL vehicle (too late?)", incoming.m_fromSerial, incoming.m_toSerial);
		return a->offset;
	}
	if ( vehicle->isRemote() )
	{
		dbugLog(_FLA, "rv:%u v:%u received resupply message for REMOTE VEHICLE", incoming.m_fromSerial, incoming.m_toSerial);
		return a->offset;
	}

	vehicle->acceptResupply(incoming.m_fromSerial, incoming.m_fromHandle, nullptr, a->endpoint->fromAddress());

	return a->offset;
}

static SINT32
_recvResupplyAccepted(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<HostResupplyAccepted>() == false )
		return -EINVAL;

	auto& incoming = a->read<HostResupplyAccepted>();
	HostVehicle* vehicle = hostFindVehicleLocal(incoming.m_supplierID);
	if ( vehicle == nullptr || vehicle->isRemote() == true )
		return a->offset;
    dbugLog(_FLD, "v:%u supply to %s acknowledged", incoming.m_supplierID, Chat::Name(incoming.m_recipient).c_str());
	vehicle->scoreResupply(incoming.m_recipient);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_registerInterHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Host::CallbackX)
	{
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::MovePlayer_Y, _recvMovePlayer);
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::ForceDespawn_Y, _recvForceDespawn);

		REGISTER_RECEIVER((UCHAR)Host::Callbacks::CaptureScore_Y, _recvCaptureScore);
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::CaptureState_Y, _recvCaptureState);
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::ResupplyIssue_Y, _recvResupplyIssue);
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::ResupplyAccepted_Y, _recvResupplyAccepted);
	}
	END_CALLBACK_REGISTRATION(Host::CallbackX)
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_registerClientCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Comm::CallbackX)
	{
		REGISTER_SENDER(GRANT_VEHICLE_JOIN_Y, _sendGrantVehicleJoin, _GRANT_VEHICLE_JOIN);
		REGISTER_SENDER(ACTIVATE_CLIENT_Y, _sendClientActivate, _ACTIVATE_CLIENT);
		REGISTER_SENDER(RESUPPLY_ISSUE_Y, _sendResupplyIssue, _XFER_RESUPPLY_ISSUE);

		REGISTER_RECEIVER(VEH_JOIN_REQUEST_Y, _recvVehicleJoinRequest);
		REGISTER_RECEIVER(UPDATE_Y, _recvUpdateInfo);

		REGISTER_SENDRECV(CLIENT_HIT_Y, _sendClientVehicleHit, _recvClientVehicleHit, _CLIENT_HIT);
		REGISTER_SENDRECV(PLAYER_EXIT_Y, _sendPlayerExit, _recvPlayerExit, _PLAYER_EXIT);

		REGISTER_MSG_SENDER(DEATH_BY_BUILDING_Y, DEATH_BY_BUILDING);

		REGISTER_RECEIVER(KILL_NOTICE_Y, _recvKillNotice);

		REGISTER_SENDER(MOVE_Y, _sendMove, _SEND_MOVE);

		REGISTER_RECEIVER(DESPAWNED_Y, _recvDespawned);

		REGISTER_RECEIVER(BCAST_VEH_UPDATE_Y, _recvVehicleBcast);

		REGISTER_SENDER(CAPTURE_STATE_Y, _sendCaptureState, _SEND_CAPTURE_STATE);

		extern SINT32 cellVehicleRemoval(ACTION_OBJECT* const a);
		REGISTER_RECEIVER(BCAST_VEH_REMOVE_Y, cellVehicleRemoval);

		REGISTER_SENDER(GAME_TIME_UPDATE_Y, Host::TimeUpdate::SendFunction, Host::TimeUpdate::Callback);

		REGISTER_RECEIVER(HOST_HIT_FWD_Y, commProcessApplyHit);
	}
	END_CALLBACK_REGISTRATION(Comm::CallbackX)
}

//////////////////////////////////////////////////////////////////////
//

SINT32
commInstallHostCallbacks()
{
	SINT32 retval;

	if ( (retval = commPackInit()) < 0)
		return retval;

	if ( (retval = updtPackInit()) < 0)
		return retval;

	_registerClientCallbacks();
	_registerInterHostCallbacks();
	wwiiRegisterPlayerMessageCallback();

	dbugLog(_FLN, "installed commHostCallbacks");

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Send/receive resupply messages between cells

void
hostSendResupplyIssue(UINT32 cellIP, const sortieid_t toSerial, const sortieid_t fromSerial, const ChatHandle fromHandle)
{
	assert( cellIP != 0 );
	assert( toSerial != 0 );
	assert( fromSerial != 0 );
	assert( fromHandle != 0LL );

	const SINT32 cellIndex = serverGetPeerIndex(cellIP, CELL_HOST);
	if ( cellIndex < 0 )
		return;

	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, (UCHAR)Host::Callbacks::ResupplyIssue_Y));
	outgoing.construct<HostResupplyIssue>(toSerial, fromSerial, fromHandle);
	outgoing.send(cellIndex);
}

//////////////////////////////////////////////////////////////////////
// Acknowledgment that someone received resupply

void
hostSendResupplyAccepted(UINT32 cellIP, const sortieid_t sendTo, const ChatHandle byHandle)
{
	assert(cellIP != 0);

	const SINT32 cellIndex = serverGetPeerIndex(cellIP, CELL_HOST);
	if ( cellIndex < 0 )
		return;

	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, (UCHAR)Host::Callbacks::ResupplyAccepted_Y));
	outgoing.construct<HostResupplyAccepted>(sendTo, byHandle);
	outgoing.send(cellIndex);
}

