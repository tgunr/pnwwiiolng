#include "syscfg.h" // Path is project specific.
// Date: Wed Nov 1 16:55:26 2000
//
//	This is where all declarations for commPack routines for universal
//	game specific object structures thatwill be transmited
//	accross the network must reside or be included into.  All commPack
//	routines must have an entry in this file.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#if platBuild == BUILD_SERVER
#include "include/clustercfg.h"
#endif

#include "strt/strtCPFlags.h"
#include "teul/teulEndpointDefs.h"
#include "updt/updtPackers.h"
#include "updt/updtTypes.h"
#include "wwii/wwiiPack.h"

#if platBuild == BUILD_SERVER
#include "host/host.h"
#include "host/hostGridTypes.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "host/hostVehicleViewing.h"
#include "server/serverServerInfo.h"
#include "teul/teulEndpoint.h"
#include "teul/teulUtils.h"
#include "weap/weapDefs.h"
#include "include/wthrTime.h"
#include "inst/instTypes.h"
#include "inst/instHost.h"
#include "wwii/wwiiThreads.h"
#include "teul/teulPacket.h"
#else	// g++ under fedora doesn't like #else if ...
#include "include/vars.h"
#include "client/GameClient.h"
#include "client/ClientSquad.h"
#include "client/ww2HostState.h"
#endif
#include "wwii/wwiiPackBool.h"
#include "include/wthrTimeFactors.h"

#include "host/hostTiming.h"

// Helper macros for cleaning up all the client/server ifdefs.
#if platBuild == BUILD_SERVER
# define ON_SERVER( _expr_ ) _expr_
# define ON_CLIENT( _expr_ )
#else
# define ON_SERVER( _expr_ )
# define ON_CLIENT( _expr_ ) _expr_
#endif

#define P2_BASE			10000	// Fast X/Y is position expressed in 100ms.

#define FIRST_TIME_TURRET_SIZE ((8*sizeof(turret_t))+1)

#define FE_REMOVE_TIMEOUT	20000

#define MAX_UDP_DATA   1400

#undef DEBUG_UPDATE_PACK

// The host update rate should be tied closely to the client's update rate,
// where host/client here is the sender.
//
// "MaxU" is a "best case" value; the actual number sent may be significantly
// smaller depending on space available in the world update buffer/packet.
//
// See updtEnums.h::FE_UPDATE_INTERVAL
UpdateFraming framings[UpdateFraming::_MODES] =
{
    // MaxV                         Intvl   MaxU
    //   |                             |     ^--   Max vehicles per update
    //   |                             ^-----|--   Interval between updates in ms
    //   ^-----------------------------|-----|--   Max vehicles to track
	{    88,   UpdateFraming::_LARGE, 100,    20, "um:0 88 Limit 20/Update 100ms" }
,	{   112,   UpdateFraming::_LARGE, 100,    28, "um:1 112 Limit 28/Update 100ms" }
, 	{   128,   UpdateFraming::_LARGE, 100,    32, "um:2 128 Limit 32/Update 100ms" }
, 	{   146,   UpdateFraming::_LARGE, 100,    36, "um:3 146 Limit 36/Update 100ms" }
, 	{   184,   UpdateFraming::_LARGE, 100,    48, "um:4 184 Limit 48/Update 100ms" }
};

#if platBuild != BUILD_SERVER
UINT32 g_framingMode;
UINT32 g_hostUpdateInterval;
#endif

//////////////////////////////////////////////////////////////////////
// Initialization for the update packing system.

SINT32	updtPackInit()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Pack a turret.

static SINT32 _packTurret(PackMode mode, UCHAR* const data, SINT32 offset, TURRET_POS* tp, SINT32 size)
{
	return teulPackVoid(mode, data, offset, tp, sizeof(*tp));
}

//////////////////////////////////////////////////////////////////////
// Pack a vehicle's X/Y/Z position.

static SINT32 _packVehiclePosition(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_POSITION* const vp, SINT32 size)
{
	offset = teulPackSINT32(mode, data, offset, &vp->m_x, 24);
	offset = teulPackSINT32(mode, data, offset, &vp->m_y, 24);
	offset = teulPackSINT32(mode, data, offset, &vp->m_z, 24);
	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack common components. OGS: No idea why this is separated out.

static SINT32 _packVehicleCommon(PackMode mode, UCHAR* const data, SINT32 offset, SHARED_VEHICLE_INFO* const vehInfo, const UINT32 present)
{
	if (present & PR_RIDING)
	{
		offset = teulPackUINT32(mode, data, offset, &vehInfo->rideIndex, 0);
		offset = teulPackUINT16(mode, data, offset, &vehInfo->seatNumber, 8);
	}

	if (present & PR_RELPOS)
	{
		offset = teulPackVoid(mode, data, offset, &vehInfo->relPos, sizeof(vehInfo->relPos));
	}

	if (present & PR_VEH_VIS)
		offset = teulPackUINT32(mode, data, offset, &vehInfo->visBits, 0);

	// pack remaining header elements
	if (present & PR_STATE0)
		offset = teulPackUINT16(mode, data, offset, &vehInfo->stateFlags0, 0);

	if (present & PR_STATE1)
		offset = teulPackUINT16(mode, data, offset, &vehInfo->stateFlags1, 0);

	return offset;
}

#if platBuild == BUILD_CLIENT && defined(DEBUG_WW2) // && !defined(RELEASE_CLIENT)
struct NewVehicleHistory
{
	TeulTimeMS	receivedAt;
	std::wstring description;
};
static std::vector<NewVehicleHistory> nvHistory;

std::vector<std::wstring> updtGetNvHistory()
{
	std::vector<std::wstring> results;
	if ( !Online )
		return results;

	if ( !PlayerClient() || !PlayerClient()->CellState() || !PlayerClient()->CellState()->Connected() )
		return results;

	static const TeulTimeMS lifespan = 30 * 1000;	// 10 seconds
	if ( nvHistory.empty() )
	{
		results.push_back(L"No new vehicles\n");
		return results;
	}

	const TeulTimeMS cutoff = teulClock64 - lifespan;
	while ( nvHistory.empty() == false && nvHistory.front().receivedAt <= cutoff )
	{
		nvHistory.erase(nvHistory.begin());
	}

	std::wstring result = L"";
	wchar_t buffer[128];
	for ( auto it = nvHistory.begin(), end = nvHistory.end(); it != end; ++it )
	{
		swprintf(buffer, sizeof(buffer) - 1, L"[%08llu] ", teulClock64 - it->receivedAt);
		result = buffer;
		result += it->description;
		results.push_back(result);
	}

	return results;
}

static void
_recordNewHistoryEntry(const SHARED_VEHICLE_INFO* const vehInfo, const ChatHandle packedCallsign)
{
	NewVehicleHistory entry;
	entry.receivedAt = teulClock64;
	entry.description = L'+';
	entry.description += chatGetWideCallsign(packedCallsign);
	entry.description += L'(';
	switch (vehInfo->side)
	{
	case WWII_NO_SIDE: entry.description += L"No Side?"; break;
	case ALLIED: entry.description += L"Allied"; break;
	case AXIS: entry.description += L"Axis"; break;
	case NEUTRAL: entry.description += L"Neutral?"; break;
	default: entry.description += L"Bad side?"; break;
	}
	entry.description += L") OneTimeFlags";

	if ( vehInfo->oneTimeBits == 0 )
		entry.description += L":None";

	if ( vehInfo->oneTimeBits & OTB_TRAINER )
		entry.description += L":Trnr";
	if ( vehInfo->oneTimeBits & OTB_IN_MISSION )
		entry.description += L":MyMsn";
	if ( vehInfo->oneTimeBits & OTB_MISSION_LEADER )
		entry.description += L":MsnLdr";
	if ( vehInfo->oneTimeBits & OTB_IN_BRIGADE )
		entry.description += L":MyBde";
	if ( vehInfo->oneTimeBits & OTB_USE_DECALS )
		entry.description += L":Decal";
	if ( vehInfo->oneTimeBits & OTB_BUILDER )
		entry.description += L":Bldr";
	if ( vehInfo->oneTimeBits & OTB_HIGH_COMMAND )
		entry.description += L":HC";
	if ( vehInfo->oneTimeBits & OTB_IN_SQUAD )
		entry.description += L":Sqd";

	nvHistory.push_back(entry);
}
#endif

//////////////////////////////////////////////////////////////////////
// Pack first time header ("one time") data for a vehicle.

static SINT32 _packONE_TIME_INFO(PackMode mode, UCHAR* const data, SINT32 offset, SHARED_VEHICLE_INFO* const vehInfo, SINT32 size, const WORLD_UPDATE* const wu)
{
	offset = wwiiPackWeaponSpec(mode, data, offset, &vehInfo->wSerial.spec, 0);
	if ( mode == TEUL_UNPACK )
	{
		vehInfo->country = vehInfo->spec().wCountry;
		vehInfo->side = vehInfo->spec().side();
		if ( vehInfo->side == NEUTRAL )
			vehInfo->side = WWII_NO_SIDE;
	}

	ChatHandle packedCallsign = 0LL;
	bool packSquadName = false;
	static const SINT32 numOneTimeBits = 8;
	UINT16 oneTimeBits = 0;
#if platBuild == BUILD_SERVER
	if ( mode == TEUL_PACK )
	{
		HostVehicle* viewer = wu ? wu->myVehicle : vehInfo->m_vehicle;
		oneTimeBits = vehInfo->m_vehicle->getOneTimeBitsFor(viewer);

		// Try to avoid packing the squad name
		if ( wu == nullptr || ((oneTimeBits & OTB_IN_SQUAD) == 0 && vehInfo->squadHandle > 0 && (vehInfo->side == wu->myVehicle->side() || wu->myVehicle->isBuzzard())) )
			packSquadName = true;

		if ( packSquadName )
			packedCallsign = vehInfo->squadHandle;
	}
#endif
	offset = teulPackUINT16(mode, data, offset, &oneTimeBits, numOneTimeBits);
	if ( mode == TEUL_UNPACK )
		vehInfo->oneTimeBits = oneTimeBits;

	offset = wwiiPackRankNo(mode, data, offset, &vehInfo->rank, 7);
	offset = wwiiPackBool(mode, data, offset, packSquadName);

	if ( packSquadName )
		offset = wwiiPackHandle(mode, data, offset, &packedCallsign, 0);
	else
		packedCallsign = 0LL;

#if platBuild != BUILD_SERVER
	if ( mode == TEUL_UNPACK )
	{
		if(oneTimeBits & OTB_IN_SQUAD)
			chatGetCallsign(vehInfo->squadName, PlayerClient()->PlayerSquad().shortHandle());
		else
			chatGetCallsign(vehInfo->squadName, packedCallsign);
	}
#endif

	// Pack the callsign chat-style to save 2 bytes
	if ( mode == TEUL_PACK )
	{
#if platBuild == BUILD_SERVER
		if ( vehInfo->m_hideCallsign == true )
		{
			static const ChatHandle gmHandle = chatGetHandle("GameMgr");
			packedCallsign = gmHandle;
		}
		else
		{
			if ( vehInfo->packedCallsign == 0 )
				vehInfo->packedCallsign = chatGetHandle(vehInfo->callsign);
			packedCallsign = vehInfo->packedCallsign;
		}
#else
		packedCallsign = chatGetHandle(vehInfo->callsign);
#endif
	}
	offset = wwiiPackHandle(mode, data, offset, &packedCallsign, 0);
	if ( mode == TEUL_UNPACK )
	{
		chatGetCallsign(vehInfo->callsign, packedCallsign);
#if platBuild == BUILD_SERVER
		vehInfo->packedCallsign = packedCallsign;
#endif
	}

	const bool packDecal = (oneTimeBits & OTB_USE_DECALS) == OTB_USE_DECALS;
	if ( packDecal )
		offset = teulPackUCHAR(mode, data, offset, &vehInfo->decal, 8);
	else if (mode == TEUL_UNPACK)
		vehInfo->decal = 0;

	offset = wwiiPackBrigadeID(mode, data, offset, vehInfo->brigadeID, 0);

#if platBuild == BUILD_CLIENT && defined(DEBUG_WW2) // && !defined(RELEASE_CLIENT)
	_recordNewHistoryEntry(vehInfo, packedCallsign);
#endif

	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack the header information for a vehicle.

static SINT32 _packVehicleHead(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_UPDATE_HEADER* const vupHeader, SINT32 presentSize, const WORLD_UPDATE* const wu)
{
	UINT16 localTimeStamp;  // local meaning just limited scope

#if platBuild == BUILD_SERVER
	SHARED_VEHICLE_INFO*	vehInfo = vupHeader->vehInfo;
	if (mode == TEUL_UNPACK)
	{
		///OGS: ARGH! We actually let the player tell us their vehicle number???
		UINT32 myIndex = 0;
		offset = teulPackUINT32(mode, data, offset, &myIndex, 0);
		if ( offset <= 0 )
			LogAbort("Failed unpacking vehicle update: %d", offset);

		HostVehicle* const vehicle = hostFindVehicleLocal(myIndex);
		if (vehicle == nullptr)
		{
#ifdef BETA_RELEASE
			LogAbort("BAD VEHICLE DATA (claimed %u) offset:%d", myIndex, offset);
#else
			dbugLog(_FLA, "Invalid vehicle index: %u", myIndex);
			return -EINVAL;
#endif
		}
		else
			vehInfo = vupHeader->vehInfo = &vehicle->vehicleData;
	}
	else
	{
		offset = teulPackUINT32(mode, data, offset, &vehInfo->index, 0);
	}
#else
	SHARED_VEHICLE_INFO*	vehInfo = &vupHeader->vehInfo;

	offset = teulPackUINT32(mode, data, offset, &vehInfo->index, 0);
#endif

	offset = teulPackUINT32(mode, data, offset, &vupHeader->present, presentSize);

	if (mode == TEUL_PACK)
	{
#if platBuild == BUILD_SERVER
		// if packing, only ever pack the commander's time
		localTimeStamp = vehInfo->cmdrTimeStamp;
#else
		// for the client, it is save to just go ahead and store what we have
		localTimeStamp = vehInfo->timeStamp;
#endif
	}

	offset = teulPackUINT16(mode, data, offset, &localTimeStamp, 16);

	// if unpacking, deal with timestamp properly
	if (mode == TEUL_UNPACK)
	{
#if platBuild == BUILD_SERVER
		// make sure this timestamp reflects last time vehicle was updated by either party
		vehInfo->timeStamp = localTimeStamp;

		// this is currently unused but only applies to the host - keep it here just in case later reference
		// is put back in.
		vehInfo->hostPosTimeStamp = teulClock64;

		if (vupHeader != nullptr && vupHeader->player != nullptr && vupHeader->player->station != UPDT_OTHER)
		{
			vehInfo->cmdrTimeStamp = localTimeStamp;
		}

#else
		// restore timestamp for client and update position if included
		vehInfo->timeStamp = localTimeStamp;
		if (vupHeader->present & PR_POSITION)
		{
			vehInfo->posTimeStamp = vehInfo->timeStamp;
		}
#endif
	}

	//pack up common elements
	offset = _packVehicleCommon(mode, data, offset, vehInfo, vupHeader->present);

	UCHAR fireLow = vupHeader->fireMask & 0x00ff;
	UCHAR fireHigh = (UCHAR)((vupHeader->fireMask & 0xff00) >> 8);
	if ( vupHeader->present & PR_FIRE1 )
		offset = teulPackUCHAR(mode, data, offset, &fireLow, 0);
	if ( vupHeader->present & PR_FIRE2 )
		offset = teulPackUCHAR(mode, data, offset, &fireHigh, 0);
	if ( mode == TEUL_UNPACK )
		vupHeader->fireMask = ((UINT16)fireHigh << 8) | ((UINT16)fireLow);

	// one time to or from client
	if (vupHeader->present & PR_ONE_TIME)
		offset = _packONE_TIME_INFO(mode, data, offset, vehInfo, 0, wu);

	if (vupHeader->present & PR_KILLED)
		offset = teulPackUINT32(mode, data, offset, (UINT32*)&vupHeader->killedBy, 0);

	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack turrets for a vehicle.

static SINT32 _packVehicleTurrets(PackMode mode, UCHAR* const data, SINT32 offset, TURRET_HEADER* const turretHeader, SINT32 size)
{
	TURRET_STRUCT* turret = &(turretHeader->vehInfo->turret);
	UINT16 count = 0;

	offset = teulPackUINT16(mode, data, offset, &turretHeader->vt.turretMask, 8);

	for ( UINT32 i = 0; i < (UINT32)MAX_TURRETS; ++i )
	{
		const UINT32 mask = 1 << i;
		if ( turretHeader->vt.turretMask & mask )
		{
			offset = _packTurret(mode, data, offset, &turret->pos[i], 0);

//			dbugWrite(_FLA, "turret:%s %d, x%x y:%x", (mode== TEUL_UNPACK)?"U":"P", i, turret->pos[i].x, turret->pos[i].y);

			count++;
#if platBuild == BUILD_SERVER
			if (mode == TEUL_UNPACK)
			{
				turret->clock64[i] = teulClock64;
				turret->valid |= mask;
			}
#endif
		}
		else if ((turretHeader->vt.turretMask >> i) == 0)
			break;

	}

	return offset;
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packVEHICLE_VEL_HIGH(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_VEL_HIGH* const vv, SINT32 /*size*/)
{
	return teulPackVoid(mode, data, offset, vv, sizeof(*vv));
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packVEHICLE_ATTVEL_HIGH(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_ATTVEL_HIGH* const vv, SINT32)
{
	return teulPackVoid(mode, data, offset, vv, sizeof(*vv));
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packVEHICLE_ACCEL_HIGH(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_ACCEL_HIGH* const va, SINT32)
{
	return teulPackVoid(mode, data, offset, va, sizeof(*va));
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packVEHICLE_ATT_HIGH(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_ATT_HIGH* const vt, SINT32)
{
	return teulPackVoid(mode, data, offset, vt, sizeof(*vt));
}

//////////////////////////////////////////////////////////////////////
// Pack vehicle 'auxilliary' information: velocity, accel, etc.

static SINT32 _packVEHICLE_AUX_INFO(PackMode mode, UCHAR* const data, SINT32 offset, SHARED_VEHICLE_INFO* const vehInfo, SINT32)
{
	offset = _packVEHICLE_VEL_HIGH(mode, data, offset, &vehInfo->velocity, 0);
	offset = _packVEHICLE_ACCEL_HIGH(mode, data, offset, &vehInfo->acceleration, 0);
	offset = _packVEHICLE_ATTVEL_HIGH(mode, data, offset, &vehInfo->rotational, 0);
	offset = _packVEHICLE_ATT_HIGH(mode, data, offset, &vehInfo->attitude, 0);

	return offset;
}

#if platBuild == BUILD_SERVER
// Translate the vehicle's actual position into an update-centered offset.
static void
_translatePositionToOffset(const VEHICLE_POSITION& position, const WORLD_UPDATE& wu, VEHICLE_POSITION& offset)
{
	offset = position;
	offset -= wu.header.center;
}
#else
// Translate an update-centered offset into an actual position.
// Unlike the previous version, this also modifies the incoming network data.
static void
_translateOffsetToPosition(VEHICLE_POSITION& offset, const WORLD_UPDATE& wu, VEHICLE_POSITION& position)
{
	position = offset;
	position += wu.header.center;
}
#endif

#if platBuild == BUILD_SERVER
static SINT32 _validateTurrets(Viewing& viewing, VEHICLE_UPDATE_HEADER* const vupHeader, const bool updtIsNew)
{
	struct TURRET_STRUCT* turret;

	vupHeader->vt.numTurrets = 0;

# if platBuild == BUILD_SERVER // Not much use given we know we are teul server from above?
		turret =  &(vupHeader->vehInfo->turret);
# else
		turret =  &(vupHeader->vehInfo.turret);
# endif

	for (size_t t = 0;t < 8;t++)
	{
		const UINT16 mask = 1 << t;
		if (updtIsNew || (turret->valid & mask) )
		{
			if ( updtIsNew || turret->clock64[t] > viewing.m_lastSent )
			{
				vupHeader->vt.numTurrets++;
				viewing.m_turretsPending |= mask;
			}
		}
	}

	return vupHeader->vt.numTurrets;
}

static void _setTurrets(Viewing& viewing, VEHICLE_TURRETS* const vt)
{
	vt->turretMask = 0;
	vt->numTurrets = 0;
	if (viewing.m_turretsPending != 0)
	{
		for ( size_t t = 0; t < MAX_TURRETS; ++t )
		{
			const UINT16 mask = (0x0001 << t);

			if (viewing.m_turretsPending & mask)
			{
				vt->turretMask |= mask;
				viewing.m_turretsPending &= ~mask;

				vt->numTurrets++;
			}
			else if ( viewing.m_turretsPending == 0 )
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Figure out what's changed and build a header describing which of
// the fields we're going to be sending.

static void _initVehicleHeader(VEHICLE_UPDATE_HEADER* const vupHeader, Viewing& viewing)
{
	const SHARED_VEHICLE_INFO* const vehInfo = vupHeader->vehInfo;

	const bool updtIsNew = (viewing.m_lastSent + FE_REMOVE_TIMEOUT < teulClock64);

	if (updtIsNew)
	{
		// First time update, we need to send a keyframe - i.e. everything.
		viewing.m_fireMask = vupHeader->fireMask = vehInfo->fire.state;
		viewing.m_stateFlags0 = vehInfo->stateFlags0;
		viewing.m_stateFlags1 = vehInfo->stateFlags1;
		viewing.m_seatNumber = vehInfo->seatNumber;
		viewing.m_visBits = vehInfo->visBits;
		viewing.m_rideIndex = vehInfo->rideIndex;
		viewing.m_position = vupHeader->position = vehInfo->position;

		vupHeader->present = (PR_FIRE_FLAGS|PR_STATE0|PR_STATE1|PR_VEH_VIS|PR_POSITION|PR_ONE_TIME|PR_RIDING);
	}
	else
	{
		// We've described this vehicle to the player before, try
		// to only send things that have changed.
		vupHeader->present = 0;

		vupHeader->fireMask = vehInfo->fire.state;
		// Check each weapon to see if it was fired since the viewer
		// last saw the player. If so, then we need to send a fire bit
		// so that they know that gun flashed. Lousy way to do this.
		for (SINT32 t = 0; t < MAX_GUNS; t++)
		{
			const size_t mask = 1ULL << t;
			if ((vehInfo->fire.valid & mask) != 0)
			{
				if ( vehInfo->fire.clock64[t] > viewing.m_lastSent )
				{
					// if the fire valid and timestamp has been updated since the last time an update was sent...
					// overide the fire bit and set the xmit flag
					vupHeader->fireMask |= mask;
#ifdef HEAVY_DEBUG
					dbugLog(_FLD, "used fire bit overide %d set at %" PR64BitUT "s for update last sent %" PR64BitUT "s", t, vehInfo->fire.clock64[t], viewing.m_lastSent);
#endif
				}
			}
			else
				vupHeader->fireMask &= ~mask;
		}

		// Does this produce a different mask than we sent last time?
		const UINT16 firediff = vupHeader->fireMask ^ viewing.m_fireMask;
		if ( firediff )
		{
			// last sent != to send state, set send flag
			if ( firediff & 0x0ff )
				vupHeader->present |= PR_FIRE1;
			if ( firediff & 0xff00 )		// remaining bits
				vupHeader->present |= PR_FIRE2;

#ifdef HEAVY_DEBUG
			dbugLog(_FLD, "sending new fire mask %x last %d", vupHeader->fireMask, viewing.m_fireMask);
#endif
			// last now == generated firemask to send...
			viewing.m_fireMask = vupHeader->fireMask;
		}

		// Has the player switched riding seats/ride?
		if ( viewing.m_seatNumber != vehInfo->seatNumber || viewing.m_rideIndex != vehInfo->rideIndex )
		{
			viewing.m_seatNumber = vehInfo->seatNumber;
			viewing.m_rideIndex = vehInfo->rideIndex;
			vupHeader->present |= PR_RIDING;
		}

		// Do we need to update state flags?
		if ( viewing.m_stateFlags0 != vehInfo->stateFlags0)
		{
			viewing.m_stateFlags0 = vehInfo->stateFlags0;
			vupHeader->present |= PR_STATE0;
		}
		if ( viewing.m_stateFlags1 != vehInfo->stateFlags1)
		{
			viewing.m_stateFlags1 = vehInfo->stateFlags1;
			vupHeader->present |= PR_STATE1;
		}
		if ( viewing.m_visBits != vehInfo->visBits)
		{
			viewing.m_visBits = vehInfo->visBits;
			vupHeader->present |= PR_VEH_VIS;
		}

//		if ( vehInfo->hostPosTimeStamp >= viewing.m_lastSent )
		{
			vupHeader->present |= PR_POSITION;
			viewing.m_position = vehInfo->position;
		}
	}

	if ( _validateTurrets(viewing, vupHeader, updtIsNew) > 0 )
	{
		vupHeader->present |= PR_TURRET;
	}
}
#endif

SINT32
updtPackWORLD_UPDATE(const PackMode mode, UCHAR* data, SINT32 offset, WORLD_UPDATE* const wu, SINT32 /*size*/)
{
	TIMER_USE_HERE

		ON_SERVER(assert(wu->myVehicle != nullptr));
	ON_SERVER(VEHICLE_UPDATE_HEADER vhdr);

	TIMER_ACCUMULATOR(pack_initvh);
	TIMER_ACCUMULATOR(pack_resrv);
	TIMER_ACCUMULATOR(pack_head);
	TIMER_ACCUMULATOR(pack_xltpos);
	TIMER_ACCUMULATOR(pack_vpos);
	TIMER_ACCUMULATOR(pack_vaux);
	TIMER_ACCUMULATOR(pack_settur);
	TIMER_ACCUMULATOR(pack_tur);
	TIMER_ACCUMULATOR(pack_resize);

	size_t packetSize = 0;
	bool sizeLimitReached = false;

	for ( UINT32 vehNo = 0; (vehNo < wu->header.numVehicles) && !sizeLimitReached; ++vehNo )
	{
		ON_SERVER
		(
			Viewing& viewing = *(wu->viewings[vehNo]);

			TIMER_START
			// Build the world update by looking for changes and
			// sending deltas.
			VEHICLE_UPDATE_HEADER* const vupHeader = &vhdr;
			vupHeader->vehInfo = const_cast<SHARED_VEHICLE_INFO*>(&viewing.m_vehicle.vehicleData);
			SHARED_VEHICLE_INFO* const vehInfo = vupHeader->vehInfo;
			_initVehicleHeader(vupHeader, viewing);
			TIMER_STOP_ADD(pack_initvh);

			TIMER_START
			// Server uses a Teul::Packet to provide a dynamic buffer for
			// building the updates into. Because it's still using Teul::Pack
			// we need to fake the data pointer at the start of each iteration
			// and set offset back to zero.
			const size_t startingSize_bytes = wu->packet->payloadSizeBytes();
			wu->packet->setPayloadCapacityBytes(startingSize_bytes + 250);
			data = (UCHAR*)wu->packet->writePointer();
			offset = 0;
			TIMER_STOP_ADD(pack_resrv);
		)
		ON_CLIENT
		(
			// The client processes it.
			VEHICLE_UPDATE_HEADER* const vupHeader	= &wu->vehUpdate[vehNo];
			SHARED_VEHICLE_INFO* const vehInfo		= &vupHeader->vehInfo;
			vehInfo->resolution = 0;
		)

		TIMER_START
		offset = _packVehicleHead(mode, data, offset, vupHeader, PR_NUM_BITS, wu);
		TIMER_STOP_ADD(pack_head);
		if ( offset < 0 )
		{
			dbugLog(_FLA, "couldnt pack vehicle head for vehicle slot %d", vehNo);
			return offset;
		}

		// The vehicle's position and orientation etc.
		if ( vupHeader->present & PR_POSITION )
		{
			TIMER_START
			// On the server, shorten the position by making it relative to
			// where your vehicle was when we started this process.
			ON_SERVER(  _translatePositionToOffset(vehInfo->position, *wu, vupHeader->position)  );
			TIMER_STOP_ADD(pack_xltpos);

			TIMER_START
			offset = _packVehiclePosition(mode, data, offset, &vupHeader->position, 0);
			TIMER_STOP_ADD(pack_vpos);
			if ( offset < 0 )
			{
				dbugLog(_FLA, "couldnt pack position for vehicle slot %d", vehNo);
				return offset;
			}

			TIMER_START
			offset = _packVEHICLE_AUX_INFO(mode, data, offset, vehInfo, 0);
			TIMER_STOP_ADD(pack_vaux);
			if ( offset < 0 )
			{
				dbugLog(_FLA, "couldnt pack AUX_INFO for vehicle slot %d", vehNo);
				return offset;
			}

			// On the client, we need to re-base the position we received back
			// into an absolute position.
			ON_CLIENT(  _translateOffsetToPosition(vupHeader->position, *wu, vehInfo->position)  );

			ON_CLIENT(  vupHeader->position = vehInfo->position  );
		}

		// The vehicle's turrets.
		if ( vupHeader->present & PR_TURRET )
		{
			TURRET_HEADER turretHeader;
			turretHeader.vehInfo	= vehInfo;

			TIMER_START
			ON_SERVER(  _setTurrets(viewing, &turretHeader.vt)  );
			TIMER_STOP_ADD(pack_settur);

			TIMER_START
			offset = _packVehicleTurrets(mode, data, offset, &turretHeader, 0);
			TIMER_STOP_ADD(pack_tur);

			if ( offset < 0 )
			{
				dbugLog(_FLA, "couldnt pack turrets for vehicle slot %d", vehNo);
				return offset;
			}

			ON_CLIENT(  wu->vehUpdate[vehNo].vt.turretMask = turretHeader.vt.turretMask  );
		}

		packetSize += Teul::BitsToBytes(offset);

		ON_SERVER
		(
			// we only know this entry's size after its been packed, so check it now for busting our limit
			if (packetSize < MAX_UDP_DATA)
			{
				viewing.m_lastSent = teulClock64;
				// Increase the actual packet size.
				TIMER_START
					wu->packet->setPayloadSizeBytes(startingSize_bytes + Teul::BitsToBytes(offset));
				TIMER_STOP_ADD(pack_resize);
			}
			else
			{
				sizeLimitReached = true;
			}
		);

		assert( (offset & 7) == 0 );
	}

#if defined(DEBUG_UPDATE_PACK)
	ON_SERVER(  dbugLog(_FLD, "wu: ntp = %u, mvpf = %u, vislist = %u, nv = %u, off = %d, myMTU = %d", numToPack, framing.maxVehiclesPerFrame(), wu->myVehicle->m_visList.size(), wu->numVehicles, offset, myMTUbits)  );
#endif

	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack high precision versions of velocity/accel data.

static SINT32 _packUPDATE_HIGH(PackMode mode, UCHAR* const data, SINT32 offset, SHARED_VEHICLE_INFO* const svi, SINT32 size)
{
	offset = _packVEHICLE_VEL_HIGH(mode, data, offset, &svi->velocity, 0);
	offset = _packVEHICLE_ACCEL_HIGH(mode, data, offset, &svi->acceleration, 0);
	offset = _packVEHICLE_ATTVEL_HIGH(mode, data, offset, &svi->rotational, 0);
	offset = _packVEHICLE_ATT_HIGH(mode, data, offset, &svi->attitude, 0);

	return offset;
}

#if platBuild == BUILD_SERVER

static void _badVehicle(SINT32 vehIndex)
{
	dbugLog(_FLA, "**** INVOKED _badVehicle -- BEWARE ****");
}

static SINT32 _getBroadcastVehicle(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_HOLDER* vhold, UINT32 vehIndex)
{
	static HostVehicle tempVehicle;
	SINT32 retval;

	vhold->serial = vehIndex;

	if(vhold->present & PRX_FIRST_TIME)
	{
		// Look for the vehicle locally, incase this is an update.
		vhold->vehicle = hostFindVehicleLocal(vehIndex);
		dbugLog(_FLD, "rv:%u received first time", vehIndex);

		if (vhold->vehicle == nullptr)
		{
			// If we don't have it, then we need to create it.
			retval  = tempVehicle.packFirstTime(mode, data, offset);
			if ( retval >= 0 )
			{
				vhold->vehicle = hostCreateRemoteVehicle(tempVehicle.instance(), vehIndex, tempVehicle.spec());
				if(vhold->vehicle)
					dbugLog(_FLI, "rv:%u created remote vehicle %p", vehIndex, vhold->vehicle);
			}
			else
			{
				dbugLog(_FLA, "rv:%u couldnt unpack first time info error %d", vehIndex, retval);
				return retval;
			}
		}

		if (vhold->vehicle == nullptr || ! (vhold->vehicle->isRemote()))
		{
			if (Server::myServerInfo.stats.status == Server::SERVER_ACCEPTING)
			{
				_badVehicle(vehIndex);
			}

			dbugLog(_FLA, "rv:%u could not create vehicle node %p is not valid", vehIndex, vhold->vehicle);
			return -EINVAL;
		}
	}
	else
	{
		vhold->vehicle = hostFindVehicleLocal(vehIndex);
		if (vhold->vehicle == nullptr )
		{
			static TeulTimeMS nextBlip = teulClock64;
			if ( Server::myServerInfo.stats.status == Server::SERVER_ACCEPTING && teulClock64 >= nextBlip )
			{
				dbugLog(_FLA, "rv:%u not found", vehIndex);
				nextBlip = teulClock64 + 500;
			}
			// subsitute a dummy vehicle so we can at least unpack
			vhold->vehicle = &tempVehicle;
			vhold->vehicle->setRemote();
			vhold->serial = 0;
		}
		else if ( !vhold->vehicle->isRemote() )
		{
			LogAbort("rv:%u got broadcast for non-remote vehicle", vehIndex);
		}
	}

	return 0;
}

SINT32
updtPackVehicleBroadcast(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_HOLDER* vhold, SINT32 size)
{
	SHARED_VEHICLE_INFO* vehInfo = nullptr;
	TURRET_HEADER turretHeader;
	UINT32 vehIndex;
	SINT32 retval;

	if ( mode == TEUL_PACK)
	{
		if (vhold == nullptr || vhold->vehicle == nullptr)
			return -EINVAL;

		vehIndex = vhold->vehicle->index;
	}

	offset = teulPackUINT32(mode, data, offset, &vehIndex, 0);

	offset = teulPackUINT32(mode, data, offset, &vhold->present, PRX_NUM_BITS);

	if (mode == TEUL_UNPACK)
	{
		retval = _getBroadcastVehicle(mode, data, offset, vhold, vehIndex);
		if ( retval < 0 )
			return retval;
	}

	vehInfo = &(vhold->vehicle->vehicleData);

	// must follow _getBroadcast since it will unpack the first time as well
	if (vhold->present & PRX_FIRST_TIME)
	{
		offset = vhold->vehicle->packFirstTime(mode, data, offset);
		if ( mode == TEUL_UNPACK )
			dbugLog(_FLI, "rv:%u got broadcast one time", vehIndex);
	}

	if ( mode == TEUL_UNPACK && vhold->present & PRX_REALLY_ACTIVE )
	{
		if ( vhold->vehicle->isReallyActive() == false )
		{
			dbugLog(_FLD, "rv:%u setting remote really active", vehIndex);
			vhold->vehicle->setReallyActive(true);
		}
	}

	// handle timestamps
	offset = teulPackUINT16(mode, data, offset, &vhold->timeStamp, 0);
	offset = teulPackUINT16(mode, data, offset, &vhold->posTimeStamp, 0);

	if (mode == TEUL_UNPACK)
	{
		if (vhold->present & PR_POSITION)
		{
			// Position messages must be from the controller.
			if ( (vhold->present & PR_FROM_CONTROLLER) == 0 )
				dbugLog(_FLA, "rv:%u illegal PR_POSITION not allowed for non control player", vehIndex);

			vehInfo->hostPosTimeStamp = teulClock64;
		}

		if ( vhold->present & PR_FROM_CONTROLLER )
		{
			vehInfo->timeStamp	= vhold->timeStamp;
			vehInfo->posTimeStamp	= vhold->posTimeStamp;
			// NOTE: This was a multi-crew addition and needs to exist when sending out remote vehicle status
			vehInfo->cmdrTimeStamp  = vhold->timeStamp;
		}
	}

	//pack up common elements
	offset = _packVehicleCommon(mode, data, offset, vehInfo, vhold->present);

	if (vhold->present & PR_FIRE_FLAGS)
		offset = teulPackUINT16(mode, data, offset, &vhold->fireMask, 0);

	if ( vhold->present & PR_POSITION)
	{
		offset = teulPackVoid(mode, data, offset, &vehInfo->position, sizeof(vehInfo->position));

		offset = _packUPDATE_HIGH(mode, data, offset, vehInfo, 0);
	}

	if (vhold->present & PR_TURRET)
	{
		turretHeader.vt.numTurrets = vhold->numTurrets;
		turretHeader.vt.turretMask = vhold->turretMask;
		turretHeader.vehInfo = vehInfo;
		offset = _packVehicleTurrets(mode, data, offset, &turretHeader, 0);
	}

	if  (vhold->serial == 0)
	{
		vhold->vehicle = nullptr;
	}

	FULL_OBJ_ID inBuilding;
	if ( mode == TEUL_PACK && vhold->vehicle )
		inBuilding.FromKey(vhold->vehicle->m_inBuilding);
	offset = wwiiPackFullObjID(mode, data, offset, &inBuilding, 0);
	if ( mode == TEUL_UNPACK && vhold->vehicle )
		vhold->vehicle->setInBuilding(inBuilding);

	return offset; // returns offset for init or next routine
}
#endif

#if platBuild == BUILD_SERVER
//////////////////////////////////////////////////////////////////////
// Pack my one time header
SINT32
HostVehicle::packFirstTime(PackMode mode, UCHAR* const data, SINT32 offset)
{
	offset = _packONE_TIME_INFO(mode, data, offset, &vehicleData, 0, nullptr);
	offset = teulPackUINT32(mode, data, offset, &vehicleData.squad, 0);
	offset = wwiiPackHandle(mode, data, offset, &vehicleData.squadHandle, 0);
	offset = teulPackUINT32(mode, data, offset, &m_missionID, 0);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&m_flags, 0);

//	if (mode == TEUL_UNPACK)
//	{
//		wSerial.spec = vehicleData.vehicleType;
//	}

	offset = teulPackVoid(mode, data, offset, &seats, sizeof(UINT32) * MAX_WEAP_STATIONS);

	ACTION_OBJECT a;
	a.packed  = data;
	a.offset = offset;

	return m_instance.packer(mode, &a);
}
#endif

// client to host update packet....
SINT32 updtPackSHARED_VEHICLE_INFO(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_UPDATE_HEADER* vupHeader, SINT32 size)
{
	// On the host, packVehicleHead will look up the vehicle and set
	// vehInfo/vupHeader accordingly
	offset = _packVehicleHead(mode, data, offset, vupHeader, PRX_NUM_BITS, nullptr);
	if ( offset < 0 )
		return offset;

	ON_SERVER(  SHARED_VEHICLE_INFO* const vehInfo = vupHeader->vehInfo  );
	ON_CLIENT(  SHARED_VEHICLE_INFO* const vehInfo = &vupHeader->vehInfo  );

	if ( vupHeader->present & PR_POSITION)
	{
		offset = teulPackVoid(mode, data, offset, &vupHeader->position, sizeof(vupHeader->position));
		if ( offset < 0 )
			return offset;

		if ( mode == TEUL_UNPACK )
		{
			vehInfo->position = vupHeader->position;
		}

		offset = _packUPDATE_HIGH(mode, data, offset, vehInfo, 0);
	}

	if (vupHeader->present & PR_TURRET)
	{
		TURRET_HEADER turretHeader;

		turretHeader.vt.numTurrets = vupHeader->vt.numTurrets;
		turretHeader.vt.turretMask = vupHeader->vt.turretMask;
		turretHeader.vehInfo = vehInfo;
//		dbugWrite(_FLN, "check mask: %x\n", turretHeader->vt.turretMask);
		offset = _packVehicleTurrets(mode, data, offset, &turretHeader, 0);

		if (mode == TEUL_UNPACK)
		{
			vupHeader->vt.numTurrets = turretHeader.vt.numTurrets;
			vupHeader->vt.turretMask = turretHeader.vt.turretMask;
		}
	}

	// Always say what update we think we are on
	offset = teulPackUINT32(mode, data, offset, &(vupHeader->updateNo), 0);
	offset = teulPackUINT32(mode, data, offset, &(vupHeader->updatePrime), 0);

	if (offset < 0)
		return offset;

	// If we're a ground vehicle, we want to tell the host which building we're in
	bool isInBuilding = (vehInfo->fpsBuildingID.id != 0);
	offset = wwiiPackBool(mode, data, offset, isInBuilding);
	if ( isInBuilding )
	{
		offset = teulPackUINT16(mode, data, offset, &vehInfo->fpsBuildingID.id, 0);
		offset = wwiiPackBool(mode, data, offset, vehInfo->fpsInCaptureBuilding);
	}

#if platBuild == BUILD_SERVER
	if ( mode == TEUL_UNPACK )
	{
		if ( isInBuilding == false )
		{
			vehInfo->fpsBuildingID = nullObjID;
			vehInfo->fpsInCaptureBuilding = false;
		}
		else
		{
			// Build the fullObjID
			vehInfo->fpsBuildingID.cell.ox = (vehInfo->position.x() / P2_BASE) >> 3;
			vehInfo->fpsBuildingID.cell.oy = (vehInfo->position.y() / P2_BASE) >> 3;
			vehInfo->fpsBuildingID.objClass = vehInfo->fpsBuildingID.objType = 0;
		}
	}
#endif

	return offset; // returns offset for init or next routine
}

SINT32 updtPackCP_HIT_HEADER(PackMode mode, UCHAR* const data, SINT32 offset, CP_HIT_HEADER* hh, SINT32 size)
{
	SINT32	i;

	offset = teulPackUINT32(mode, data, offset, (UINT32*)&hh->numHits, 8);

	for (i = 0; i < hh->numHits; i++)
	{
		offset = wwiiPackFullObjID(mode, data, offset, &hh->list[i].object, 0);
		offset = teulPackSINT64(mode, data, offset, &hh->list[i].damage, 0);
	}

	return offset;
}

SINT32
WORLD_UPDATE::packRemoveList(PackMode mode, ACTION_OBJECT* const a)
{
	a->packer(mode, numRemove, 8);
	a->bytePacker(mode, removeList, sizeof(*removeList) * numRemove);

	return a->offset;
}

