#include <syscfg.h> // Path is project specific.
// Date: Sun Jun 19 14:58:45 CDT 2005
//
// Code that processes a spawn grant response and tries to launch
// a vehicle into the world.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "include/logging.h"
#include "map/mapPlayer.h"
#include "miss/missionMission.h"
#include "mm/mmHostMessages.h"
#include "mm/mmSpawnGrantResponse.h"
#include "server/serverInterface.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverServerInfo.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtMapTypes.h"
#include "strt/strtTypes.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulBasic.h"
#include "weap/weapDefs.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiThreads.h"
#include "host/hostVehicleRequest.h"
#include "wwii/wwiiServerTypeEnums.h"

extern SINT32 mapSendHostSupercellData(SINT32 teulIndex);	// From mapHostSupercellData.cpp

//////////////////////////////////////////////////////////////////////
// Exception classes for failing during spawn grant

class SpawnGrantFailure
{
public:
	explicit SpawnGrantFailure(int code, const char* const reason, bool loseVehicle) : m_code(code), m_reason(reason), m_loseVehicle(loseVehicle) {}
	virtual ~SpawnGrantFailure() {}

	int m_code;
	const char* const m_reason;
	bool m_loseVehicle;
};

class SGFLostCrew : public SpawnGrantFailure
{
public:
	explicit SGFLostCrew(const char* const reason) : SpawnGrantFailure(SPAWN_LOST_CREW, reason, true) {}
	virtual ~SGFLostCrew() {}
};

class SGFSpawningDisabled : public SpawnGrantFailure
{
public:
	explicit SGFSpawningDisabled(const char* const reason, bool loseVehicle)
		: SpawnGrantFailure(SPAWN_DISABLED_BY_SERVER, reason, loseVehicle) {}
	virtual ~SGFSpawningDisabled() {}
};

//////////////////////////////////////////////////////////////////////
// Constructor for a VehicleRequest

Host::VehicleRequest::VehicleRequest(const instanceid_t& instanceID_, const MM_SPAWN_GRANT_RESPONSE& grant, const Player& commander, const Player* const crew)
	: m_instanceID(instanceID_)
	, m_weaponSerial(grant.weaponSerial)
	, m_brigadeID(commander.brigadeID())
	, m_missionID(commander.missionID())
	, m_players()
	, m_handle(commander.chatHandle())
	, m_decal(commander.decal())
	, m_rank(commander.effectiveRank())
	, m_flags(0)
{
	m_players.push_back(commander.playerID());
	if ( crew )
		m_players.push_back(crew->playerID());
}

//////////////////////////////////////////////////////////////////////
// Populate a packet buffer with mission packet data and
// associate the data to the player's sendInfo

static void
_sendVehicleToCell(const MM_SPAWN_GRANT_RESPONSE& grant, const Player& commander, const Player* const crew, Server::ServerInfo* const server)
{
	Host::VehicleRequest vr(commander.instance().id(), grant, commander, crew);

	if ( g_logDebug )
		dbugLog(_FLD, "v:%u sending vehicle to cell %s with players %u,%u"
						, vr.m_weaponSerial.serial
						, teulGetAddress(server->stats.address)
						, vr.playerNo(0)
						, vr.playerNo(1)
						);

	vr.send(server);
}

//////////////////////////////////////////////////////////////////////
// check the crew component of a spawn grant response

static void
_checkCrew(const UINT32 playerID, const Player* const crew, const Player* const commander)
{
	if ( playerID == 0 )
		return;

	if ( crew == NULL )
		throw SGFLostCrew("crew absconded");
	if ( crew->teulIndex() < 0 )
		throw SGFLostCrew("crew not connected");
	if ( !crew->validMissionPacket() )
		throw SGFLostCrew("invalid crew mission packet");
	if ( crew->country() != commander->country() )
		throw SGFLostCrew("country mismatch with crew");
	if ( crew->brigadeID() != commander->brigadeID() )
		throw SGFLostCrew("brigade mismatch with crew");
	if ( crew->missionID() != commander->missionID() )
		throw SGFLostCrew("mission mismatch with crew");
}

//////////////////////////////////////////////////////////////////////
// If the request is for a mobile spawn, get the co-ordinates if the
// request is valid and we can honor it.

static inline UINT32
_getMobileSpawn(const Player* const commander, FLOAT64* const x, FLOAT64* const y, FLOAT64* const z)
{
	Mission* const mission = commander->mission();
	if ( !mission || mission->isMobileSpawnAvailable() == false )
		return 0;

#if !defined(DOROTHY)
	///TODO: Determine if the mobile spawn is suitable for the vehicle
	if ( commander->weapon().spec.wCategory != _INFANTRY )
		return 0;
#endif

	MobileSpawn& spawner = mission->spawner();

	*x = spawner.m_x;
	*y = spawner.m_y;
	*z = spawner.m_z;

	// Add one to the player's counter
	spawner.incrementSpawnees(commander->playerID());

	return 1; // spawner.m_vehicleID;
}

//////////////////////////////////////////////////////////////////////
// Find the best cell host for a spawn request based on the commander.

static Server::ServerInfo*
_getBestCell(const MM_SPAWN_GRANT_RESPONSE& grant, const Player* const commander)
{
	Server::ServerInfo* const server = Server::ServerInfo::GetBest(CELL_HOST);
	if ( server == NULL )
		throw SGFSpawningDisabled("no cell hosts available for spawning", true);
	return server;
}

//////////////////////////////////////////////////////////////////////
// Send the spawn-in map data to the commander & his crew.

static void
_sendMapData(Player* const player)
{
	if ( player->instance().hasStrat() == false )
		return;

	const SINT32 cmdrResult = mapSendHostSupercellData(player->teulIndex());
	if ( cmdrResult < 0 )
		throw SpawnGrantFailure(SPAWN_ERROR, "Unable to send strat to player", false);
}

//////////////////////////////////////////////////////////////////////
// Figure out where the player will be spawning if they're using a
// mobile spawn or direct spawn or something.

static void
_determineSpawnCoordinates(const MM_SPAWN_GRANT_RESPONSE& grant, const Player* const commander, FLOAT64* const x, FLOAT64* const y, FLOAT64* const z, UINT32* const mobileSpawnID)
{
	if ( commander->m_useCustomSpawnLocation )
	{
		*mobileSpawnID = grant.weaponSerial.serial;
		*x = commander->m_customSpawnLocation.x;
		*y = commander->m_customSpawnLocation.y;
		*z = commander->m_customSpawnLocation.z;
	}
	else if ( commander->m_wantMobileSpawn )
		*mobileSpawnID = _getMobileSpawn(commander, x, y, z);
}

//////////////////////////////////////////////////////////////////////
// Forward the given player off to the cell host.

static void
_sendPlayerInfoToCell(Player& player, Server::ServerInfo& server, const MM_SPAWN_GRANT_RESPONSE& grant, const Player& commander, const bool ejectable)
{
	// Add the final touches needed to the mission packet.
	player.missionPacket().sortie().sortieID = grant.weaponSerial.serial;
	player.m_flags.canEject = ejectable;
	player.missionPacket().player().decal = commander.decal();

	// Notify the cell host and tell the client's teulkit to connect to it.
	player.mirrorToCellHost(&server);
}

//////////////////////////////////////////////////////////////////////
// Send spawn notify to a player.

static SINT32
_sendSpawnNotify(Player& player, const MM_SPAWN_GRANT_RESPONSE& grant, const Player& commander, const UINT32& mobileSpawnID, const FLOAT64& x, const FLOAT64& y, const FLOAT64& z)
{
	player.setRequestedCapacity(false);
	player.setReceivedCapacity(false);

	return mmSendSpawnNotify(player.teulIndex()
							, grant.weaponSerial
							, mobileSpawnID, x, y, z
							, commander.decal(), commander.isOfficer(), commander.isBuilder(), commander.isMissionLeader()
							);
}

//////////////////////////////////////////////////////////////////////
// The central part of processing the request

static LOGICAL
_processSpawnGrantResponse(const MM_SPAWN_GRANT_RESPONSE& grant
							, Player* const commander
							, PlayerStatusNode* const commandPlayer
							, Player* crew
							, PlayerStatusNode* const crewPlayer
							)
{
	if ( grant.code != SPAWN_GRANTED )
	{
		throw SpawnGrantFailure(grant.code, "strat host denied spawn", true);
	}

	if ( !commander->validMissionPacket() )
		throw SpawnGrantFailure(SPAWN_DISABLED_BY_SERVER, "commander missing mission packet", true);

	// See if there is a server available for them.
	Server::ServerInfo* const server = _getBestCell(grant, commander);

	_sendMapData(commander);

	// Check if the crew component is valid.
	_checkCrew(grant.crew, crew, commander);

	// Tell the cell host about the vehicle.
	_sendVehicleToCell(grant, *commander, crew, server);

	// Obtain any pertinent mobile spawn information.
	FLOAT64 x = 0.0f, y = 0.0f, z = 0.0f;
	UINT32 mobileSpawnID = 0;

	_determineSpawnCoordinates(grant, commander, &x, &y, &z, &mobileSpawnID);

	const bool ejectable = (commander->permissions(CAN_BE_EJECTED) || (crew && crew->permissions(CAN_BE_EJECTED)));

	// Add the final touches to the mission packet and
	_sendPlayerInfoToCell(*commander, *server, grant, *commander, ejectable);
	// Tell the commander's front end to spawn.
	const SINT32 cmdrResult = _sendSpawnNotify(*commander, grant, *commander, mobileSpawnID, x, y, z);
	if ( cmdrResult < 0 )
		throw SGFSpawningDisabled("error sending spawn notify to player", true);

	commander->newSortie(grant.weaponSerial.serial, true);
	commander->SetState(PERS_SPAWNING);
	commander->updateMissionTaker(false);
	commander->updateChat();

	if ( crew != NULL )
	{
		_sendPlayerInfoToCell(*crew, *server, grant, *commander, ejectable);
		//Tell the crew front end to spawn
		SINT32 crewResult = _sendSpawnNotify(*crew, grant, *commander, mobileSpawnID, x, y, z);
		if ( crewResult < 0 )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s error sending spawn notify to crew: %d"
							, crew->teulIndex(), crew->playerID(), crew->callsign(), crewResult);
			crew->SetWeapon(NULL_WEAP_SERIAL);
			crew->updateMissionTaker(false);
			crew->updateChat();
			crew = NULL;
		}
		else
		{
			crew->newSortie(grant.weaponSerial.serial, false);
			crew->SetState(PERS_SPAWNING);
			crew->updateMissionTaker(false);
			crew->updateChat();
		}
	}

	///TODO: Add the "mount" parameter somehow (which vehicle you're riding)

	const Mission* mission = commander->mission();
	const WEAPON_SPEC& spec = commander->weapon().spec;

	if ( g_logDebug )
	{
		auto& instance = commander->instance();
		dbugLog(_FLD, "Notifying cell:%s spawning t:%d,p:%u,c:%s sq:%u(%s) ct:%d,cp:%u,cc:%s in:%u.%u.%llu.%llu %s#%d [%f/%f/%f]"
						, teulGetAddress(grant.serverIP)
						, commander->teulIndex(), grant.commander, commander->callsign()
						, commander->squadID(), Chat::Name(commander->squadHandle()).c_str()
						, ((crew) ? crew->teulIndex() : 0), grant.crew, ((crew) ? crew->callsign() : "n/a")
						, instance.category(), instance.type(), instance.handle(), instance.id()
						, weapPrintName(&spec), commander->weapon().serial
						, x, y, z
						);
	}

	// Create the sortie record
	DBThread::Do(DBThread::Default
		, "CALL wwii.p_add_sortie("
			" %u"
			",%u,%u"
			",%u,%u"
			",%u"
			",%u"
			",%u,%u,%u,%u"
			",%u,%u"
			",%u"
			")"
			, grant.weaponSerial.serial
			, commander->playerID(), commander->personaID()
			, (crew ? crew->playerID() : 0), (crew ? crew->personaID() : 0)
			, commander->brigadeID()
			, (mission ? mission->originFac()->uOID : 0)
			, static_cast<UINT32>(spec.wCountry), static_cast<UINT32>(spec.wCategory), static_cast<UINT32>(spec.wClass), static_cast<UINT32>(spec.wType)
			, (mission ? mission->missionType() : 0), (mission ? mission->missionID() : 0)
			, (grant.serverIP & 255)
	);

	return TRUE;
}

void
mmFakeSpawnGrantResponse(Player* const player, const UINT32 serialNumber)
{
	MM_SPAWN_GRANT_RESPONSE grant;

	grant.code = SPAWN_GRANTED;
	grant.weaponSerial = player->weapon();
	grant.weaponSerial.serial = serialNumber;
	player->SetWeapon(grant.weaponSerial);

	Server::ServerInfo* const server = Server::ServerInfo::GetBest(CELL_HOST);
	if ( server == NULL )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s v:%u No cells available"
						, player->teulIndex(), player->playerID(), player->callsign()
						, serialNumber);
		return;
	}

	grant.serverIP = server->stats.address;

	grant.commander = player->playerID();
	grant.crew = 0;
	grant.capacity = 0;

	player->m_wantMobileSpawn = true;

	mmProcessSpawnGrantResponse(grant);

	player->m_wantMobileSpawn = false;
}

LOGICAL
mmProcessSpawnGrantResponse(const MM_SPAWN_GRANT_RESPONSE& grant)
{
	PlayerStatusNode* const commandPlayer = ServerInterface::PlayerStatusNodeByPlayerID(grant.commander);
	if ( commandPlayer == NULL )
	{
		dbugLog(_FLA, "p:%u v:%u No commandPlayer for vehicle to spawn", grant.commander, grant.weaponSerial.serial);
		return FALSE;
	}

	Player* const commander = (Player*)commandPlayer->localInfo;
	if ( commander == NULL )
	{
		dbugLog(_FLA, "p:%u v:%u commander is NULL", grant.commander, grant.weaponSerial.serial);
		return FALSE;
	}

	PlayerStatusNode* crewPlayer = NULL;
	Player* crew = NULL;
	if ( grant.crew > 0 )
	{
		crewPlayer = ServerInterface::PlayerStatusNodeByPlayerID(grant.crew);
		if ( crewPlayer != NULL )
			crew = (Player*)crewPlayer->localInfo;
	}

	try
	{
		// We are basically an evaluation/wrapper shell for _process...
		return _processSpawnGrantResponse(grant, commander, commandPlayer, crew, crewPlayer);
	}
	catch (SpawnGrantFailure& e)
	{
		dbugLog(_FLA, "t:%d p:%u c:%s v:%d crew:%d %s [FAIL: %d, lv:%s]"
						, commander->teulIndex()
						, commander->playerID()
						, commander->callsign()
						, grant.weaponSerial.serial
						, grant.crew
						, e.m_reason
						, e.m_code
						, (e.m_loseVehicle ? "yes" : "no")
						);

		commander->postDenied(grant.weaponSerial.spec, e.m_code, e.m_loseVehicle);
	}

	return FALSE;
}

