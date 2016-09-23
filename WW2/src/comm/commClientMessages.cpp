#include <syscfg.h> // Path is project specific.
// Communication with the cell host.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "client/client.h"
#include "client/clientResupplyIssue.h"
#include "client/GameClient.h"
#include "comm/commClientMessages.h"
#include "comm/commConstants.h"
#include "comm/commPackers.h"
#include "fp/fpFPS.h"
#include "pers/persmain.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPacket.h"
#include "updt/updtInfDeath.h"
#include "updt/updtPackers.h"
#include "updt/updtTypes.h"
#include "updt/updtVehicleHit.h"
#include "veh/vehMain.h"
#include "veh/vehupdate.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPack.h"

// WWIIOL specific.
#include "strt/strtCaptureState.h"
#include "strt/strtClientMessages.h"
#include "strt/strtConstants.h"
#include "strt/strtEventTypes.h"
#include "updt/updtDeathByBuilding.h"
#include "weap/weapList.h"

TEUL_CALLBACK*				commUPDATE;
TEUL_CALLBACK*				CLIENT_KILL;
TEUL_CALLBACK*				HOST_KILL;
TEUL_CALLBACK*				commCLIENT_HIT;
static TEUL_CALLBACK*		_PLAYER_EXIT;
static TEUL_CALLBACK*		_REQ_JOIN_VEHICLE;

// WWIIOL specific.
TEUL_CALLBACK*				commSTRAT_DAMAGE;

SINT32 g_lastPing = -1;

SINT32 commSendPlayerExit(SINT32 teulIndex, PLAYER_DESTINATION destination,	PLAYER_EXIT_CODE exitCode, FULL_OBJ_ID facility)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( teulIndex < 0 )
		return -EINVAL;

	// Make sure nagle is disabled so that this actually gets sent.
	endpoint->setNagleEnabled(false);

	PLAYER_EXIT pe = { destination, exitCode, facility };
	return teulSend(teulIndex, _PLAYER_EXIT, &pe);
}

SINT32 commSendVehicleJoin(SINT32 teulIndex, VEHICLE_REQUEST* const lvr)
{
	return teulSend(teulIndex, _REQ_JOIN_VEHICLE, lvr);
}

static SINT32 _sendVehicleJoinRequest( ACTION_OBJECT* const a )
{
	return commPackVEHICLE_REQUEST(TEUL_PACK, a->packed, a->offset, (VEHICLE_REQUEST*) a->localRawData, sizeof(VEHICLE_REQUEST));
}

static SINT32 _sendPeriodicUpdate( ACTION_OBJECT* const a )
{
	COMM_UPDATE_BUFFER* const cub = (COMM_UPDATE_BUFFER*)(a->localRawData);
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, cub->data, cub->size);
}

static SINT32 _sendStratDamage( ACTION_OBJECT* const a )
{
	return updtPackCP_HIT_HEADER(TEUL_PACK, a->packed, a->offset, (CP_HIT_HEADER*) a->localRawData, 0);
}

static SINT32 _sendClientVehicleHit(ACTION_OBJECT* const a)
{
	Vehicle::Hit* const hit = (Vehicle::Hit*)(a->localRawData);
	hit->packer(TEUL_PACK, a);
	return a->offset;
}

static SINT32 _sendPlayerExit(ACTION_OBJECT* const a)
{
	PLAYER_EXIT* const pe = (PLAYER_EXIT*) a->localRawData;

	dbugWrite(_FLA,"sending player exit %d dest %d",pe->destination,pe->exitCode);

	return commPackPlayerExit(TEUL_PACK, a->packed, a->offset, pe, 0);
}

static SINT32 _recvPlayerExit(ACTION_OBJECT* const a)
{
	PLAYER_EXIT pe;
	a->offset = commPackPlayerExit(TEUL_UNPACK, a->packed, a->offset, &pe, 0);
	dbugWrite(_FLA, "received player exit destination %d", pe.destination);

	clientForcedCellExit(a->teulIndex,pe.exitCode);

#if platBuild == BUILD_SERVER
#pragma message "move to clientForcedCellExit?"
	// OGS: Probably not, because that would cause a feedback loop.
	commSendPlayerExit(a->teulIndex,  PDEST_MAP, pe.exitCode, pe.facility);
#endif

	return a->offset;
}

static SINT32 _recvVehicleAcknowledge( ACTION_OBJECT* const a )
{
	VEHICLE_REPLY	vReply;

	a->offset = commPackVEHICLE_REPLY(TEUL_UNPACK ,a->packed , a->offset, &vReply, 0);
	clientProcessVehicleRequestResponse(a->teulIndex, &vReply);

	return a->offset;
}

#include "wwii/wwiiRunningAverage.h"

static RunningAverage<UINT32, 120> pingHistogram;
SINT32 g_averagePing = 0;

SINT32 commRecvWorldUpdateData(const SINT32 teulIndex, UCHAR* const data, SINT32 offset, const SINT32 size_bytes)
{
	WORLD_UPDATE wu;
	memset(&wu, 0, sizeof(WORLD_UPDATE));

	// #1 unpack the header.
	wu.header = *(WORLD_UPDATE::Header*)(data);
	offset += Teul::BytesToBits(sizeof(wu.header));

	offset = updtPackWORLD_UPDATE(TEUL_UNPACK, data, offset, &wu, 0);

	// Client side sanity check on the vehicle count.
	if ( wu.header.numVehicles > MAX_VEH_PER_UPDATE )
	{
		clientErrorLog("badNV");
		return -EINVAL;
	}

	// Note: the values are IN the World Update so you could just use that.
	g_framingMode = wu.header.framingMode;
	g_hostUpdateInterval = framings[g_framingMode].updateInterval;

	if ( wu.header.numVehicles > framings[g_framingMode].maxVehiclesPerFrame() )
	{
		clientErrorLog("badNVF");
		return -EINVAL;
	}

	for(UINT32 i = 0; i < wu.header.numVehicles; i++)
	{
		vehupdateProcessPositionUpdateMsg(&wu.vehUpdate[i]);
	}

	clientWorldUpdate(teulIndex, &wu);

	const size_t offInBytes = Teul::BitsToBytes(offset);
	if ( offInBytes < (size_t)size_bytes )
	{
		TeulTimeMS& lastPing = *(UINT64*)(data + offInBytes);
		teulSyncClock();
		g_lastPing = std::min((UINT32)((teulClock64 - lastPing) / 2), (UINT32)1);
		pingHistogram.add(g_lastPing);
		g_averagePing = pingHistogram.average();
	}
	else
	{
		g_lastPing = -1;
		g_averagePing = 0;
	}
#ifdef VEHICLES_GAME_INFO
	vehSaveLastWorldUpdate(wu);
#endif

	return offset;
}

static SINT32 _recvWorldUpdate( ACTION_OBJECT* const a )
{
#ifdef MULTI_CLIENT
	if(Online)
	{
#endif
	dbugWrite(_FLW, "index: %d time: %llu", a->teulIndex, teulClock64);
	a->offset = commRecvWorldUpdateData(a->teulIndex, a->packed, a->offset, a->sizeInBytes());
#ifdef MULTI_CLIENT
	}
#endif
	return a->offset;
}

static SINT32 _recvHostVehicleHit( ACTION_OBJECT* const a )
{
	Vehicle::Hit hit(a);

	vehProcessVehicleHit(hit);

	// fill out the local vehicle data from this information.
	dbugWrite(_FLA,"Vehicle Hit: from %d sent %llu numcomponentshit %zu\n"
				, hit.m_shooter, hit.m_timestamp, hit.m_components.size());

	return a->offset;
}

static SINT32 _recvRemoveList(ACTION_OBJECT* const a)
{
	WORLD_UPDATE wu;
	wu.packRemoveList(TEUL_UNPACK, a);

	for(UINT32 i = 0; i < wu.numRemove; i++)
	{
		vehicleRemoveVehicle(wu.removeList[i]);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Notify the host when someone kills us

void
commSendKillNotice(SINT32 teulIndex, sortieid_t myVehicleID, sortieid_t hostVehicleID)
{
	// ensure we don't make this call more than once per sortie
	static sortieid_t lastKillReportedOnID = 0;

	if(myVehicleID == lastKillReportedOnID)
		return;

	lastKillReportedOnID = myVehicleID;

	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, KILL_NOTICE_Y), hostVehicleID);
	outgoing.send(teulIndex);

	vehupdateUpdate(TRUE);
}

//////////////////////////////////////////////////////////////////////
// Incoming notice from the host that we have been killed by a
// building state change. No actual data supplied.

template<>
SINT32
DEATH_BY_BUILDING::receive(SINT32 teulIndex, DEATH_BY_BUILDING& incoming)
{
	// We've been killed by the last building we were in, die die die
	persKillCrew();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a resupply issued statement

static SINT32
_recvResupplyIssue(ACTION_OBJECT* const a)
{
	ResupplyIssue incoming(a);
	incoming.process();
	return 0;
}

ResupplyIssue::ResupplyIssue(ACTION_OBJECT* const a)
{
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &m_fromHostIndex, 0);
	a->offset = wwiiPackHandle(TEUL_UNPACK, a->packed, a->offset, &m_fromHandle, 0);
}

//////////////////////////////////////////////////////////////////////
// Host has told us to move

static SINT32
_recvMove(ACTION_OBJECT* const a)
{
	mlVehicleData* const VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData)
		return -EINVAL;

	FLOAT64VECTOR pos;
	SINT32 value;

	a->offset = teulPackSINT32(TEUL_UNPACK, a->packed, a->offset, &value, 0);
	pos.x = (FLOAT64) value;

	a->offset = teulPackSINT32(TEUL_UNPACK, a->packed, a->offset, &value, 0);
	pos.y = (FLOAT64) value;

	a->offset = teulPackSINT32(TEUL_UNPACK, a->packed, a->offset, &value, 0);
	pos.z = (FLOAT64) value;

	if ( a->offset > 0 ) VehicleData->RenderPos = VehicleData->VehiclePos = pos;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive an infantry death message.

static SINT32 _recvInfDeath(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(ww2::Client, client);
	/*InfDeath& death = */a->read<InfDeath>();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Send a death notice to the host.

SINT32
InfDeath::Send(SINT32 teulIndex, const FLOAT32 x_, FLOAT32 y_, FLOAT32 z_, const sortieid_t killedByIndex_, const ChatHandle killedByHandle_
				, const DeathType deathType_, const bool byEnvironment_, const UCHAR deathAngle_, const UCHAR deathForce_
				)
{
	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, INF_DEATH_Y));
	outgoing.construct<InfDeath>(x_, y_, z_, killedByIndex_, killedByHandle_, deathType_, byEnvironment_, deathAngle_, deathForce_);
	return outgoing.send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Receive a capture state message.

static SINT32
_recvCaptureState(ACTION_OBJECT* const a)
{
	CaptureState incoming(a);

	// let the client know about the new capture message
	PlayerClient()->ProcessCaptureState(incoming);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a Strat::EventPacket containing one or more updates.

static SINT32
_recvStratEvents(ACTION_OBJECT* const a)
{
	Strat::EventPacket incoming(a);

	// Do nothing unless the packer finds valid data.
	if ( a->offset > 0 )
	{
		PlayerClient()->ProcessStratEventPacket(incoming);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a game time update from the cell host.

#include "host/hostTimeUpdate.h"

static SINT32 _recvGameTimeUpdate(ACTION_OBJECT* const a)
{
	Host::TimeUpdate update(a);
	PlayerClient()->ProcessGameTimeUpdate(update);
	return a->offset;
}

SINT32 commInstallClientCallbacks(void)
{
	SINT32 retval;

	if ( (retval = commPackInit()) < 0)
		return retval;

	if ( (retval = updtPackInit()) < 0)
		return retval;

	BEGIN_CALLBACK_REGISTRATION(Comm::CallbackX)

		REGISTER_RECEIVER(WORLD_UPDATE_Y, _recvWorldUpdate);
		REGISTER_RECEIVER(GRANT_VEHICLE_JOIN_Y, _recvVehicleAcknowledge);
		REGISTER_RECEIVER(HOST_HIT_Y, _recvHostVehicleHit);

		REGISTER_SENDER(VEH_JOIN_REQUEST_Y, _sendVehicleJoinRequest, _REQ_JOIN_VEHICLE);
		REGISTER_SENDER(UPDATE_Y, _sendPeriodicUpdate, commUPDATE);
		REGISTER_SENDER(CLIENT_HIT_Y, _sendClientVehicleHit, commCLIENT_HIT);

		REGISTER_SENDRECV(PLAYER_EXIT_Y, _sendPlayerExit, _recvPlayerExit, _PLAYER_EXIT);

		REGISTER_RECEIVER(INF_DEATH_Y, _recvInfDeath);

		REGISTER_RECEIVER(RESUPPLY_ISSUE_Y, _recvResupplyIssue);

		REGISTER_RECEIVER(MOVE_Y, _recvMove);

		REGISTER_RECEIVER(REMOVE_LIST_Y, _recvRemoveList);
		REGISTER_RECEIVER(GAME_TIME_UPDATE_Y, _recvGameTimeUpdate);

		// WW2OL specific.
		REGISTER_SENDER(DAMAGE_LIST_Y, _sendStratDamage, commSTRAT_DAMAGE);
		REGISTER_RECEIVER(CAPTURE_STATE_Y, _recvCaptureState);
		REGISTER_MSG_RECEIVER(DEATH_BY_BUILDING_Y, DEATH_BY_BUILDING);
		REGISTER_RECEIVER(STRAT_EVENTS_Y, _recvStratEvents);

	END_CALLBACK_REGISTRATION
}

