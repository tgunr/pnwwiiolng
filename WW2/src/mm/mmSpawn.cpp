#include <syscfg.h> // Path is project specific.
// Date: Wed Apr 18 15:39:04 2001
//
// Spawn functions in the mmHost
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatBuzzardSpawn.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatSupportRPCMessages.h"
#include "include/arenaLockEnums.h"
#include "include/logging.h"
#include "map/mapPlayer.h"
#include "miss/mission.h"
#include "miss/missionMission.h"
#include "miss/missionTypes.h"
#include "mm/mmHostMessages.h"
#include "mm/mmMain.h"
#include "mm/mmSerialNumberRequest.h"
#include "mm/mmSpawnGrantResponse.h"
#include "mm/mmSpawn.h"
#include "server/serverServerInfo.h"	// For Server::ServerInfo::GetBest
#include "strt/strtConstants.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtMapMessages.h"
#include "strt/strtMapTypes.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtPostListReply.h"
#include "strt/strtTypes.h"
#include "teul/teulEndpointDefs.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiServerTypeEnums.h"	// For CELL_HOST
#include "wwii/wwiiSortieManager.h"
#include "wwii/wwiiPPOLoader.h"

extern PPOSpecLoader specLoader;

extern SortieManager g_sortieReturns;

//////////////////////////////////////////////////////////////////////
// Check to see if a weapon is a buzzard or not

static void
_setWeapSerial(Player& player, const WEAPON_SERIAL& weapon, const MM_VEH_STATE state)
{
	player.SetWeapon(weapon);
	player.SetVehicleState(state);
	player.updateMissionTaker(false);
	player.m_useCustomSpawnLocation = false;
	player.missionPacket().m_saved = false;
}

void
Player::cancelWeapon(bool notifyStrat)
{
	BETAInfo(teulIndex());

	m_useCustomSpawnLocation = false;

	switch(vehicleState())
	{
		case MM_VEH_POST_REQ:
			break;
		case MM_VEH_RESERVED:
		case MM_VEH_PRERESERVED:
		case MM_VEH_WAITING_CREW:
		case MM_VEH_HAS_CREW:
		case MM_VEH_JOINED:
		case MM_VEH_POST_LIST_REQ:
			if ( notifyStrat )
			{
				dbugLog(_FLD, "t:%d p:%u c:%s cancelling vehicle %d", teulIndex(), playerID(), callsign(), weapon().serial);
				strtSendMMSpawnCancel(mapGetStratIndex(), playerID(), m_weapOrigin, weapon());
			}
			break;
		case MM_VEH_NONE:
			// Do nothing
		break;
		case MM_VEH_INXFER:
			// Do even less
		return;

		default:
			dbugLog(_FLA, "t:%d p:%u c:%s illegal vehicle state %d"
							, teulIndex(), playerID(), callsign()
							, vehicleState()
							);
		break;
	}

	SetVehicleState(MM_VEH_NONE);

	if ( weapon().serial != 0 )
	{
		if ( missionPacket().m_saved == false )
		{
			// Make sure the database gets updated.
			g_sortieReturns.AddSortie(weapon().serial);
			missionPacket().m_saved = true;
		}
		SetWeapon(NULL_WEAP_SERIAL);
		updateMissionTaker(false);
	}
}

static bool
_spawnDisabledByGM(const Player& player)
{
	if ( !host_config.isArenaLocked(AL_SPAWN) )
		return false;

	if ( player.isPlaynet() || isupper(player.callsign()[0]) )
		return false;

	if ( player.permissions(ALL_SPAWN) )
		return false;

	dbugLog(_FLW, "t:%d p:%u c:%s spawn denied - arena locked", player.teulIndex(), player.playerID(), player.callsign());

	return true;
}


//////////////////////////////////////////////////////////////////////
//

void
mmSpawnDeleteVehicle(Player& player)
{
	BETAInfo(player.teulIndex());

	// If the player has a vehicle pre-reserved, bye bye.
	if ( player.hasVehicle() && player.isConnected() )
		mmSendPostDeny(player.teulIndex(), player.weapon().spec, SPAWN_RESERVATION_CANCELLED, true);

	if ( player.isSpawned() == false )
		player.cancelWeapon(true);
}

bool
mmProcessFEPostRequest(Player& player, const FE_POST_REQUEST& request)
{
	BETAInfo(player.teulIndex());

	if ( _spawnDisabledByGM(player) )
	{
		player.postDenied(player.weapon().spec, SPAWN_DISABLED_BY_GM, false);
		return false;
	}

	if ( Server::ServerInfo::GetBest(CELL_HOST) == nullptr )
	{
		player.postDenied(player.weapon().spec, SPAWN_DISABLED_BY_SERVER, false);
		return false;
	}

	player.clearSortieResults(false);

	player.SetSideLock();
	player.m_wantMobileSpawn = false;

	SINT32 stratIndex = mapGetStratIndex();

	switch(player.vehicleState())
	{
		case MM_VEH_HAS_CREW:
		case MM_VEH_WAITING_CREW:
		case MM_VEH_RESERVED:
		case MM_VEH_PRERESERVED:
			// see if we are requesting the same vehicle at the same facility, if so we keep the vehicle
			if (memcmp(&player.weapon().spec,&request.weapon,sizeof(WEAPON_SPEC)) == 0 && player.m_weapOrigin == request.facility)
			{
				if ( player.hasVehicle() && request.immediateSpawn )
					return strtSendMMSpawnGrantRequest(stratIndex, player.weapon(), player);

				MM_POST_ACCEPT postAccept;

				postAccept.multicrew = false;
				postAccept.immediateSpawn = false;
				postAccept.portal = player.m_portal;
				postAccept.customerID = player.playerID();
				postAccept.weaponSerial = player.weapon();
				postAccept.facility = player.m_weapOrigin;

				return mmProcessPostAccept(player, postAccept, MM_ACCEPT_UPDATE);
			}
			// Else PASSTHRU
		default:
		{
			// Cancel any existing request
			player.cancelWeapon(true);

			if ( player.CheckWeaponMask(request.weapon.asUINT16()) == false )
			{
				mmSendPostDeny(player.teulIndex(), player.weapon().spec, SPAWN_ACCOUNT_MASK, true);
				return false;
			}

			// Flag player as in the process of requesting a weapon
			player.SetVehicleState(MM_VEH_POST_REQ);

			const UINT32 missionID = player.missionID();

			player.m_weapOrigin = player.missionPacket().sortie().originFac();

			//Ask the strat system about the message
			strtSendMMPostRequest(stratIndex
							, player.playerID()
							, player.callsign()
							, missionID
							, player.brigadeID()
							, player.permissionMask()
							, player.m_weapOrigin
							, request.weapon
							, false
							, request.immediateSpawn
							);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Find a spawn object at a facility which can spawn a particular weapon category

static ObjectStruct*
_findOrigin(const FacilityStruct* facility, UINT8 weaponCategory)
{
	ObjectStruct* object;
	for ( object = facility->objectList; object != nullptr; object = object->next )
	{
		if ( object->spawnAbility.weap.wCategory == weaponCategory )
			return object;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Handle a "spawn me one of these" request from the command line client
// NOTE: Caller is responsible for ensuring player is not null

bool
mmDirectSpawn(Player& player, const WEAPON_SPEC weapon, bool setIdentity, bool useCustomLocation, const FLOAT64VECTOR& customLocation)
{
	BETAInfo(player.teulIndex());

	try
	{
		// Yank my association with any existing sortie/selection
		player.cancelWeapon(true);
		player.clearSortieResults(false);

		// We may need to establish this player's identity
		// or assert one over their current setup.
		if ( setIdentity )
		{
			WWII_COUNTRY country = (WWII_COUNTRY)weapon.wCountry;
			BRANCH_ENUM branch = (player.branch() != BRANCH_NOT_USED) ? player.branch() : BRANCH_LAND;
			Mission* const mission = missionGetInstanceMission(player.instance().id(), country, branch);
			if ( !mission )
				throw "unable to find system mission";
			if ( !mission->brigade() )
				throw "mission does not have a brigade";

			if ( player.country() != country || player.branch() != mission->branch() )
			{
				///TODO: Combine select and create with one database query and have
				///TODO: done in the background, please.
				// People are usually going to have a persona
				if ( player.selectPersona(0, country, mission->branch()) != PLS_LOADED )
				{
					// But they might not. This is not the best way to do this.
					player.createPersonaRecord(country, mission->branch());
					if ( player.selectPersona(0, country, mission->branch()) != PLS_LOADED )
						throw "unable to select necessary persona";
				}
			}
			if ( player.brigade() != mission->brigade() )
			{
				player.ExitBrigade(PERS_SELECTING_BRIGADE);
				player.SetBrigade(mission->brigade());
			}
			if ( player.mission() != mission && player.selectMission(mission, true) != MC_JOIN )
				throw "unable to join mission";
		}

		if ( player.country() != weapon.wCountry )
			throw "Country mismatch";
		if ( !player.brigade() )
			throw "No brigade selected";
		if ( !player.mission() )
			throw "No mission selected";

		// Figure out the origin spawner
		ObjectStruct* origin = _findOrigin(player.mission()->originFac(), weapon.wCategory);
		if ( origin == nullptr )
		{
			// Check other facilities.
			FacilityStruct* fac = player.mission()->originCP()->facilityList;
			for (; origin == nullptr && fac != nullptr; fac = fac->next )
			{
				origin = _findOrigin(fac, weapon.wCategory);
			}
			if ( origin == nullptr && fac && useCustomLocation )
				origin = fac->objectList;
			if ( origin == nullptr )
				throw "no spawner for this weapon category";
		}

		SINT32 errType = (player.instance().isCampaign() ? Debug::ERR_MINOR : Debug::ERR_INFO);
		player.log(_FL, errType, "m:%u weap:%s loc:%s(%g,%g,%g) direct spawn"
							, weapon.wCountry, weapPrintName(weapon)
							, (useCustomLocation) ? "yes":"no"
							, customLocation.x, customLocation.y, customLocation.z);

		SERIAL_NUMBER_REQUEST snr;
		snr.data().playerID = player.playerID();
		snr.data().weapon = weapon;
		snr.data().serial = 0;
		snr.send(mapGetStratIndex(), SERIAL_NUMBER_REQUEST::EXCHANGE);

		WEAPON_SERIAL fakeSerial;
		fakeSerial.source = origin->fullObjID;
		fakeSerial.serial = 0;
		fakeSerial.spec = weapon;

		_setWeapSerial(player, fakeSerial, MM_VEH_NONE);

		if ( useCustomLocation )
		{
			player.m_useCustomSpawnLocation = true;
			player.m_customSpawnLocation = customLocation;
		}

		return true;
	}
	catch ( const char* message )
	{
		player.log(_FLA, "m:%u weap:%s %s"
							, weapon.wCountry, weapPrintName(weapon)
							, message);
	}
	catch ( std::exception &e )
	{
		player.log(_FLA, "m:%u weap:%s exception: %s"
							, player.teulIndex(), player.playerID(), player.callsign()
							, weapon.wCountry, weapPrintName(weapon)
							, e.what());
	}

	player.postDenied(weapon, SPAWN_COMPLEX_DISABLED, true);

	return false;
}

template<>
SINT32
SERIAL_NUMBER_REQUEST::receive(SINT32 teulIndex, SERIAL_NUMBER_REQUEST& incoming)
{
	const SerialNumberRequest& reply = incoming.data();
	Player* player = PlayerInterface::Lookup(reply.playerID);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "p:%u player not online for serial number", reply.playerID);
		return -EINVAL;
	}

	if ( player->vehicleState() != MM_VEH_NONE
		|| player->weapon().serial != 0
		|| !COMPARE_SPECS(player->weapon().spec, reply.weapon) )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s got serial number out of sequence"
						, player->teulIndex(), player->playerID(), player->callsign());
		return -EINVAL;
	}

	mmFakeSpawnGrantResponse(player, reply.serial);

	return 0;
}

//Handles the post accept handed it by the strat server.  Looks up the
//	appropriate client and forwards the message to them

bool
mmProcessPostAccept(Player& player, const MM_POST_ACCEPT& post, UINT32 acceptType)
{
	MM_VEH_STATE reservationType;
	bool mobileSpawnAvailable = false;

	BETAInfo(player.teulIndex());

	//Initialize the numbers of postRequests
	player.SetWeapon(post.weaponSerial);
	player.m_portal = post.portal;
	player.m_weapOrigin = post.facility;
	player.m_wantMobileSpawn = false;

    FacilityStruct* facility = strtGetFacility(post.facility);
    dbugLog(_FLD, "t:%d p:%u c:%s reserved %s#%d [" PRF_WEAPON_SPEC "] at %s [" PRF_FULL_OBJ_ID "]"
                    , player.teulIndex(), player.playerID(), player.callsign()
                    , weapPrintName(player.weapon().spec), post.weaponSerial.serial, PRA_WEAPON_SPEC(player.weapon().spec)
                    , facility->name, PRA_FULL_OBJ_ID(post.facility)
                    );

	if ( post.multicrew )
		reservationType = MM_VEH_WAITING_CREW;
	else
		reservationType = MM_VEH_RESERVED;

	if ( player.mission() )
	{
		// NOTE: I have strong feelings this code doesn't accomplish anything, but not going to take it out yet
		// Dorothy allows any vehicle to spawn through a mobile spawn
#if !defined(DOROTHY)
		// let ppo subtype/id and weapon class determine spawn ability
		MobileSpawn& spawner = player.mission()->spawner();
        	if ( specLoader.ppoIsSpawnAllowed(spawner.m_stoSubType,
                        player.weapon().spec.wClass, player.weapon().spec.wType))
#endif
			mobileSpawnAvailable = player.mission()->isMobileSpawnAvailable();
		dbugLog(_FLI, "Mobile Spawn available flag = %d", mobileSpawnAvailable);
	}

	switch ( acceptType )
	{
	  case MM_ACCEPT_NEW:
		mmSendPostAccept(player.teulIndex(), post.weaponSerial, true, post.portal, mobileSpawnAvailable);
		break;

	  case MM_ACCEPT_UPDATE:
		mmSendPostAccept(player.teulIndex(), post.weaponSerial, false, post.portal, mobileSpawnAvailable);
		break;

	  case MM_ACCEPT_RTB_RESERVED:
	  	reservationType = MM_VEH_PRERESERVED;
	  	// We don't notify the client
	  	break;

	  default:
		dbugLog(_FLW, "t:%d p:%u c:%s Unrecognized POST_ACCEPT_TYPE %d", player.teulIndex(), player.playerID(), player.callsign(), acceptType);
	  	break;
	}

	_setWeapSerial(player, post.weaponSerial, reservationType);

	if (post.immediateSpawn)
	{
#ifdef DEBUG
		player.debug("Immediate spawn request", "sending grantRequest");
#endif

		//Go ahead and ask for permission to spawn
		strtSendMMSpawnGrantRequest(mapGetStratIndex(), post.weaponSerial, player);
	}
	else
		player.updateChat();

	return true;
}

//Handles the post deny handed it by the strat server.  Looks up the
//	appropriate client and forwards the message to them
bool
mmProcessPostDeny(const MM_POST_DENY& denial)
{
	//Find the customer that the message is destined for
	Player* const player = PlayerInterface::Lookup(denial.customerID);
	if (player == nullptr || player->isAtLobby() == false)
	{
		return false;
	}

	BETAInfo(player->teulIndex());

	// Forward the denial to the customer
	player->postDenied(denial.weapon, denial.reasonCode, true);

	return true;
}

//Handles a post Expired coming from the strat to the mmHost.  Forwards
//	it on to all who need to know about it
bool
mmProcessPostExpired(const MM_POST_EXPIRED& expiry)
{
	//Find the customer that the message is destined for
	Player* const player = PlayerInterface::Lookup(expiry.customerID);
	if (player == nullptr || player->isAtLobby() == false)
	{
		return false;
	}

	BETAInfo(player->teulIndex());

	player->cancelWeapon(false);

	//If they canceled it, we don't want to tell them about it.  That would
	//	be redundant now, wouldn't it? :)
	if ( player->isSpawned() == false &&  expiry.cancelled == false )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sending post expired for %s", player->teulIndex(), player->playerID(), player->callsign(), weapPrintName(expiry.weaponSerial.spec));
		mmSendPostExpired(player->teulIndex(), expiry.weaponSerial);
	}

	return true;
}

//Handles a postListRequest from the client
bool
mmProcessFEPostListRequest(Player& player, const FE_POST_LIST_REQUEST& request)
{
	BETAInfo(player.teulIndex());

	///FIXME: Just say "OK" if the vehicle is what I already have reserved
	player.cancelWeapon(true);
	player.SetVehicleState(MM_VEH_POST_LIST_REQ);
	UINT32 missionID = player.missionID();
	player.m_weapOrigin = player.missionPacket().sortie().originFac();

	//Ask the strat system about the message
	strtSendMMPostListRequest(mapGetStratIndex(), player.playerID(), player.callsign(), missionID, player.brigadeID(), player.m_weapOrigin);

	return true;
}

//Handles a postListReply coming from the strat to the mmHost.
SINT32
PostListReply::Process(ACTION_OBJECT* const a)
{
	if ( a->m_client )
		return -EINVAL;
	if ( a->canRead<PostListReply>() == false )
		return -EINVAL;

	auto& reply = a->read<PostListReply>();

	//Find the customer that the message is destined for
	Player* const player = PlayerInterface::Lookup(reply.m_customerID);
	if (player == nullptr || player->isAtLobby() == false)
		return a->offset;

	BETAInfo(player->teulIndex());

	const FacilityStruct* const facility = strtGetFacilityByOID(reply.m_facilityOID);
	FULL_OBJ_ID fullObjID = (facility != nullptr) ? facility->fullObjID : nullObjID;

	//Translate the posted weapons to commanded weapons
	static UCHAR _buffer[Teul::MaxPacket];
	UINT32 postsFound = 0;
	COMMANDED_WEAPON* commandedWeapon = nullptr;

	if ( a->canRead<MM_POSTED_WEAPON>() == true )
	{
		commandedWeapon = (COMMANDED_WEAPON*)_buffer;

		//We are keeping a posts found count so we don't try and send
		//	information to the front end on commanders we cannot find
		Player* commander;
		while ( a->canRead<MM_POSTED_WEAPON>() == true )
		{
			auto& weapon = a->read<MM_POSTED_WEAPON>();
			commander = PlayerInterface::Lookup(weapon.commander);
			if ( commander == nullptr || commander->isAtLobby() == false )
				continue;

			if ( commander->brigade() != player->brigade() )
				continue;

			if ( commander->mission() != player->mission() )
				continue;

			//copy the parts we want out of the structures
			commandedWeapon[postsFound].handle = commander->chatHandle();
			commandedWeapon[postsFound].weaponSerial = weapon.weaponSerial;
			commandedWeapon[postsFound].action = reply.m_action;

			++postsFound;
		}
	}

	mmSendPostListResponse(player->teulIndex(), fullObjID, postsFound, commandedWeapon);

	return a->offset;
}

//Handles someone making a crew request.
bool
mmProcessFECrewRequest(Player& player, const FE_CREW_REQUEST& request)
{
	BETAInfo(player.teulIndex());

	switch(player.vehicleState())
	{
		case MM_VEH_HAS_CREW:
		case MM_VEH_WAITING_CREW:
		case MM_VEH_PRERESERVED:
		case MM_VEH_RESERVED:
			player.SetVehicleState(MM_VEH_WAITING_CREW);
			player.updateMissionTaker(false);
			strtSendMMPostInfoUpdate(mapGetStratIndex(),player.playerID(), request.weaponSerial, true);
		return true;

		default:
			player.cancelWeapon(true);
		return false;
	}
}

//Handles someone making a join request.  This is more complicated, b/c
//	we have to ask the front end the player is wanting to join up with
//	whether its ok or not
bool
mmProcessFEJoinRequest(Player& player, const FE_JOIN_REQUEST& request)
{
	dbugLog(_FLD, "t:%d p:%u c:%s requesting to join v:%d", player.teulIndex(), player.playerID(), player.callsign(), request.weaponSerial.serial);

	//Ask the strat system about the message
	strtSendMMJoinListRequest(mapGetStratIndex(), player.playerID(), request.weaponSerial, player.chatHandle(), true);

	return true;
}

//If we are using this information to join a person to another
// 	if (postInfoResponse->join)
bool
mmProcessJoinListResponse(const MM_JOIN_LIST_RESPONSE& response)
{
	//Find the customer that the message is destined for
	Player* const player = PlayerInterface::Lookup(response.commandID);
	if ( player == nullptr || player->isAtLobby() == false )
	{
		return false;
	}

	BETAInfo(player->teulIndex());

	//Send the poster a weapon letting them know that someone want to join
	mmSendJoinPermRequest(player->teulIndex(), response.crewID, response.crewHandle);

	return true;
}

//Handles a JoinPermResponse
bool
mmProcessFECrewAccept(Player& player, const FE_CREW_ACCEPT& response)
{
	BETAInfo(player.teulIndex());

	//Send the join message to the strat system apprising it of the join,
	//	asking it as to whether its okay
	strtSendMMJoinRequest(mapGetStratIndex(), response.joinRequestID, player.weapon());

	return true;
}

//Handles the join Accept coming from the strat system.  Tell the accepted
//	person everything is good to go, then tells the other unlucky people that
//	they were not chosen to accompany the person on their trip into the magic
//	wwiionline kingdom
bool
mmProcessJoinAccept(const MM_JOIN_ACCEPT& accept)
{
	Player* const poster = PlayerInterface::Lookup(accept.postedWeapon.commander);
	if ( poster == nullptr || poster->isAtLobby() == false )
	{
		return false;
	}
	dbugLog(poster->teulIndex(), "commander");

	Player* const joiner = PlayerInterface::Lookup(accept.postedWeapon.crew);
	if ( joiner == nullptr || poster->isAtLobby() == false )
	{
		return false;
	}
	dbugLog(poster->teulIndex(), "crew");

	mmSendJoinAccept(joiner->teulIndex(), accept.postedWeapon);
	_setWeapSerial(*poster, accept.postedWeapon.weaponSerial, MM_VEH_HAS_CREW);
	_setWeapSerial(*joiner, accept.postedWeapon.weaponSerial, MM_VEH_JOINED);

	return true;
}

//Handles a join deny.  Tells the person that tried to join that it was
//	denied.  Don't know that this should EVER happen, but just in case...
bool
mmProcessJoinDeny(const MM_JOIN_DENY& denial)
{
	//Find the customer that the message is destined for
	Player* const player = PlayerInterface::Lookup(denial.customerID);
	if ( player == nullptr || player->isAtLobby() == false )
	{
		dbugLog(_FLA, "p:%u no player for this customerID, ignoring joinDeny", denial.customerID);
		return false;
	}

	BETAInfo(player->teulIndex());

	player->cancelWeapon(false);
	mmSendJoinDeny(player->teulIndex(), MM_JOIN_FAIL_UNKNOWN_ERROR);

	return true;
}

//Handles someone killing a join request on their posted vehicle
bool
mmProcessFEJoinPermKill(Player& player, const FE_JOIN_PERM_KILL& kill)
{
	BETAInfo(player.teulIndex());

	///TODO: Verify the kill vehicle matches the current vehicle.
	// tell strat to ditch all crew
	strtSendMMPostInfoUpdate(mapGetStratIndex(), player.playerID(), player.weapon(), false);

	return true;
}

//Handles the canceling of a post.
bool
mmProcessFEPostCancel(Player& player, const FE_POST_CANCEL& request)
{
	SINT32 stratIndex = mapGetStratIndex();

	BETAInfo(player.teulIndex());

	strtSendMMSpawnCancel(stratIndex, player.playerID(), player.m_weapOrigin, player.weapon());
	player.SetVehicleState(MM_VEH_NONE);
	player.cancelWeapon(false);

	return true;
}

//Handles a client spawn.  Asks the strat server to spawn the people
bool
mmProcessFEClientSpawn(Player& player, const FE_CLIENT_SPAWN& request)
{
	BETAInfo(player.teulIndex());

	Mission* mission = player.mission();
	if ( mission == nullptr )
	{
		PlayerLog(_FLA, player, "attempted spawn without mission");
		player.postDenied(request.weaponSerial.spec, SPAWN_COMPLEX_DISABLED, true);
		return false;
	}

	if ( !player.mission()->ongoing() )
	{
		PlayerLog(_FLA, player, "attemped spawn on closed mission");
		player.postDenied(request.weaponSerial.spec, SPAWN_COMPLEX_DISABLED, true);
		return false;
	}

	if ( !player.validMissionPacket() )
	{
		PlayerLog(_FLA, player, "mission packet not ready for spawning");
		player.postDenied(request.weaponSerial.spec, SPAWN_COMPLEX_DISABLED, true);
		return false;
	}

	if ( _spawnDisabledByGM(player) )
	{
		PlayerLog(_FLI, player, "denied: arena locked");
		player.postDenied(request.weaponSerial.spec, SPAWN_DISABLED_BY_GM, false);
		return false;
	}

	if ( request.wantMobileSpawn && !mission->isMobileSpawnAvailable() )
	{
		PlayerLog(_FLI, player, "msp: denied: mobile spawn unavailable");
		player.postDenied(request.weaponSerial.spec, SPAWN_MOBILE_UNAVAILABLE, false);
		return false;
	}

	if ( !player.hasAvailableVehicle()
		|| memcmp(&request.weaponSerial.spec, &player.weapon().spec, sizeof(WEAPON_SPEC)) != 0)
	{
		PlayerLog(_FLI, player, "denied: no vehicle reserved here");
		player.postDenied(request.weaponSerial.spec, SPAWN_WEAPON_NOT_AT_FACILITY, true);
		return false;
	}

	if ( !player.allowLaunch() )
	{
		PlayerLog(_FLI, player, "denied: launch timer");
		player.postDenied(request.weaponSerial.spec, SPAWN_DELAYED, false);
		return false;
	}

	player.SetSideLock();
	player.m_wantMobileSpawn = request.wantMobileSpawn;
#ifdef BETA_RELEASE
	dbugLog(_FLD, "t:%d p:%u c:%s requested %s spawn", player.teulIndex(), player.playerID(), player.callsign()
					, ((player.m_wantMobileSpawn) ? "mobile" : "normal")
					);
#endif

	//Ask the strat system about the message
	strtSendMMSpawnGrantRequest(mapGetStratIndex(), request.weaponSerial, player);

	return true;
}

//handles a crew bailing message
bool
mmProcessCrewBailed(const MM_CREW_BAILED& crewBailed)
{
	Player* const player = PlayerInterface::Lookup(crewBailed.comanderID);
	if ( player == nullptr || player->isAtLobby() == false )
	{
		dbugLog(_FLW, "p:%u no player for this customerID, ignoring bail message", crewBailed.comanderID);
		return false;
	}

	BETAInfo(player->teulIndex());
	mmSendCrewBailed(player->teulIndex(), crewBailed.crewID, 0);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Handle the reception of a Buzzard Spawn message from the chat host

void
mmProcessBuzzardSpawn(SINT32 teulIndex, const BuzzardSpawn& request)
{
	Player* player = nullptr;
	try
	{
		player = PlayerInterface::Lookup(request.m_playerID);
		if ( player == nullptr || player->isConnected() == false )
			throw "Not connected to map host. Please relog";
		if ( player->isSpawned() == true )
			throw "You'll have to despawn first.";

		if ( !isValidSide(player->side()) || !player->brigade() || !player->mission() )
			throw "Must have a side/brigade/mission selected";

		WEAPON_SPEC myBuzzard = weapFindWeaponByName("uk_buzzard");
		if ( myBuzzard.wCountry == 0 || myBuzzard.wType == 0 )
			throw "Failed to aquire buzzard (internal error)";
		myBuzzard.wCountry = player->country();

		mmDirectSpawn(*player, myBuzzard, false, request.m_useLocation, request.m_location);
	}
	catch (const char* reason)
	{
		if ( player )
		{
			dbugLog(_FLA, "t:%d p:%u c:%s m:%u %s"
							, teulIndex, player->playerID(), player->callsign(), player->missionID()
							, reason);
			player->message(LSTR_CAUTION, LSTR_UNABLE_TO_SPAWN, reason);
		}
		else
		{
			dbugLog(_FLA, "t:%d %s", teulIndex, reason);
			chatSendLocalizedMessage(request.m_playerID, LSTR_CAUTION, LSTR_UNABLE_TO_SPAWN, reason);
		}
	}
}

void
mmSpawnCommitSortieClosures()
{
}
