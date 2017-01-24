#include <syscfg.h> // Path is project specific.
// Created: Tue Nov  8 13:28:55 CST 2005
//
// Member functions for class HostVehicle
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLocalizedMessage.h"
#include "chat/chatModReport.h"
#include "chat/chatName.h"
#include "chat/chatSupportRPCMessages.h"
#include "comm/commHostMessages.h"
#include "comm/commTypes.h"
#include "host/hostCellNet.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "host/hostVehicleImpl.h"
#include "host/hostVehicleViewing.h"
#include "include/arenaLockEnums.h"
#include "inst/instDescriptor.h"
#include "score/scoreDeath.h"
#include "score/scorePacket.h"
#include "score/scoreResupply.h"
#include "server/serverIndexes.h"
#include "strt/strtCaptureScore.h"
#include "strt/strtCaptureState.h"
#include "strt/strtConstants.h"
#include "strt/strtCPColliders.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "teul/teulPacket.h"
#include "updt/updtTypes.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiThreads.h"

std::map<ChatHandle, HostVehicle*> g_vehicleLookup;

//////////////////////////////////////////////////////////////////////
// Look up a vehicle by it's chat handle.

HostVehicle*
HostVehicle::Lookup(const ChatHandle handle)
{
	auto it = g_vehicleLookup.find(handle);
	return ( it != g_vehicleLookup.end() ? it->second : nullptr );
}

//////////////////////////////////////////////////////////////////////
// Reset a vehicle implementation to base values

void
HostVehicleImpl::reset()
{
	m_players.clear();
	memset(&m_locality, 0, sizeof(m_locality));
	m_nearCP = nullptr;

	m_resupplies.clear();

	m_lastResupplier = 0;

	m_stopped = 0;

	m_captureActive = false;
	m_lastCaptureFacility = 0;
	resetCaptureTick();
}

//////////////////////////////////////////////////////////////////////
// Forward the capture ticker to prevent this player from
// contributing to capture for a little while.

void
HostVehicleImpl::resetCaptureTick()
{
	// make up for fact we are sending 10 times per second and being off just slightly will result in a late arrival
	TeulTimeMS tickInterval = (TeulTimeMS)host_config.capture.tickInterval_s * 1000 - 100;
	// We need some kind of minimum interval, I chose 250ms.
	if ( tickInterval == 0 )
		tickInterval = 250;
	m_nextCaptureTick += tickInterval;
}

//////////////////////////////////////////////////////////////////////
// Destructor for a host vehicle impl

HostVehicleImpl::~HostVehicleImpl()
{
	if ( !m_players.empty() )
	{
		dbugLog(_FLW, "Deleting vehicle with non-empty player list");
	}
}

//////////////////////////////////////////////////////////////////////
// Return the capture progress of the player's current capture
// building, or -1 if not applicable.

SINT32
_getCaptureProgress(const HostVehicle& vehicle)
{
	if ( vehicle.m_isInCaptureBuilding == false || vehicle.m_inBuildingObject == nullptr )
		return -1;
	const FacilityStruct* const facility = vehicle.m_inBuildingObject->facility;
	if ( facility == nullptr || facility->isCaptureEnabled() == false )
		return -1;

	return facility->m_captureCounter;
}

//////////////////////////////////////////////////////////////////////
// Check to see if this vehicle needs to send locality updates for its
// players.
///CAVEAT: Since multicrew is fairly uncommon, for now we send
/// individual per-player updates

void
HostVehicle::checkLocality(Teul::Packet& packet)
{
#ifdef DEV_BUILD
	assert( isRemote() == false );
#endif

	if ( (m_nextLocUpdate > teulClock64) | (m_despawnTimerActive == true) | (isActive() == false) | (m_impl->m_players.empty() == true) )
		return;
	if ( exitState != PEXIT_UNINITIALIZED )
		return;

	bool firstUpdate = (m_nextLocUpdate == 0LL);

	// Interval for sending movement updates
	static const TeulTimeMS _UPDATE_INTERVAL_MS = 1000;
	m_nextLocUpdate = teulClock64 + _UPDATE_INTERVAL_MS;

	// Check if the position has moved but not trivially.
	const FLOAT32 x_m = (FLOAT32)x_cm() / 100;
	const FLOAT32 y_m = (FLOAT32)y_cm() / 100;
	const FLOAT32 z_m = (FLOAT32)z_cm() / 100 - 2000;

	LocalityUpdate& loc = m_impl->m_locality;
	const bool posChanged = (
			firstUpdate
			|| x_m != loc.m_x
			|| y_m != loc.m_y
			);

	const SINT32 captureProgress = _getCaptureProgress(*this);
	const bool bldgChanged = (
			firstUpdate
			|| m_isInCaptureBuilding != loc.m_inCaptureBldg
			|| captureProgress != loc.m_progress
			);

	if ( !posChanged && !bldgChanged && z_m == loc.m_z )
		return;

	// Default changed to whether or not we're doing our first update
	loc.m_cpChanged = firstUpdate;

	if ( posChanged )
	{
		CPStruct* nearestCP = nullptr;
		if ( m_inBuildingObject && m_inBuildingObject->facility )
			nearestCP = m_inBuildingObject->facility->cp;
		if ( nearestCP == nullptr )
			nearestCP = strtFindClosestCP((SINT32)x_m, (SINT32)y_m);

		if ( firstUpdate || nearestCP != m_impl->m_nearCP )
		{
#ifdef BETA_RELEASE
			if ( nearestCP )
				dbugLog(_FLD, "v:%u c:%s approaching area of %s"
							, index
							, callsign()
							, nearestCP->name);
			else if ( m_impl->m_nearCP )
				dbugLog(_FLD, "v:%u c:%s departing area of %s"
							, index
							, callsign()
							, m_impl->m_nearCP->name);
			else
				dbugLog(_FLD, "v:%u c:%s spawned in the wilds"
							, index
							, callsign()
							);
#endif
			m_impl->m_nearCP = nearestCP;
			loc.m_cpChanged = true;
			loc.m_nearCP = nearestCP ? nearestCP->fullObjID : nullObjID;
		}
	}
	if ( bldgChanged )
	{
		loc.m_inCaptureBldg = m_isInCaptureBuilding;
		if ( loc.m_inCaptureBldg && m_inBuildingObject )
		{
			loc.m_captureFac = m_inBuildingObject->facility->fullObjID;
			loc.m_progress = captureProgress;
		}
		else
		{
			loc.m_captureFac = nullObjID;
			loc.m_progress = -1;
		}
	}

	if ( posChanged )
	{
		loc.m_x = x_m;
		loc.m_y = y_m;
	}
	loc.m_z = z_m;
	loc.m_riding = vehicleData.rideIndex;
	loc.m_onAWS = false;

	for ( auto it = m_impl->m_players.begin(), end = m_impl->m_players.end(); it != end; ++it )
	{
		Player* player = *it;
		if ( player->exitState != 0 )
			continue;

		loc.m_lastPing = 0;

		if ( player->m_onAWS == true )
			loc.m_onAWS = true;

		loc.m_playerID = player->playerID();
		packet.append(loc);
	}
}

//////////////////////////////////////////////////////////////////////
// Reset/clear all data.

void
HostVehicle::initialize(VEHICLE_HOLDER* const _holder, HostVehicleImpl* const _impl, UINT32 vehicleID, const WEAPON_SERIAL& ws, const WEAPON* weapon)
{
	memset(this, 0, sizeof(*this));

	m_holder = _holder;
	m_impl = _impl;
	m_impl->reset();
	vehicleData.m_vehicle = this;

	index = vehicleID;
	state = VEH_RESERVED;
	serial() = ws;
	m_bias = weapon->bias;
	friendlyBias = weapon->m_friendlyBias;
	enemyBias = weapon->m_enemyBias;

	m_side = (WWII_SIDE)ws.spec.side();
	vehicleData.country = ws.spec.wCountry;
	vehicleData.side = side();
	m_holder->timeStamp = teulClock();
	m_spawnTime = teulTime();
	m_cell = nullptr;

//	m_nextUpdateClock = ~0ULL;

	static const WEAPON_SPEC buzzardSpec = weapFindWeaponByName("uk_buzzard");
	static const WEAPON_SPEC skullSpec = weapFindWeaponByName("uk_skull");
	m_isBuzzard = ( serial().spec.wCategory == buzzardSpec.wCategory && serial().spec.wClass == buzzardSpec.wClass
					&& (serial().spec.wType == buzzardSpec.wType || serial().spec.wType == skullSpec.wType) );

	static const WEAPON_SPEC fighterSpec = weapCreateSpec("AC_fighter.0");	///THREAD UNSAFE
	m_isFighter = ( serial().spec.wCategory == fighterSpec.wCategory && serial().spec.wClass == fighterSpec.wClass );

	m_maxLod_cm2 = weapon->maxLod_cm2;

//	syncTime = 0;

	// Set the update framing context
	selectBestFramingMode();

	g_vehicleLookup[chatHandle()] = this;
}

//////////////////////////////////////////////////////////////////////
// Accept a resupply injection

void
HostVehicle::acceptResupply(UINT32 fromSerial, const ChatHandle fromHandle, HostVehicle* fromVehicle, const UINT32 fromAddress)
{
	if ( players().empty() )
	{
		dbugLog(_FLD, "v:%u no players to receive resupply", index);
		return;
	}

	// Remember the last people to resupply me.
	m_impl->m_lastResupplier = fromSerial;

    Player* vehCommander = activeCommander();
    UINT32 id = (vehCommander != nullptr) ? vehCommander->playerID() : 0;
    dbugLog(_FLD, "p:%u c:%s v:%u accepting resupply from %s"
                    , id, callsign(), index
                    , Chat::Name(fromHandle).c_str()
                    );

	commSendClientResupplyIssue(this, fromSerial, fromHandle);

	if ( fromAddress != 0 )
		hostSendResupplyAccepted(fromAddress, fromSerial, vehicleData.packedCallsign);
	else if ( fromVehicle != nullptr )
		fromVehicle->scoreResupply(vehicleData.packedCallsign);
}

//////////////////////////////////////////////////////////////////////
// Attach the controlling player to a vehicle, and use their details
// to populate various vehicle fields.

void
HostVehicle::attachControllingPlayer(const Player& player)
{
	assert( isRemote() == false );

	dbugLog(_FLI, "t:%d p:%u c:%s attaching controller to v:%u spec:%u.%u.%u.%u sq:%u in:%u.%u.%" PR64BitUT ".%" PR64BitUT ""
					, player.teulIndex(), player.playerID(), player.callsign(), index
					, spec().wCountry, spec().wCategory, spec().wClass, spec().wType
					, player.squadID()
					, player.instance().category(), player.instance().type(), player.instance().handle(), player.instance().id()
					);

	// Clone this player's name.
	vehicleData.packedCallsign = player.chatHandle();
	chatGetCallsign(vehicleData.callsign, vehicleData.packedCallsign);

	// Transfer me to the instance.
	setInstance(player.instance());

	///TODO:134: Assign sortie properties from the mission packet.

	// Copy other attributes.
	vehicleData.rank = player.effectiveRank();
	vehicleData.squad = player.squadID();
	vehicleData.squadHandle = player.squadHandle();
	vehicleData.brigadeID = player.brigadeID();
	m_missionID = player.missionID();
	m_flags = player.m_flags;

	// Reset the update framing for this vehicle
	selectBestFramingMode();
}

//////////////////////////////////////////////////////////////////////
// Can this vehicle see hidden buzzards?

bool
HostVehicle::canSeeHiddenBuzzards() const
{
	const Player* const cmdr = commander();
	return ( cmdr != nullptr && cmdr->isEffectiveGM() == true );
}

//////////////////////////////////////////////////////////////////////
// Return a pointer to the commander of the vehicle's player or NULL

Player*
HostVehicle::activeCommander() const
{
	PLAYER_LIST::const_iterator it;
	for ( it = players().begin(); it != players().end(); ++it )
	{
		Player* player = (*it);
		if ( player != nullptr && player->teulIndex() >= 0 && player->active && player->station != UPDT_OTHER )
			return player;
	}

	return nullptr;
}

Player*
HostVehicle::commander() const
{
	PLAYER_LIST::const_iterator it;
	for ( it = players().begin(); it != players().end(); ++it )
	{
		Player* player = (*it);
		if ( player != nullptr && player->station != UPDT_OTHER )
			return player;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// How many seconds before the vehicle can despawn?

static UINT32
_despawnTimeLimit(const WEAPON_SPEC spec, bool isInBuilding)
{
	// Arena sets zero despawn time.
	if ( host_config.isArenaOpen() == false )
		return 0;

	UINT32 baseDespawnTime = host_config.arena.despawnTime_s;
	if ( baseDespawnTime == 0 )
		return baseDespawnTime;

	WEAPON* weapon = weapLookup(spec);
	if ( weapon != nullptr && weapon->access() >= 8 )
	{
		// System vehicles can instantly despawn
		return 1;
	}

	// Halve despawn time for people in buildings
	if ( isInBuilding )
		baseDespawnTime = (baseDespawnTime + 1) / 2;

	return baseDespawnTime;
}

DespawnTimer&
HostVehicle::despawnTime() const
{
	static DespawnTimer timer;
	timer.reset();

	///CAVEAT: Non-campaign players and dead troopers despawn upon hitting 'ESC"
	if ( instance().isCampaign() == false || (state == VEH_DEAD && spec().wCategory == _INFANTRY) )
	{
		timer.m_despawn = 0;
		return timer;
	}

	// Timer must be non-zero for us to respond
	if ( !m_despawnTimerActive )
		return timer;

	// Return the number of ms
	TeulDeltaMS elapsed = (teulClock64 - m_despawnStarted) / 1000;

	TeulDeltaMS despawnRemaining = _despawnTimeLimit(spec(), isInBuilding()) - elapsed;
	TeulDeltaMS promoteRemaining = host_config.despawnPromotionTime_s - elapsed;

	timer.m_despawn = (SINT32)((despawnRemaining > 0) ? despawnRemaining : 0);
	if ( host_config.despawnPromotionTime_s > 0 && state != VEH_DEAD )
		timer.m_promotion = (SINT32)((promoteRemaining > 0) ? promoteRemaining : 0);

	return timer;
}

//////////////////////////////////////////////////////////////////////
// Forward a resupply message to the relevant client.

void
HostVehicle::issueResupply(const Player* const player, UINT32 toSerial)
{
	assert( toSerial != 0 );
	if ( toSerial == index )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s v:%u attempting to resupply themself!"
						, player->teulIndex(), player->playerID(), player->callsign(), index);
		Chat::SendModReport(player->matchHandle(), Chat::ModResupply);
	}

	HostVehicle* toVehicle = hostFindVehicleLocal(toSerial);
	if ( toVehicle == nullptr )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s v:%u invalid vehicle %u (gone away?)"
						, player->teulIndex(), player->playerID(), player->callsign(), index, toSerial);
		return;
	}

	if ( toVehicle->isRemote() == true )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s v:%u forwarding resupply #%u to cell %u"
						, player->teulIndex(), player->playerID(), player->callsign(), index, toSerial, toVehicle->serverAddress & 255);
		hostSendResupplyIssue(toVehicle->serverAddress, toSerial, index, player->chatHandle());
	}
	else
	{
		dbugLog(_FLI, "t:%d p:%u c:%s v:%u handing resupply to %u"
						, player->teulIndex(), player->playerID(), player->callsign(), index, toSerial);
		toVehicle->acceptResupply(index, player->chatHandle(), this, 0);
	}
}

//////////////////////////////////////////////////////////////////////
// Calculate how many vehicles I am currently allowed to see

UINT32
HostVehicle::maxVisibleVehicles() const
{
	// Start with what their update mode allows.
	return framings[framingMode].maxVehicles;
}

//////////////////////////////////////////////////////////////////////
// Return the player lists for a vehicle

const PLAYER_LIST& HostVehicle::players() const
{
	return m_impl->m_players;
}

PLAYER_LIST& HostVehicle::players()
{
	return m_impl->m_players;
}

//////////////////////////////////////////////////////////////////////
// Accumulate score for resupply actions

void
HostVehicle::scoreResupply(const ChatHandle ofHandle)
{
	for ( auto pit = players().begin(); pit != players().end(); ++pit )
	{
		const Player* const player = *pit;
		Scoring::Packet<Scoring::Resupply> packet;
		Scoring::Actor actSupplier(this, player);
		Scoring::Resupply evtResup(actSupplier, ofHandle);
		packet.setData(evtResup);
		serverSendPacketToType(packet, MAP_HOST);
	}
}

//////////////////////////////////////////////////////////////////////
// Flag the crew of the vehicle as having died

void
HostVehicle::setCrewDead()
{
	state = VEH_DEAD;
	vehicleData.globalFlags |= VEHG_DEAD;
	timeOfDeath = teulTime();

	interruptCapture();
}

//////////////////////////////////////////////////////////////////////

void
HostVehicle::setDespawnTimer(bool wantOn)
{
	if ( wantOn && !m_despawnTimerActive )
	{
		m_despawnStarted = teulClock64;
	}
	if ( wantOn != m_despawnTimerActive )
	{
		m_despawnTimerActive = wantOn;
#ifdef BETA_RELEASE
		dbugLog(_FLI, "v:%u c:%s %s despawn timer"
						, index
						, callsign()
						, (m_despawnTimerActive ? "starting" : "stopping")
						);
#endif
		interruptCapture();
	}
}

//////////////////////////////////////////////////////////////////////
// Interrupt the capture process.

void
HostVehicle::interruptCapture()
{
	if ( m_impl->m_captureActive )
	{
		dbugLog(_FLD, "p:%u c:%s v:%u interrupting capture"
						, seats[0], callsign(), index);

		// Determine which status to send.
		Strat::CaptureContributionEvent::Status status = Strat::CaptureContributionEvent::Interrupted;

		if ( state == VEH_DEAD )
			status = Strat::CaptureContributionEvent::Killed;
		else if ( m_isInCaptureBuilding == false )
			status = Strat::CaptureContributionEvent::Left;

		sendCaptureContribution(activeCommander(), status);
	}
	else
		m_impl->resetCaptureTick();
}

//////////////////////////////////////////////////////////////////////
// Set my instance

void
HostVehicle::setInstance(const Instance::Descriptor& _instance)
{
    dbugLog(_FLD, "v:%u setting instance %u.%u.%" PR64BitUT ".%" PR64BitUT ""
                , index
                , _instance.category(), _instance.type(), _instance.handle(), _instance.id());
	m_instance = _instance;
}

//////////////////////////////////////////////////////////////////////
// Test whether we need to hide a GM callsign

bool
HostVehicle::getHideCallsign(const HostVehicle* const viewer) const
{
	if ( this == viewer )
		return false;
	if ( this->isBuzzard() == false || viewer->isBuzzard() == true )
		return false;

	const Player* const player = viewer->activeCommander();
	if ( player != nullptr && player->stationID == UPDT_LONE && (player->permissions(IS_PLAYNET | GAME_MANAGER)) )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set and return the one time bits according to a particular viewer

UINT32
HostVehicle::getOneTimeBitsFor(const HostVehicle* const viewer) const
{
	UINT32 oneTimeBits = 0;

	const bool hideCallsign = getHideCallsign(viewer);
	if ( (viewer->side() == side() && hideCallsign == false) || viewer->isBuzzard() )
	{
		if ( squadID() > 0 && squadID() == viewer->squadID() )
			oneTimeBits |= OTB_IN_SQUAD;
	}
	if ( viewer->side() == side() )
	{
		if ( brigadeID() > 0 && brigadeID() == viewer->brigadeID() )
			oneTimeBits |= OTB_IN_BRIGADE;
		if ( missionID() > 0 && missionID() == viewer->missionID() )
			oneTimeBits |= OTB_IN_MISSION;
	}

	const Player* const cmdr = commander();
	if ( cmdr )
	{
		if ( cmdr->isTrainer() )
			oneTimeBits |= OTB_TRAINER;
		if ( cmdr->isMissionLeader() )
			oneTimeBits |= OTB_MISSION_LEADER;
		if ( cmdr->isBuilder() )
			oneTimeBits |= OTB_BUILDER;
		if ( cmdr->isOfficer() )
			oneTimeBits |= OTB_HIGH_COMMAND;
	}

	if ( vehicleData.decal > 0 )
		oneTimeBits |= OTB_USE_DECALS;

	return oneTimeBits;
}

//////////////////////////////////////////////////////////////////////
// Flag the vehicle itself as being dead

bool
HostVehicle::setVehicleKilled(sortieid_t bySortieID)
{
	if ( (vehicleData.globalFlags & VEHG_KILLED) != 0 )
	{
		return false;
	}

	vehicleData.globalFlags |= VEHG_KILLED;

	if ( (bySortieID != 0) & (m_killedBySortieID == 0) )
	{
		m_killedBySortieID = bySortieID;
	}

	Scoring::Packet<Scoring::Death> packet;
	Scoring::Actor actVictim{this};
	Scoring::Actor actShooter{};
	actShooter.setSortie(m_killedBySortieID, NULL_WEAP_SPEC, false);
	if ( m_killedBySortieID != 0 )
	{
		const HostVehicle* const killer = hostFindVehicleLocal(m_killedBySortieID);
		if ( killer != nullptr )
		{
			actShooter = Scoring::Actor(killer);
		}
	}

	Scoring::Death evtDeath(actVictim, actShooter, x_cm(), y_cm(), z_cm());
	packet.setData(evtDeath);

	if ( !serverSendPacketToType(packet, MAP_HOST) )
	{
		dbugLog(_FLA, "unable to send kill message to map host");
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Check to see if this vehicle is stopping/has stopped

bool
HostVehicle::isMoving() const
{
	if ( vehicleData.velocity.velX != 0 || vehicleData.velocity.velY != 0	|| vehicleData.velocity.velZ != 0 )
	{
		// 5 seconds to be considered at a stop.
		m_impl->m_stopped = teulTime() + 5;
		return true;
	}
	else
	{
		return ( teulTime() < m_impl->m_stopped );
	}
}

//////////////////////////////////////////////////////////////////////
// Check if I am inside a capture building

WWII_SIDE
HostVehicle::isInCaptureBuilding() const
{
	if ( !m_inBuilding || !m_isInCaptureBuilding || m_inBuildingObject == nullptr || m_inBuildingObject->m_isCaptureBuilding == false )
		return WWII_NO_SIDE;

	if ( m_inBuildingObject->facility == nullptr )
	{
		dbugLog(_FLA, "XXX capture building object isn't tied into strat");
		return WWII_NO_SIDE;
	}

	if ( m_inBuildingObject->facility->canBeCaptured() == false )
		return WWII_NO_SIDE;

	return m_inBuildingObject->m_side;
}

static void _reportBuildingIssue(TeulTimeMS& nextBlip, const char* const fmt, ...) FORMAT_FN(2, 3);
static void _reportBuildingIssue(TeulTimeMS& nextBlip, const char* const fmt, ...)
{
	assert( fmt != nullptr );
	if ( nextBlip > teulClock64 )
		return;

	char error[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(error, sizeof(error)-1, fmt, args);
	va_end(args);

	dbugLog(_FLA, "%s", error);
	chatSendAdminMessage("%s", error);

	nextBlip = teulClock64 + 5000;
}

//////////////////////////////////////////////////////////////////////
// Set the impl's inCaptureBuilding flag

void
HostVehicle::setInCaptureBuilding(bool newValue)
{
	if ( newValue == true && m_inBuildingObject == nullptr )
	{
		if ( m_inBuilding == 0ULL )
		{
			static TeulTimeMS nextBlip = 0ULL;
			_reportBuildingIssue(nextBlip, "p:%u c:%s v:%u client reporting 'in-capture' with a null building ID", seats[0], vehicleData.callsign, index);
			newValue = false;
		}
		else if ( m_inBuildingObject == nullptr )
		{
			static TeulTimeMS nextBlip = 0ULL;
			FULL_OBJ_ID bid;
			bid.FromKey(m_inBuilding);
			_reportBuildingIssue(nextBlip, "p:%u c:%s v:%u client reporting 'in-capture' from unrecognized building " PRF_FULL_OBJ_ID ""
											, seats[0], vehicleData.callsign, index, PRA_FULL_OBJ_ID(bid));
			newValue = false;
		}
		else if ( m_inBuildingObject->facility == nullptr )
		{
			FULL_OBJ_ID bid;
			bid.FromKey(m_inBuilding);
			dbugLog(_FLW, "p:%u c:%s v:%u client reporting in-capture for non-strat building " PRF_FULL_OBJ_ID " %s"
											, seats[0], vehicleData.callsign, index, PRA_FULL_OBJ_ID(bid), m_inBuildingObject->name);
			newValue = false;
		}
		else if ( m_inBuildingObject->facility->m_captureBuilding != m_inBuildingObject )
		{
			static TeulTimeMS nextBlip = 0ULL;
			const FacilityStruct* const fac = m_inBuildingObject->facility;
			const ObjectStruct* const obj = fac->m_captureBuilding;
			_reportBuildingIssue(nextBlip, "p:%u c:%s v:%u client reporting in-capture for non-capture building. inbldg:%s (%d,%d.%u), fac:%s (%d,%d.%u), capbldg:%s (%d,%d.%u)"
											, seats[0], vehicleData.callsign, index
											, m_inBuildingObject->name, m_inBuildingObject->fullObjID.cell.ox, m_inBuildingObject->fullObjID.cell.oy, m_inBuildingObject->fullObjID.id
											, fac->name, fac->fullObjID.cell.ox, fac->fullObjID.cell.oy, fac->fullObjID.id
											, obj->name, obj->fullObjID.cell.ox, obj->fullObjID.cell.oy, obj->fullObjID.id
											);
			newValue = false;
		}
	}

	if ( m_isInCaptureBuilding != newValue )
	{
		m_isInCaptureBuilding = newValue;
		interruptCapture();

		// Did we just enter a building?
		if ( m_isInCaptureBuilding && m_inBuildingObject->facility != nullptr )
		{
#ifdef BETA_RELEASE
			message(LSTR_BETA, LSTR_GENERIC, "Entered capture building");
#endif
			dbugLog(_FLD, "p:%u c:%s v:%u entered %s cap bldg"
							, seats[0], callsign(), index
							, m_inBuildingObject->facility->name
							);
			// Send them a CaptureState message.
			CaptureState capState;
			capState.populate(m_inBuildingObject->facility, side(), 0, false);
			sendCaptureState(capState);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Send a capture contribution event message to the strat host.

void
HostVehicle::sendCaptureContribution(Player* const player, UINT32 status)
{
	if ( player == nullptr || host_config.isArenaLocked(AL_CAPTURE) )
		return;

	if ( m_inBuildingObject )
		m_impl->m_lastCaptureFacility = m_inBuildingObject->facility->uOID;
	UINT32 facilityOID = m_impl->m_lastCaptureFacility;
	if ( !m_inBuildingObject )
		m_impl->m_lastCaptureFacility = 0;

	if ( facilityOID == 0 )
	{
		if ( m_inBuildingObject )
		{
			dbugLog(_FLD, "p:%u c:%s v:%u 0 facilityID for capture"
							, seats[0], callsign(), index
							);
		}
		return;
	}

	// Tell the strat host to contribute this player to the capture
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_CAPTURE_CONTRIB_Y));
	outgoing.construct<Strat::CaptureContributionEvent>(
							(Strat::CaptureContributionEvent::Status)status
							,	facilityOID
							,	player->chatHandle()
							,	player->playerID()
							,	index
							,	player->missionID()
							,	player->brigadeID()
						);
	serverSendPacketToType(outgoing, STRAT_HOST);
	dbugLog(_FLD, "p:%u c:%s v:%u 0 sending cap contrib for %s"
				, seats[0], callsign(), index
				, m_inBuildingObject && m_inBuildingObject->facility
					? m_inBuildingObject->facility->name
					: "UNKNOWN!"
				);

	m_impl->m_captureActive = ( status == Strat::CaptureContributionEvent::Active );

	m_impl->resetCaptureTick();
}

//////////////////////////////////////////////////////////////////////
// Check if a player is contributing to the capture of a facility

void
HostVehicle::checkCapture()
{
	// Must be in a capturable building, and the client must think
	// they are in a qualifying part of the building
	if ( isInCaptureBuilding() == WWII_NO_SIDE )
		return;

	// No contribution if you are dead or have the despawn timer up.
	if ( state == VEH_DEAD || m_despawnTimerActive )
		return;

	Player* const player = activeCommander();
	if ( player == nullptr )
		return;

	// Must have the "CAN_CAPTURE" permission to capture stuff.
	if ( player->permissions(CAN_CAPTURE) == false )
		return;

	// Constrain frequency of contributions
	if ( m_impl->m_nextCaptureTick > teulClock64 )
		return;

	// Have we been told the building is capturable?
	FacilityStruct& fac = *m_inBuildingObject->facility;
	switch ( fac.m_captureStage )
	{
	case FacilityStruct::_CAP_DISABLED:
#ifdef CLOSED_BETA
//		player->message(LSTR_BETA, LSTR_GENERIC, "Facility is not currently capturable");
#endif
	break;

	case FacilityStruct::_CAP_PENDING:
#ifdef BETA_RELEASE
//		player->messagef(LSTR_BETA, LSTR_GENERIC, "Facility is not capturable for another %u seconds", fac.getCaptureRemaining());
#endif
		if ( m_inBuildingObject->facility->getCaptureRemaining() > 0 )
		{
			CaptureState capState;
			capState.populate(m_inBuildingObject->facility, side(), 0, false);
			sendCaptureState(capState);
		}
	break;

	case FacilityStruct::_CAP_ENABLED:
	{
#ifdef CLOSED_BETA
//		player->messagef(LSTR_BETA, LSTR_GENERIC, "Facility is at ~%u%%, sending capture contribution", fac.m_captureCounter);
#endif
		sendCaptureContribution(player, Strat::CaptureContributionEvent::Active);

		return;
	}

	default:
#ifdef BETA_RELEASE
		player->message(LSTR_BETA, LSTR_GENERIC, "Facility in unknown capture state, not attempting capture submission");
#endif
	break;
	}

	interruptCapture();
}

//////////////////////////////////////////////////////////////////////
// Send a message to all the players in a vehicle.

bool
HostVehicle::message(int prefix, int msgid, const char* const t1, const char* const t2) const
{
	Chat::LocalizedMessage lmsg(prefix, msgid, t1, t2);
	PLAYER_LIST::const_iterator pit;
	for ( pit = players().begin(); pit != players().end(); ++pit )
	{
		if ( *pit )
			(*pit)->message(lmsg);
	}
	return true;
}

namespace Host { namespace Vehicles {

// A struct that combines all three components of a
// vehicle record: holder, vehicle and impl. This helps
// keep all 3 together in memory.
struct FullVehicle
{
	VEHICLE_HOLDER	holder;
	HostVehicle		vehicle;
	HostVehicleImpl	impl;
};

// The free list.
using FREE_VEHICLES = std::vector<FullVehicle*>;
static FREE_VEHICLES _freeVehicles;
static std::vector<FullVehicle*> _allocations;

	//////////////////////////////////////////////////////////////////////
	// Allocate an array of FullVehicles.

	void
	AllocateArray(UINT32 num)
	{
		FullVehicle* const data = new FullVehicle[num];
		_allocations.push_back(data);

		_freeVehicles.resize(_freeVehicles.size() + num);
		for ( UINT32 i = 0; i < num; ++i )
		{
			FullVehicle& entry = data[i];

/*
			memset(&entry.holder, 0, sizeof(entry.holder);
			entry.holder.vehicle = &entry.vehicle;
			entry.vehicle.holder = &entry.holder;
			entry.impl.m_vehicle = &entry.vehicle;
*/

/*
			entry.vehicle.prev = &entry.vehicle;
*/

			_freeVehicles[_freeVehicles.size() - (i + 1)] = &entry;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Release all the memory we allocated.

	void
	FreeArray()
	{
		for ( auto it = _allocations.begin(); it != _allocations.end(); ++it )
		{
			delete [] *it;
		}
		_allocations.clear();
	}

	//////////////////////////////////////////////////////////////////////
	// Return a free Vehicle Holder for me.

	VEHICLE_HOLDER*
	GetHolder(UINT32 index, const WEAPON_SERIAL& ws, const WEAPON* const weapon)
	{
		// Get the last entry.
		while ( _freeVehicles.empty() == true )
		{
			AllocateArray(256);
		}
		FullVehicle* const entry = _freeVehicles.back();
		_freeVehicles.pop_back();

		memset(&entry->holder, 0, sizeof(entry->holder));
		entry->holder.vehicle = &entry->vehicle;

		entry->vehicle.initialize(&entry->holder, &entry->impl, index, ws, weapon);

		return &entry->holder;
	}

	//////////////////////////////////////////////////////////////////////
	// Return a full-vehicle holder to the free list.

	void
	FreeHolder(VEHICLE_HOLDER* const holder)
	{
		_freeVehicles.push_back((FullVehicle*)holder);
	}

} }

