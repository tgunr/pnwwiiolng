#include <syscfg.h> // Path is project specific.
// This module provides thee main updater loop
// for the game host;
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "include/clustercfg.h"

#include "versionNo.h"
#include <stack>

#include "chat/chatCellHostSupportMessages.h"
#include "chat/chatCellXfer.h"
#include "chat/chatConstants.h"
#include "chat/chatDirectMessage.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatModReport.h"
#include "chat/chatName.h"
#include "chat/chatSupportRPCMessages.h"
#include "comm/commConstants.h"
#include "comm/commHostMessages.h"
#include "comm/commTypes.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseWrapper.h"
#include "host/hostAntiHack.h"
#include "host/hostAutomation.h"
#include "host/hostCellNet.h"
#include "host/hostFraming.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostStats.h"
#include "host/hostTimeUpdate.h"
#include "host/hostTiming.h"
#include "host/hostVehicle.h"
#include "host/hostVehicleRequest.h"
#include "hostproj/hostversion.h"
#include "include/wthrTime.h"
#include "inst/instHostInstance.h"
#include "miss/missHitArrayNode.h"
#include "miss/missionTypes.h"
#include "mm/mmCellHostSupportMessages.h"
#include "mm/mmPack.h"
#include "score/scoreCapture.h"
#include "score/scoreDespawn.h"
#include "score/scoreGuard.h"
#include "score/scorePacket.h"
#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverIndexes.h"
#include "server/serverInitializeProcess.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverInterface.h"
#include "server/serverMessages.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverPersonaXfer.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverServerInfo.h"
#include "server/serverTypes.h"
#include "strt/strtCaptureState.h"
#include "strt/strtCPColliders.h"
#include "strt/strtData.h"
#include "strt/strtEvents.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtSyncSupportMessages.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdater.h"
#include "strt/strtUpdaterHostMessages.h"
#include "strt/strtUpdaterSupportMessages.h"
#include "teul/teulBlipRepeater.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpoint.h"
#include "teul/teulHousekeeper.h"
#include "teul/teulKit.h"
#include "teul/teulPacket.h"
#include "teul/teulTickSample.h"
#include "teul/teulTransmission.h"
#include "teul/teulUtils.h"
#include "updt/updtDeathByBuilding.h"
#include "updt/updtPackers.h"
#include "weap/weapList.h"
#include "wwii/wwiiActivate.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHash.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiHostMessages.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiRTBCodes.h"
#include "wwii/wwiiSortieManager.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"

extern SINT32 hostInstallAllCallbacks();

DBCONNECTION dbConnection;

extern std::map<ChatHandle, HostVehicle*> g_vehicleLookup;

namespace Host
{
	Stats totalStats, currentStats;
}

#ifdef DEBUG_UPDATES
UINT32 remoteVehicles = 0;
UINT32 vehiclesSkipped = 0;
UINT32 totalVehicles = 0;
UINT32 qualifiedVehicles = 0;
UINT32 vehiclesReviewed = 0;
UINT32 vehUpdatesSent = 0;
UINT32 vehMcrewCommanderUpdatesSent = 0;
UINT32 vehMcrewCrewUpdatesSent = 0;
UINT32 xmissionsFailed = 0;
UINT32 vehiclesThrottled = 0;
UINT32 numberOfUpdatesBehind = 0;
UINT32 numVehiclesReported = 0;
#endif

#ifdef DEBUG
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
    #define DEF_ERR_FLAGS LOG_ALL_TO_STDERR
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
    #define DEF_ERR_FLAGS STD_ERR
#endif

PLAYERS g_players;
#include "teul/teulConfigVar.h"
extern ConfigUint cv_maxCellPlayers;
extern ConfigAddr cv_baseUpdtAddress;
static SINT32 _userConnections = 0;

char hostname[32];
UINT32 _stratHost = 0;

SINT32 updateBcastSend = -1, updateBcastRecv = -1;

static UINT32				_personaCounter;
static UINT32				_vehicleCounter;

/*
static SINT32 iterationTime = -1, iterationLast = -1;
*/

#include <unordered_map>
typedef std::unordered_map<sortieid_t, class HostVehicle*> VEHICLE_HASH;
static VEHICLE_HASH _vehicleHash;

typedef std::vector<HostVehicle*> VEHICLES;

// Tracking of players in a given building
typedef std::set<HostVehicle*> OCCUPANTS;
struct Occupancy
{
	Occupancy() : object(nullptr), occupants() {}
	ObjectStruct*	object;
	OCCUPANTS		occupants;
};
typedef std::unordered_map<ChatHandle, Occupancy> OCCUPANCY_MAP;
static OCCUPANCY_MAP occupancyMap;

// Set this to true to prevent logins to the server.
bool g_serverReserved = false;
static std::atomic<bool> g_exitHandlerRequired;
static std::atomic<bool> g_hostTerminating;
// True while we are allowing incoming connections.
static bool g_accepting = false;

static SINT32 _releaseVehicleList(Player& player);
static void _releaseVehicle(VEHICLE_HOLDER* vehicleHold, FULL_OBJ_ID toFacility);
static void _exitVehicle(Player& persona);

static SINT32	_personaXfer(Player& player, PLAYER_EXIT& pe);

static SortieManager g_sortieSpawns("`spawn_time` = NOW()", "`spawn_time` IS NULL");
static SortieManager g_sortieReturns("`spawn_time` = IF(`spawn_time` IS NULL,`added`,`spawn_time`), `return_time` = NOW()", "`return_time` IS NULL");

//////////////////////////////////////////////////////////////////////
// In the case of a crash, close all of the current sorties.

static void
_closeAllOpenSortiesOnThisCell()
{
	DBThread::Do(DBThread::Default
		, "UPDATE wwii_sortie"
			" SET"
				" spawn_time = IF(spawn_time IS NULL,NOW(),spawn_time)"
				",return_time = NOW()"
			" WHERE return_time IS NULL and cell_host_ip = %u"
			, (Server::localHost & 255)
	);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

//////////////////////////////////////////////////////////////////////
// Announce the shutdown of the host

void exitHandler()
{
	if ( g_exitHandlerRequired.exchange(false) == false )
		return;

	chatSendAdminMessage("cell host #%u terminated", Server::localHost & 255);

	if ( g_accepting == true )
	{
		_closeAllOpenSortiesOnThisCell();
	}

	std::this_thread::yield();

	Server::myServerInfo.stats.status = Server::SERVER_INITIALIZING;
	Server::myServerInfo.sync();

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	teulCloseTransport();

	Host::Vehicles::FreeArray();

	extern void strtReleaseSupercells();
	strtReleaseSupercells();
}

//////////////////////////////////////////////////////////////////////
// Remove a player from all the vehicles in their list

static void
_extractPlayerFromVehiclesList(Player& player, HostVehicle& vehicle)
{
	player.changeFPSBuilding(nullObjID, false);
	PLAYER_LIST::iterator it = find(vehicle.players().begin(), vehicle.players().end(), &player);
	if ( it != vehicle.players().end() )
	{
		vehicle.players().erase(it);
		betaLog(_FLI, "t:%d p:%u c:%s v:%u extracting player from vehicle crew:%d, players:%" PRSizeT ""
						, player.teulIndex(), player.playerID(), player.callsign(), vehicle.index
						, vehicle.crew, vehicle.players().size()
						);
		vehicle.crew--;
	}
}

//////////////////////////////////////////////////////////////////////
// return the IP address of the current strat host

UINT32
hostGetStratAddress(void)
{
	return _stratHost;
}

//////////////////////////////////////////////////////////////////////
// Lookup a vehicle from the list of locally-owned vehicle

HostVehicle*
hostFindVehicleLocal(UINT32 vehicleIndex)
{
	VEHICLE_HASH::iterator it = _vehicleHash.find(vehicleIndex);
	return ( it != _vehicleHash.end() ? it->second : nullptr );
}

//////////////////////////////////////////////////////////////////////
// Tear-up a vehicle instance for a vehicle controlled by another
// server process.

HostVehicle*
hostCreateRemoteVehicle(const Instance::Descriptor& instance_, sortieid_t index, WEAPON_SPEC spec)
{
	Host::VehicleRequest vr(instance_.id(), index, spec);

#ifdef DEBUG
	if ( index == 0 )
		dbugLog(_FLW, "hostCreateRemoteVehicle(serial = 0) - invalid serial");
#endif

	HostVehicle* vehicle = hostFindVehicleLocal(index);
	if (vehicle != nullptr)
	{
		dbugLog(_FLA,"vehicle %d already exists",index);
		return nullptr;
	}

	vr.m_weaponSerial.source = nullObjID;
	vr.m_weaponSerial.serial = index;
	vr.m_weaponSerial.spec   = spec;

	vehicle = vr.execute();
	if ( !vehicle )
	{
		dbugLog(_FLA, "v:%u could not create remote vehicle", index);
		return nullptr;
	}

	vehicle->setRemote();
	vehicle->setInstance(instance_);

	return vehicle;
}

//////////////////////////////////////////////////////////////////////
// Add our vehicle to the vehicle hash and incorporate the sortie/vehicle id

static void
_addVehicleToHash(HostVehicle* const vehicle, sortieid_t serial)
{
	// Use the serial number as the index
	vehicle->index = vehicle->vehicleData.index = serial;

	// insert it into the vehicle hash by the serial index
	_vehicleHash.insert(std::pair<sortieid_t, HostVehicle*>(serial, vehicle));
}

//////////////////////////////////////////////////////////////////////
// Create and populate vehicle instance.

HostVehicle*
Host::VehicleRequest::execute()
{
	const WEAPON_SERIAL& ws = m_weaponSerial;

	dbugLog(_FLD, "in:%llu v:%u c:%s executing host vehicle request", m_instanceID, m_weaponSerial.serial, Chat::Name(m_handle).c_str());

	// OGS: There's some upper limit on the size of
	// weapon serials, probably for packer reasons.
	// Seems to be a 30 bit limit.
	if ( ws.serial == 0 )
	{
		dbugLog(_FLA,"illegal serial 0");
		return nullptr;
	}

	// Verify the weapon data before trying to use other values
	const WEAPON* const weapon = weapLookup(ws.spec);
	if (weapon == nullptr)
	{
		dbugLog(_FLA,"could not find weapon definition for vehicle %u spec %u.%u.%u.%u"
					, ws.serial, ws.spec.wCountry, ws.spec.wCategory, ws.spec.wClass, ws.spec.wType);
		return nullptr;
	}

	if ( hostFindVehicleLocal(ws.serial) != nullptr )
	{
		dbugLog(_FLA,"vehicle %u already in use!!", ws.serial);
		return nullptr;
	}

	VEHICLE_HOLDER* const vehHolder = Host::Vehicles::GetHolder(ws.serial, ws, weapon);
	if ( vehHolder == nullptr )
		return nullptr;

	// Shorthand.
	HostVehicle* const vehicle = vehHolder->vehicle;
	vehicle->vehicleData.packedCallsign = m_handle;
	chatGetCallsign(vehicle->vehicleData.callsign, m_handle);

	dbugLog(_FLI, "p:%u p:%u c:%s v:%u created vehicle (" PRF_WEAPON_SPEC ":%s) players:%u"
					, playerNo(0)
					, playerNo(1)
					, vehicle->callsign()
					, ws.serial
					, PRA_WEAPON_SPEC(ws.spec)
					, weapon->name
					, numPlayers()
					);

	vehicle->vehicleData.decal = m_decal;

	_addVehicleToHash(vehicle, ws.serial);

	for ( UINT32 i = 0; i < numPlayers(); i++ )
	{
		vehicle->seats[i] = playerNo(i);
		if ( vehicle->seats[i] > 0 )
			vehicle->crew++;
	}

	return vehicle;
}

//////////////////////////////////////////////////////////////////////
// Determine which station within a vehicle a given persona is occupying

bool
_determineStation(HostVehicle& vehicle, Player& player)
{
	SINT32 station = -1;
	UINT32 stations = 0;

	player.stationID = 0;

	// Check the entire list of stations for our ID
	for ( size_t i = 0; i < MAX_WEAP_STATIONS; i++ )
	{
		if ( vehicle.seats[i] > 0 )
		{
			stations++;
			if ( vehicle.seats[i] == player.playerID() )
				station = i;
		}
	}

	// If we hit the end of the array, we're not in the vehicle
	if ( station < 0 )
	{
		dbugLog(_FLA, "player %u not in player list for vehicle %u [%d stations]"
					, player.playerID(), vehicle.index, stations);
		return false;
	}

	player.stationID = station;

	if ( station > 0 )
		player.station = UPDT_OTHER;
	else // Commander
		player.station = ( stations == 1 ) ? UPDT_LONE : UPDT_CONTROL;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Join a player to a vehicle

static SINT32
_joinVehicle(Player& player, const VEHICLE_REQUEST& vRequest)
{
	PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByPlayerID(player.playerID());
	if ( psn == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u attempt to join vehicle without a player status node", player.teulIndex(), player.playerID());
		return -EINVAL;
	}

#ifdef BETA_RELEASE
	if ( player.playerID() <= 1 )
	{
		dbugLog(_FLA, "attempt to join vehicle with playerID of %u", player.playerID());
		return -EINVAL;
	}
#endif

	HostVehicle* const vehicle = hostFindVehicleLocal(vRequest.weapon.serial);
	if (vehicle == nullptr)
	{
		dbugLog(_FLAS,"couldnt find or allocate vehicle %d for player id:%d"
					, vRequest.weapon.serial, player.playerID());
		return -EINVAL;
	}

	if ( vehicle->isRemote() )
	{
		dbugLog(_FLA, "p:%u v:%u attempting to join remote vehicle!", player.playerID(), vehicle->index);
		commSendPlayerExit(player.teulIndex(), PDEST_MAP, PEXIT_DEAD, nullObjID);
		return -EINVAL;
	}

	if ( player.vehicle != nullptr )
	{
		if ( player.vehicle != vehicle )
		{
			dbugLog(_FLW, "p:%u v:%u player has prior vehicle when executing joinVehicle", player.playerID(), player.vehicle->index);
			return -EINVAL;
		}

		_extractPlayerFromVehiclesList(player, *vehicle);
		player.vehicle = nullptr;
	}

	if ( ! _determineStation(*vehicle, player) )
		return -EPERM;

	if ( player.stationID == 0 )
	{
		vehicle->players().insert(vehicle->players().begin(), &player);
	}
	else
	{
		vehicle->players().push_back(&player);
	}

	player.active = false;

	// point to that vehicle from our list
	if ( player.vehicle != nullptr )
	{
		LogAbort("player has non-null vehicle pointer");
	}

	player.setVehicle(vehicle);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Process an incoming player request to be attached to a vehicle

SINT32
hostJoinVehicle(Player& player, const VEHICLE_REQUEST& vRequest)
{
	const SINT32 retval = _joinVehicle(player, vRequest);
	if (retval < 0)
	{
		dbugLog(_FLA, "NOT GOOD: %u attempted join of %s(%d) failed, got %d: sending back to map"
						, player.playerID()
						, weapPrintName(vRequest.weapon.spec)
						, vRequest.weapon.serial
						, retval
						);
		commSendPlayerExit(player.teulIndex(),  PDEST_MAP, PEXIT_DEAD, nullObjID);
	}

	return retval;
}

//////////////////////////////////////////////////////////////////////
// Construct a player from a PersonaXfer record, i.e. to associate
// with an incoming vehicle.

Player::Player(const Server::PersonaXfer& personaXfer)
	: wwiiPlayer()
	, m_lastTimeRx(0ULL)
	, m_lastTimeRxed(0ULL)
	, vehicle(nullptr)
	, m_antiHack(nullptr)
	, state(Player::_RESERVED)
	, stationID(0)
	, exitState(PEXIT_UNINITIALIZED)
	, m_numConnects(0)
	, m_connectedAt(teulTime())
	, m_lastConnected(teulTime())
	, m_nextVelocityCheck(0)
	, m_awsTrigger(0)
	, m_expectUpdateNo(0)
	, m_updatePrime(0)
	, m_ejectionCountdown(0)
	, m_updatesSinceRX(0)
	, m_updatesFailed(0)
	, m_lastIndex(-1)
	, m_timeToLive(0)
	, station(0)
	, active(false)
	, sendClose(true)
	, m_hasSentUpdate(false)
	, m_badUpdateSeq(false)
	, m_badUpdatePrime(false)
	, m_hasExited(false)
	, m_onAWS(false)
	, m_isDespawning(false)
	, m_hasDied(false)
	, exitFacility(nullObjID)
	, m_fpsBuildingID(nullObjID)
{
	this->state = Player::_RESERVED;
	this->sendClose = TRUE;

	try
	{
		if ( personaXfer.data.auxData == nullptr )
		{
			dbugLog(_FLA, "p:%u c:%s Received persXfer without mission packet [pc#%d]"
							, personaXfer.dispatch().m_customerID, personaXfer.dispatch().m_callsign, _personaCounter + 1);
		}
		else if ( personaXfer.data.auxSize <= 0 )
		{
			dbugLog(_FLA, "p:%u c:%s Received persXfer with 0 size mission packet [pc#%d]"
							, personaXfer.dispatch().m_customerID, personaXfer.dispatch().m_callsign, _personaCounter + 1);
		}
		else
		{
			((wwiiPlayer*)this)->operator=((wwiiPlayer*)personaXfer.data.auxData);
		}
	}
	catch ( std::exception& e )
	{
		this->m_playerID = personaXfer.dispatch().m_customerID;
		this->m_teulIndex = -EINVAL;
		dbugLog(_FLAS, "%s", e.what());
		throw std::runtime_error("persona create failed");		///Terminal
	}

	assert ( playerID() == personaXfer.dispatch().m_customerID );

	dbugLog(_FLI, "p:%u c:%s aux:%p pers:%u created [pc#%d]"
					, playerID()
					, callsign()
					, personaXfer.data.auxData
					, personaID()
					, _personaCounter + 1
					);
}

//////////////////////////////////////////////////////////////////////
// Destructor for Player object.

Player::~Player() {}

//////////////////////////////////////////////////////////////////////
// Called by teulServer when a new persona is arriving and we have
// to create the associated structures for it.

SINT32
Player::Create(const Server::PersonaLogin& persXfer, PlayerStatusNode& psn)
{
	try
	{
		Player* const player = new Player(persXfer);

		g_players.insert(player);

		psn.localInfo = player;

		player->setFramingMode(host_config.update.defaultUM);

		_personaCounter++;

		return (player->playerID());
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "error creating persona for incoming player: %s", e.what());
		return -EINVAL;
	}
}

///TODO:PERSIST: Combine personaExit, personaRelease and exitPlayer

//////////////////////////////////////////////////////////////////////
// Clean up a player who is exiting/has exited.

static void
_exitPlayer(Player& player)
{
	if ( player.m_hasExited == true )
		return;

	player.m_hasExited = true;

	if ( player.sortieID() > 0 && player.playerID() > 0 )
	{
		DBThread::Do(DBThread::Default
				, "UPDATE wwii_sortie"
					" SET"
						" spawn_time = IF(spawn_time IS NULL, NOW(), spawn_time)"
						",return_time = NOW()"
						",alt_f4ed = 0"
					" WHERE sortie_id = %u AND player_id = %u"
					, player.sortieID()
					, player.playerID()
		);
	}

	SINT32 teulIndex = player.teulIndex();

	PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByPlayerID(player.playerID());
	if ( psn == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u no player data", teulIndex, player.playerID());
		if ( teulIndex >= 0 )
			teulClose(teulIndex, Teul::Closing::ExitNoPlayer);
	}
	else
	{
		_releaseVehicleList(player);

		player.Release(teulIndex, Server::ReleaseType::EConn);

		if ( teulIndex >= 0 )
			teulClose(teulIndex, Teul::Closing::ExitWithPlayer);
	}
}

//////////////////////////////////////////////////////////////////////
// Called by teulServer when a player is starting to leave the server
// or cluster.

static SINT32
_personaExit(const SINT32 teulIndex, PlayerStatusNode* const psn, Server::PersonaExit& persXfer)
{
	if ( psn == nullptr )
	{
		dbugLog(_FLW, "t:%d exiting with null player", teulIndex);
		return 0;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s personaExit", teulIndex, psn->dispatch().m_customerID, psn->dispatch().m_callsign);

	Player* const player = (Player*)(psn->localInfo);
	if ( player == nullptr )
	{
		dbugLog(_FLA,"t:%d p:%u c:%s exiting, no persona information found"
					, teulIndex
					, psn->dispatch().m_customerID
					, psn->dispatch().m_callsign
					);
		return -EINVAL;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s exiting %u", teulIndex, psn->dispatch().m_customerID, psn->dispatch().m_callsign, persXfer.exitCode);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Called by teulServer when it is about to release a player and the
// host needs to release any resources still attached to it.

static SINT32
_personaRelease(SINT32 teulIndex, PlayerStatusNode* const psn, Server::ReleaseType releaseType)
{
	Player* const player = (Player*)psn->localInfo;
	if ( player == nullptr )
		return -EINVAL;

	// Make sure they're no-longer in an instance.
	player->SetInstance(Instance::g_invalid);

	if (player->state == Player::_FREE)
	{
		dbugLog(_FLA, "t:%d p:%u attempt to release free player", teulIndex, player->playerID());
		return -EINVAL;
	}

	player->changeFPSBuilding(nullObjID, false);

	auto it = find(g_players.begin(), g_players.end(), player);
	assert( it != g_players.end() );
	g_players.erase(it);

	dbugLog(_FLD,"t:%d p:%u c:%s releasing player type %u", player->teulIndex(), player->playerID(), player->callsign(), static_cast<int>(releaseType));

	if (player->exitState == 0)
		player->exitState = PEXIT_DEAD;

	// You aren't going anywhere special if you aren't RTBing
	if ( player->exitState != PEXIT_RTB )
		player->exitFacility = nullObjID;

	_releaseVehicleList(*player);
	player->vehicle = nullptr;

	player->state = Player::_FREE;

	if ( player->teulIndex() >= 0 )
		teulSetPersonaData(player->teulIndex(), nullptr);

	if ( _personaCounter <= 0)
	{
		dbugLog(_FLA,"release too many players");
		_personaCounter = 0;
	}
	else
		_personaCounter--;

	psn->localInfo = nullptr;

	delete player;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Start the process of removing a player from the server

SINT32
hostPlayerLeave(Player& player, PLAYER_EXIT& pe)
{
	player.log("hostPlayerLeave");

	if ( pe.destination == PDEST_MAP && player.m_isDespawning )
	{
		dbugLog(_FLN, "t:%d p:%u c:%s ignoring duplicate PDEST_MAP"
						, player.teulIndex()
						, player.playerID()
						, player.callsign()
						);
		return 0;
	}

	player.exitState = pe.exitCode;
	player.exitFacility = pe.facility;

	if ( player.vehicle )
	{
		const bool isVehicleDead = (player.vehicle->vehicleData.globalFlags & VEHG_KILLED) == VEHG_KILLED;
		const bool isResupply = (player.vehicle->missionPacket().m_missionType == RESUPPLY);

		Teul::Packet outgoing(Teul::PacketId(Scoring::CallbackX, Scoring::EVENT_Y));
		Scoring::Actor actDespawnee(player.vehicle, &player);
		outgoing.construct<Scoring::Despawn>(actDespawnee, pe.facility, (UINT32)pe.exitCode, (UINT32)pe.destination, player.vehicle->x_cm(), player.vehicle->y_cm(), player.vehicle->z_cm(), isVehicleDead, isResupply);
		outgoing.setUnencrypted();
		serverSendPacketToType(outgoing, MAP_HOST);
	}

	if (pe.destination == PDEST_MAP)
	{
		player.m_isDespawning = true;

		dbugLog(_FLI, "t:%d p:%u pe:%u xfer to map at facility " PRF_FULL_OBJ_ID ""
				, player.teulIndex()
				, player.playerID()
				, pe.exitCode
				, PRA_FULL_OBJ_ID(pe.facility)
				);

		_personaXfer(player, pe);
	}
	else if (pe.destination == PDEST_EXIT)
	{
		dbugLog(_FLW, "t:%d p:%u pe:%u exiting player to exitServer, facility " PRF_FULL_OBJ_ID ""
				, player.teulIndex()
				, player.playerID()
				, pe.exitCode
				, PRA_FULL_OBJ_ID(pe.facility)
				);

		_exitPlayer(player);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Flag to ourselves that this player has finished spawning and is
// now activated.

void
hostActivatePlayer(Player* const player)
{
	player->activate();
	if ( player->teulIndex() >= 0 )
		Host::TimeUpdate::Send(player->teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Send CaptureState updates to all the eligible players in a
// given building.

void
hostSendCaptureStatesToOccupants(const CaptureState& incoming)
{
	CaptureState enemyState;
	enemyState.invert(incoming);
	OCCUPANCY_MAP::iterator it = occupancyMap.find(incoming.m_fidKey);
	if ( it != occupancyMap.end() )
	{
		OCCUPANTS& occupants = it->second.occupants;
		for ( OCCUPANTS::iterator oit = occupants.begin(); oit != occupants.end(); ++oit )
		{
			HostVehicle* veh = *oit;
			if ( veh->isInCaptureBuilding() )
				veh->sendCaptureState(veh->side() == (WWII_SIDE)incoming.m_forSide ? incoming : enemyState);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Create an inverted capture state for enemy players.

void
CaptureState::invert(const CaptureState& rhs)
{
	m_stage = rhs.m_stage;
	if ( m_stage == CapEnabled )
		m_counter = 100 - rhs.m_counter;
	else
		m_counter = rhs.m_counter;
	m_spawnable = false;
	m_rate = -rhs.m_rate;
}

//////////////////////////////////////////////////////////////////////
// Recognize that a building has changed vis levels and
// kill any players in the building

static void
_killOccupants(const instanceid_t instanceID, OCCUPANTS& occupants, UINT32 bySortieID, const char* byBuilding)
{
	if ( occupants.empty() )
		return;

	std::vector<SINT32> indexes(occupants.size());
	indexes.clear();

	for ( auto it = occupants.begin(), end = occupants.end(); it != end; ++it )
	{
		HostVehicle* const veh = *it;
		if ( veh->state == VEH_DEAD || (veh->vehicleData.globalFlags & VEHG_KILLED) != 0 )
			continue;
		if ( PN_UNLIKELY(veh->isActive() == false ||veh->players().empty() || veh->instance().id() != instanceID) )
			continue;
		if ( veh->killedBySortieID() != 0 )
			continue;
		if ( ! veh->setVehicleKilled(bySortieID) )
			continue;

		for ( auto pit = veh->players().begin(), pend = veh->players().end(); pit != pend; ++pit )
		{
			const SINT32 teulIndex = (*pit)->teulIndex();
			if ( teulIndex >= 0 )
			{
#ifdef BETA_RELEASE
				char buf[16];
				strprintf(buf, "%u", bySortieID);
				(*pit)->message(LSTR_BETA, LSTR_KILLED_BY_BUILDING, byBuilding, buf);
#endif
				indexes.push_back(teulIndex);
			}
		}
	}

	if ( indexes.empty() )
		return;

#ifdef BETA_RELEASE
	char message[2000];
	char* p = message;
	for ( auto it = indexes.begin(), end = indexes.end(); it != end; ++it )
	{
		p += sprintf(p, "%d ", *it);
	}
	dbugLog(_FLI, "DBB Notices to: %s", message);
#endif

	TeulTransmission broadcast(DEATH_BY_BUILDING::callback());
	DEATH_BY_BUILDING outgoing(DEATH_BY_BUILDING::STUB_ONLY);
	broadcast.populate(&outgoing);
	broadcast.send(indexes.begin(), indexes.end());
}

void
hostBuildingVisLevelChange(ObjectStruct* object, const instanceid_t inInstance)
{
	// Rebuild is not fatal, all other levels are
	if ( object->visLevel() == VISLEVEL_NORMAL )
		return;

	// Check to see if we have any occupants in this building
	OCCUPANCY_MAP::iterator it = occupancyMap.find(object->fullObjID.Key());
	if ( it != occupancyMap.end() )
		_killOccupants(inInstance, it->second.occupants, object->m_damagedBy, object->name);
}

//////////////////////////////////////////////////////////////////////
// Award a capture point to all but one player inside a building

void
hostGiveCaptureScoreToPlayers(const FULL_OBJ_ID buildingID, WWII_SIDE forSide)
{
	OCCUPANCY_MAP::iterator it = occupancyMap.find(buildingID.Key());
	if ( PN_UNLIKELY(it == occupancyMap.end()) )
		return;

	for ( auto inIt = it->second.occupants.begin(), inEnd = it->second.occupants.end(); inIt != inEnd; ++inIt )
	{
		HostVehicle* const veh = *inIt;
		if ( veh->state == VEH_DEAD || veh->isActive() == false || veh->players().empty() )
			continue;
		if ( veh->instance().isCampaign() == false )
			continue;
		for ( auto pit = veh->players().begin(), pend = veh->players().end(); pit != pend; ++pit )
		{
			Player* const player = *pit;

			dbugLog(_FLD, "Processing player %s (%d) for scoring: ", Chat::Name(player->chatHandle()).c_str(), player->playerID());

			if ( player->active == false || player->exitState == PEXIT_DEAD || player->teulIndex() < 0 || player->vehicle == nullptr )
			{
				continue;
			}

			// Give to cappers
			if ( player->side() == forSide )
			{
				dbugLog(_FLD, "Crediting player %s (%d) with capture", Chat::Name(player->chatHandle()).c_str(), player->playerID());
				Scoring::Packet<Scoring::Capture> packet;
				Scoring::Actor actCapper(player->vehicle, player);
				Scoring::Capture evtCapture(actCapper, buildingID, forSide);
				packet.setData(evtCapture);
				serverSendPacketToType(packet, MAP_HOST);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Send a scoring event for guarding a building.

static void
_scoreGuarding(Player* const player)
{
	if ( player->vehicle->m_guardingSince <= 0 )
		return;

	if ( player->vehicle->m_inBuildingObject != nullptr )
	{
		const SINT32 seconds = (UINT32)((teulClock64 - player->vehicle->m_guardingSince) / 1000ULL);
		if ( seconds > (host_config.score.guardPointPer_m * 60) )
		{
			Scoring::Packet<Scoring::Guard> packet;
			Scoring::Actor actGuard(player->vehicle, player);
			Scoring::Guard evtGuard(actGuard, player->vehicle->m_inBuildingObject->fullObjID, player->side(), (UINT32)seconds);
			packet.setData(evtGuard);
			serverSendPacketToType(packet, MAP_HOST);
		}
	}

	player->vehicle->m_guardingSince = 0;
}

//////////////////////////////////////////////////////////////////////
// True/False: is the player guarding a friendly capturable

static bool
_isGuarding(Player* const player)
{
	// Is this feature enabled?
	if ( host_config.score.guardPointPer_m == 0 )
		return false;

	HostVehicle* const vehicle = player->vehicle;
	assert( vehicle != nullptr );

	const WWII_SIDE& playerSide = player->side();

	// Only applies to infantry
	if ( _INFANTRY != vehicle->spec().wCategory || vehicle->isInCaptureBuilding() != playerSide )
		return false;

	const ObjectStruct* building = vehicle->m_inBuildingObject;
	if ( building == nullptr || building->facility == nullptr || building->facility->canBeCaptured() == false )
		return false;

	// Not in an enemy spawnable building.
	if ( building->m_side != playerSide )
		return false;

	// Radio state changed.
	if ( building->facility->isCapturable() == false )
	{
		// The table got reset because the AO went away or something
		// otherwise the side check would have failed.
		_scoreGuarding(player);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Check to see if the player is guarding a friendly capturable.

static void
_checkPlayerForGuarding(Player* player, bool changingBuilding)
{
	auto const vehicle = player->vehicle;
	if ( PN_UNLIKELY(!vehicle) )
		return;

	// Are they inside a friendly spawnable?
	const bool isGuarding = _isGuarding(player);
	if ( vehicle->m_guardingSince && (false == isGuarding || true == changingBuilding) )
	{
		_scoreGuarding(player);
	}

	if ( true == isGuarding && vehicle->m_guardingSince == 0 )
	{
		vehicle->m_guardingSince = teulClock64;
	}
}

//////////////////////////////////////////////////////////////////////
// Set the current containing building for a vehicle.

void
HostVehicle::setInBuilding(const FULL_OBJ_ID newBuildingID)
{
	// Extract them from the occupancy map for the current building (if any)
	if ( m_inBuilding != 0 && isRemote() == false )
	{
		OCCUPANCY_MAP::iterator it = occupancyMap.find(m_inBuilding);
		if ( it != occupancyMap.end() )
		{
			it->second.occupants.erase(this);
			if ( it->second.occupants.empty() )
			{
				occupancyMap.erase(it);
			}
		}
	}

	interruptCapture();
	setInCaptureBuilding(false);

	if ( newBuildingID.id != 0 )
	{
		m_inBuilding = newBuildingID.Key();
		m_inBuildingObject = strtGetObject(newBuildingID);
		m_enteredBuilding = teulClock64;
		if ( isRemote() == false )
		{
			static TeulTimeMS nextBlip = 0ULL;
			if ( m_inBuildingObject == nullptr )
			{
				if ( nextBlip <= teulClock64 )
				{
					dbugLog(_FLA, "v:%u c:%s reporting 'in building' " PRF_FULL_OBJ_ID ", but we do not have any data for that building!"
									, index, vehicleData.callsign, PRA_FULL_OBJ_ID(newBuildingID));
					chatSendAdminMessage("v:%u c:%s reporting 'in building' " PRF_FULL_OBJ_ID ", but we do not have any data for that building!"
									, index, vehicleData.callsign, PRA_FULL_OBJ_ID(newBuildingID));
					nextBlip = teulClock64 + 5000;
				}
				return;
			}

			// Update the relevant occupancy map for where they arrived
			Occupancy& occupancy = occupancyMap[m_inBuilding];
			if ( occupancy.object == nullptr && m_inBuildingObject != nullptr )
				occupancy.object = m_inBuildingObject;
			occupancy.occupants.insert(this);
		}
	}
	else
	{
		m_inBuilding = 0;
		m_inBuildingObject = nullptr;
		m_enteredBuilding = teulClock64;
	}
}

//////////////////////////////////////////////////////////////////////
// Move a players position to a different building

void
Player::changeFPSBuilding(const FULL_OBJ_ID newBuildingID, bool inCaptureBuilding)
{
	if ( vehicle == nullptr )
	{
		m_fpsBuildingID = nullObjID;
		return;
	}

	if ( fpsBuildingID() == newBuildingID )
	{
		// Don't allow the client to say we're in a capture building
		// when it doesn't look like we are.
		vehicle->setInCaptureBuilding(inCaptureBuilding);
		_checkPlayerForGuarding(this, false);
		return;
	}

#if defined(BETA_RELEASE) || defined(TEST_RELEASE)
	if ( newBuildingID.id != 0 )
	{
		ObjectStruct* obj = strtGetObject(newBuildingID);
		dbugLog(_FLD, "t:%d p:%u c:%s v:%u entered " PRF_FULL_OBJ_ID " [%s]"
						, teulIndex(), playerID(), callsign(), vehicle->index
						, PRA_FULL_OBJ_ID(newBuildingID)
						, (obj ? obj->name : "*UNKNOWN*")
						);
# if defined(BETA_RELEASE)
		if ( obj )
		{
			char description[sizeof(obj->name) + sizeof(obj->filename) + sizeof(" []") + 2];
			strprintf(description, "%s [%s]", obj->name, obj->filename);
			message(LSTR_BETA, LSTR_ENTERED_BUILDING, description);
		}
		else
		{
			char fidBuffer[FULL_OBJ_ID::PrintLen];
			message(LSTR_BETA, LSTR_ENTERED_UNKNOWN_BUILDING, weapPrintFullObjIDC(&newBuildingID, fidBuffer));
		}
	}
	else
	{
		message(LSTR_BETA, LSTR_LEFT_BUILDING);
# endif
	}
#endif

	// Update which building we believe them to be in
	m_fpsBuildingID = newBuildingID;

	vehicle->setInBuilding(newBuildingID);

	vehicle->setInCaptureBuilding(inCaptureBuilding);

	_checkPlayerForGuarding(this, true);
}

//////////////////////////////////////////////////////////////////////
// Called by teulServer when a player is being transfered to the map host.

static SINT32
_personaXfer(Player& player, PLAYER_EXIT& pe)
{
	try
	{
		PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(player.playerID());
		if ( psn == nullptr )
			throw std::domain_error("No data for this player");

		serverReleasePlayer(player.teulIndex(), player.playerID(), player.OTP(), pe.exitCode, Server::ReleaseType::Redirected);

		return 0;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d p:%u personaXfer failed: %s"
						, player.teulIndex()
						, player.playerID()
						, e.what()
						);
	}

	_exitPlayer(player);
	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
// Notify chat host of bad ticks for a given player.

static void
_badTicks(const SINT32 teulIndex, wwiiPlayer* const player_, const UCHAR* const data)
{
	auto* const sample = (const Teul::TickSample*)(data);
	if ( player_ == nullptr )
	{
		dbugLog(_FLW, "t:%d clock drift host:%llu, client:%llu, delta:%lld, ratio:%.6g", teulIndex, sample->host_ms, sample->client_ms, sample->delta, sample->ratio);
		return;
	}

	Player& player = *static_cast<Player*>(player_);

	if ( player.m_antiHack == nullptr )
		player.m_antiHack = new AntiHack;

	auto& ah = *player.m_antiHack;

	if ( ah.m_lastSpeedhackBlip > teulTime() - 30 && std::abs(ah.m_lastSpeedhackDelta - sample->delta) < 100 )
		return;

	++ah.m_numBlips;
	ah.m_lastSpeedhackDelta = sample->delta;
	ah.m_lastSpeedhackBlip = teulTime();

	dbugLog(_FLW, "t:%d p:%u c:%s v:%u blip:%u clock drift host:%llu, client:%llu, delta:%lld, ratio:%.6g", player.teulIndex(), player.playerID(), player.callsign(), player.sortieID(), ah.m_numBlips, sample->host_ms, sample->client_ms, sample->delta, sample->ratio);

	chatSendTickSample(CELL_HOST, player.playerID(), data);
}

//////////////////////////////////////////////////////////////////////
// Handle an error on a persona connection.

static void
_personaErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	// Nothing to report.
	if ( teulIndex < 0 )
		return;

	wwiiPlayer* player = ServerInterface::LookupByTeulIndex(teulIndex);
	if ( abs(error) == ETEUL_ILLEGAL_TICKS )
	{
		return _badTicks(teulIndex, player, data);
	}

	// A clean log out by a player should finish with the
	// door closing: it's actually healthy for a connection
	// to close with no player connection.
	if ( player == nullptr )
	{
		if ( error == ENOTCONN )
		{
			dbugLog(_FLN, "t:%d closing cleanly (no player info) err:%d", teulIndex, error);
			return;
		}
		else
		{
			// Possibly someone logging in or something.
			dbugLog(_FLM, "t:%d err:%d on non-player connection", teulIndex, error);
		}
	}
	else
	{
		if ( error == ENOTCONN && teulIndex == player->teulIndex() )
		{
			dbugLog(_FLI, "t:%d p:%u c:%s v:%u disconnected", teulIndex, player->playerID(), player->callsign(), player->sortieID());
			player->SetTeulIndex(-1);
		}
		else
		{
			dbugLog(_FLI, "t:%d p:%u c:%s v:%u error %d", teulIndex, player->playerID(), player->callsign(), player->sortieID(), error);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Accept an incoming client-connection

static void
_acceptFunction(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d incoming connection from %s:%hu conn#%d"
					, endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort()
					, ++_userConnections
					);
}

//////////////////////////////////////////////////////////////////////
// Determine if a player is in a post-connect grace period

bool
Player::isInGracePeriod(UINT32 gracePeriod_s, bool includeFirstConnection) const
{
	if ( !includeFirstConnection && m_numConnects == 0 )
		return false;
	return ( m_lastConnected + gracePeriod_s >= teulTime() );
}

//////////////////////////////////////////////////////////////////////
// Reset the update sequence for a given player.

void
Player::resetUpdateSeq()
{
	m_expectUpdateNo = 0;
	m_updatePrime = INIT_UPDATE_PRIME;
}

//////////////////////////////////////////////////////////////////////
// Called when a player sends packets with ID sequences
// that don't make sense.

static void
_illegalPacketSequence(SINT32 teulIndex)
{
	if ( teulIndex < 0 )
		return;

	Player* persona = (Player*)ServerInterface::LookupByTeulIndex(teulIndex);
	if ( persona == nullptr )
		return;

	// Give them a grace period after reconnecting
	if ( persona->isInGracePeriod(60) )
		return;

	dbugLog(_FLW, "t:%d p:%u invalid packet sequence", teulIndex, persona->playerID());

	if ( !host_config.arena.checksums || !persona->m_flags.canEject )
		return;

#ifndef BETA_RELEASE
	if ( persona->isPlaynet() )
		return;
#endif

	// Flag them so it becomes sticky
	persona->m_flags.checksumsDirty = true;
	commForcePlayerExit(*persona, "invalid packet sequence", true);
}

//////////////////////////////////////////////////////////////////////
// error callback for TCP connections

static void
_updaterErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, const SINT32 error_)
{
	const UINT32 error = abs(error_);
	Host::currentStats.add(Host::Stats::ID::NetError);

	static bool inError = false;

	if ( inError )
	{
		// Avoid nested error reporting
		return;
	}

	inError = true;

	if ( error == ETEUL_ILLEGAL_PACKET_ORDER )
	{
		_illegalPacketSequence(teulIndex);
	}
	else if ( error == ETEUL_ILLEGAL_TICKS )
	{
		return _badTicks(teulIndex, ServerInterface::LookupByTeulIndex(teulIndex), data);
	}
	else if ( error != ENOTCONN )
	{
		dbugLog(_FLW, "t:%d error %d", teulIndex, error);
	}
	else if ( error == ENOTCONN && teulIndex >= 0 )
	{
		_userConnections--;

		wwiiPlayer* player = ServerInterface::LookupByTeulIndex(teulIndex);

		if ( player != nullptr )
		{
			dbugLog(_FLI, "t:%d p:%u c:%s connection closed, %d connections remaining"
						, teulIndex
						, player->playerID()
						, player->callsign()
						, _userConnections
						);
			player->Release(teulIndex, Server::ReleaseType::EConn);
		}
		else
		{
			dbugLog(_FLI, "t:%d personaless connection closed, %d connections remaining"
						, teulIndex
						, _userConnections
						);
		}
	}

	inError = false;
}

static void _killUpdater(int /*signo*/)
{
	dbugLog(_FLA, "killing host");
	if ( g_hostTerminating.exchange(true) == true)
		exit(1);
}

//////////////////////////////////////////////////////////////////////
// Put a vehicle update into the send queue.
///NOTE: Vehicles are currently sent immediately via Raknet.

#define _SIZEOF_TIMESTAMP	4
#define _SIZEOF_PSTAMP	2
#define _SIZEOF_WEAP		4
#define _SIZEOF_POS_DIFF	3
bool
hostAddVehicleToBroadcast(HostVehicle* vehicle, VEHICLE_UPDATE_HEADER* vh)
{
	if ( !g_clustered )
		return false;

	VEHICLE_HOLDER vehHold;
	memset(&vehHold, 0, sizeof(vehHold));

	vehHold.vehicle = vehicle;
	vehHold.serial = vehicle->serial().serial;
	vehHold.present = vh->present;
	vehHold.fireMask = vh->fireMask;
	vehHold.timeStamp = vehicle->vehicleData.timeStamp;
	vehHold.posTimeStamp = vehicle->vehicleData.posTimeStamp;

	if ( hostNeedVehicleFirstTime(vehHold.serial) == true )
		vehHold.present |= PRX_FIRST_TIME;

	if ( vehHold.present & PRX_FIRST_TIME )
	{
		vehHold.numTurrets = 8;
		vehHold.turretMask = 0xff;
		vehHold.present = PR_SET_ALL;
	}
	else
	{
		vehHold.turretMask = vh->vt.turretMask;
		vehHold.numTurrets = static_cast<UINT8>(vh->vt.numTurrets);
		vehHold.present &= PR_MASK;
	}

	if ( vehicle->isReallyActive() == true )
		vehHold.present |= PRX_REALLY_ACTIVE;

	UCHAR buffer[256];
	SINT32 offset = updtPackVehicleBroadcast(TEUL_PACK, buffer, 0, &vehHold, 0);
	if ( offset < 0 )
	{
		threadLog(_FLA, "updtPackVehicleBroadcast failed: %d", offset);
		return false;
	}

	return hostSendVehicleBroadcast(buffer, offset);
}

//////////////////////////////////////////////////////////////////////
// New map host has announced itself on the teulServer network.

static void
_newMapHost(const Server::HeartBeat& heartbeat, const bool isEntering)
{
	if ( heartbeat.status == Server::SERVER_ACCEPTING )
	{
		dbugLog(_FLN, "detected map host at %s", teulGetAddress(heartbeat.address));
	}
}

static void
_newStratHost(const Server::HeartBeat& heartbeat, const bool isEntering)
{
	if ( heartbeat.status == Server::SERVER_ACCEPTING )
	{
		_stratHost = Server::BroadcastMaskAddress(heartbeat.address);
		dbugLog(_FLN, "detected strat host at %s", teulGetAddress(_stratHost));
	}
}

//////////////////////////////////////////////////////////////////////
//

static void
_addNewServer(Server::ServerInfo* server, const bool isEntering)
{
	if ( server->stats.type == CHAT_HOST )
	{
		return;
	}
	if ( server->stats.type == STRAT_HOST )
	{
		_newStratHost(server->stats, isEntering);
		return;
	}
	if ( server->stats.type == MAP_HOST )
	{
		_newMapHost(server->stats, isEntering);
		return;
	}

	if ( server->stats.type != CELL_HOST )
		return;

	if ( ((isEntering && server->stats.status == Server::SERVER_ACCEPTING )
			|| (server->stats.status == Server::SERVER_INITIALIZING) ) )
	{

		if (isEntering && server->stats.status == Server::SERVER_ACCEPTING )
			dbugLog(_FLN,"reconnected server detected");
		else
			dbugLog(_FLN,"new cellhost detected");

		///TODO: Send my vehicle list to this server
	}
}

//////////////////////////////////////////////////////////////////////
// Hunt for and remove long-dead vehicles in the remote list
// Returns the time we should next check for ghosts
//
// Ghosts are caused when a remote vehicle manages to remain in our
// remote list when it has actually gone away. For this purpose we assign
// remote vehicles a maximum life time within which we must receive an
// update for the vehicle.

void
_expireVehicles()
{
	const TeulTimeMS localCutoff = teulClock64 - FORCE_CLIENT_TIMEOUT + 1000;
	const TeulTimeMS remoteCutoff = localCutoff - SERVER_FAIL_INTERVAL + 1000;
	size_t removals = 64;

	// Get a list of vehicles to remove.
	for ( auto it = _vehicleHash.begin(), end = _vehicleHash.end(); it != end;)
	{
		HostVehicle* const vehicle = it->second;
		++it;

		if ( PN_UNLIKELY(!vehicle->isReallyActive()) )
			continue;

		if ( PN_LIKELY(vehicle->lastRX > remoteCutoff) )
			continue;
		if ( PN_LIKELY(vehicle->isRemote() == false && vehicle->lastRX > localCutoff) )
			continue;

		dbugLog(_FLD, "v:%u c:%s forcing exit due to lack of updates", vehicle->index, vehicle->callsign());
		vehicle->forceExit();

		--removals;
		if ( removals <= 0 )
			break;
	}
}

//////////////////////////////////////////////////////////////////////
// For a vehicle and it's players to exit.

void
HostVehicle::forceExit()
{
	if ( isRemote() == false && players().empty() == false )
	{
		// Ejecting the players will cause
		// the destruction of the vehicle...
		PLAYER_LIST myPlayers = players();
		for ( auto it = myPlayers.begin(); it != myPlayers.end(); ++it )
		{
			Player* const player = *(it);
			if ( player != nullptr )
				_exitPlayer(*player);
		}

		// We can't do anything else here because the
		// vehicle may have been released.

		return;
	}

	_releaseVehicle(m_holder, nullObjID);
}

//////////////////////////////////////////////////////////////////////
// Callbacks for teulServer to update us as server processes within the cluster
// transition through their various states or notify us they're still alive.

static SINT32
_serverEnterCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN, "server %s added code %d", teulGetAddress(server->stats.address), static_cast<int>(scode));
	_addNewServer(server, TRUE);
	return 0;
}

static SINT32
_serverLeaveCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN, "server %s left code %d", teulGetAddress(server->stats.address), static_cast<int>(scode));
	///REVIEW: At this point we should probably remove the vehicles from this server
	return 0;
}

static SINT32
_serverStatusCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN, "received server %s status changed to %d code %d"
		, teulGetAddress(server->stats.address), server->stats.status
		, static_cast<int>(scode));
	_addNewServer(server, FALSE);
	return 0;
}

//////////////////////////////////////////////////////////////////////
// teulServer callback when a persona completes its login and we can
// start talking to them.

static SINT32
_personaActivate(SINT32 teulIndex , PlayerStatusNode* psn)
{
	Player* player = (Player*)psn->localInfo;
	if ( PN_UNLIKELY(player == nullptr) )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s couldnt find persona to activate"
						, teulIndex
						, psn->dispatch().m_customerID
						, psn->dispatch().m_callsign
						);
		return -EINVAL;
	}

	if ( player->teulIndex() >= 0 )
		teulSetPersonaData(player->teulIndex(), nullptr);

	dbugLog(_FLN,"t:%d p:%u c:%s login#%u [formerly %d]"
					, teulIndex, psn->dispatch().m_customerID, psn->dispatch().m_callsign
					, player->m_numConnects, player->teulIndex());

	player->resetUpdateSeq();

	player->SetTeulIndex(teulIndex);

	player->m_updatesSinceRX = 0;
	player->m_hasSentUpdate = 0;

	if ( player->m_numConnects == 0 )
		player->m_connectedAt = teulTime();
	else
		player->setFramingMode(player->m_framingMode);

	player->m_lastConnected = teulTime();
	player->m_numConnects++;

	WWII_ACTIVATE activate;

	memset(&activate, 0, sizeof(activate));

	activate.serverType = CELL_HOST;
	activate.hostIP = Server::myServerInfo.stats.address;
	activate.port = WWII::Net::Ports::CellHost;

	const SINT32 retval = wwiiSendActivate(teulIndex, &activate);
	if ( PN_UNLIKELY(retval < 0) )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s wwiiSendActivate failed: %d"
						, teulIndex
						, psn->dispatch().m_customerID
						, psn->dispatch().m_callsign
						, retval
						);
	}

	if ( PN_UNLIKELY(player->vehicle && player->vehicle->isRemote()) )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s seems to have REMOTE vehicle %u"
						, teulIndex
						, psn->dispatch().m_customerID
						, psn->dispatch().m_callsign
						, player->vehicle->index);
	}

	if ( player->vehicle != nullptr )
		player->vehicle->updateGrid();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Write the current values of the error counters to the database

static void
_writeEventCounters()
{
#if defined(ENABLE_CELL_EVENTS)
	static const UINT32 WRITE_INTERVAL = 60;	// Once a minute
	static UINT32 nextWrite = teulTime() + WRITE_INTERVAL;
	if ( nextWrite > teulTime() )
		return;

	if ( ! g_eventCounter.empty() )
	{
		char* statement = threadsAllocText();
		char* p = statement + sprintf(statement, "!audit!INSERT INTO events (host,event,number) VALUES");
		UINT32 host = Server::localHost & 255;

		EVENT_COUNTER::iterator it;
		bool started = false;
		for ( auto it = g_eventCounter.begin(), end = g_eventCounter.end(); it != end; ++it )
		{
			p += sprintf(p, "%c(%u,'%s',%llu)", (started) ? ',':' ', host, it->first.c_str(), it->second);
			started = true;
		}

		DBThread::Do(statement);

		g_eventCounter.clear();
	}

	nextWrite = teulTime() + WRITE_INTERVAL;
#endif
}

//////////////////////////////////////////////////////////////////////
// true/false: does this vehicle have crew members with active,
// established, network connections?

bool
HostVehicle::hasConnectedPlayers() const
{
	for ( auto it = players().cbegin(), end = players().cend(); it != end; ++it )
	{
		if ( PN_LIKELY((*it)->teulIndex() >= 0) )
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Sort the vehicle list based on next update

static void
_getVehiclesWaitingForUpdates(VEHICLES& vehicles, const UINT32 maxVehicles)
{
	TIMER_USE_HERE

	TIMER_START
	vehicles.clear();
	vehicles.reserve(maxVehicles);
	TIMER_STOP("gvwfu_alloc");

#ifdef DEBUG_UPDATES
	const TeulTimeMS lowend = teulClock64 - 100;
	bool reportedLagging = false;
#endif
	TIMER_START
	for (auto it = _vehicleHash.begin(), end = _vehicleHash.end(); it != end;)
	{
#ifdef DEBUG_UPDATES
		++totalVehicles;
#endif
		HostVehicle* const vehicle = it->second;
		++it;

#ifdef DEV_BUILD
		assert( vehicle != nullptr );
#endif

		if ( vehicle->isRemote() )
		{
#ifdef DEBUG_UPDATES
			++remoteVehicles;
#endif

			continue;
		}

		if ( vehicle->m_nextUpdateClock > teulClock64 )
		{
#ifdef DEBUG_UPDATES
			++vehiclesSkipped;
#endif
			continue;
		}

		unsigned int fail = 0;
		if ( PN_UNLIKELY((vehicle->index == 0) | (vehicle->isActive() == false) | (vehicle->m_holder == nullptr)) )
			++fail;
		else if ( PN_UNLIKELY(vehicle->hasConnectedPlayers() == false) )
			++fail;

		if ( PN_LIKELY(fail == 0) )
		{
#ifdef DEBUG_UPDATES
			++qualifiedVehicles;

			if ( (!reportedLagging) & (vehicle->m_nextUpdateClock < lowend) )
			{
				threadLog(_FLA, "c:%s v:%u*** LAGGING BEHIND UPDATES: wanted update %llums ago"
								, vehicle->callsign(), vehicle->index, teulClock64 - vehicle->m_nextUpdateClock);
				reportedLagging = true;
			}
#endif
			vehicles.push_back(vehicle);
		}
		else
		{
			vehicle->setNextUpdate(vehicle->m_updateInterval);
		}
	}
	TIMER_STOP("gvwfu_sel");

	if ( PN_UNLIKELY(vehicles.empty()) )
		return;

	if ( vehicles.size() > maxVehicles )
	{
		TIMER_START
		dbugLog(_FLA, "%" PRSizeT " vehicles want updates, limit is %u", vehicles.size(), maxVehicles);
		sort(vehicles.begin(), vehicles.end(), [](auto lhs, auto rhs) { return lhs->m_nextUpdateClock < rhs->m_nextUpdateClock; });
		vehicles.resize(maxVehicles);
		TIMER_STOP("gvwfu_shrink");
	}

#ifdef DEBUG_UPDATES
	vehiclesReviewed = vehicles.size();
#endif

	TIMER_START
	sort(vehicles.begin(), vehicles.end(), [](auto lhs, auto rhs) { return lhs->m_cell < rhs->m_cell; });
	TIMER_STOP("gvwfu_sort");

	TIMER_START
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::HOST_LOCALITY_UPDATES_Y));
	for ( auto it = vehicles.begin(), end = vehicles.end(); it != end; ++it )
	{
		HostVehicle* const vehicle = (*it);
		vehicle->setNextUpdate(vehicle->m_updateInterval);
		vehicle->checkLocality(outgoing);
	}
	if ( outgoing.empty() == false )
		serverSendPacketToType(outgoing, CHAT_HOST);
	TIMER_STOP("gvwfu_loc");
}

//////////////////////////////////////////////////////////////////////
// When a player has earned themselves an ejection, we start a timer
// to that ejection. This avoids exposing the detection mechanism.
// We then send the client a* request* to disconnect, and allow it
// a period of time to honor this request before forcibly disconnecting
// them. This allows us to detect individuals who experiment with
// command interception.

static inline bool
_checkPlayerEjectionCountdown(Player* persona)
{
	if ( PN_LIKELY(persona->m_ejectionCountdown <= 0) )
		return false;

	if ( --persona->m_ejectionCountdown > 0 )
		return false;

	// Your ejection period has timed out
	const char* callsign = persona->callsign();
	dbugLog(_FLW, "t:%d p:%u c:%s ejecting mod hacker", persona->teulIndex(), persona->playerID(), callsign);
	chatSendKickNotice(persona->playerID(), callsign, "illegal client behavior");
	commSendPlayerExit(persona->teulIndex(), PDEST_EXIT, PEXIT_DEAD, nullObjID);
	commDropPlayer(*persona, false);

	return true;
}

//////////////////////////////////////////////////////////////////////
// After a player has been sent an ejection, with start a second
// timer - a "Time To Live". If the player has not honored the
// despawn request in this time, they are forcibly removed from the
// server.

static inline bool
_checkPlayerTimeToLive(Player* player)
{
	if ( PN_LIKELY(player->m_timeToLive <= 0) )
		return false;
	player->m_timeToLive--;
	if ( player->m_timeToLive > 0 )
		return false;

	dbugLog(_FLW, "t:%d exhausted timeToLive", player->teulIndex());
	commDropPlayer(*player, true);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Check if a persona is update-starved

static inline bool
_starvedUpdates(Player* player, HostVehicle* vehicle, bool despawnTimerActive)
{
	if ( host_config.update.starvationLimit_s <= 0 )
		return false;

#ifdef DISABLE_STARVATION_LIMITER
	if ( player->flags.canEject == false )
		return false;
#endif
	if ( vehicle->isReallyActive() == false )
		return false;
	if ( PN_UNLIKELY(player->isPlaynet()) )
		return false;
	if ( PN_UNLIKELY(despawnTimerActive) )
		return false;

	++player->m_updatesSinceRX;
#ifdef BETA_RELEASE
	if ( (player->m_updatesSinceRX & 0x0f) == 0 )
		dbugLog(_FLI, "t:%d p:%u updates sent since last receive: %02x", player->teulIndex(), player->playerID(), player->m_updatesSinceRX);
#endif

	const UINT32 maxUpdatesBetweenRxs = (FE_UPDATES_PER_SECOND * host_config.update.starvationLimit_s);
	if ( PN_UNLIKELY(player->m_updatesSinceRX > maxUpdatesBetweenRxs) )
	{
		commForcePlayerExit(*player, "*too few updates from client", true);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Modify a world-update packet to include the ping offset.

static void
_calculatePingTime(Teul::Packet& packet, Player* const player, size_t pingOffset)
{
	const TeulTimeMS backdate = teulClock64 - player->m_lastTimeRxed;
	const TeulTimeMS backdatedRxTime = (player->m_lastTimeRxed > 0) ? player->m_lastTimeRx + backdate : 0ULL;
	packet.payloadRef<TeulTimeMS>(pingOffset) = backdatedRxTime;
}

//////////////////////////////////////////////////////////////////////
// Send WORLD_UPDATE teulPacket to a particular player.

static bool
_sendPacket(Teul::Packet& packet, Player* const player, const SINT32 sortieID, size_t pingOffset)
{
	_calculatePingTime(packet, player, pingOffset);

	SINT32 retval = packet.send(player->teulIndex());
	if ( retval >= 0 )
	{
		player->m_updatesFailed = 0;
		return true;
	}

	if ( (player->m_updatesFailed++ & 7) == 0 )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s v:%d couldn't send world update(#%u): %d"
					, player->teulIndex()
					, player->playerID()
					, player->callsign()
					, sortieID
					, player->m_updatesFailed
					, retval
					);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Build the next update for this vehicle and dispatch it to any
// players who are associated with the vehicle.
///NOTE: This methodology requires that the updates be based on
///NOTE: the updateFraming of the lowest-bandwidth player.

static bool
_sendNextUpdate(HostVehicle* const vehicle)
{
	// "x_HOLDER"s are used to store a pointer to an entity and
	// any contextual data for that pointing in linked lists.
	// Think of them as "#define x_HOLDER(ptr) About(ptr)
	// Storing them separately is supposed to provide some
	// efficiency by keeping all the "holding" data in a compact
	// area of memory.

	VEHICLE_HOLDER* vehHolder = vehicle->m_holder;
#ifdef DEV_BUILD
	if ( PN_UNLIKELY(vehHolder == nullptr) )
	{
		dbugLog(_FLA, "v:%u vehicle has null holder", vehicle->index);
		return false;
	}
#endif

	// Better not to stack-allocate the world update?
	WORLD_UPDATE wu;
	memset(&wu, 0, sizeof(wu));

	wu.myVehicle = vehicle;
	vehicle->m_worldUpdate = &wu;

	// Prep the world update.

//	TIMER_RESET
//	TIMER_USE_HERE

//	TIMER_START
	// Go over our previous vis list and check for vehicles which have
	// gone out of range or despawned.
	// Create our new world update / vis list by scanning the nearby
	// "sector"s of the cell-grid for vehicles and shuffling them
	// through all kinds of linked lists to build our final vis list.
	vehicle->UpdateVehicleList();
//	TIMER_STOP("updatevlist");

//	TIMER_START
	// Produce the vis list in 3 levels of accuracy and prepare
	// as packed data.
	vehicle->CreateWorldUpdate();
//	TIMER_STOP("createwu");

	// Send vehicle removes.
//	TIMER_START
	vehicle->SendRemoveList();
//	TIMER_STOP("sendrmv");

	assert( wu.header.numVehicles <= framings[vehicle->framingMode].maxVehiclesPerFrame() );

#ifdef DEBUG_UPDATES
	numVehiclesReported += wu.header.numVehicles;
#endif

	// Assume 16-bytes per vehicle.
//	TIMER_START
	Teul::Packet packet(Teul::PacketId(Comm::CallbackX, WORLD_UPDATE_Y));
	packet.reserve(sizeof(wu.header) + wu.header.numVehicles * 80);
	wu.packet = &packet;

	packet.append(wu.header);
//	TIMER_STOP("pktprep");

//	vehicle->m_lastUpdateCount = (UINT32)numUpdates;
//	vehicle->m_lastUpdateSent = teulClock64;

	// Pack the world update into "out.message"; if this is a multi-crew
	// vehicle, this will save us having to pack it for each player.
	//TIMER_START
	const SINT32 packResult = updtPackWORLD_UPDATE(TEUL_PACK, nullptr, 0, &wu, 0);
//	TIMER_STOP("pack");
	if ( PN_UNLIKELY(packResult < 0) )
	{
		dbugLog(_FLA, "v:%u c:%s pack world update failed: %d"
						, vehicle->index, vehicle->callsign()
						, packResult);
//		TIMER_REPORT(50)
		return true;
	}

	const size_t pingOffset = wu.packet->payloadSizeBytes();
	wu.packet->append(teulClock64);
#if defined(ENABLE_CELL_EVENTS)
	UINT64& maxCounter = g_eventCounter["wu-size-max"];
	if ( (UINT64)payloadSizeBytes > maxCounter )
	{
		maxCounter = (UINT64)payloadSizeBytes;
	}
#endif

	// Track how many packets behind the connections are
	UINT32 updatesBehind = 0;
	bool updatesFailing = false;

	// Get a list of people we need to send to.
	std::vector<Player*> players;

	//send to all players on this vehicle
	bool result = false;
	for (auto it = vehicle->players().begin(), end = vehicle->players().end(); it != end; )
	{
		Player* const player = *it;
		++it;

		if ( PN_UNLIKELY(!player->active || player->teulIndex() < 0) )
			continue;

//		TIMER_START
		if ( PN_UNLIKELY(_checkPlayerTimeToLive(player)) )
			continue;
//		TIMER_STOP("plyr:chkttl");

//		TIMER_START
		if ( PN_UNLIKELY(_checkPlayerEjectionCountdown(player)) )
			continue;
//		TIMER_STOP("plyr:ejectctr");

//		TIMER_START
		if ( PN_UNLIKELY(_starvedUpdates(player, vehicle, vehHolder->vehicle->m_despawnTimerActive)) )
			continue;
//		TIMER_STOP("plyr:starved");

		if ( PN_UNLIKELY(player->m_updatesSinceRX > updatesBehind) )
			updatesBehind = player->m_updatesSinceRX;

#ifdef HEAVY_DEBUG
		dbugLog(_FLD, "t:%d p:%u c:%s v:%d world update nv:%d sz:%d"
						, player->teulIndex()
						, player->playerID()
						, player->callsign()
						, vehicle->index
						, wu.numVehicles
						, dataOut.size
						);
#endif
		players.push_back(player);
	}

//	TIMER_START
	if ( players.empty() == false )
	{

#ifdef DEBUG_UPDATES
		++vehUpdatesSent;
#endif
		// Because of encryption, we can't just send the original to
		// the crew, we have to send a copy.
		const size_t size = players.size();
		for ( size_t i = 1; i < size; ++i )
		{
			Teul::Packet copy(*wu.packet);
			if ( _sendPacket(copy, players[i], vehicle->index, pingOffset) )
			{	
				result = true;
#ifdef DEBUG_UPDATES
				++vehMcrewCrewUpdatesSent;
#endif
			}
			else
			{
#ifdef DEBUG_UPDATES
				++xmissionsFailed;
#endif
				updatesFailing = true;
			}
		}

		if ( _sendPacket(*wu.packet, players[0], vehicle->index, pingOffset) )
		{
			result = true;
#ifdef DEBUG_UPDATES
			++vehMcrewCommanderUpdatesSent;
#endif
		}
		else
		{
#ifdef DEBUG_UPDATES
			++xmissionsFailed;
#endif
			updatesFailing = true;
		}
	}
//	TIMER_STOP("sendpkt");

	// If we had trouble sending updates, we may need to throttle the
	// rate at which we're sending data, or the amount of data.

	if ( host_config.update.errorThrottling )
	{
		if ( updatesFailing )
		{
#ifdef DEBUG_UPDATES
			++vehiclesThrottled;
#endif

			// Add an extra 1.5s to the next update
			vehicle->setNextUpdate(1500);
		}
		else if ( updatesBehind > 1 )
		{
#ifdef DEBUG_UPDATES
			numberOfUpdatesBehind += updatesBehind;
#endif
			const UINT32 delay = updatesBehind * 25;
			if ( delay >= 1000 )
			{
				vehicle->m_nextUpdateClock += 1000;
			}
			else
			{
				vehicle->m_nextUpdateClock += delay;
			}
		}
	}
//	TIMER_REPORT(100)

	return result;
}

//////////////////////////////////////////////////////////////////////
// Central server loop.

extern void
_runHost(void)
{
//	Teul::housekeeper cpEventsHousekeeper(Strat::Events::Publish, 100);

	// Time out ghosts every half a second.
	Teul::housekeeper expiryHouseKeeper(_expireVehicles, 500);

	Server::myServerInfo.stats.status = Server::SERVER_ACCEPTING;
	Server::myServerInfo.sync();

	wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

	VEHICLES nextVehicles;

	////////////////////////////////////////////////////////////
	// This loop is the core of the cell host.
	teulSyncClock();

	TIMER_USE_HERE

	for ( ; ; )
	{
		TIMER_RESET

		if (PN_UNLIKELY(g_hostTerminating == true))
			return;

		// Because dbugLog is not threadsafe, any worker
		// threads have to queue up their log messages for
		// us to consume. Write upto 64 log entries.
		TIMER_START
		threadsCallNextCallback();
		threadsLogging();
		TIMER_STOP("threads");

		TIMER_START
		teulUpdateTransport(25);
		TIMER_STOP("networking");

		// The host self-instruments with a number of "event"
		// counters, which are periodically written to
		// a database.
		_writeEventCounters();

		// Make sure that the cell host is in accepting mode.
		///TODO: Kinda retarded to keep doing this.
		if ( PN_UNLIKELY(!g_accepting && !g_serverReserved) )
		{
			g_accepting = true;
			Server::myServerInfo.stats.m_maxPlayers = cv_maxCellPlayers.GetUint32();
			Server::myServerInfo.sync();
		}

		// Send any cell-to-cell vehicle updates generated.
		hostPushVehicleBroadcasts();

		// For any sorties that have completed the spawning
		// process, save off the data.
		///TODO: Delegate this to a freaking thread.
		TIMER_START
		g_sortieSpawns.Commit();
		// And for any sorties that are despawning, save
		// off that data.
		///TODO: Delegate to a thread.
		g_sortieReturns.Commit();
		TIMER_STOP("commits");

		// Run housekeeping routines.
		///TODO: Move to general housekeeping.
//		cpEventsHousekeeper.poll();

		// Process any new cell host events
		///TODO: Move to general housekeeping.
		TIMER_START
		strtUpdaterProcessEvents();
		TIMER_STOP("strtevents");

		// Every half second or so, expire timed out vehicles.
		///TODO: Move to general houskeeping.
		TIMER_START
		expiryHouseKeeper.poll();
		TIMER_STOP("expiry");

		// Force the dispatch of any queued up "vehicle despawned"
		// notifications to the rest of the cluster.
		TIMER_START
		hostQueueVehicleRemove(0, 0);
		TIMER_STOP("qremove");

		// If the funny farm is enabled, run it, so long as we're
		// not a reserved (no-users) cell host.
#if defined(DEV_BUILD) || defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		if ( g_serverReserved == false )
			hostRunFakeVehicles();
#endif

		////////////////////////////////////////////////////////////
		// GENERATE WORLD UPDATES.
		//
		// We're now going to check for vehicles that are due their
		// next world update.
		//
		// If we have problems sending all of the updates in an
		// alotted period of time, we reduce the number of updates
		// we are trying to send per loop.
		//
		// In practice, this seems to be a bad idea.

		static const UINT32 _MAX_UPDATES = 512;
		static const UINT32 _MIN_UPDATES = 64;
		// Start out by assuming we can manage at least _MAX_UPDATES per pass.
		static UINT32 maxUpdates = _MAX_UPDATES;
		// Remember when we last hit a glitch so we don't over-report.
		static UINT32 lastBlip = 0;

		// Search the local vehicle list for suitable vehicles to
		// send updates to.
		TIMER_START
		_getVehiclesWaitingForUpdates(nextVehicles, maxUpdates);
		TIMER_STOP("gvwfu");

		UINT32 updatesSent = 0;

#ifdef UPDATE_LOOP_DEBUGGING
		UINT64 minTicks = ~0ULL;
		UINT64 maxTicks = 0ULL;
		UINT64 totalTicks = 0ULL;
#endif

		// Remember when we started.
		const TeulTimeMS started = teulClock64;
		// Maximum time we should be spending generating updates
		// in any given pass.
		static const TeulTimeMS _MAX_TIME_PER_PASS_MS = 50;
		// Calculate the end time.
		const TeulTimeMS updateCycleTimeout = started + _MAX_TIME_PER_PASS_MS;

		// Now generate world updates for all of the vehicles we selected.
		TIMER_START
		for ( auto it = nextVehicles.begin(), end = nextVehicles.end(); it != end; ++it )
		{
			HostVehicle* const vehicle = *it;

#ifdef HEAVY_DEBUG
			betaLog(_FLD, "c:%s v:%u sending update fm:%u iv:%ums", callsign, index, vehicle->framingMode, vehicle->m_updateInterval);
#endif

			// Send this vehicle it's update.
			if ( _sendNextUpdate(vehicle) )
				++updatesSent;
		}
		TIMER_STOP("updts");
		TIMER_REPORT(250)

#ifdef DEBUG_UPDATES
		static UINT32 nextStatsReport = teulTime();
		if ( teulTime() >= nextStatsReport )
		{
			// Report output to client stats every second
			dbugLog(_FLI, "TV=%u, RV=%u, VS=%u, QV=%u, VR=%u, VUS=%u, VMCU=%u, VCRU=%u, NVR=%u, XF=%u, VT=%u, NU=%u", 
				totalVehicles, remoteVehicles, vehiclesSkipped, qualifiedVehicles, vehiclesReviewed, vehUpdatesSent, vehMcrewCommanderUpdatesSent, 
				vehMcrewCrewUpdatesSent, numVehiclesReported, xmissionsFailed, vehiclesThrottled, numberOfUpdatesBehind); 
			totalVehicles = remoteVehicles = vehiclesSkipped = qualifiedVehicles = vehiclesReviewed = vehUpdatesSent = 0;
			vehMcrewCommanderUpdatesSent = vehMcrewCrewUpdatesSent = xmissionsFailed = vehiclesThrottled = 0;
			numberOfUpdatesBehind = numVehiclesReported = 0;
			nextStatsReport = teulTime() + 1;
		}
#endif

		if ( PN_UNLIKELY(nextVehicles.size() >= _MIN_UPDATES) )
		{
			if ( teulClock64 > updateCycleTimeout )
			{
				// We overran, we should try sending less updates
				dbugLog(_FLW, "taking too long to send vehicle updates (%" PRSizeT "): %llums elapsed"
								, nextVehicles.size(), teulClock64 - started);
				if ( maxUpdates > _MIN_UPDATES )
				{
					--maxUpdates;
					dbugLog(_FLW, "reducing max updates to %u", maxUpdates);
				}
				lastBlip = teulTime();
			}
			else if ( nextVehicles.size() >= maxUpdates && maxUpdates < _MAX_UPDATES && teulClock64 < updateCycleTimeout - 5 && lastBlip <= teulTime() - 5 )
			{
				// We sent a full load of updates, we aren't sending the max
				// and it's been 5 seconds since the last blip. Bump the updates
				++maxUpdates;
				dbugLog(_FLN, "increasing max updates to %u", maxUpdates);
				lastBlip = teulTime();
			}

			static UINT32 nextReport = teulTime();
			if ( teulTime() >= nextReport )
			{
				// Report on the number of updates every 30 minutes
				dbugLog(_FLI, "update loop is using %u maxUpdates", maxUpdates);
				nextReport = teulTime() + (30 * 60);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Receive an activation for a vehicle

void
hostActivateVehicle(HostVehicle* newVehicle, bool checkPlayers)
{
	// We don't activate a vehicle until all of its crew are active
	if ( checkPlayers && newVehicle->isRemote() == false )
	{
		if ( newVehicle->players().empty() )
		{
			dbugLog(_FLW, "v:%u tried to activate empty vehicle", newVehicle->index);
			return;
		}

		PLAYER_LIST::iterator it;
		for ( it = newVehicle->players().begin(); it != newVehicle->players().end(); ++it )
		{
			if ( !(*it)->active )
			{
				return;
			}
		}

		g_sortieSpawns.AddSortie(newVehicle->index);
	}

	if ( newVehicle->isRemote() == true )
	{
		if ( g_logDebug )
			dbugLog(_FLD, "v:%u activating remote vehicle", newVehicle->index);
	}
	else
	{
		if ( g_logDebug )
		{
			const SHARED_VEHICLE_INFO& info = newVehicle->vehicleData;
			dbugLog(_FLI, "p:%u,%u c:%s v:%u activating local vehicle at %d.%d.%d"
							, newVehicle->seats[0], newVehicle->seats[1]
							, info.callsign, newVehicle->index
							, newVehicle->x_cm(), newVehicle->y_cm(), newVehicle->z_cm()
							);
		}
	}

	newVehicle->setActive(true);
	newVehicle->state = VEH_IN_USE;
	newVehicle->lastRX = teulClock64;

//	hostSetUpdateFraming(newVehicle, PREFERRED_UPDATE_FRAMING_MODE);

	newVehicle->updateGrid();

	newVehicle->m_spawnTime = teulTime();

	if ( newVehicle->isRemote() == false )
	{
		_vehicleCounter++;

		newVehicle->selectBestFramingMode();
	}
}

//////////////////////////////////////////////////////////////////////
// CAUTION:
//  This function was intended to remove instances of non-locally
//  controlled vehicles. Its functionality has been adapted and you
//  should now consider its status as "dubious".


void
hostRemoveRemoteVehicle(UINT32 serial, PLAYER_EXIT_CODE exitCode)
{
	dbugLog(_FLD, "v:%u attempting release of remote vehicle", serial);
	HostVehicle* const vehicle = hostFindVehicleLocal(serial);
	if ( vehicle == nullptr )
	{
		dbugLog(_FLA, "v:%u no such remote vehicle to remove???", serial);
		return;
	}
	if ( !vehicle->isRemote() )
	{
		dbugLog(_FLA, "v:%u remove remote vehicle request on local vehicle", serial);
		return;
	}

	vehicle->exitState = exitCode;
	_releaseVehicle(vehicle->m_holder, nullObjID);
}

//////////////////////////////////////////////////////////////////////
// Kick a player from the game as a GM action.

void
hostSmiteCustomer(const playerid_t playerID)
{
	Player* const player = (Player*)ServerInterface::LookupByPlayerID(playerID);
	if ( player == nullptr )
		return;

	dbugLog(_FLN, "t:%d p:%u c:%s Smiting customer"
					, player->teulIndex()
					, player->playerID()
					, player->callsign()
					);

	_exitPlayer(*player);

	if ( player->teulIndex() >= 0 )
	{
		teulClose(player->teulIndex(), Teul::Closing::Smite);
	}
}

//////////////////////////////////////////////////////////////////////
// Return a vehicle to the free pool

static void
_releaseVehicle(VEHICLE_HOLDER* const vehicleHold, FULL_OBJ_ID toFacility)
{
	if (PN_UNLIKELY(vehicleHold == nullptr))
	{
		dbugLog(_FLAS,"_releaseVehicle: ERROR vehicleHold = null!!");
		return;
	}

	HostVehicle* const vehicle = vehicleHold->vehicle;
	_vehicleHash.erase(vehicle->index);

	// Clear out our named lookup.
	{
		auto it = g_vehicleLookup.find(vehicle->chatHandle());
		if ( it != g_vehicleLookup.end() && it->second == vehicle )
			g_vehicleLookup.erase(it);
	}

	const UINT32 index = vehicle->index;

	if ( vehicle->isRemote() == false )
	{
		g_sortieReturns.AddSortie(index);

		betaLog(_FLD, "v:%u unscheduling vehicle updates", index);
		updateUnscheduleVehicle(vehicle);
	}

	if (vehicle->state == VEH_FREE)
	{
		dbugLog(_FLA, "v:%u attempt to release free vehicle", index);
		return;
	}

	dbugLog(_FLD, "v:%u/%u _releaseVehicle: toFacility = " PRF_FULL_OBJ_ID ""
					, index
					, vehicle->index
					, PRA_FULL_OBJ_ID(toFacility)
					);

	if (vehicle->isActive() )
	{
		if ( vehicle->isRemote())
		{
			dbugLog(_FLD, "v:%u is remote", vehicle->index);
		}
		else
		{
#ifdef NEW_BCODE
			hostQueueVehicleRemove(index, vehicle->exitState);
#endif

			if ( _vehicleCounter > 0 )
				_vehicleCounter--;
		}
		dbugLog(_FLD, "v:%u extracted vehicle", index);
	}

	vehicle->setInBuilding(nullObjID);
	vehicle->removeFromGrid();

	dbugLog(_FLD, "v:%u removed from grid", index);

	vehicle->index = 0;
	vehicle->setActive(false);
	vehicle->setReallyActive(false);
	vehicle->m_spawnTime = 0;
	memset(&vehicle->serial(), 0, sizeof(WEAPON_SERIAL));
	vehicle->state = VEH_DELETED;
	memset(&vehicle->vehicleData, 0, sizeof(vehicle->vehicleData));

	Host::Vehicles::FreeHolder(vehicleHold);

	return;
}

//////////////////////////////////////////////////////////////////////
// When a player loses their connection, they are provided a grace
// period to re-establish the connection and resume control of their
// vehicle. The vehicle itself is placed onto the "limbo list" until
// it times out and is then despawned.

#ifdef LIMBO_VEHICLES
static void
_exitLimboVehicle(HostVehicle* vehicle)
{
	dbugLog(_FLN, "exitLimboVehicle(%d)", vehicle->index);
	if (vehicle == nullptr)
	{
		dbugLog(_FLA,"_exitLimboVehicle: vehicle is null");
		return;
	}

	vehicle->exitState = PEXIT_DEAD;

	// When the control player for a multi crew vehicle leaves we
	// send everyone back to the map.
	if ( !vehicle->players().empty() )
	{
		PLAYERS::iterator it = vehicle->players().begin();
		while ( it != vehicle->players().end() )
		{
			Player* player = *it;
			++it;
			commSendPlayerExit(player->teulIndex(),  PDEST_MAP, PEXIT_DEAD, nullObjID);
		}
	}
	else
		_releaseVehicle(vehicle->holder, nullObjID);
}
#endif

//////////////////////////////////////////////////////////////////////
// Remove a player from their current vehicle.

static void
_exitVehicle(Player& player)
{
	HostVehicle* const vehicle = player.vehicle;
	if ( vehicle == nullptr )
		return;

	_extractPlayerFromVehiclesList(player, *vehicle);

	FULL_OBJ_ID exitFacility;
	PLAYER_EXIT_CODE exitState = PEXIT_DEAD;
	if ( player.station == UPDT_CONTROL || player.station == UPDT_LONE )
	{
		exitState = vehicle->exitState = player.exitState;
		exitFacility = player.exitFacility;
	}
	else
	{
		exitFacility = nullObjID;
	}

	// When the control player for a multi crew vehicle leaves we
	// send everyone back to the map.
	if ( !vehicle->players().empty() && player.station == UPDT_CONTROL)
	{
		PLAYER_LIST::iterator it = vehicle->players().begin();
		while ( it != vehicle->players().end() )
		{
			Player* exitPlayer = *it;
			++it;
			if ( exitPlayer->state != Player::_TX_MAP )
			{
				commSendPlayerExit(exitPlayer->teulIndex(), PDEST_MAP, exitState, exitFacility);
			}
		}
	}

	if ( vehicle->players().empty() )
		_releaseVehicle(vehicle->m_holder, exitFacility);
	else
	{
		// Otherwise if people are still on it, check if the update mode
		// is still appropriate
		vehicle->selectBestFramingMode();
	}
}

//////////////////////////////////////////////////////////////////////
// Indicate to a player/vehicle that it should despawn.

void
hostForceVehExit(HostVehicle* vehicle)
{
	///TODO: Flag this somehow so that if the player is in mid-reconnection,
	///they won't silently and invalidly resume their connection

	FULL_OBJ_ID exitFacility = nullObjID;
	PLAYER_EXIT_CODE exitState = PEXIT_DEAD;
	auto& vplayers = vehicle->players();
	for ( auto it = vplayers.begin(); it != vplayers.end(); ++it )
	{
		auto player = *it;
		if ( player->station == UPDT_CONTROL )
		{
			exitState = player->exitState;
			exitFacility = player->exitFacility;
		}
	}

	for ( auto it = vplayers.begin(); it != vplayers.end(); )
	{
		Player* const player = *it;
		++it;

		if ( player->teulIndex() >= 0 && player->state != Player::_TX_MAP )
		{
			commSendPlayerExit(player->teulIndex(), PDEST_MAP, exitState, exitFacility);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Tidy up all of the vehicles associated with this player.
///TODO: Make sure that this takes multicrewing into account???

static SINT32
_releaseVehicleList(Player& player)
{
	if ( player.vehicle == nullptr )
		return 0;

	player.changeFPSBuilding(nullObjID, false);

	UINT32 index = player.vehicle->index;
	if ( index == 0 )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s vehicle pointer to vehicle zero"
						, player.teulIndex(), player.playerID(), player.callsign());
	}
	else
	{
		HostVehicle* const vehicle = hostFindVehicleLocal(index);
		if ( vehicle != nullptr && vehicle != player.vehicle )
		{
			dbugLog(_FLA, "t:%d p:%u c:%s player.vehicle and hostFindVehicleLocal disagree"
							, player.teulIndex(), player.playerID(), player.callsign());
		}
		else if ( vehicle != nullptr )
			_exitVehicle(player);
	}

	player.vehicle = nullptr;

	return 0;
}

void
#ifdef CLOSED_BETA
Player::processUpdateValues(UINT32 /*updateNo*/, UINT32 /*prime*/)
{
	return;
}
#else
Player::processUpdateValues(UINT32 updateNo, UINT32 prime)
{
	bool ejectworthy = false;

	if ( !m_badUpdateSeq )
	{
		if ( m_hasSentUpdate && !isInGracePeriod(60) && updateNo != m_expectUpdateNo )
		{
			dbugLog(_FLA, "t:%d p:%u c:%s*** BAD UPDATE SEQUENCE: exp:%u, got:%u"
							, teulIndex(), playerID(), callsign()
							, m_expectUpdateNo, updateNo);
			m_badUpdateSeq = true;
			ejectworthy = true;

			Chat::SendModReport(matchHandle(), Chat::ModUpdateSeq);
		}

		m_expectUpdateNo = updateNo + 1;
	}

	if ( !m_badUpdatePrime )
	{
		if ( m_hasSentUpdate )
		{
			const char* issue = nullptr;
			if ( prime > m_updatePrime )
				issue = "tampered";
			else if ( prime <= MIN_UPDATE_PRIME - UPDATE_PRIME_DEC )
				issue = "low";
			else if ( prime > INIT_UPDATE_PRIME )
				issue = "high";

			if ( issue )
			{
				dbugLog(_FLA, "t:%d p:%u c:%s*** BAD PRIME: %s had:%u got:%u"
								, teulIndex(), playerID(), callsign()
								, issue, m_updatePrime, prime);

				Chat::SendModReport(matchHandle(), Chat::ModUpdatePrime);

				m_badUpdatePrime = true;
				ejectworthy = true;
			}
		}

		m_updatePrime = prime;
	}

	// Do we need to consider ejecting this individual?
	if ( !ejectworthy || m_ejectionCountdown > 0 || m_timeToLive > 0 )
		return;

	// Is ejection enabled?
	if ( !m_flags.canEject && !host_config.arena.kickModHacks )
		return;

	m_ejectionCountdown = (rand() % 120) + (3 * (rand() % 30));
}
#endif

//////////////////////////////////////////////////////////////////////
// IRC Commands

void
hostOpenCell(Host::CommandMsg& cmd)
{
	if ( g_serverReserved == true )
	{
		g_serverReserved = false;
		chatSendAdminMessage("cell host #%u unlocked by %s", Server::localHost & 255, cmd.From());
	}
	else
	{
		cmd.Reply(Host::CommandMsg::_Caution, "Cell #%u is already unlocked.", Server::localHost & 255);
	}
}

void
hostLockCell(Host::CommandMsg& cmd)
{
	if  ( g_serverReserved == false )
	{
		g_serverReserved = true;
		g_accepting = false;
		Server::myServerInfo.stats.m_maxPlayers = 0;
		Server::myServerInfo.sync();
		chatSendAdminMessage("cell host #%u locked by %s", Server::localHost & 255, cmd.From());
	}
	else
	{
		cmd.Reply(Host::CommandMsg::_Caution, "Cell #%u is already locked.", Server::localHost & 255);
	}
}

//////////////////////////////////////////////////////////////////////
// Report the current host status.

void
hostStatusCommand(Host::CommandMsg& cmd)
{
	const char* rstate = (g_serverReserved ? "*RESERVED*" : "open");
	const char* astate = (g_accepting ? "accepting logins" : "not accepting logins");
	cmd.Reply(Host::CommandMsg::_Reply, "Cell #%u %s / %s. %d/%d players.", (Server::localHost & 255), rstate, astate, Server::myServerInfo.stats.numPlayers(), Server::myServerInfo.stats.maxPlayers());
}

//////////////////////////////////////////////////////////////////////
// List who is actually on the cell host.

void
hostWhoCommand(Host::CommandMsg& cmd)
{
	char buffer[1024];
	buffer[0] = '\0';

	char* ptr = buffer + sprintf(buffer, "Cell #%u has %" PRSizeT " player(s).", (Server::localHost & 255), g_players.size());

	UINT32 count = 0;
	for ( auto it = g_players.begin(); it != g_players.end() && count < 64; ++it, ++count  )
	{
		*ptr += ' ';
		chatGetCallsign(ptr, (*it)->chatHandle());
	}
	if ( count < g_players.size() )
		strcat(ptr, "...");

	cmd.Reply(Host::CommandMsg::_Reply, buffer);
}

//////////////////////////////////////////////////////////////////////
// Check for private messages.

void
_ircChannelMessage(irc_session_t* session, const char* event, const char* origin, const char ** params, unsigned int count)
{
	if ( origin[0] != IRC::_SERVER_PREFIX && count >= 2 && params[1][0] != '.' )
	{
		int hostnameLen = strlen(hostname);
		if ( strncmp(params[1], hostname, hostnameLen) == 0 && params[1][hostnameLen] == ' ' )
		{
			memmove(const_cast<char*>(params[1]), params[1] + hostnameLen + 1, strlen(params[1] + hostnameLen));
			wwiiIRCDefaultPrivMsg(session, event, origin, params, count);
			return;
		}
		else if ( strncmp(params[1], hostname+4, 3) == 0 && params[1][3] == ' ' )
		{
			memmove(const_cast<char*>(params[1]), params[1] + 4, strlen(params[1] + 3));
			wwiiIRCDefaultPrivMsg(session, event, origin, params, count);
			return;
		}
	}
	wwiiIRCDumpEvent(session, event, origin, params, count);
}

//////////////////////////////////////////////////////////////////////

static bool _waitForStratPeering()
{
	Teul::BlipRepeater nextReport(15000);
	Server::ServerInfo* server = nullptr;
	for (;;)
	{
		if ( g_hostTerminating )
			return false;
		teulUpdateTransport(25);
		server = Server::ServerInfo::GetBest(STRAT_HOST, true);
		if (server != nullptr)
			break;
		if (nextReport.Ready())
			dbugLog(_FLN, "waiting for strat host");
	}

	_stratHost = Server::BroadcastMaskAddress(server->stats.address);
	dbugLog(_FLN,"strat host is at %s", teulGetAddress(_stratHost));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Main entry point.

static const char* g_serverName = "hostd";

int
main(const int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, CELL_HOST, _updaterErrorFunction, _killUpdater, argc, argv, {
		{ "-db", "Deprecated.", false, [](const char*) {} },
		{ "--reserved", "Flag cell as reserved for recordings.", false, [](const char*) { g_serverReserved = true; } }
	});
	dbugSetWriteLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	threadsReadDBEnvironment();
	threadsSetup();

	dbugLog(_FLN, "Cell Host starting. Compiled %s at %s ===", __DATE__, __TIME__);

	wwiiIRCInitialize(Server::localHost, true);
	ircCallbacks.event_channel = _ircChannelMessage;
	wwiiIRCRegisterCommand("opencell", hostOpenCell);
	wwiiIRCRegisterCommand("status", hostStatusCommand);
	wwiiIRCRegisterCommand("who", hostWhoCommand);
	wwiiIRCActivate(true);

	dbugLog(_FLI, "Installing host callbacks");
	if (hostInstallAllCallbacks() < 0)
		LogAbort("error trying to install callbacks!");
	dbugLog(_FLI, "Installing update callbacks");
	if ( strtUpdateHostInstallCallbacks() < 0 )
		LogAbort("failed to installed update host callbacks");
	dbugLog(_FLI, "Installing sync callbacks");
	if ( strtSyncSupportInstallCallbacks() < 0 )
		LogAbort("failed to register sync support callbacks");

	dbugLog(_FLI, "Installing mmhost support callbacks");
	if ( mmCellHostSupportCallbacks() < 0 )
		LogAbort("failed to register mmhost support callbacks");

	// Remnant of the old stratUpdater process, which sent
	// players terrain "announcements" of new (to them) areas
	dbugLog(_FLN, "Initializing updater systems");
	strtUpdaterInit();

	Host::Vehicles::AllocateArray(MAX_HOST_VEHICLES);

	// initialize the time system
	wthrInitTime();

	static const UINT32 ClientTimeoutTime_ms = 40 * 1000;
	if ( !teulOpenListenInterface( WWII::Net::Ports::CellHost, _acceptFunction, ClientTimeoutTime_ms, _personaErrorFunction) )
		LogAbort("Unable to open tcp listen port %u", WWII::Net::Ports::CellHost);

	sprintf(hostname, "cell%u", Server::localHost & 255);

	const UINT32 updateNet = cv_baseUpdtAddress.GetUint32();
	if ( updateNet != 0 )
	{
		dbugLog(_FLM, "Clustering enabled: Broadcast network %s specified.", cv_baseUpdtAddress.GetString());

		const UINT32 updateBcastHost = cv_baseUpdtAddress.GetUint32() | 0x000000ff;
		updateBcastRecv = teulOpenSend(updateBcastHost, WWII::Net::Ports::PrivateBcast, nullptr, 0, PROTOCOL_UDB, nullptr);
		dbugLog(_FLN, "t:%d updateBcastRecv:%s", updateBcastRecv, teulGetAddress(updateBcastHost));

		const UINT32 updateDirectHost = cv_baseUpdtAddress.GetUint32() | (Server::localHost & 0x000000ff);
		updateBcastSend = teulOpenSend(updateDirectHost, WWII::Net::Ports::PrivateBcast, nullptr, 0, PROTOCOL_UDB, nullptr);
		dbugLog(_FLN, "t:%d updateBcastSend:%s", updateBcastSend, teulGetAddress(updateDirectHost));
	}
	else
	{
		g_clustered = false;
		dbugLog(_FLM, "Clustering disabled: No broadcast network address provided.");
	}

	serverOpenBroadcastIndexes();

	Server::myServerInfo.stats.port = WWII::Net::Ports::CellHost;

	SINT32 initResult =
	serverInit(HOST_VERSION
				, CELL_HOST, MAP_HOST
				, _serverEnterCallback
				, _serverLeaveCallback
				, _serverStatusCallback
				, Player::Create
				, _personaRelease
				, _personaActivate
				, _personaExit
				, nullptr
				, _personaErrorFunction);
	ABORT_IF(initResult == -EINVAL);

	Server::myServerInfo.stats.m_maxPlayers = (g_serverReserved ? 0 : cv_maxCellPlayers.GetUint32());

	Server::myServerInfo.flags = (Server::Flags)(Server::SVRFLAG_SEND_PSTAT | Server::SVRFLAG_RELOGIN);

	wwiiIRCChannelMessage("-sys", "[[ Syncing ]]");

	g_exitHandlerRequired = true;
	atexit(exitHandler);

	Server::myServerInfo.stats.status = Server::SERVER_INITIALIZING;
	Server::myServerInfo.sync();

	// Build collision boxes for all CPs
	strtBuildCPColliders();

	dbugLog(_FLN, "Starting updater systems");
	strtUpdaterProcessEvents();

	dbugLog(_FLI, "host initialized, at address %s ready for players", teulGetAddress(Server::localHost));

	std::unique_ptr<StatsThread> statsThread(new StatsThread);
	statsThread->start();
	threadsEnable();

	// It's a sure bet that any sorties currently open with our cell_host_ip
	// aren't spawned in any more - so mark them all as closed.
	_closeAllOpenSortiesOnThisCell();

	if (!_waitForStratPeering() )
		return 0;

	// Wait for the chat host to start.
	teulSyncClock();
	const TeulTimeMS timeout = teulClock64 + 15 * 1000;
	TeulTimeMS longTimeout = teulClock64 + 30 * 1000;
	while ( teulClock64 < timeout || (teulClock64 < longTimeout && serverPeeringWithType(CHAT_HOST) == false) )
	{
		if (g_hostTerminating)
			return 0;
		teulUpdateTransport(50);
		threadsLogging();
	}

	if ( !g_accepting && !g_serverReserved )
		g_accepting = true;

	// And just to be safe
	for ( UINT32 i = 0; i < 100; ++i )
	{
		teulUpdateTransport(10);
		threadsLogging();
	}

	threadsLogging();

	if (g_hostTerminating)
		return 0;

	chatSendAdminMessage("cell host #%u %s", Server::localHost & 255, (g_serverReserved ? "*RESERVED*" : "online"));

	_runHost();

	statsThread->cancel();
	statsThread->join();

	exitHandler();
}

void HCUnit::setHomeCPID(const FULL_OBJ_ID id)
{
	m_homeCPID = id;
}

void
hostUserAbortedApp(Player* player)
{
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
	chatSendBroadcastAll(LSTR_NOTE, LSTR_PLAYER_ALT_F4ED, player->callsign());
#endif

	// So note it in the mission packets
	///TODO:135: Create an "alt-f4" event.

	///TODO: Ensure this results in a sortie closure
	if ( player->vehicle && player->vehicle->crew <= 1 )
	{
		player->vehicle->setVehicleKilled(0);
		player->vehicle->setCrewDead();
	}

	commDropPlayer(*player, false);
}

//////////////////////////////////////////////////////////////////////
// Prevent a vehicle from having any more updates.

void
updateUnscheduleVehicle(HostVehicle* const vehicle)
{
	vehicle->m_nextUpdateClock = ~0ULL;
}

//////////////////////////////////////////////////////////////////////
// Set a specific schedule for a particular vehicle

void
updateScheduleVehicle(HostVehicle* vehicle, UINT32 interval)
{
#ifdef DEBUG
	assert( vehicle->isRemote() == false );
#endif

	vehicle->m_updateInterval = interval;
	vehicle->m_nextUpdateClock = teulClock64 + interval;
}

struct InstanceFilterFn
{
	bool operator() (const wwiiPlayer* const player) const { return player->instance().isCampaign(); }
};

void
Host::TimeUpdate::Broadcast()
{
	TeulTransmission broadcast(Host::TimeUpdate::Callback);

	broadcast.populate(this);
	broadcast.send(g_players.begin(), g_players.end(), InstanceFilterFn());
}

//////////////////////////////////////////////////////////////////////
// Attach player and vehicle.

void
Player::setVehicle(HostVehicle* const vehicle_)
{
	vehicle = vehicle_;
	m_sortieID = vehicle->index;

	// The player in-charge of the vehicle provides the vehicle mission packet
	if ( station == UPDT_CONTROL || station == UPDT_LONE )
		vehicle->attachControllingPlayer(*this);
}

//////////////////////////////////////////////////////////////////////
// Send a packet to all players attached to a vehicle.

void
HostVehicle::sendPacket(Teul::Packet& packet_)
{
	if ( players().empty() )
		return;
	if ( players().size() == 1 )
	{
		Player* const player = players().front();
		if ( player->teulIndex() >= 0 )
			packet_.send(player->teulIndex());
	}
	else
	{
		std::vector<SINT32> indexes;
		for ( auto it = players().begin(); it != players().end(); ++it )
		{
			Player* const player = (*it);
			if ( player->teulIndex() >= 0 )
				indexes.push_back(player->teulIndex());
		}

		for ( auto it = indexes.begin(); it != indexes.end(); ++it )
		{
			packet_.send(*it);
		}
	}
}

