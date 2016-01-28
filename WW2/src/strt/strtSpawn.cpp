#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 23 10:31:33 2001
//
// All the spawn initialization and functions for use in the strat System
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "versionNo.h"

#include <algorithm>

#if platOS != OS_WIN32
#include <netinet/in.h>
#endif

#include "include/arenaLockEnums.h"
#include "strt/strtConstants.h"
#include "strt/strtServerTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtDB.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFacilityWeaponList.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtSpawn.h"
#include "strt/strtIO.h"
#include "strt/strtData.h"
#include "weap/weapList.h"
#include "weap/weapInternal.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtHostMessages.h"
#include "cnvy/cnvyMain.h"
#include "chat/chatHandle.h"
#include "strt/strtWeaponsReturn.h"
#include "strt/strtWeaponEntry.h"
#include "wwii/wwiiConfig.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatSupportMessages.h"
#include "strt/strtHostDB.h"
#include "strt/strtCP.h"
#include "strt/strtSupply.h"
#include "strt/strtHostLua.h"
#include "strt/strtHCUnits.h"
#include "strt/strtPostListReply.h"
#include "server/serverServerInfo.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "wwii/wwiiExitCodes.h"

#define SPAWN_TIMEOUT (9 * 60 * 60)

#ifdef SPAWN_RESTRICT
SPAWN_RESTRICTION spawnRestriction = { 0, "\0", 0, 0, 0 };
#endif // SPAWN_RESTRICT

///REVIEW:TOES
//Weapons that have been spawned, no longer attached to facilities
static POSTED_WEAPON_LIST _spawnedWeapons;

static bool _canSpawn(const TOE_::WeaponInstance*
						, const StratHCUnit& brigade
						, const FacilityStruct& facility
						, const WeaponEntry& spawnEntry
						, const PermissionMask& permMask
						, bool includeZeros
						, bool isReserve
						);
static void _validateCrew(POSTED_WEAPON& currentWeapon);

static LOGICAL _setupSpawnObject(ObjectStruct& object);

extern const std::string& sdmTranslateName(const std::string& from);

//////////////////////////////////////////////////////////////////////
//

static bool
_spawnDisabledByGM(const PermissionMask& permMask)
{
	if ( !host_config.isArenaLocked(AL_SPAWN) )
		return false;

	if ( (permMask & (IS_PLAYNET|ALL_SPAWN|GAME_MANAGER)) != 0 )
		return false;

	// Otherwise, we aren't letting you
	return true;
}

//////////////////////////////////////////////////////////////////////
//

static UINT16
_constructWeaponSerial(WEAPON_SERIAL* serial, WeaponEntry* spawnEntry)
{
	assert( serial != nullptr );
	assert( spawnEntry != nullptr );

	// Cycle the spawner slots
	UINT32 portal = rand() % spawnEntry->portals().size();
	const ObjectStruct& spawner = spawnEntry->portals(portal);

	serial->source = spawner.fullObjID;
	serial->serial = 0;
	serial->spec = spawnEntry->spec();

	return portal;
}

//////////////////////////////////////////////////////////////////////
//

struct PostRequestFail
{
	PostRequestFail(const char* withWhat, SPAWN_DENY_REASONCODE withCode)
		: m_what(withWhat), m_code(withCode)
		{}
	const char* what() const { return m_what; }
	SPAWN_DENY_REASONCODE code() const { return m_code; }
	const char* const m_what;
	SPAWN_DENY_REASONCODE m_code;
};

//////////////////////////////////////////////////////////////////////
// Indicate that this facility can spawn the weapons in a given
// WEAPON_LIST through a specific building (object)

static void
_addWeaponListToFacility(FacilityStruct* facility, WEAPON_LIST* weaponList, ObjectStruct& object, UINT8 category)
{
	assert( facility != nullptr );
	assert( weaponList != nullptr );

	///TODO:+TOE:Minor A copy of this for every facility is handy, but surely very wasteful?
	WEAPON_LIST::iterator it;
	for ( it = weaponList->begin(); it != weaponList->end(); ++it )
	{
		if ( category == 0 || it->first.wCategory == category )
			facility->addWeaponSpawner(*it->second, object);
	}
}

//////////////////////////////////////////////////////////////////////
// Inner part of process post request

static POSTED_WEAPON*
_postWeapon(WeapJoinRequestNode* joinNode
			, FacilityStruct& facility
			, bool isReserve
			, WEAPON_SPEC weapon
			, SINT32 missionID
			, UINT32 brigadeID
			, UINT32 supplyUnitID
			, const PermissionMask& permMask
			, UINT32 commander
			, const ChatHandle& handle
			, bool isImmediateSpawn
			, bool isMulticrew
			)
{
	if ( joinNode == nullptr )
	{
		dbugLog(_FLA, "postWeapon called with NULL join node");
		return nullptr;
	}

	StratHCUnit* unit = StratHCUnit::Lookup(brigadeID);
	if ( unit == nullptr )
	{
		dbugLog(_FLA, "p:%u c:%s hc:%u invalid brigadeID"
						, commander, Chat::Name(handle).c_str(), brigadeID);
		return nullptr;
	}

	StratHCUnit* supplyUnit;
	if ( supplyUnitID == brigadeID )
		supplyUnit = unit;
	else
	{
		supplyUnit = StratHCUnit::Lookup(supplyUnitID);
		if ( supplyUnit == nullptr )
		{
			dbugLog(_FLA, "p:%u c:%s hc:%u sup:%u invalid supplyUnitID"
						, commander, Chat::Name(handle).c_str(), brigadeID, supplyUnitID);
			return nullptr;
		}
	}

	WeaponEntry* spawnEntry = facility.weaponEntry(weapon);
	if ( spawnEntry == nullptr )
		return nullptr;

	TOE_::WeaponInstance* toeEntry = supplyUnit->toeWeaponInstance(weapon);
	if ( toeEntry == nullptr && spawnEntry->weapon().isSystemWeapon() == false && !(permMask & ALL_SPAWN) )
	{
		dbugLog(_FLA, "p:%u c:%s hc:%u sup:%u requested non-toe weapon: %s"
						, commander, Chat::Name(handle).c_str(), brigadeID, supplyUnitID
						, weapPrintName(&weapon));
		return nullptr;
	}

	if ( facility.supplyingFacility(*supplyUnit, false) == nullptr )
	{
		if ( facility.supplyingFacility(*unit, false) == nullptr )
		{
			dbugLog(_FLA, "p:%u c:%s hc:%u sup:%u no supply link"
							, commander, Chat::Name(handle).c_str(), brigadeID, supplyUnitID);
			return nullptr;
		}
		dbugLog(_FLN, "p:%u c:%s hc:%u sup:%u switching supply to brigade"
							, commander, Chat::Name(handle).c_str(), brigadeID, supplyUnitID);
		supplyUnitID = brigadeID;
		supplyUnit = unit;
	}

	//We found it!!!
	if ( _canSpawn(toeEntry, *supplyUnit, facility, *spawnEntry, permMask, false, isReserve) == false )
		return nullptr;

	// If we are spawning a weapon with the ALL_SPAWN permission,
	// or if we are spawning a weapon for which none are in stock, then
	// don't try to reinforce me.
	bool isNonSupplied = false;
	if ( toeEntry == nullptr || toeEntry->weapon().isSupplied(permMask) == false )
	{
		isNonSupplied = true;
#ifdef BETA_RELEASE
		if ( toeEntry == nullptr || (permMask & ALL_SPAWN) )
			TELL_TESTER(commander, "you have all spawn");
		else if ( toeEntry->access() == 0 )
			TELL_TESTER(commander, "free weapon");
		else if ( toeEntry->weapon().isSystemWeapon() )
			TELL_TESTER(commander, "system weapon");
#endif
	}
	else if ( isReserve == false && toeEntry->present() == 0 )
	{
		isNonSupplied = true;
		TELL_TESTER(commander, "Non-supplied mission vehicle");
	}

	WEAPON_SERIAL serial;
	UINT16 portal = _constructWeaponSerial(&serial, spawnEntry);

	POSTED_WEAPON* pw;
	pw =  new POSTED_WEAPON(serial, portal, missionID
							, commander, handle, 0
							, unit, supplyUnit, &facility
							, isMulticrew, isImmediateSpawn, isNonSupplied);
	if ( pw == nullptr )
		LogAbort("Failed to create POSTED_WEAPON entry");

	POSTED_WEAPON& postedWeapon = *pw;

	//Populate its fields with good values
	joinNode->AddToList(&postedWeapon.m_crewList);

	//insert it into the postedWeapon List for this facility
	postedWeapon.post(spawnEntry, false);

	///TODO:+TOE: This code needs to be moved to the StratHCUnit or TOE, it shouldn't be done this directly here.
	if ( postedWeapon.isNonSupplied() == false )
	{
		//If they are on a mission, take one of the mission weapons
		//Don't schedule reinforcement -- if the server craps out,
		//we want these vehicles returned more quickly
		if ( toeEntry && toeEntry->present() > 0 )
			toeEntry->setPresent(toeEntry->present() - 1);

		dbugLog(_FLD,"facility %s %s %s mission %s for %s - %u/%u"
					, facility.name
					, (isReserve) ? "reserved" : "posted"
					, (spawnEntry->isThrottled()) ? "throttled" : "non-throttled"
					, unit->name()
					, Chat::Name(handle).c_str()
					, (toeEntry ? toeEntry->present() : 0)
					, (toeEntry ? toeEntry->capacity() : 0)
					);
	}

	return &postedWeapon;
}

//////////////////////////////////////////////////////////////////////
// Allocate a new join node for a playerID, making sure we delete any
// pre-existing node.

static WeapJoinRequestNode*
_getNewJoinNode(UINT32 playerID, const ChatHandle& handle, UINT32 missionID)
{
	WeapJoinRequestNode* joinNode = WeapJoinRequestNode::Lookup(playerID);
	if ( joinNode )
		delete joinNode;
	return new WeapJoinRequestNode(playerID, handle, missionID);
}

//////////////////////////////////////////////////////////////////////
//

bool
strtProcessPostRequest(SINT32 teulIndex, const MM_POST_REQUEST& request, MM_POST_ACCEPT& accept, MM_POST_DENY& deny)
{
	//these parts of the deny don't change
	deny.customerID = request.customerID;
	deny.weapon = request.weapon;

	// Test for GM spawn limits
	if ( _spawnDisabledByGM(request.permMask) )
	{
		deny.reasonCode = SPAWN_DISABLED_BY_GM;
		return false;
	}

	//Get the facility first
	FacilityStruct* facility = strtGetFacility(request.facility);
	if (facility == nullptr)
	{
		dbugLog(_FLW, "p:%u c:%s couldn't find facility " PRF_FULL_OBJ_ID " denying post"
				, request.customerID, Chat::Name(request.handle).c_str()
				, PRA_FULL_OBJ_ID(request.facility)
				);

		deny.reasonCode = SPAWN_FACILITY_NOT_FOUND;

		return false;
	}

	try
	{
		if ( facility->isOpen() == false )
			throw PostRequestFail("facility closed", SPAWN_FACILITY_CLOSED);

		if ( g_logDebug )
			dbugLog(_FLD, "p:%u c:%s br:%u m:%u post request %s @ %s"
						, request.customerID
						, Chat::Name(request.handle).c_str()
						, request.brigadeID
						, request.missionID
						, weapPrintName(&request.weapon)
						, facility->name
						);

		UINT32 supplyUnitID = strtGetSupplyingUnit(*facility, request.brigadeID);
		if ( supplyUnitID == 0 )
			supplyUnitID = request.brigadeID;

		// Make sure it contains our brigade
		StratHCUnit* unit = StratHCUnit::Lookup(supplyUnitID);
		if ( unit == nullptr || (!unit->isBrigade() && !unit->isDivision()) )
			throw PostRequestFail("not a recognized brigade/division", SPAWN_NO_BRIGADE_SUPPLY);

		FacilityStruct* supplyFacility = facility->supplyingFacility(*unit, false);
		if ( supplyFacility == nullptr )
			throw PostRequestFail("no supplying facility", SPAWN_NO_BRIGADE_SUPPLY);

		WeapJoinRequestNode* joinNode = _getNewJoinNode(request.customerID, request.handle, request.missionID);
		if ( joinNode == nullptr )
			throw PostRequestFail("getNewJoinNode returned NULL", SPAWN_FACILITY_NOT_FOUND);

		//Now we setup the entry in the posted weapon list
		POSTED_WEAPON* postedWeapon;
		postedWeapon = _postWeapon(joinNode
								, *facility
								, false
								, request.weapon
								, request.missionID
								, request.brigadeID
								, supplyUnitID
								, request.permMask
								, request.customerID
								, request.handle
								, request.immediateSpawn != FALSE
								, request.multicrew != FALSE
								);

		if ( !postedWeapon )
		{
			TELL_TESTER(request.customerID, "request for %s at %s denied", weapPrintFullName(&request.weapon), facility->name);
			delete joinNode;
			throw PostRequestFail("no posted weapon", SPAWN_WEAPON_NOT_AT_FACILITY);
		}

		TELL_TESTER(request.customerID, "posted #%u %s at %s"
					, postedWeapon->serial()
					, weapPrintFullName(&request.weapon)
					, facility->name
					);

		// When we are successful, the join request node will linger
		//Likewise this part of the post accept
		accept.customerID = request.customerID;
		accept.immediateSpawn = request.immediateSpawn;
		accept.multicrew = request.multicrew;
		accept.facility = request.facility;
		accept.portal = postedWeapon->m_portal;
		accept.weaponSerial = postedWeapon->m_weaponSerial;

		return true;
	}
	catch ( PostRequestFail& e )
	{
		dbugLog(_FLN, "p:%u c:%s br:%u m:%u %s@%s postReqFail %u: %s"
						, request.customerID
						, Chat::Name(request.handle).c_str()
						, request.brigadeID
						, request.missionID
						, weapPrintName(&request.weapon)
						, facility->name
						, e.code()
						, e.what()
						);

		deny.reasonCode = e.code();
	}

	return false;
}

void
MM_POSTED_WEAPON::copy(const POSTED_WEAPON& rhs)
{
	weaponSerial = rhs.m_weaponSerial;
	portal = rhs.m_portal;
	commander = rhs.m_commander;
	crew = rhs.m_crew;
	missionID = rhs.m_missionID;
}

//////////////////////////////////////////////////////////////////////
// Detach everyone from the vehicle - joiners and crew, and possily
// even the commander too.

void
POSTED_WEAPON::detachCrewAndJoiners(bool includeCommander)
{
	if ( g_logDebug )
		dbugLog(_FLD, "p:%u v:%u deleting %s crew (crew:%p, join:%p)", m_commander, serial(), (includeCommander ? "all" : "joined"), &m_crewList, &m_joinList);

	if ( includeCommander )
	{
		m_crewList.clear(0, false);
		assert( m_crewList.empty() == true );
	}
	else
	{
		m_crewList.clear(m_commander, false);
	}

	if ( m_status != POSTED_WEAPON::_POSTED && m_status != POSTED_WEAPON::_RESERVED )
	{
		detachJoiners(false);
		return;
	}

	detachJoiners(true);

	if ( m_crew != 0 )
	{
		strtSendMMJoinDeny(m_crew, MM_JOIN_FAIL_POSTER_DENIED_ACCESS);
		m_crew = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Detach the join list.

void
POSTED_WEAPON::detachJoiners(bool announceFails)
{
	m_joinList.clear(0, announceFails);
}

static void
_cleanUpJoinNode(WeapJoinRequestNode* joinNode)
{
	switch(joinNode->m_joinList->m_type)
	{
		case JOIN_LIST::_FREE:
			dbugLog(_FLA,"illegal node state JOIN_LIST_FREE on spawnCancel of id %d",joinNode->m_playerID);
			break;
		case JOIN_LIST::_FACILITY:
			if ( g_logDebug )
			{
				const FULL_OBJ_ID& fid = joinNode->m_joinList->m_facility->fullObjID;
				dbugLog(_FLD, "deleting join %p id %d post from facility " PRF_FULL_OBJ_ID ""
						, joinNode, joinNode->m_playerID
						, PRA_FULL_OBJ_ID(fid)
						);
			}
			break;
		case JOIN_LIST::_POST_JOIN_REQ:
		case JOIN_LIST::_POST_CREW:
		{
			if ( joinNode->m_joinList->m_weapon == nullptr )
				break;
			POSTED_WEAPON& postedWeapon = *joinNode->m_joinList->m_weapon;

			if (joinNode->m_playerID == postedWeapon.m_commander)
			{
				_validateCrew(postedWeapon);

				// We have to be able to push the vehicle count up by 1.
				postedWeapon.returnWeapon(true, false);
				return;
			}
			else
			{
				if (joinNode->m_playerID == postedWeapon.m_crew)
				{
					//Found it.  Take the crewmember off
					postedWeapon.m_crew = 0;
					postedWeapon.setMulticrew(false);
				}

				strtSendMMCrewBailed(postedWeapon.m_commander, joinNode->m_playerID);
			}
		}
	}

	delete joinNode;
}

//Processes a post list request.
SINT32
strtProcessPostListRequest(SINT32 teulIndex, const MM_POST_LIST_REQUEST& request)
{
	FacilityStruct* facility;
	WeapJoinRequestNode* joinNode;

	//Get the facility first
	facility = strtGetFacility(request.facility);
	if (facility == nullptr)
	{
		betaLog(_FLD, "NULL facility for post list request");
		return -EINVAL;
	}

	StratHCUnit* unit = StratHCUnit::Lookup(request.brigadeID);
	if ( unit == nullptr )
	{
		dbugLog(_FLI, "hcu:%u unrecognized brigade", request.brigadeID);
		return -EINVAL;
	}

	joinNode = _getNewJoinNode(request.crewID, request.crewHandle, request.missionID);
	if ( !joinNode )
	{
		betaLog(_FLW, "NULL join node for post list request");
		return -EINVAL;
	}

	assert( facility->m_joinList != nullptr );

	joinNode->AddToList(facility->m_joinList);

	// Find all the posted weapons that are valid for us.
	const size_t maxPossibleSize = sizeof(PostListReply) + sizeof(MM_POSTED_WEAPON) * unit->postedWeaponList().size() + 16;
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_POST_LIST_Y));
	outgoing.reserve(maxPossibleSize);
	outgoing.construct<PostListReply>(COMMANDED_WEAPON_ADD, request.crewID, facility->uOID);

	UINT32 weapons = 0;
	POSTED_WEAPON_LIST::const_iterator it;
	for ( it = unit->postedWeaponList().begin(); it != unit->postedWeaponList().end();++it )
	{
		const POSTED_WEAPON& weapon = *(*it);
		if ( weapon.m_missionID != request.missionID
			|| (weapon.m_status != POSTED_WEAPON::_POSTED && weapon.m_status != POSTED_WEAPON::_RESERVED)
			|| weapon.m_multicrew == false )
			continue;

		MM_POSTED_WEAPON weaponMM(weapon.m_weaponSerial, weapon.m_portal, weapon.m_commander, weapon.m_missionID, weapon.m_crew);
		outgoing.append(weaponMM);
		++weapons;
	}

	// Queue the RPC
	outgoing.send(teulIndex);

	return weapons;
}

LOGICAL
strtProcessJoinRequest(const MM_JOIN_REQUEST& request, MM_JOIN_ACCEPT& accept, MM_JOIN_DENY& deny)
{
	ObjectStruct* object;
	POSTED_WEAPON* currentWeapon;
	UINT32 serial = request.weaponSerial.serial;

	//Setup reasonable values for the return
	accept.customerID = request.customerID;
	deny.customerID = request.customerID;

#ifdef NEVER
	if ( _spawnDisabledByGM(0) )
	{
		deny.reasonCode = SPAWN_DISABLED_BY_GM;
		return FALSE;
	}
#endif

	//Get the facility first
	object = strtGetObject(request.weaponSerial.source);
	if (object == nullptr)
	{
		deny.reasonCode = SPAWN_FACILITY_NOT_FOUND;
		return FALSE;
	}

	FacilityStruct& facility = object->Facility();
	if ( facility.side != request.weaponSerial.spec.side() )
	{
		deny.reasonCode = SPAWN_FACILITY_CLOSED;
		return FALSE;
	}

	currentWeapon = strtFindPostedWeapon(serial);
	if ( currentWeapon == nullptr )
	{
		deny.reasonCode = SPAWN_WEAPON_NOT_AT_FACILITY;
		dbugLog(_FLW, "p:%u couldn't find v:%u", request.customerID, serial);
		return FALSE;
	}
	if ( currentWeapon->m_crew != 0 )
	{
		deny.reasonCode = SPAWN_WEAPON_NOT_AT_FACILITY;
		dbugLog(_FLN, "p:%u v:%u join request denied: already has crew", request.customerID, serial);
		return FALSE;
	}

	//If we found it, *AND* nobody else has jumped in it
	WeapJoinRequestNode* joinNode = WeapJoinRequestNode::Lookup(request.customerID);
	if (joinNode == nullptr)
	{
		deny.reasonCode = SPAWN_WEAPON_NOT_AT_FACILITY;
		dbugLog(_FLW, "p:%u got NULL joinNode", request.customerID);
		return FALSE;
	}

	joinNode->AddToList(&currentWeapon->m_crewList);

	currentWeapon->m_crew = request.customerID;

	// Tell everyone on my join list to go away.
	currentWeapon->m_joinList.clear(0, true);

	currentWeapon->setMulticrew(false);

	accept.postedWeapon.copy(*currentWeapon);

	return TRUE;
}

static void
_validateCrew(POSTED_WEAPON& post)
{
	WeapJoinRequestList::const_iterator it;
	for ( it = post.m_crewList.begin(); it != post.m_crewList.end(); ++it )
	{
		WeapJoinRequestNode* joinNode = *it;
		if ( joinNode->m_playerID == 0 || WeapJoinRequestNode::Lookup(joinNode->m_playerID) != joinNode )
		{
			LogAbort("p:%u v:%u invalid crew %p (%u)!!", post.m_commander, post.m_weaponSerial.serial, joinNode, joinNode->m_playerID);
		}
	}

	dbugLog(_FLD, "p:%u c:%s v:%u crew valid for vehicle %p", post.m_commander, Chat::Name(post.m_handle).c_str(), post.m_weaponSerial.serial, &post);
}

//This function processes a spawn grant request
LOGICAL
strtProcessSpawnGrantRequest(const MM_SPAWN_GRANT_REQUEST& request, MM_SPAWN_GRANT_RESPONSE& response)
{
	UINT32 serial = request.weaponSerial.serial;

	//Populate the response;
	response.weaponSerial = request.weaponSerial;
	response.serverIP = 0;
	response.commander = request.playerID;
	response.crew = 0;

	//If found, send the commander and crew back to the mmhost
	POSTED_WEAPON* currentWeapon = strtFindPostedWeapon(serial);
	if (currentWeapon == nullptr)
	{
		dbugLog(_FLA, "v:%u spawnGrant: couldn't find weapon", serial);
		response.code = SPAWN_WEAPON_NOT_AT_FACILITY;
		return FALSE;
	}

	if ( currentWeapon->m_facility == nullptr )
	{
		dbugLog(_FLI, "v:%u null facility", serial);
		response.code = SPAWN_FACILITY_NOT_FOUND;
		return FALSE;
	}
	if ( currentWeapon->m_facility->isOpen() == false )
	{
		dbugLog(_FLI, "v:%u no spawning: facility %s is closed", serial, currentWeapon->m_facility->name);
		response.code = SPAWN_FACILITY_CLOSED;
		return FALSE;
	}

	//Pick the server that I'm going to send commander and crew to
	Server::ServerInfo* server = Server::ServerInfo::GetBest(CELL_HOST);
	if (server == nullptr)
	{
		dbugLog(_FLA, "v:%u spawnGrant: couldn't find cell server to send players to", serial);
		response.code = SPAWN_DISABLED_BY_SERVER;
		return FALSE;
	}

	// Incase they've changed mission
	if ( currentWeapon->m_status == POSTED_WEAPON::_RESERVED )
		currentWeapon->m_missionID = request.missionID;

	switch ( currentWeapon->m_status )
	{
		case POSTED_WEAPON::_RESERVED:
			betaLog(_FLD, "v:%u received spawnRequest for reserved %s"
						, serial
						, weapPrintName(&currentWeapon->spec())
						);
			break;
		case POSTED_WEAPON::_POSTED:
			betaLog(_FLD, "v:%u received spawnRequest for new %s"
						, serial
						, weapPrintName(&currentWeapon->spec())
						);
			break;
		case POSTED_WEAPON::_SPAWNED:
			dbugLog(_FLA,"v:%u recieved spawnRequest for spawned vehicle weapon %p!", serial, currentWeapon);
			response.code = SPAWN_ERROR;
			return FALSE;
			break;
		case POSTED_WEAPON::_FREE:
		case POSTED_WEAPON::_CANCELLED:
		default:
			LogAbort("v:%u invalid status %d weapon %p!", serial, currentWeapon->m_status, currentWeapon);
			response.code = SPAWN_ERROR;
			return FALSE;
			break;
	}

	// Place a spawned weapon entry and schedule a new reinforcement
	if ( !currentWeapon->spawn() )
	{
		dbugLog(_FLW, "Spawn Error in currentWeapon->spawn!");
		response.code = SPAWN_ERROR;
		return FALSE;
	}

	TELL_TESTER(currentWeapon->m_commander
				, "v#%u spawning %s at %s"
				, serial
				, weapPrintFullName(&currentWeapon->spec())
				, currentWeapon->m_facility->name
				);

	response.serverIP = server->stats.address;
	response.commander = currentWeapon->m_commander;
	response.crew = currentWeapon->m_crew;

	response.code = SPAWN_GRANTED;

	return TRUE;
}

//This function processes a post info request
LOGICAL
strtProcessJoinListRequest(SINT32 teulIndex, const MM_POST_INFO_REQUEST& request)
{
	ObjectStruct* object;
	FacilityStruct* facility;
	POSTED_WEAPON* currentWeapon;

	//Find the posted Vehicle
	object = strtGetObject(request.weaponSerial.source);
	if (object == nullptr)
		return FALSE;

	facility = object->facility;
	if (facility == nullptr)
		return FALSE;

	WeapJoinRequestNode* joinNode = WeapJoinRequestNode::Lookup(request.customerID);
	if (joinNode == nullptr)
	{
		// Why not create one, eh?
		return FALSE;
	}

	// Find the posted weapon
	currentWeapon = strtFindPostedWeapon(request.weaponSerial.serial);
	if ( currentWeapon == nullptr || currentWeapon->multicrew() == false )
		return FALSE;

	// Remove the crew from the facility list of joiners and move them onto
	// the vehicle's list of wannabes.
	joinNode->AddToList(&currentWeapon->m_joinList);

	strtSendMMJoinListResponse(currentWeapon->m_commander, joinNode->m_playerID, joinNode->m_handle);

	return TRUE;
}

//This function processes a post info request
LOGICAL
strtProcessPostInfoRequest(const MM_POST_INFO_REQUEST& request, MM_POST_INFO_RESPONSE& response)
{
	ObjectStruct* object;
	FacilityStruct* facility;

	response.join = request.join;

	//Find the posted Vehicle
	object = strtGetObject(request.weaponSerial.source);
	if (object == nullptr)
		return FALSE;

	facility = object->facility;
	if (facility == nullptr)
		return FALSE;

	// Find the posted weapon
	//If found, send the posted weapon back to the mmhost
	POSTED_WEAPON* currentWeapon = strtFindPostedWeapon(request.weaponSerial.serial);
	if (currentWeapon == nullptr || ! currentWeapon->multicrew())
		return FALSE;

	response.postedWeapon.copy(*currentWeapon);
	response.customerID = request.customerID;

	return TRUE;
}

void
POSTED_WEAPON::setMulticrew(bool isMulticrew)
{
	assert( m_facility->m_joinList != nullptr );

	if ( m_multicrew == isMulticrew )
		return;

	m_multicrew = isMulticrew;

	COMMANDED_WEAPON_ACTION action = ( isMulticrew ) ? COMMANDED_WEAPON_ADD : COMMANDED_WEAPON_DELETE;

	WeapJoinRequestList::const_iterator it;
	for ( it = m_facility->m_joinList->begin(); it != m_facility->m_joinList->end(); ++it )
	{
		const WeapJoinRequestNode* node = *it;
		if ( node->m_missionID != m_missionID )
			continue;

		MM_POSTED_WEAPON weaponMM(this->m_weaponSerial, this->m_portal, this->m_commander, this->m_missionID, this->m_crew);

		Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_POST_LIST_Y));
		outgoing.reserve(sizeof(PostListReply) + sizeof(weaponMM));
		outgoing.construct<PostListReply>(action, node->m_playerID, m_facility->uOID);
		outgoing.append(weaponMM);
		serverSendPacketToType(outgoing, MAP_HOST);
	}
}

//////////////////////////////////////////////////////////////////////
//This function processes a post info update

bool
strtProcessPostInfoUpdate(MM_POST_INFO_UPDATE* infoUpdate)
{
	if (infoUpdate == nullptr)
		return false;

	// Lookup the weapon
	POSTED_WEAPON* currentWeapon = strtFindPostedWeapon(infoUpdate->weaponSerial.serial);
	if (currentWeapon == nullptr)
		return false;

	currentWeapon->detachCrewAndJoiners(false);
	currentWeapon->setMulticrew(infoUpdate->multicrew != FALSE);

	return true;
}

//////////////////////////////////////////////////////////////////////
//

LOGICAL
strtProcessSpawnCancel(MM_SPAWN_CANCEL* spawnCancel)
{
	if (spawnCancel == nullptr)
		return FALSE;

	WeapJoinRequestNode* joinNode = WeapJoinRequestNode::Lookup(spawnCancel->customerID);
	if (! joinNode)
		return FALSE;

	_cleanUpJoinNode(joinNode);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Examine the facility suggested in toFacility and determine if
// it is suitable for this weapon to return to. If so, attempt to
// locate a spawn object for this weapon at that facility

static FacilityStruct *
_verifyRTBFacility(const WEAPON_SPEC& spec, const FULL_OBJ_ID& rtbFacility, const ChatHandle& handle)
{
	static const ChatHandle systemHandle = chatGetHandle("SYSTEM");
	Chat::Name callsign((handle == 0ULL) ? systemHandle : handle);

	// Nothing to do if we have a null object ID.
	if ( rtbFacility.id == 0 )
	{
		// No facility means the client doesn't think you're rtbing any place
		dbugLog(_FLI, "vrtb: %s facilityID", callsign.c_str());
		return nullptr;
	}

	FacilityStruct*	facility = strtGetFacility(rtbFacility);
	if ( facility == nullptr )
	{
		dbugLog(_FLW, "vrtb: no facility matches " PRF_FULL_OBJ_ID " for %s"
						, PRA_FULL_OBJ_ID(rtbFacility)
						, callsign.c_str());
		return nullptr;
	}

	// It must be a spawn-type facility or else
	if ( !facility->m_spawnable )
	{
		dbugLog(_FLW, "vrtb: %s had non-spawnable facility", callsign.c_str());
		return nullptr;
	}

	// Host of reasons we might exclude this facility
	if ( facility->isOpen() == false )
	{
#ifdef DEBUG
		dbugLog(_FLD, "vrtb: %s@%s: facility is closed", callsign.c_str(), facility->name);
#endif
		return nullptr;
	}

	// Non-matching ownerships
	if ( facility->side != spec.side() )
	{
		dbugLog(_FLD, "vrtb: %s@%s wrong side", callsign.c_str(), facility->name);
		return nullptr;
	}

	return facility;
}

//////////////////////////////////////////////////////////////////////
// Reserve a clone of this weapon at m_facility

bool
POSTED_WEAPON::reReserve()
{
	// Remove all passengers.
	detachCrewAndJoiners(false);
	setMulticrew(false);

	try
	{
		if ( m_commander == 0 || m_handle == 0LL )
			throw "Vehicle has no commander";

		if ( m_facility == nullptr )
			throw "No facility";

		if ( m_facility->open == false )
			throw "Facility closed";

		TOE_::WeaponInstance* toeEntry = m_supplyingBrigade->toeWeaponInstance(spec());
		if ( toeEntry == nullptr )
			throw "Weapon not on supplying unit's TOE";

		WeaponEntry* spawnEntry = m_facility->weaponEntry(spec());
		if ( spawnEntry == nullptr )
			throw "No spawn entry at this facility";

		if ( _canSpawn(toeEntry, *m_supplyingBrigade, *m_facility, *spawnEntry, 0LL, true, true) == false )
			throw "Spawn denied";

		// Relinquish my old serial number.
		UINT32 oldSerial = serial();
		_spawnedWeapons.remove(this);
		_unregister();

		// Register me a new serial
		m_portal = _constructWeaponSerial(&m_weaponSerial, spawnEntry);
		_register();

		post(spawnEntry, true);

		dbugLog(_FLD, "p:%u c:%s hcu:%u(%s) v:%u=>%u posted reserve @ %s"
						, m_commander, Chat::Name(m_handle).c_str()
						, m_brigade->unitID(), m_brigade->callsign()
						, oldSerial, serial()
						, m_facility->name
						);

		chatSendLocalizedMessage(m_commander, LSTR_NOTE, LSTR_RESERVED_AT, weapPrintFullName(&spec()), m_facility->name);

		return true;
	}
	catch ( const char* reason )
	{
		TELL_TESTER(m_commander, "re-reserve failed: %s", reason);
		dbugLog(_FLW, "p:%u c:%s v:%u reReserve failed: %s"
						, m_commander, Chat::Name(m_handle).c_str(), serial()
						, reason
						);
		detachCrewAndJoiners(true);

		return false;
	}
}

//// End of code pertaining to local pool returns

//////////////////////////////////////////////////////////////////////
//

#ifdef BETA_RELEASE
void
_reportWeaponStatus(const POSTED_WEAPON* currentWeapon, const WEAPON_SPEC& spec, bool rtbPromoted)
{
	if ( currentWeapon->m_status == POSTED_WEAPON::_SPAWNED )
	{
		const char* deadOrAlive = (currentWeapon->isDead()) ? "Dead" : "Alive";
		const char* RTBstate;
		switch ( currentWeapon->m_playerExitCode )
		{
			case PEXIT_UNINITIALIZED: RTBstate = "INVALID"; break;
			case PEXIT_RTB: RTBstate = "RTB"; break;
			case PEXIT_DEAD: RTBstate = "Dead"; break;
			case PEXIT_OVER_ENEMY_TERRITORY: RTBstate = "MIA"; break;
			case PEXIT_OVER_FRIENDLY_TERRITORY: RTBstate = "Rescued"; break;
			default:
				RTBstate = "-unknown-?";
			break;
		}
		TELL_TESTER(currentWeapon->m_commander, "You returned %s%s and your %s was %s"
						, RTBstate
						, (rtbPromoted) ? "*" : ""
						, weapPrintFullName(&spec)
						, deadOrAlive
						);
	}
}
#endif // BETA_RELEASE

//////////////////////////////////////////////////////////////////////
//

void
WeaponsReturn::process() const
{
	POSTED_WEAPON* const currentWeapon = strtFindPostedWeapon(m_sortieID);
	if ( currentWeapon == nullptr )
	{
		dbugLog(_FLW, "v:%u no match for returning weapon", m_sortieID);
		return;
	}

	// Copy the status fields across from the return into the vehicle
	currentWeapon->m_playerExitCode = m_exitCode;
	currentWeapon->m_isDead = m_isVehicleDead;

	static char info[128];
	sprintf(info, "p:%u c:%s v:%u return:", currentWeapon->m_commander, Chat::Name(currentWeapon->m_handle).c_str(), m_sortieID);

	dbugLog(_FLD, "%s to " PRF_FULL_OBJ_ID " co-ords %d.%d.%d ex:%u %s %s %s"
			, info
			, PRA_FULL_OBJ_ID(m_rtbFacility)
			, m_position.x(), m_position.y(), m_position.z()
			, m_exitCode
			, (m_isVehicleDead ? "dead" : "alive")
			, (m_hasBeenHit ? "hasBeenHit" : "unhit")
			, (m_resupplyMission ? "resupply" : "stdsupply")
			);

	FacilityStruct*	destFacility = currentWeapon->m_facility;

#ifdef BETA_RELEASE
	_reportWeaponStatus(currentWeapon, currentWeapon->spec(), m_rtbPromoted);
#endif

	FacilityStruct*		rtbFacility = nullptr;
	SINT32				capacityTeulIndex = -1;
	MM_CAPACITY_REQUEST capacityRequest;
	SINT32				crewIndex = -1;
	MM_CAPACITY_REQUEST	crewRequest;
	bool				overSupply = false;

	if ( currentWeapon->m_status == POSTED_WEAPON::_SPAWNED && m_exitCode == PEXIT_RTB )
	{
		WeapJoinRequestNode* joinNode = nullptr;
		// Has a commander, so it will need a capacity request
		if ( currentWeapon->m_commander > 0 )
		{
			joinNode = WeapJoinRequestNode::Lookup(currentWeapon->m_commander);
			if ( joinNode != nullptr && joinNode->m_capacityTeulIndex >= 0 )
			{
				dbugLog(_FLD, "%s requires a capacity response", info);
				capacityTeulIndex = joinNode->m_capacityTeulIndex;
				capacityRequest = joinNode->m_capacityRequest;

				// Clear the joinNode flag
				joinNode->m_capacityTeulIndex = -1;
			}
		}

		if ( currentWeapon->m_crew > 0 )
		{
			joinNode = WeapJoinRequestNode::Lookup(currentWeapon->m_crew);
			if ( joinNode != nullptr && joinNode->m_capacityTeulIndex >= 0 )
			{
				crewIndex = joinNode->m_capacityTeulIndex;
				crewRequest = joinNode->m_capacityRequest;
				joinNode->m_capacityTeulIndex = -1;
			}
		}

		// Determine the RTB facility
		if ( m_isVehicleDead == false )
		{
			rtbFacility = _verifyRTBFacility(currentWeapon->spec(), m_rtbFacility, currentWeapon->m_handle);

			if ( rtbFacility != nullptr )
			{
				betaLog(_FLD, "%s rtbFacility is %s", info, rtbFacility->name);

				if ( rtbFacility->weaponEntry(currentWeapon->spec()) != nullptr )
				{
					if ( rtbFacility != destFacility )
					{
						betaLog(_FLD, "%s found new destination: %s", info, rtbFacility->name);
						destFacility = rtbFacility;
						overSupply = true;
					}
					else
					{
						betaLog(_FLD, "%s rtbing to origin: %s", info, rtbFacility->name);
					}
				}
			}
		}
	}

	if ( currentWeapon->m_status == POSTED_WEAPON::_SPAWNED && m_exitCode == PEXIT_RTB )

	dbugLog(_FLD,"%s status: %d. exitCode:%d. isDead:%d. %s returning from %s to %s [%s]. cti:%d"
			, info
			, currentWeapon->m_status
			, m_exitCode
			, m_isVehicleDead
			, (currentWeapon->m_crewList.empty()) ? "EMPTY_LIST" : Chat::Name(currentWeapon->m_crewList.front()->m_handle).c_str()
			, currentWeapon->m_facility->name
			, destFacility->name
			, (overSupply) ? "O/SUP" : "LMTD"	// oversupply or limited
			, capacityTeulIndex
			);

	bool reserveAgain =
		capacityTeulIndex >= 0
		&& rtbFacility != nullptr
		&& rtbFacility == destFacility
		&& currentWeapon->m_status == POSTED_WEAPON::_SPAWNED
		&& currentWeapon->m_playerExitCode == PEXIT_RTB
		&& currentWeapon->isDead() == false
		;

	bool vehicleDelivery = false;
	if ( reserveAgain && m_resupplyMission == true
			&& currentWeapon->m_brigade
			&& rtbFacility->cp == currentWeapon->m_brigade->homeCP()
			&& rtbFacility->cp != currentWeapon->m_supplyingBrigade->homeCP() )
	{
		reserveAgain = false;
		vehicleDelivery = true;
		rtbFacility = currentWeapon->m_facility;
	}

	currentWeapon->m_facility = destFacility;

	bool needsReturn = true;
	if ( reserveAgain == true )
	{
		// Try and pre-reserve the weapon instead
		if ( currentWeapon->reReserve() == true )
			needsReturn = false;
	}

	bool vehicleDelivered = false;
	if ( needsReturn )
	{
		vehicleDelivered = currentWeapon->returnWeapon(overSupply, m_hasBeenHit);
	}

	// Live clients need a capacity response to finish their debrief
	if ( capacityTeulIndex >= 0 )
	{
		// Are we relocating?
		FULL_OBJ_ID destObjID = nullObjID;
		if ( vehicleDelivery == true && vehicleDelivered == true )
			destObjID = rtbFacility->fullObjID;
		else if ( destFacility->side == currentWeapon->spec().side() )
			destObjID = destFacility->fullObjID;
		strtReplyToCapacityRequest(capacityTeulIndex, capacityRequest, destObjID, vehicleDelivered);
		if ( crewIndex >= 0 )
			strtReplyToCapacityRequest(crewIndex, crewRequest, destObjID, vehicleDelivered);
	}

	return;
}

void
strtExpireWeapon(POSTED_WEAPON& weapon)
{
	assert( weapon.m_facility != nullptr );

	dbugLog(_FLI, "c:%s v:%u expiring %s", Chat::Name(weapon.m_handle).c_str(), weapon.m_weaponSerial.serial, weapPrintName(&weapon.spec()));

	strtSendPostExpired(weapon.m_commander, weapon.m_weaponSerial, false);

	weapon.returnWeapon(true, false);
}

//////////////////////////////////////////////////////////////////////
// Prepare the vehicle lists to and from the databases;
// load TOE templates, etc.

void
strtInitSpawn(void)
{
#if defined(IS_STRAT_HOST)
	// Make sure all the vehicles we know about are now in the database.
	strtdbPopulateVehTypesTable();
#endif

	// Now read in data from it
	strtdbReadVehicleAccess();

#if defined(IS_STRAT_HOST)
	// Save any un-saved, modified CVC values
	strtdbSaveVehicleCVCs();
#endif

	// Read in the TOEs and Brigades
	strtLoadTOEsAndHCUnits();
}

//////////////////////////////////////////////////////////////////////
// Introduce the weapons in a spawn.xml virtual class to the list of
// spawnable vehicles through a given member of a facility

static void
_setupClassSpawn(ObjectStruct& object, WEAPON_SPEC classSpec)
{
	WEAPON_LIST* weaponList = weapFindSpawnList(classSpec);
//	if ( weaponList == NULL )
//	{
//		dbugLog(_FLW, "%s: empty list for %s in spawn.xml", object.name, object.filename);
//		return;
//	}

	// Well then, this is our spawn ability, isn't it?
//	object.spawnAbility.weap = classSpec;

	_addWeaponListToFacility(object.facility, weaponList, object, 0);
}

//Spawn is setup on a facility level
SINT32
strtSetupSpawn(FacilityStruct* facility)
{
	ObjectStruct* object;

	if (facility == nullptr)
		return -1;

	if ((facility->country <= WWII_NO_COUNTRY) || (facility->country >= MAX_COUNTRIES))
	{
		dbugLog(_FLA, "facility %s ownership is %d, not a known country", facility->name, facility->country);
		return -1;
	}

	facility->m_joinList = new JOIN_LIST(JOIN_LIST::_FACILITY, facility, nullptr);

	for ( object = facility->objectList; object != nullptr; object = object->next )
	{
		if ( object->type != BUILDING_SPAWN )
			continue;

//		WEAPON_SPEC classSpec = weapFindClass(object->filename);
//		if ( classSpec.wClass != 0 )
		if ( object->spawnAbility.weap.wCategory == 0 && object->spawnAbility.weap.wClass > 0 )
		{
			dbugLog(_FLI, "setting up class spawn for %s/%s", facility->name, object->name);
//			_setupClassSpawn(*object, classSpec);
			_setupClassSpawn(*object, object->spawnAbility.weap);
			continue;
		}

#ifdef DEBUG_HEAVY
		dbugLog(_FLI, "setting up spawn for facility %s, country %d", facility->name, country);
#endif
		_setupSpawnObject(*object);
	}

	return 0;
}

void
strtExpireSpawnedWeapons()
{
#ifdef BETA_RELEASE
	LOGICAL	debugSupplyEnabledMemory = strtGetDebugSupply();

	strtDisableDebugSupply();
#endif

	UINT32 timeout = teulTime() - SPAWN_TIMEOUT;

	dbugLog(_FLD,"checking spwaned list for timeouts");

	//While the smallest timestamp is over 4 hours old
	while ( !_spawnedWeapons.empty() && _spawnedWeapons.front()->m_posted <= timeout )
	{
		POSTED_WEAPON* weapon = _spawnedWeapons.front();
		dbugLog(_FLD, "v:%u has been spawned for %u seconds, returning to spawn list"
                       	, weapon->m_weaponSerial.serial
                       	, teulTime() - weapon->m_posted);

		// If it has resupply pending, tough.
		weapon->detachReinforcement();

		//Return it to the facility, dont allow oversupply
		weapon->returnWeapon(false, false);

		assert( _spawnedWeapons.front() != weapon );
	}

#ifdef BETA_RELEASE
	if ( debugSupplyEnabledMemory )
		strtEnableDebugSupply();
#endif
}

//////////////////////////////////////////////////////////////////////
// This used to be more complex, because it had to pull lists for
// each country, now that it uses the same process as setup class
// the two could probably be merged.

static LOGICAL
_setupSpawnObject(ObjectStruct& object)
{
	if ( object.facility == nullptr )
		LogAbort("obj:%s not attached to any facility", object.name);

	FacilityStruct& facility = *object.facility;

	// Get the list of vehicles that can be spawned at this category
	// of facility
	WEAPON_SPEC spawnAbility = object.spawnAbility.weap;

	WEAPON_LIST* weaponList = weapFindSpawnList(spawnAbility);
	if ( weaponList == nullptr || weaponList->empty() )
	{
		static const std::string s_DealDock = sdmTranslateName("Deal Dock");
		if ( strcmp(facility.name, s_DealDock.c_str()) == 0 )
		{
			// I think this is intended to only print the error once
			dbugLog(_FLA,"Could not setup spawn for object %s " PRF_FULL_OBJ_ID ", no class for " PRF_WEAPON_SPEC ""
						, object.name
						, PRA_FULL_OBJ_ID(object.fullObjID)
						, PRA_WEAPON_SPEC(spawnAbility)
						);
		}
		return FALSE;
	}

	_addWeaponListToFacility(object.facility, weaponList, object, spawnAbility.wCategory);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Report on the current supply-or-not state of a facility

FacilityWeaponList::REASON_CODE
_diagnoseSupply(const StratHCUnit& unit, const FacilityStruct& facility, const FacilityStruct*& supplyFacility)
{
	supplyFacility = nullptr;

	if ( facility.isOpen() == false )
		return FacilityWeaponList::_CLOSED;

	if ( facility.side != unit.side() )
		return FacilityWeaponList::_ENEMY_OWNS;

	FacilityStruct* mate;
	FacilityStruct* mateAB;

//	if ( facility.country != country )
//		return FacilityWeaponList::_WRONG_OWNER;

	switch ( facility.type )
	{
		case FACILITY_FIREBASE:
			if ( facility.armyBase->isOpen() == false || facility.armyBase->side != unit.side() )
				return FacilityWeaponList::_SUPPLY_CLOSED;
			supplyFacility = facility.findLocalAB(unit.side());
			if ( supplyFacility == nullptr || supplyFacility->canABSupply(unit.side()) == false )
				return FacilityWeaponList::_NOT_LINKED;
		break;

		case FACILITY_DEPOT:
			supplyFacility = facility.findLocalAB(unit.side());
			if ( supplyFacility != nullptr )
			{
				if ( supplyFacility->side != unit.side() )
					return FacilityWeaponList::_NOT_LINKED;
				if ( supplyFacility->uOID != facility.uOID )
				{
					if ( supplyFacility->cp == unit.homeCP() )
						return FacilityWeaponList::_NORMAL;
					// Are we allowed to link back to our brigade?
					if ( unit.isBrigade() == false || facility.canFollowSupplyLink() == false )
						return FacilityWeaponList::_NOT_LINKED;
				}
			}

			// We couldn't find a source for this depot.
			// Start by assuming we're in self-supply mode.
			supplyFacility = &facility;

			if ( facility.m_incomingLink == nullptr && facility.m_outgoingLink == nullptr )
				return FacilityWeaponList::_CUT_BY_NO_LINKS;

			// Is our mate friendly?
			mate = facility.m_incomingLink->startDepot;
			if ( mate == nullptr )
				return FacilityWeaponList::_CUT_BY_NO_LINKS;

			// Make sure its a reciprocal link
			if ( mate != facility.m_incomingLink->endDepot )
				return FacilityWeaponList::_CUT_BY_NO_LINKS;

			if ( facility.m_outgoingLink->hasEnemyFirebase(unit.side()) )
				return FacilityWeaponList::_CUT_BY_FB;

			// Don't own both ends of the link
			if ( mate->side != unit.side() )
				return FacilityWeaponList::_CUT_BY_MATE;

			if ( mate->cp != unit.homeCP() )
				return FacilityWeaponList::_NOT_LINKED;

			mateAB = mate->findLocalAB(unit.side());
			if ( mateAB == nullptr || mateAB == mate )
				return FacilityWeaponList::_CUT_OFF;

			if ( mateAB->cp->side != unit.side() )
				return FacilityWeaponList::_CUT_OFF;

			supplyFacility = mateAB;

			return FacilityWeaponList::_NORMAL;
		break;

		default:
			supplyFacility = facility.supplyingFacility(unit, false);
			if ( supplyFacility == nullptr )
				return FacilityWeaponList::_NOT_LINKED;
		break;
	}

	// Depots have been dealt with, now we know we're a military fac.

	if ( supplyFacility->side != unit.side() )
		return FacilityWeaponList::_WRONG_OWNER;

	if ( facility.cp->side != unit.side() )
		return FacilityWeaponList::_ENEMY_OWNS_CP;

	if ( facility.cp->controllingSide != unit.side() )
		return FacilityWeaponList::_ENEMY_CONTROLS_CP;

	if ( supplyFacility->isOpen() == false )
		return FacilityWeaponList::_SUPPLY_CLOSED;

	return FacilityWeaponList::_NORMAL;
}

static const UINT32 _NUM_ACCESS_LEVELS = 16;
char _accessLevels[_NUM_ACCESS_LEVELS];

//////////////////////////////////////////////////////////////////////
// Determine the player rank required for a particular access level

void
strtInitializeVehicleAccessLevels()
{
	WEAPON fakeWeapon;

	fakeWeapon.spec().wCountry = 1;

	// For each access level, find the lowest rank
	// that can access the weapon
	bool disableRankLimits = (host_config.arena.vehicleRestrictions == false);
	for ( UINT32 a = 0; a < _NUM_ACCESS_LEVELS; ++a )
	{
		fakeWeapon.m_access = a;
		const Rank* rank;
		for ( UINT32 r = 0; r <= Rank::max(); ++r )
		{
			rank = Rank::lookup(r);
			if ( rank && fakeWeapon.isPermitted(rank->permissions(), disableRankLimits) )
			{
				dbugLog(_FLI, "Access level %u requires rank %u", a, r);
				_accessLevels[a] = r;
				break;
			}
		}
	}

	dbugLog(_FLI, "Done initializing vehicle access levels");
}

//////////////////////////////////////////////////////////////////////
// Prepare a list of what vehicles the player can see at a given
// facility for them to render as a spawn list.

bool
FacilityWeaponList::populate(const StratHCUnit& unit, const FacilityStruct& facility, UINT32 playerID, const PermissionMask& permMask)
{
	const bool allWeapons = ((permMask & ALL_SPAWN) != 0);

	m_refreshSeconds = host_config.supply.listRefresh_s;
	m_numWeapons = 0;

	if ( !unit.isSpawnable() )
	{
		m_status = FacilityWeaponList::_BDE_DISABLED;
		return TRUE;
	}

	// Is this facility open ?
	const FacilityStruct* supplyFacility = nullptr;
	m_status = _diagnoseSupply(unit, facility, supplyFacility);

	if ( m_status > FacilityWeaponList::_NORMAL )
		return TRUE;

	if ( (m_status > FacilityWeaponList::_INVALID && m_status < FacilityWeaponList::_NORMAL) || supplyFacility == nullptr )
		supplyFacility = &facility;

	if ( facility.type == FACILITY_DEPOT && supplyFacility == &facility )
		m_status = FacilityWeaponList::_CUT_OFF;

	const FacilityStruct::WEAPON_ENTRIES& weapons = facility.weaponEntries();
	if ( weapons.empty() )
	{
		m_status = FacilityWeaponList::_NON_SPAWNABLE;
		return TRUE;
	}

	if ( m_status == FacilityWeaponList::_NORMAL && unit.isSupplyInTransit() )
		m_status = FacilityWeaponList::_SUPPLY_IN_TRANSIT;

	m_nextSupplyM = (unit.getSecondsToNextSupply() + 59) / 60;

	betaLog(_FLI, "p:%u fac:%s aw:%s", playerID, facility.name, (allWeapons ? "yes" : "no"));

	unit.checkThrottleWindows(&facility);

	FacilityStruct::WEAPON_ENTRIES::const_iterator it;
	UINT32 numWeapons = 0;
	for ( it = weapons.begin(); it != weapons.end(); ++it )
	{
		const WeaponEntry& spawnEntry = *it->second;

		// Make sure that the brigade allows spawning of these.
		const TOE_::WeaponInstance* toeEntry = unit.toeWeaponInstance(spawnEntry.spec());
		if ( toeEntry == nullptr && spawnEntry.spec().wCountry != unit.country() )
			continue;
		SINT16 access = (toeEntry ? toeEntry->access() : spawnEntry.access());

		m_weapons[numWeapons] = spawnEntry.spec();
		WeaponCounter& counter = m_counters[numWeapons];
		m_levels[numWeapons] = _accessLevels[access];

		if ( _canSpawn(toeEntry, unit, facility, spawnEntry, permMask, true, false) == true )
		{
			spawnEntry.getCounter(&counter, toeEntry);
			if ( counter.capacity > 0 || allWeapons || counter.current > 0 || spawnEntry.weapon().isSupplied(permMask) == false )
			{
				if ( counter.current == 0 && (allWeapons == false && spawnEntry.weapon().isSystemWeapon() == false) )
					counter.spawnable = (counter.capacity > 0) ? WeaponCounter::NOT_PRESENT : WeaponCounter::NOT_DEPLOYED;
				else
					counter.spawnable = WeaponCounter::SPAWNABLE;
				++numWeapons;
			}
		}
		else if ( toeEntry && spawnEntry.weapon().isPlayerWeapon() && toeEntry->capacity() > 0 )
		{
			// Anyone can spawn these and they are deployed here (> 0 capacity)
			// but you don't have permission to spawn them
			spawnEntry.getCounter(&counter, toeEntry);
//			counter.setCurrent(0);
			counter.spawnable = WeaponCounter::NOT_PERMITTED;
			++numWeapons;
		}
	}

	m_numWeapons = numWeapons;

	if ( m_numWeapons == 0 )
	{
		if ( m_status < FacilityWeaponList::_CUT_OFF || m_status >= FacilityWeaponList::_NORMAL )
			m_status = FacilityWeaponList::_EMPTY;
	}
	else
	{
		// Test whether you need any kind of warning
		if ( facility.side == facility.cp->side )
		{
			if ( facility.type == FACILITY_ARMYBASE && facility.cp->isOverrun() )
				chatSendLocalizedMessage(playerID, LSTR_WARNING, LSTR_BASE_OVERRUN, facility.cp->name);
			else if ( facility.cp->isFallingBack() )
				chatSendLocalizedMessage(playerID, LSTR_WARNING, LSTR_FALLBACK_ISSUED, facility.cp->name);
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Print a message about a return attempt

static void
_returnLog(const POSTED_WEAPON* weapon, const char* message)
{
	dbugLog(_FLD, "v:%u hcu:%u sup:%u rtb:%s %s"
				, weapon->m_weaponSerial.serial
				, weapon->m_brigade->unitID()
				, weapon->m_supplyingBrigade->unitID()
				, weapon->m_facility->name
				, message
				);
}

//////////////////////////////////////////////////////////////////////
// Attempt to return posted weapon to its brigade

static bool
_returnToBrigade(POSTED_WEAPON* weapon)
{
	FacilityStruct* supplyFacility = weapon->m_facility->supplyingFacility(*weapon->m_brigade, false);
	if ( supplyFacility == nullptr )
		return false;
	if ( weapon->m_brigade == nullptr )
		return false;

	if ( weapon->m_brigade != weapon->m_supplyingBrigade )
	{
		if ( weapon->m_brigade->depositWeapons(weapon->spec(), 1, true, nullptr) == false )
			return false;
		_returnLog(weapon, "manually-supplied to brigade");
		weapon->detachReinforcement();
		chatSendLocalizedMessage(weapon->m_commander, LSTR_NOTE, LSTR_EQUIPMENT_DELIVERED, weapon->m_brigade->shortName());
	}
	else
	{
		if ( weapon->m_brigade->depositWeapons(weapon->spec(), 1, true, weapon->m_resupplyTicket) == false )
			return false;
		_returnLog(weapon, "returned to original brigade");
		assert( weapon->m_resupplyTicket == nullptr );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Attempt to return posted weapon to its original unit
// Call this first to favor returning divisional equipment to its
// division rather than allowing easy div->bde oversupply in same-town

static bool
_returnToOrigin(POSTED_WEAPON* weapon)
{
	FacilityStruct* supplyFacility = weapon->m_facility->supplyingFacility(*weapon->m_supplyingBrigade, false);
	if ( supplyFacility == nullptr )
		return false;
	if ( weapon->m_supplyingBrigade->depositWeapons(weapon->spec(), 1, true, weapon->m_resupplyTicket) == false )
		return false;

	_returnLog(weapon, "returned to origin");
	assert( weapon->m_resupplyTicket == nullptr );

	return true;
}

//////////////////////////////////////////////////////////////////////
// Schedule a "rescue" resupply ticket

static void
_rescue(POSTED_WEAPON* weapon)
{
	if ( weapon->isNonSupplied() )
		return;

	// Queue a "rescue" vehicle (15 minute delay)
	TOE_::WeaponInstance* toeEntry = weapon->m_supplyingBrigade->toeWeaponInstance(weapon->spec());
	if ( toeEntry != nullptr )
	{
		weapon->cancelReinforcement();
		UINT32 rescueTime = host_config.supply.rescueTime_m;
		if ( weapon->m_supplyingBrigade->createResupplyTicket(*toeEntry, false, rescueTime * 60, 1, nullptr) )
		{
			TELL_TESTER(weapon->m_commander, "#%u rescue scheduled for %um", weapon->m_weaponSerial.serial, rescueTime);
			_returnLog(weapon, "scheduled rescue");
		}
		else
		{
			TELL_TESTER(weapon->m_commander, "#%u rescue refused by %s (over capacity?)", weapon->m_weaponSerial.serial, weapon->m_supplyingBrigade->callsign());
			_returnLog(weapon, "rescue refused");
		}
	}
	else
	{
		TELL_TESTER(weapon->m_commander, "#%u rescue refused by %s (no entry) detaching resupply ticket", weapon->m_weaponSerial.serial, weapon->m_supplyingBrigade->callsign());
		_returnLog(weapon, "unable to rescued: detached");
		weapon->detachReinforcement();
	}
}

//////////////////////////////////////////////////////////////////////
// Process the return of a POSTED_WEAPON. Returns true if the weapon
// was delivered to its brigade from its division.

bool
POSTED_WEAPON::returnWeapon(bool allowOverstock, bool hasBeenHit)
{
	bool returnable = (isNonSupplied() == false);
	bool rtbed = false;
	bool rescue = false;
	bool rescuable = false;
	bool vehicleDelivered = false;
	if ( m_playerExitCode == PEXIT_OVER_FRIENDLY_TERRITORY && isDead() == false )
		rescuable = true;
	else if ( m_playerExitCode == PEXIT_OVER_ENEMY_TERRITORY && isDead() == false && hasBeenHit == false )
		rescuable = true;
	switch ( m_status )
	{
		case _RESERVED:
		case _POSTED:
			/* Do nothing */
			rtbed = true;
		break;

		case _SPAWNED:
			_spawnedWeapons.remove(this);
			rtbed = (returnable && m_playerExitCode == PEXIT_RTB && isDead() == false);
			rescue = (returnable && rescuable);
		break;

		case _FREE:
		case _CANCELLED:
		default:
			LogAbort("v:%u invalid status %d weapon %p!", m_weaponSerial.serial, m_status, this);
		break;
	}

	if ( returnable && rtbed == false )
		returnable = false;

	TELL_TESTER(m_commander, "processing #%u return => %s [%s, %s, %s, %s]"
					, m_weaponSerial.serial
					, (m_facility) ? m_facility->name : "<nowhere>"
					, (allowOverstock ? "overstockable" : "norm")
					, (rtbed ? "rtbing" : "non-rtb")
					, (rescue ? "rescue" : "non-resc")
					, (returnable ? "returnable" : "non-returnable")
				);

	if ( returnable == false )
	{
		TELL_TESTER(m_commander, "non-returnable");
		_returnLog(this, "non-returnable");
	}
	else if ( rtbed == true )
	{
		// First attempt to deliver the equipment back to the unit it
		// came from: if you spawn divisional equipment, it should try
		// to return to the division before falling thru to a brigade
		// that may be at the same town.
		// After that, try to deliver it to your brigade at this
		// location. If that fails, finally, "rescue" the equipment.

		if ( _returnToOrigin(this) == true )
		{
			TELL_TESTER(m_commander, "returned #%u to origin", m_weaponSerial.serial);
			rescue = false;
		}
		else if ( _returnToBrigade(this) == true )
		{
			TELL_TESTER(m_commander, "returned #%u to brigade", m_weaponSerial.serial);
			rescue = false;
			if ( m_brigade && m_supplyingBrigade && m_brigade->parent() == m_supplyingBrigade )
				vehicleDelivered = true;
		}
		else
		{
			TELL_TESTER(m_commander, "could not return #%u", m_weaponSerial.serial);
			_returnLog(this, "failed to deliver");
			rescue = true;
		}
	}

	if ( rescue == true )
	{
		_rescue(this);
	}

	// At this stage, we should detach from our reinforcement if we
	// are still attached to him so that he will resupply
	detachReinforcement();

	// Detach the multicrew
	detachCrewAndJoiners(true);

	// Eliminate the post now
	cancelPost();

	delete this;

	return vehicleDelivered;
}

//////////////////////////////////////////////////////////////////////
// Bump a posted weapon to spawned state
///ASSUMPTION: The weapon is /supposed/ to have been posted or
///ASSUMPTION: reserved before being passed to spawn().

bool
POSTED_WEAPON::spawn()
{
	WeaponEntry* spawnEntry = m_facility->weaponEntry(spec());
	if ( spawnEntry == nullptr )
	{
		dbugLog(_FLW, "v:%u can't grant request for %s at %s - no spawn entry"
						, m_weaponSerial.serial
						, weapPrintName(&spec())
						, m_facility->name
						);
		return false;
	}

	// Remove the "Reservation has timed out" expiry ticket
	if ( m_expiryUpdate != nullptr )
		cnvyRemoveWeaponFromExpireList(*this);

	// If we are spawning someplace other than where we were reserved,
	// we may need to relocate our throttle.
	if ( m_postingEntry != spawnEntry && m_supplyingBrigade != nullptr )
	{
		m_supplyingBrigade->decrementThrottle(this);
		m_postingEntry = spawnEntry;
		m_supplyingBrigade->incrementThrottle(this);
		m_postingEntry = nullptr;
	}

	if ( m_postingEntry != nullptr )
		m_postingEntry = nullptr;

	// If the reservation has been for 5 minutes, don't fidget the reinforcement
	static const UINT32 MIN_REINFORCEMENT_SHUFFLE_TIME_S = 5 * 60;
	// If its been more than 5 minutes since you posted me, then we'll
	// cancel the reinforcement and post a new one.
	bool reinforcementExpired = (m_resupplyTicket == nullptr || teulTime() - m_posted > MIN_REINFORCEMENT_SHUFFLE_TIME_S);

	if ( m_posted != 0 && reinforcementExpired )
		cancelReinforcement();

	//Set the posted timestamp to be the spawn time
	_spawnedWeapons.push_back(this);

	m_posted = teulTime();
	m_status = _SPAWNED;

	// If we didn't or don't have a reinforcement, schedule a new one.
	if ( reinforcementExpired )
		scheduleReinforcement();

	dbugLog(_FLI, "v:%u spawn()d %s at %s - %s"
					, m_weaponSerial.serial
					, weapPrintName(&spec())
					, m_facility->name
					, ((reinforcementExpired) ? "reinforced" : "standing reinf")
					);

	return true;
}

//Returns true or false depending on whether the facility can currently
//	spawn the weaponSpec provided
static bool
_canSpawn(
		const TOE_::WeaponInstance* toeEntry
		, const StratHCUnit& unit
		, const FacilityStruct& facility
		, const WeaponEntry& spawnEntry
		, const PermissionMask& permMask
		, bool includeZeros
		, bool isReserve
		)
{
	if ( unit.side() != facility.side )
		return false;

	if ( spawnEntry.isAccessible(facility) == false )
		return false;

	bool disableRankLimits = host_config.arena.vehicleRestrictions == false;
	if ( isReserve == false && spawnEntry.weapon().isPermitted(permMask, disableRankLimits) == false )
		return false;

	if ( facility.side != facility.cp->side && facility.isMilitaryFacility() )
		return false;

	if ( spawnEntry.weapon().isSupplied(permMask) == false || (permMask & ALL_SPAWN) )
		return true;

	if ( toeEntry == nullptr )
		return false;

	if ( host_config.supply.enabled == false )
		return true;

	if ( includeZeros == true || isReserve == true )
		return true;

	// Check for present + capacity.
	if ( toeEntry->present() <= 0 && (includeZeros == false || toeEntry->capacity() == 0) )
		return false;

	//check to see if the weapon is available
	//	it might not be available due to
	//		*1. spawn limits (regular)
	//		*2. spawn limits (mission)
	//		*3. contention
	//		*4. rank

	// Check for throttling
	if ( spawnEntry.isThrottled() )
	{
		UINT32 used = unit.checkThrottleWindow(&facility, toeEntry->spec());
		if ( used > 0 && used >= toeEntry->throttleCapacity() )
			return false;
	}

	//Otherwise, I guess you can spawn
	return true;
}
