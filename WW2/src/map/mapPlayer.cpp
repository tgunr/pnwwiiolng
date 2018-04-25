#include <syscfg.h> // Path is project specific.
// Created: Thu Feb  1 13:26:21 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "chat/chatConstants.h"
#include "chat/chatGroupPermissions.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatPersonaUpdate.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatTypes.h"
#include "inst/instHost.h"				// For instAddPlayerToCampaignInstance
#include "map/mapArenaDescription.h"
#include "map/mapBrigades.h"
#include "map/mapChecksums.h"
#include "map/mapData.h"
#include "map/mapDB.h"
#include "map/mapHCUnitLocations.h"
#include "map/mapPersonaSummary.h"
#include "map/mapPlayer.h"
#include "map/mapSwingMeter.h"
#include "omg/omgChat.hxx"
#include "miss/missionDamageEvent.h"
#include "miss/mission.h"
#include "miss/missionHostMessages.h"
#include "miss/missionMission.h"
#include "miss/missionResults.h"
#include "mm/mmHostMessages.h"
#include "mm/mmMain.h"
#include "mm/mmPack.h"
#include "mm/mmPolls.h"
#include "mm/mmSpawn.h"
#include "mm/mmSpawnTypes.h"
#include "score/scoreSortie.h"
#include "server/serverIndexes.h"
#include "server/serverInterface.h"
#include "server/serverPersonaXfer.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverReleaseTypeEnum.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtEvents.h"
#include "strt/strtFacilityStruct.h"
#include "teul/teulAuth.h"
#include "teul/teulBlipRepeater.h"
#include "teul/teulUtils.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiRTBCodes.h"

#include "include/logging.h"
#include "teul/teulMicroWebServer.h"

static std::mutex s_sidecountMutex;

UINT64 brigadeMovementWatermark = 0LL;

#define PERS_TIME_BITS	30
#define PERSONA_TIMEOUT	120000

#define _EXPAND_RTB(x)(x==RTB_SUCCESS?"RTB":x==RTB_BAIL_FRIENDLY?"RESCUED":x==RTB_BAIL_ENEMY?"MIA":x==RTB_DIED?"KIA":"RTB_ERR")

#if defined(BETA_RELEASE)
# define betaMessage(__msg__) message(LSTR_BETA, LSTR_GENERIC, __msg__)
#else
# define betaMessage(__msg__)
#endif

PLAYER_LIST g_chatSyncQueue;
PLAYER_LIST g_logins;

// Counts of spawned players.
UINT32	g_spawnedPlayers[MAX_SIDES];

// *ALL* players
PLAYERS	g_players;

// Side counts
WWII_SIDE g_heavySide;	// Side having most players but not necessarily overpop yet
WWII_SIDE g_overdog;	// Side that has gone overpop
WWII_SIDE g_underdog;
UINT32 g_sideImbalance;
SINT32 g_sideDeviation;
SwingMeter		g_swingMeter;
extern CPStruct* mapCPList;

const char* xferReasons[] = {
	"xrLOGIN"
,	"xrUPDATE"
,	"xrRELOGIN"
,	"xrTRANSFER"
,	"xrMIRROR"
,	"xrEXIT"
};

const char* _pstates[] =
{
	"PERS_SELECTING_PERSONA"
,	"PERS_SELECTING_BRIGADE"
,	"PERS_SELECTING_MISSION"
,	"PERS_AT_MISSION"
,	"PERS_SPAWNING"
,	"PERS_DEBRIEFING"
,	"PERS_EXITING"
};

static void _reportPlayerDetails(const Teul::uWeb::HTTPRequest &request, Teul::uWeb::HTTPResponse &response)
{
	char buffer[2048];
	int len = 0;

	{
		std::lock_guard<std::mutex> mutexLock(s_sidecountMutex);

		len = strprintf(buffer
            , "{\"stamp\":%" PR64BitUT ",\"players\":%" PRSizeT ",\"balance\":{\"above\":%u,\"delays_max\":%u,\"delays_pctg\":%u,\"margin_pctg\":%u,\"pers_delay\":%u},\"camp\":{\"allies_spawned\":%u,\"axis_spawned\":%u,\"hvyside\":%u,\"overdog\":%u,\"underdog\":%u,\"imb\":%u,\"dev\":%u},\"swing_meter\":{\"in_effect\":%d,\"meter\":%d,\"delays\":%d}}"
            , teulClock64
            , g_players.size()
            , host_config.balance.above
            , host_config.balance.delaysMax
            , host_config.balance.delaysPctg
            , host_config.balance.marginPctg
            , host_config.balance.personaDelay
            , g_spawnedPlayers[ALLIED]
            , g_spawnedPlayers[AXIS]
            , g_heavySide
            , g_overdog
            , g_underdog
            , g_sideImbalance
            , g_sideDeviation
            , g_swingMeter.inEffect()
            , g_swingMeter.meter()
            , g_swingMeter.delays()
		);
	}

	if (len > 0) {
        return Teul::uWeb::HTTPResponseBuilder(request)
            .status(200)
            .header("Access-Control-Allow-Origin", "*")
            .JSON(buffer)
            .build(response);
    } else {
        return Teul::uWeb::HTTPResponseBuilder(request)
            .status(500)
            .JSON("{\"error\":\"An unknown error occurred\"}")
            .build(response);
    }
}

static inline UINT32
_numTotalPlayers()
{
	return (g_players.empty()) ? 0 : g_players.size();
}

static inline UINT32
_numActivePlayers()
{
	///TODO: Deduct number of spawned players.
	return (g_players.empty()) ? 0 : g_players.size();
}

Player*
PlayerInterface::Lookup(UINT32 playerID)
{
	return (Player*)ServerInterface::LookupByPlayerID(playerID);
}

//////////////////////////////////////////////////////////////////////
// Check to see if either side is currently the "overdog"

static void
_checkForOverdog()
{
	// Reset the swing meter back to null values
	g_swingMeter.setMeter(0);
	g_overdog = WWII_NO_SIDE;
	g_underdog = WWII_NO_SIDE;
	g_heavySide = WWII_NO_SIDE;
	g_sideImbalance = 0;

	// Determine which side is the "overdog" (most players)
	WWII_SIDE overdog, underdog;
	int sign;
	if ( g_spawnedPlayers[ALLIED] > g_spawnedPlayers[AXIS] )
	{
		overdog = ALLIED;
		underdog = AXIS;
		sign = -1;
	}
	else
	{
		overdog = AXIS;
		underdog = ALLIED;
		sign = 1;
	}

	// Don't persue if the numbers are even
	if ( g_spawnedPlayers[overdog] == g_spawnedPlayers[underdog] )
		return;

	g_heavySide = overdog;

	// If the config specifies a value for balanceAbove,
	// don't proceed unless the overdog has more players than that.
	if ( g_spawnedPlayers[overdog] <= host_config.balance.above )
		return;

	// Find out what half the number of players is; we need the
	// number of players who HAVE picked a side, so that we don't
	// cloud the result with unselected players.
	UINT32 total = g_spawnedPlayers[overdog] + g_spawnedPlayers[underdog];
	const int mean = total / 2;

	// Calculate the percentage deviation from normal (so 60:40 is a 20% deviation)
	if ( mean > 0 )
		g_sideImbalance = (g_spawnedPlayers[overdog] * 100 / mean) - 100;
	else
		g_sideImbalance = 100;

	// Turn this into a nice +/- gradiated scale, with one point marking 5%
	g_sideDeviation = (SINT32)g_sideImbalance * (SINT32)sign;
	const int swing = g_sideDeviation / (SINT32)5;
	g_swingMeter.setMeter(swing);

	// Is overdog functionality disabled? (set to 0)
	if ( host_config.balance.marginPctg == 0 )
		return;

	// Is this lower than the pctg required to trip balancing?
	if ( g_sideImbalance < host_config.balance.marginPctg )
		return;

	// Expose the values globally
	g_overdog = overdog;
	g_underdog = underdog;

	// Only persue further when the arena is open
	if ( !host_config.isArenaOpen() )
		return;

	// Arena is open, one side has more than enough players,
	// so make sure that the persona delays are in effect
	g_swingMeter.setInEffect();

	return;
}

//////////////////////////////////////////////////////////////////////
// Spawn Delay management

const char* ls = "NONE";

static UINT32
_adjustDelays()
{
	if ( host_config.balance.delaysPctg == 0 )
		return 0;

	static WWII_SIDE delayedSide = WWII_NO_SIDE;
	static const UINT32 _CHANGE_INTERVAL = 60;		// Only adjust once per minute
	static const UINT32 _MIN_DELAY = 5;
	static UINT32 lastChange = 0;
	static UINT32 lastImbalance = 0;
	static UINT32 delay = 0;

	ls = "";

	// Nobody is overpop now and nobody was overpop last time either.
	if ( WWII_NO_SIDE == g_overdog && g_overdog == delayedSide )
	{
		lastImbalance = 0;
		return (delay = 0);
	}

	// Continuation of overpop or end of overpop
	if ( g_overdog == delayedSide || g_overdog == WWII_NO_SIDE )
	{
		// If there is no-longer an imbalance, rapidly bring down
		// the delay.
		if ( !g_swingMeter.inEffect() )
		{
			if ( delay > _MIN_DELAY )
			{
				// Bring the delay down rapidly.
				--delay;
				lastChange = teulTime();
			}
			else if ( teulTime() < lastChange + _CHANGE_INTERVAL )
			{
				// Hold the final delay for a little while.
				delayedSide = WWII_NO_SIDE;
				delay = 0;
				lastChange = teulTime();
			}

			return delay;
		}

		// Don't proceed if its been less than _CHANGE_INTERVAL seconds
		// since the last change
		if ( teulTime() < lastChange + _CHANGE_INTERVAL )
			return delay;

		SINT32 imbalanceChange = (SINT32)(g_sideImbalance - lastImbalance);
		lastImbalance = g_sideImbalance;

		if ( delay > _MIN_DELAY && g_sideImbalance < host_config.balance.delaysPctg )
		{
			// If the imbalance has been going down, take some time off
			if ( imbalanceChange < 0 )
			{
				lastChange = teulTime();
				ls = "active/descending";
				--delay;
				return delay;
			}
			// Been stable or climbing for a while
			if ( teulTime() < lastChange + 5 * _CHANGE_INTERVAL )
				return delay;

			lastChange = teulTime();
			if ( delay < host_config.balance.delaysMax )
				++delay;
			ls = "active/climbing";
			return delay;
		}

		// Coming out of an imbalance
		if ( g_overdog == WWII_NO_SIDE || g_sideImbalance < host_config.balance.delaysPctg )
		{
			///FIXME: How does this ever get reached? If this is true, then
			/// won't g_swingMeter.inEffect() be false?
			delay = 0;
			delayedSide = WWII_NO_SIDE;
			lastChange = teulTime();
			ls = "cleared";
			return delay;
		}

		// Reduction in imbalance, no increase
		if ( imbalanceChange < 0 )
		{
			ls = "over/declining";
			return delay;
		}

		// No change in imbalance, slow increment
		if ( imbalanceChange == 0 && teulTime() < lastChange + (2 * _CHANGE_INTERVAL) )
		{
			ls = "over/steady";
			return delay;
		}

		if ( delay < _MIN_DELAY )
		{
			ls = "over/bootstrap";
			delay = _MIN_DELAY;
		}
		else if ( delay < host_config.balance.delaysMax )
		{
			ls = "over/rising";
			++delay;
		}

		lastChange = teulTime();

		return delay;
	}

	// Prevent flopping.
	if ( teulTime() < lastChange + _CHANGE_INTERVAL )
		return delay;

	// Transition into over population
	if ( delayedSide == WWII_NO_SIDE )
	{
		lastChange = teulTime();
		delayedSide = g_overdog;
		ls = "reset";
		return (delay = 0);
	}

	// Final case: over population changed sides
	lastChange = teulTime();
	delayedSide = WWII_NO_SIDE;

	return (delay = 0);
}

UINT32
mapGetSideDelay(WWII_SIDE forSide)
{
	if ( forSide == g_overdog )
		return g_swingMeter.delays();
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Check the count of players spawned.

static void
_checkSpawnCounts()
{
	UINT32 spawnedCount[MAX_SIDES] = { 0, 0, 0, 0 };

	for ( auto it = g_players.begin(), end = g_players.end(); it != end; ++it )
	{
		Player* const player = (Player*)(*it);
		if ( player->instance().isCampaign() && player->isSpawned() )
			++spawnedCount[player->side()];
	}

	bool isCorrect = true;
	for ( UINT32 i = 0; i < MAX_SIDES; ++i )
	{
		if ( spawnedCount[i] != g_spawnedPlayers[i] )
		{
			dbugLog(_FLA, "SPAWN COUNT MISMATCH FOR %s: count:%u, actual:%u", _sideName[i], g_spawnedPlayers[i], spawnedCount[i]);
			isCorrect = false;
			g_spawnedPlayers[i] = spawnedCount[i];
		}
	}

	if ( isCorrect == false )
	{
		dbugLog(_FLA, "Spawned count is WRONG.");
	}
}

//////////////////////////////////////////////////////////////////////
// Check the side counts

void
mapCheckSideCounts()
{
	std::lock_guard<std::mutex> mutexLock(s_sidecountMutex);

	UINT32 playerCount = instGetCampaignPopulation();

	_checkSpawnCounts();

	const UINT32 sideCounts[MAX_SIDES] = { instGetCampaignSideCount(0), instGetCampaignSideCount(1), instGetCampaignSideCount(2), instGetCampaignSideCount(3) };
	const UINT32 spawnedPlayers = g_spawnedPlayers[1] + g_spawnedPlayers[2];

	static Teul::BlipRepeater nextReport(60 * 1000);
	if ( nextReport.Ready() )
	{
		dbugLog(_FLI, "personas: %" PRSizeT "; campaign player counts: %u+%u+%u+%u = %u. campaign spawned players: %u+%u = %u"
							, g_players.size()
							, sideCounts[0], sideCounts[1], sideCounts[2], sideCounts[3], playerCount
							, g_spawnedPlayers[1], g_spawnedPlayers[2], spawnedPlayers);

#if defined(BETA_RELEASE) || defined(PUBLIC_RELEASE)
		DBThread::Do(DBThread::Audit
			, "INSERT INTO `audit`.`player_counts`"
					" (stamp,undecided,allied,axis,spawned_allied,spawned_axis,allied_favor,axis_favor,delay)"
				"VALUES"
					" (now(),%u,%u,%u,%u,%u,%d,%d,%u)"
					, sideCounts[0] + sideCounts[3]
					, sideCounts[1], sideCounts[2]
					, g_spawnedPlayers[1], g_spawnedPlayers[2]
					, g_sideImbalance, -static_cast<SINT32>(g_sideImbalance)
					, g_swingMeter.delays()
					);
#endif
	}

	if ( host_config.balance.above > 0 )
	{
		_checkForOverdog();
		g_swingMeter.setDelays(_adjustDelays());
	}
}

//	------------------------------	Class Functions

//////////////////////////////////////////////////////////////////////
// Default MapPersona constructor

Player::Player()
	: wwiiPlayer()
	, m_groups()
	, m_permissions(&nullChatPermissions)
	{
	}

//////////////////////////////////////////////////////////////////////
// Map Persona constructor

Player::Player(SINT32 newTeulIndex, const Auth::Dispatch& newDispatch, const playerid_t newPlayerID)
	: wwiiPlayer(), m_groups(), m_permissions(&nullChatPermissions)
{
	if ( newPlayerID <= DEFAULT_PLAYER_ID )
	{
		throw std::invalid_argument("Attempted to create persona with playerID <= DEFAULT");
	}

	m_teulIndex = newTeulIndex;
	SetDispatch(newDispatch, newPlayerID);
	timeIn = teulTime();

	m_useRank = m_actualRank = Rank::lookup(0);
	assert( m_useRank != nullptr );

	if ( !selectPlayer(newPlayerID) || playerID() <= DEFAULT_PLAYER_ID )
		throw std::runtime_error("No such player");	///Terminal

	g_players.insert(this);

	m_registered = true;

	SetInstance(Instance::g_campaign);

	dbugLog(_FLD, "t:%d p:%u c:%s PERS: +persona %p active:%d total:%d", teulIndex(), playerID(), callsign(), this, _numActivePlayers(), _numTotalPlayers());
}

//////////////////////////////////////////////////////////////////////
// Test if a persona is allow to launch based on delay/timers

bool
Player::allowLaunch() const
{
	if ( m_launchPermitted )
		return true;
	if ( m_nextLaunch > (UINT32)teulTime() )
		return false;
	if ( mission() && !mission()->isSystemMission() && mission()->summary().timeLaunch > (UINT32)(teulTime() + 1) )
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////

void
Player::SetCountry(WWII_COUNTRY newCountry, WWII_SIDE newSide)
{
	const bool validNewSide = isValidSide(newSide);

	// If you're leaving a side, set a timelock for the old side
	if ( !validNewSide && isValidSide(side()) )
		SetSideLock();

	wwiiPlayer::SetCountry(newCountry, newSide);

	// If you're joining a side, set a timelock for this side
	if ( validNewSide )
	{
		SetSideLock();
		m_sessionSide = newSide;
	}
}

//////////////////////////////////////////////////////////////////////
// Given a comma-separated list of groups, populate our CHAT_GROUPS
// member with valid data.

void
Player::SetGroups(const char* groupList)
{
	if ( groupList == nullptr || *groupList == 0 )
		m_permissions = &nullChatPermissions;
	else
	{
		// Load what's in groupList into our CHAT_GROUPS member
		chatReadGroups(groupList, m_groups);

		// Update our permissions
		m_permissions = chatGroupPermissionsForGroups(m_groups);

        log(_FLD, "groups: %s perm:%" PRPermMaskT " max army/air/sea/squad:%u/%u/%u/%u"
                    , groupList
                    , m_permissions->m_permissionMask
                    , m_permissions->m_limitRank[BRANCH_LAND]
                    , m_permissions->m_limitRank[BRANCH_AIR]
                    , m_permissions->m_limitRank[BRANCH_SEA]
                    , m_permissions->m_limitSquadRank);
	}

	// Update my rank restrictions
	m_maxRank = m_permissions->m_limitRank[branch()];

	if ( personaID() > 0 )
	{
		const Rank* rankEntry = Rank::lookup(effectiveRank());
		m_useRank = (rankEntry != nullptr) ? rankEntry : m_actualRank;
	}

	updateMask();
}

//////////////////////////////////////////////////////////////////////
// Set the minute balance for metered play

void
Player::SetMeteredAccess(UINT32 newMinuteBalance)
{
	if ( m_minuteBalance != newMinuteBalance )
		m_minuteBalance = newMinuteBalance;

	// Lastly - make sure metered access charging commences
	if ( meteredAccess() )
	{
		if ( m_nextCharge < teulClockTime - MINUTE_S )
		{
			m_nextCharge = teulClockTime + MINUTE_S;
			dbugLog(_FLD, "t:%d p:%u c:%s minute charging scheduled for %us", teulIndex(), playerID(), callsign(), MINUTE_S);
		}
	}
	else
		m_nextCharge = 0;
}

//////////////////////////////////////////////////////////////////////
// Destructor for map persona

Player::~Player()
{
	if ( m_missionResults )
	{
		delete m_missionResults;
		m_missionResults = nullptr;
	}

	if ( m_registered )
	{
		if ( m_sortie )
		{
			closeCurrentSortie();
		}

		// Extract the player from the instance upfront so that
		// we don't wind up mucking about with what instance they
		// are currently in.
		SetInstance(Instance::g_invalid);

		ExitBrigade(PERS_EXITING);

		// Make sure we're in country 0
		SetCountry(WWII_NO_COUNTRY, WWII_NO_SIDE);

		// Make sure we're not still on the sync list, because we're gone!
		g_chatSyncQueue.remove(this);

		g_players.erase(this);

		if ( m_loginState > 0 )
			g_logins.remove(this);

		m_registered = false;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s ptr:%p PERS: active:%d -REL", teulIndex(), playerID(), callsign(), this, _numTotalPlayers());

	if ( teulIndex() >= 0 )
		teulSetPersonaData(teulIndex(), nullptr);
	else
	{
		///TODO: Check that this should be an ELSE
		PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(playerID());
		if ( psn != nullptr && (Player*)(psn->localInfo) == this )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s dropping localInfo reference", teulIndex(), playerID(), callsign());
			psn->localInfo = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Increase player's rank by one if they are within the constraints
// of the rank system

void
Player::bumpRank()
{
	m_promoted = false;

	const Rank* next = actualRank().next();
	if ( next == nullptr || next->level() == 0 || actualRank().points() >= 65535 )
	{
#ifdef BETA_RELEASE
		dbugLog(_FLN, "t:%d p:%u c:%s persona reached max advanceable rank", teulIndex(), playerID(), callsign());
#endif
		return;
	}

	SetRank(next->level());
	m_rankPointsEarned = 0;
	m_promoted = true;

	mapSaveRank(this, false);

	dbugLog(_FLI, "t:%d p:%u c:%s PROMOTION to %d", teulIndex(), playerID(), callsign(), next->level());
#ifdef PROMOTIONS_IN_CHAT
	///TEMPORARY
	if ( !inState(PERS_EXITING) )
		chatSendLocalizedMessage(playerID(), LSTR_NOTE, LSTR_FIELD_PROMOTION);
#endif

	updateChat();
}

//////////////////////////////////////////////////////////////////////
// Start the process of calculating a persona's last mission results
// This involves an async query to the strat host so the result
// may not come back immediately

bool
Player::calculateMissionResults(MM_PERS_STATE toState)
{
	BETAInfo(teulIndex());

	SetState(toState);
	m_promoted = false;

	if ( !validMissionPacket() )
	{
		dbugLog(_FLW, "cannot calc results with null missionPacket");
		return false;
	}

	const FULL_OBJ_ID origin = missionPacket().sortie().originFac();
	const FacilityStruct* const facility = strtGetFacility(origin);
	if ( facility == nullptr )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s has invalid origin: " PRF_FULL_OBJ_ID ""
						, teulIndex()
						, playerID()
						, callsign()
						, PRA_FULL_OBJ_ID(origin)
						);
		return false;
	}

	if ( inState(PERS_EXITING) )
	{
		// Do the tail half of the mission packet processing
		// immediately since the player has already gone away

		dbugLog(_FLD, "t:%d p:%u c:%s PERS_EXITING so going straight to evaluation", teulIndex(), playerID(), callsign());

		setReceivedCapacity(true);
		closeCurrentSortie();
		SetSideLock();

		return true;
	}

	///TODO:PERSIST: Queue refresh of this player's account data.
	// mapFillPersonaData(this);

	// Take me out of "spawned" mode
	if ( mission() != nullptr )
		mission()->updateTaker(this, RMLIST_UPDATE, true);

	if ( missionPacket().vehicle().rtbState() == RTB_SUCCESS )
	{
		if ( (requestedCapacity() == false) & (receivedCapacity() == false) )
		{
			missionSendMMCapacityRequest(this, CAPACITY_AFTER_MISSION);
			dbugLog(_FLD, "t:%d p:%u c:%s m:%u reqCap"
							, teulIndex(), playerID(), callsign(), missionID()
							);
		}
		else
		{
			dbugLog(_FLD, "t:%d p:%u c:%s m:%u reqCap:%s rxCap:%s"
							, teulIndex(), playerID(), callsign(), missionID()
							, (requestedCapacity() ? "yes" : "no")
							, (receivedCapacity() ? "yes" : "no")
							);
		}
	}
	else
	{
		// You failed.
		setReceivedCapacity(true);
		processCapacityResponse(nullObjID, NULL_WEAP_SERIAL, false);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine it the player's client has a healthy checksum

static const PermissionMask CHECKSUM_IGNORE_MASK = (VEHICLE_LEVEL8_MASK|VEHICLE_LEVEL9_MASK|GAME_MANAGER);

void
Player::checksum(UINT64 withChecksum)
{
	bool isDirty = false;
	if ( !mapCheckChecksum(dispatch().m_appVersion, withChecksum, permissions(IS_PRODUCER)) )
	{
		auto severity = host_config.arena.checksums ? Debug::ERR_WARN : Debug::ERR_INFO;
		dbugLog(_FL, severity,
						"t:%d p:%u c:%s invalid checksum %" PR64BitUT " for app:%u plat:%u"
						, teulIndex(), playerID(), callsign()
						, withChecksum
						, dispatch().m_appVersion
						, dispatch().m_platform
						);
#if defined(PUBLIC_RELEASE)
		isDirty = true;
#endif
	}

	if ( isPlaynet() || permissions(CHECKSUM_IGNORE_MASK) )
		SetChecksumIsDirty(false);
	else
		SetChecksumIsDirty(isDirty);
}

//////////////////////////////////////////////////////////////////////
// Check to see if a player's account grants them access to the
// specified vehicle.

bool
Player::CheckWeaponMask(UINT16 weaponSpecShort) const
{
	if ( permissions(UNMASKED_VEHICLES) )
		return true;
	if ( chatPermissions() == nullptr )
		return false;
	return chatPermissions()->m_vehicleGrants.find(weaponSpecShort) != chatPermissions()->m_vehicleGrants.end();
}

//////////////////////////////////////////////////////////////////////
// Clear out this player's mission taker list

void
Player::clearMissionTakerList()
{
	RoomList emptyList;
	emptyList.m_numEntries = 1;
	memset(&emptyList.m_entry[0], 0, sizeof(RoomListEntry));
	emptyList.m_entry[0].m_action = RMLIST_INIT;

	if ( sendToClientRoomList(teulIndex(), emptyList) < 0 )
		dbugLog(_FLW, "sendToClientRoomList failed");
}

//////////////////////////////////////////////////////////////////////
// Clear out current sortie results

void
Player::clearSortieResults(bool leavingMission)
{
	m_repairRecord.clear();
	m_damageRecord.clear();
	m_useCustomSpawnLocation = false;

	if ( leavingMission )
	{
		missionPacket().flushVolatiles();
		m_hasValidMissionPacket = false;
	}
	else
	{
		missionPacket().discardSortieData();
	}
}

//////////////////////////////////////////////////////////////////////
// Garbage collect and cleanup the mission packet and all trace
// of mission subscription this player had

void
Player::deflateMissionPacket()
{
	clearSortieResults(true);

	m_launchPermitted = isPlaynet();
	m_nextLaunch = 0;
}

//////////////////////////////////////////////////////////////////////
// Evaluate a players mission packet for scoring purposes

#ifdef BETA_RELEASE
# define BING(msg) if ( !inState(PERS_EXITING) ) messagef(LSTR_BETA, LSTR_GENERIC, "Sortie Results: %s", msg)
#else
# define BING(msg)
#endif

bool
Player::evaluateMissionPacket()
{
	m_promoted = false;

	if ( !validMissionPacket() )
	{
		BING("no mission packet to evaluate");
		dbugLog(_FLA, "t:%d p:%u c:%s attempting to evaluate NULL mission packet"
						, teulIndex()
						, playerID()
						, callsign()
						);
		return false;
	}

	m_mp.wasSuccessful = m_mp.successful(weaponSpec());

	const UINT32 pointsScored = m_mp.scoring().points = (UINT32)(calculateSortieScore());

	dbugLog(_FLD, "t:%d p:%u player %s missionID %d missionType %s rtb %s successful %d totalKills %d"
				, teulIndex()
				, playerID()
				, callsign()
				, missionID()
				, _missionTypes[m_mp.m_missionType]
				, _EXPAND_RTB(m_mp.vehicle().rtbState())
				, m_mp.wasSuccessful
				, pointsScored
				);

	//increment their points
	if ( pointsScored == 0 || actualRank().kingOfTheHill() )
		return true;

	if ( useRank().level() >= maxRank() && useRank().next() != nullptr )
	{
		messagef(LSTR_NOTE, LSTR_WOULD_HAVE_SCORED, "%u", pointsScored);
		return true;
	}

	const UINT32 oldPoints = m_rankPointsEarned;

	// Avoid massive overflows
	if ( pointsScored >= actualRank().points() || pointsScored + m_rankPointsEarned > actualRank().points() )
		m_rankPointsEarned = actualRank().points();
	else
		m_rankPointsEarned += pointsScored;

	if ( !inState(PERS_EXITING) )
	{
#ifdef BETA_RELEASE
		messagef(LSTR_BETA, LSTR_GENERIC, "Mission Score: %d. Rank: %d. Points: %d/%d"
					, pointsScored
					, realRank()
					, m_rankPointsEarned
					, actualRank().points()
					);
#endif
		dbugLog(_FLI, "t:%d p:%u c:%s pers:%u m:%d score:%d, rank:%d, points:%u=>%u/%u"
					, teulIndex()
					, playerID()
					, callsign()
					, personaID()
					, missionID()
					, pointsScored
					, realRank()
					, oldPoints
					, m_rankPointsEarned
					, actualRank().points()
					);
	}

	if ( m_rankPointsEarned >= actualRank().points() )
		bumpRank();
	else
		mapSaveRank(this, false);

	// Give the database threads chance to eat the queue
	std::this_thread::yield();

	return true;
}

void
Player::ExitBrigade(UINT32 withState)
{
	// Can't be in a mission and not in a brigade
	ExitMission();

	SetState((MM_PERS_STATE)withState);

	// If we're not in a brigade, nothing needs doing
	if ( brigadeID() == HCUnitID::null )
		return;

	brigade()->removePlayer(this);

	wwiiPlayer::ExitBrigade(withState);
	SetBrigade(nullptr);

	SetState((MM_PERS_STATE)withState);

	updateChat();
}

//////////////////////////////////////////////////////////////////////
// Exit the player's current mission

void
Player::ExitMission()
{
	if ( mission() )
	{
		if ( isMissionLeader() )
			mission()->changeLeadership(nullptr);
		clearMissionTakerList();
		mission()->removePlayer(playerID(), true);
		SetMission(nullptr);
		updateChat();
		SetState(PERS_SELECTING_MISSION);
	}

	deflateMissionPacket();
}

//////////////////////////////////////////////////////////////////////
// Populate a mission packet with good stuff
// Note: this doesn't put the player* on* the mission,
// use enterMission for that.

bool
Player::fillMissionPacket(Mission* forMission)
{
	clearSortieResults(true);

	SetState(PERS_SELECTING_MISSION);
	m_promoted = false;

	if ( forMission != nullptr )
	{
		if ( forMission->originFac() == nullptr )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s m:%u mission has NULL originFac", teulIndex(), playerID(), callsign(), forMission->missionID());
			return false;
		}
		if ( country() != forMission->country() )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s m:%u persona/mission country mismatch", teulIndex(), playerID(), callsign(), forMission->missionID());
			return false;
		}
		if ( branch() != forMission->brigade()->branch() )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s m:%u persona/mission branch mismatch", teulIndex(), playerID(), callsign(), forMission->missionID());
			return false;
		}

		SetMission(forMission);

		missionPacket().m_missionType = (MISSION_TYPE)forMission->missionType();
		missionPacket().sortie().originFac = forMission->summary().origin;
		missionPacket().sortie().target = forMission->summary().target;

		updateNextLaunchTime();

		SetState(PERS_AT_MISSION);
	}
	else
	{
		SetMission(nullptr);
		m_launchPermitted = isPlaynet();
		m_nextLaunch = 0;
	}

	SetPersonaMissionFlags();

	m_hasValidMissionPacket = true;

	updateChat();

	return missionID();
}

//////////////////////////////////////////////////////////////////////
// Determine if this player is the brigade leader

bool
Player::isBrigadeLeader() const
{
	return IsValid(brigadeID()) && brigade()->owner() == chatHandle();
}

//////////////////////////////////////////////////////////////////////
// Tell me when I'm next allowed to launch

UINT32
Player::nextLaunchTime() const
{
	if ( m_nextLaunch > (UINT32)teulTime() )
		return m_nextLaunch;
	if ( mission() )
		return mission()->summary().timeLaunch;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Tell chat which cell host this player spawned to

void
Player::notifyChatOfCell() const
{
	Teul::OMGPacket<Chat::CellHostUpdate> outgoing(playerID(), getSpawnedCellHost());
	serverSendPacketToType(outgoing, CHAT_HOST);
}

//////////////////////////////////////////////////////////////////////
// Persona being returned to us from the cell hosts.

bool
Player::onDespawn(SINT32 onTeulIndex)
{
	if ( onTeulIndex >= 0 && onTeulIndex != teulIndex() )
	{
		dbugLog(_FLW, "t:%d p:%u ot:%d player trying to return on wrong teulIndex", onTeulIndex, playerID(), teulIndex());
		// This looks like a mismatch.
		return false;
	}

	log(_FLD, "player returning");

	if ( isConnected() )
	{
		///TODO:PERSIST: Review... I'm thinking that we don't want to take authority for players who aren't connected,
		///              but this potentially opens up a race condition if a player is in mid-reconnect.
		///              Other option: boot the player.
		PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByTeulIndex(onTeulIndex);
		if ( psn != nullptr )
			psn->takeAuthorityForThisPlayer();
	}

	SetSideLock();

	///TODO:SCORING: This is supposed to be an ongoing thing.
	if ( state() == PERS_SPAWNING )
	{
		const bool result = calculateMissionResults(PERS_DEBRIEFING);
		if ( result == false )
		{
			log(_FLW, "unable to calculate player's mission results");
			betaMessage("mission result calculation exploded, caught fire, and said no.");		///TODO:PERSIST:Remove
			///TODO:PERSIST: Review... Should we kick the player at this point?
			closeCurrentSortie();
		}
	}
	else
	{
		dbugLog(_FLD, "t:%d p:%u c:%s not in PERS_SPAWNING, so not calculating mission results", teulIndex(), playerID(), callsign());
	}

	notifyChatOfCell();
	updateChat();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Tell the player that their post [request] has been denied.

void
Player::postDenied(const WEAPON_SPEC forWeapon, UINT32 forReasonCode, bool loseWeapon)
{
	BETAInfo(teulIndex());

	if ( loseWeapon )
		cancelWeapon(false);

	mmSendPostDeny(teulIndex(), forWeapon, forReasonCode, loseWeapon);
}

//////////////////////////////////////////////////////////////////////
// Handle the capacity response returned to us from the strat host
// during a mission-evaluation
// called by _recvMMFacilityCapacityResponse in strtMapMessages.cpp

void
Player::processCapacityResponse(const FULL_OBJ_ID rtbFacility_, const WEAPON_SERIAL& reservedWeapon_, const bool vehicleDelivered_)
{
	setReceivedCapacity(true);

	dbugLog(_FLD, "t:%d p:%u c:%s %s %s", teulIndex(), playerID(), callsign(), __FUNCTION__, (vehicleDelivered_ ? "vehicle delivered" : "vehicle kept"));

	// Player may have been fast enough to cause us to lose their
	// mission packet
	if ( !validMissionPacket() )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s no mission packet for capacity response", teulIndex(), playerID(), callsign());
		return;
	}

	m_mp.m_vehicleResupplied = vehicleDelivered_;

	closeCurrentSortie();

	if ( reservedWeapon_.serial != 0 && rtbFacility_.id != 0 )
	{	// they have a vehicle reserved for a successful rtb
		m_weapOrigin = m_mp.vehicle().rtbFacility = rtbFacility_;
		m_mp.rtbReservedWeapon = reservedWeapon_;
		SetWeapon(reservedWeapon_);
		SetVehicleState(MM_VEH_PRERESERVED);
		dbugLog(_FLD, "t:%d p:%u c:%s preserved vehicle #%d", teulIndex(), playerID(), callsign(), reservedWeapon_.serial);
	}
	else
	{	// neither persona nor vehicle rtb'd
		SetVehicleState(MM_VEH_NONE);
		m_mp.vehicle().rtbFacility = nullObjID;
		m_mp.rtbReservedWeapon = NULL_WEAP_SERIAL;
		SetWeapon(NULL_WEAP_SERIAL);

		if ( vehicleDelivered_ == true )
			dbugLog(_FLD, "t:%d p:%u c:%s vehicle delivered to destination", teulIndex(), playerID(), callsign());
		else
			dbugLog(_FLD, "t:%d p:%u c:%s vehicle lost", teulIndex(), playerID(), callsign());
	}

	SetSideLock();

	mmSendMissionResults(*this);

	if ( mission() )
		sendFullMissionTakerList();
}

//////////////////////////////////////////////////////////////////////
// Update CPs for this player.

void
Player::updateMap()
{
	// You need to have a side selected.
	if ( noSide() == true || instance().isValid() == false )
		return;
	Strat::Events::SendMapState(teulIndex(), instance().isCampaign(), mapCPList);
}

//////////////////////////////////////////////////////////////////////
// If I'm on a mission, update my room taker entry

void
Player::updateMissionTaker(bool excludeSelf)
{
	///TODO: Queue these up
	if ( mission() )
		mission()->updateTaker(this, RMLIST_UPDATE, excludeSelf);
}

//////////////////////////////////////////////////////////////////////
// Reset the next launch time and launch permitted fields,
// and determine when the player can launch again

void
Player::updateNextLaunchTime()
{
	m_launchPermitted = false;
	m_nextLaunch = 0;

	// Not on a mission, you can't launch.
	if ( !mission() )
		return;

	if ( mission()->isSystemMission() )
	{
		// If we're on a system mission we can launch
		// otherwise we're not on a mission so we can't
		m_launchPermitted = true;
		return;
	}

	// Delay player by a persona launch delay timer?
	m_nextLaunch = teulTime() + mapGetSideDelay(side());
	if ( m_nextLaunch > (UINT32)teulTime() )
		return;

	// Are they subject to a mission launch delay?
	if ( mission()->summary().timeLaunch > (UINT32)teulTime() )
		return;

	// If we're using launch cycles, you need to see one
	// complete before we'll let you spawn
	if ( host_config.mission.launchCycle_s > 0 )
		return;

	m_launchPermitted = true;
}

//////////////////////////////////////////////////////////////////////
// Present this player's roomlist flags

UINT32
Player::updateRoomListFlags()
{
	m_roomFlags = 0;

	if ( isMissionLeader() )
		m_roomFlags |= RMLIST_ROOM_OWNER;
	if ( isSpawned() )
	{
		dbugLog(_FLD, "p:%u c:%s isSpawned", playerID(), callsign());
		m_roomFlags |= RMLIST_SPAWNED;
	}
	if ( vehicleState(MM_VEH_WAITING_CREW) )
		m_roomFlags |= RMLIST_WANTS_CREW;
	else if ( vehicleState(MM_VEH_HAS_CREW) )
		m_roomFlags |= RMLIST_PILOT;
	else if ( vehicleState(MM_VEH_JOINED) )
		m_roomFlags |= RMLIST_CREW;

	///TODO: Implement other flags

	return m_roomFlags;
}

//////////////////////////////////////////////////////////////////////
// Select a brigade for my persona

bool
Player::SelectBrigade(Brigade* selection)
{
	// If you're selecting a brigade, you must be dumping your mission packet
	ExitMission();

	if ( selection != brigade() )
		ExitBrigade(PERS_SELECTING_BRIGADE);

	// But, if you're already in the brigade, we can do away with the info
	if ( brigade() != selection )
	{
		if ( !selection->accessible(this) )
			return false;

		SetBrigade(selection);

		selection->addPlayer(this);

		SetSideLock();
	}

	SetState(PERS_SELECTING_MISSION);

	updateMask();
	updateChat();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Attach me to a particular mission

MISSION_CODE
Player::selectMission(Mission* toMission, bool enter)
{
	if ( toMission == nullptr )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s %s: called with NULL mission", teulIndex(), playerID(), callsign(), __FUNCTION__);
		return MC_EXPIRED;
	}

	BETAInfo(teulIndex());

	MISSION_CODE access = toMission->accessibility(this);

	if ( access >= MC_ERROR )
	{
		return access;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s taking mission %d", teulIndex(), playerID(), callsign(), toMission->missionID());

	fillMissionPacket(toMission);

	if ( enter )
		enterMission(toMission);

	return MC_JOIN;
}

//////////////////////////////////////////////////////////////////////
// Change a players' persona information and affiliations

void
Player::selectNullPersona()
{
	// No change, no pain
	if ( personaID() != 0 )
	{
		// Change of persona details must invalidate the following
		ExitBrigade(PERS_SELECTING_PERSONA);

		SetCountry(WWII_NO_COUNTRY, WWII_NO_SIDE);
		SetBranch(BRANCH_NOT_USED);

		SetRank(0);

		m_personaID = 0;
	}

	SetState(PERS_SELECTING_PERSONA);

	// Have to tell chat what our current status is
	updateChat();
}

//////////////////////////////////////////////////////////////////////
// Function which sends an actual update to chat
// Use updateChat() to flag that you have an update that needs
// to be sent, so that updates can be aggregated, please.

void
Player::sendUpdateToChat(bool force)
{
	if ( ! (m_needsChatSync || force) )
		return;

	dbugLog(_FLD, "t:%d p:%u c:%s sending chat update", teulIndex(), playerID(), callsign());

	FULL_OBJ_ID origin, target;
	ChatHandle leader;

	if ( mission() )
	{
		origin = mission()->originCP()->fullObjID;
		target = mission()->targetCP()->fullObjID;
		leader = mission()->posterHandle();
	}
	else
	{
		origin = nullObjID;
		target = nullObjID;
		leader = 0LL;
	}

	// Since this is going to the chat host, we don't want to include
	// the chat permissions mask in the mask we send
	PermissionMask mask = m_playerMask | useRank().permissions();

	Chat::PersonaUpdate cpu(matchHandle(), country(), branch(), realRank(), instance(), sortieID(), brigadeID(), missionID(), leader, origin, target, mask, weapon().spec, isSpawned(), isChecksumDirty());
	if ( cpu.send() < 0 )
		dbugLog(_FLW, "t:%d p:%u c:%s failed to send personaupdate", teulIndex(), playerID(), callsign());

	m_needsChatSync = false;
}


//////////////////////////////////////////////////////////////////////
// Derive brigade values from a brigade pointer - OR NULL

void
Player::SetBrigade(Brigade* newBrigade)
{
	m_brigade = newBrigade;
	if ( m_brigade != nullptr )
		m_brigadeID = m_brigade->m_unitID;
	else
		m_brigadeID = HCUnitID::null;
}

//////////////////////////////////////////////////////////////////////
// Set the mission attributes

void
Player::SetMission(Mission* to)
{
	m_mission = to;
	m_missionID = (to) ? to->missionID() : 0;

	// Is this my mission?
	if ( to && to->ongoing() && !to->active() && to->playerID() == playerID() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s m:%d mission leader taking mission, marking as available"
						, teulIndex(), playerID(), callsign(), missionID());
		to->activate();
	}

	SetPersonaMissionFlags();
}

//////////////////////////////////////////////////////////////////////
// Override the setInstance so that it notifies chat.

bool
Player::SetInstance(const Instance::Descriptor& toInstance)
{
	// True if we are either switching into or out of the campaign;
	// switching between non-campaign instances or between campaign
	// personas shouldn't cause an update.
	const bool switchingToOrFromCampaign = (instance().isInstanceOf(Instance::g_campaign) ^ toInstance.isInstanceOf(Instance::g_campaign));

	// Parent setInstance returns false if there is no change.
	if ( false == wwiiPlayer::SetInstance(toInstance) )
		return false;

	// Change occurred, chat host needs to be notified.
	updateChat();

	// Clear their CP and HC Unit update markers
	m_lastHCUpdate = 0;

	if ( switchingToOrFromCampaign )
		updateMap();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set up the any persona-specific flags in the mission packet

void
Player::SetPersonaMissionFlags()
{
	updateMask();

	dbugLog(_FLD, "t:%d p:%u c:%s rank:%d perm:%" PRPermMaskT " hc:%d", teulIndex(), playerID(), callsign(), realRank(), permissionMask(), isOfficer());
}

//////////////////////////////////////////////////////////////////////
// Set the sidelock time for a persona

void
Player::SetSideLock()
{
	if ( permissions(FREE_SIDE_SWITCH) )
		return;

	WWII_SIDE lockSide = isValidSide(side()) ? side() : m_lastSide;
	if ( !isValidSide(lockSide) )
		return;

	if ( m_lastSide >= NEUTRAL && teulTime() < m_sideLockTime )
		return;

	UINT32 newTime = teulTime() + (host_config.sideLockTime_m * 60);

	// Don't bother updating the database more than once every 60s
	if ( lockSide != m_lastSide || newTime > (m_sideLockTime + 60) )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s locking side %u until %u"
						, teulIndex(), playerID(), callsign()
						, lockSide, newTime
						);

		DBThread::Do(DBThread::Default
			, "UPDATE wwii_player"
				" SET lockside = %u, locktime = %ld, modified = now()"
				" WHERE playerid = %u AND lockside < 3"
					" AND (lockside != %u OR locktime < %ld)"
			, lockSide, newTime
			, playerID()
				, lockSide, newTime
		);

		m_sideLockTime = newTime;
	}

	m_lastSide = lockSide;
}

//////////////////////////////////////////////////////////////////////
// Set the persona's rank and populate the fields

void
Player::SetRank(RANK_NO level)
{
	if ( realRank() == level )
		return;

	const Rank* entry = Rank::lookup(level);
	if ( entry == nullptr )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s tried to set rank to %d", teulIndex(), playerID(), callsign(), level);
		if ( level != 0 )
			SetRank(0);
		return;
	}

	m_realRank = level;
	m_actualRank = entry;

	if ( effectiveRank() < level )
	{
		const Rank* effectiveEntry = Rank::lookup(effectiveRank());
		m_useRank = (effectiveEntry != nullptr) ? effectiveEntry : m_actualRank;
	}
	else
		m_useRank = m_actualRank;
}

//////////////////////////////////////////////////////////////////////

void
Player::SetState(MM_PERS_STATE toState)
{
	if ( m_state != toState )
	{
		if ( m_spawnedAsSide )
		{
			--g_spawnedPlayers[m_spawnedAsSide];
			m_spawnedAsSide = WWII_NO_SIDE;
		}
		m_state = toState;
		if ( m_state == PERS_SPAWNING && instance().isCampaign() )
		{
			m_spawnedAsSide = side();
			if ( m_spawnedAsSide )
				++g_spawnedPlayers[m_spawnedAsSide];
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Set the persona's mission target

void
Player::SetTarget(const FacilityStruct* facility)
{
	if ( facility == nullptr )
		missionPacket().sortie().target = missionPacket().sortie().originFac();
	else
		missionPacket().sortie().target = facility->fullObjID;
}

//////////////////////////////////////////////////////////////////////
// Set the perona's current weapon and the flag indicating whether
// or not it is a buzzard

void
Player::SetWeapon(const WEAPON_SERIAL& toWeapon)
{
	m_weapon = toWeapon;

	static const WEAPON_SPEC buzzard = weapFindWeaponByName("uk_buzzard");
	static const WEAPON_SPEC skull = weapFindWeaponByName("uk_skull");
	m_isBuzzard = ( buzzard.wCategory == m_weapon.spec.wCategory && buzzard.wClass == m_weapon.spec.wClass
					&& (buzzard.wType == m_weapon.spec.wType || skull.wType == m_weapon.spec.wType) );
}

//////////////////////////////////////////////////////////////////////
// Flag that this user needs an update sending to chat,
// unless they already have one pending

void
Player::updateChat(bool immediate)
{
	if ( immediate ) {
		sendUpdateToChat(true);
		return;
	}

	// Don't need to do this if they have an update that hasn't already been sent
	if ( needsChatSync() )
		return;

	dbugLog(_FLD, "t:%d p:%u c:%s flagging for persona update perms:%" PRPermMaskT " (plyr:%" PRPermMaskT ", chat:%" PRPermMaskT ")"
				, teulIndex(), playerID(), callsign(), permissionMask(), m_playerMask, m_permissions->m_permissionMask);

	g_chatSyncQueue.push_back(this);
	m_needsChatSync = true;
}

//////////////////////////////////////////////////////////////////////
// Update the player's permission masks

void
Player::updateMask()
{
	// Clear the mission/brigade leader flags.
	m_playerMask &= ~(MISSION_LEADER | BRIGADE_LEADER);

	if ( m_mission && m_mission->playerID() == playerID() )
		m_playerMask |= MISSION_LEADER;
	if ( isBrigadeLeader() )
		m_playerMask |= BRIGADE_LEADER;

	PermissionMask newMask = m_playerMask | useRank().permissions() | m_permissions->m_permissionMask;

#if defined(TRAINING_SERVER)
	newMask |= POST_MISSIONS;
#endif

	if ( newMask != m_permissionMask )
	{
		m_permissionMask = newMask;
		updateChat();
	}
}

//////////////////////////////////////////////////////////////////////
// Initial login for a player - start of session

static SINT32
_playerLoggingIn(SINT32 teulIndex, const playerid_t playerID, PlayerStatusNode& psn, const Server::PersonaXfer& xfer)
{
	Player* const previous = PlayerInterface::Lookup(playerID);
	if ( previous != nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s *** Previous instance %p still present! SHOULD NOT HAPPEN"
					, teulIndex, playerID, xfer.dispatch().m_callsign, previous);

		SINT32 oldTeulIndex = previous->teulIndex();
		if ( oldTeulIndex < 0 )
			oldTeulIndex = 0;
		previous->Release(oldTeulIndex, Server::ReleaseType::Collision);
	}

	if ( xfer.data.auxData != nullptr )
		throw std::logic_error("Non-NULL xfer data in login");

	Player* const player = new Player(-EINVAL, xfer.dispatch(), playerID);
	if ( player == nullptr )
		throw std::runtime_error("Out of memory");								///Terminal

	dbugLog(_FLD, "t:%d p:%u c:%s ip:%s ptr:%p +ALLOC", teulIndex, playerID, player->callsign(), teulGetAddress(xfer.dispatch().m_clientIP), player);

	// Establish this as their actual callsign
	memcpy(psn.dispatch().m_callsign, player->callsign(), sizeof(psn.dispatch().m_callsign));

	player->selectNullPersona();

	player->timeIn = teulTime();
	psn.localInfo = player;

	g_players.insert(player);

	return player->playerID();
}
//////////////////////////////////////////////////////////////////////
// A persona-session is being ejected from the cell hosts for us to
// score and save. If a player CTDs, this occurs about 1-2 minutes
// after the player disconnects. If the player logs back in during
// that time, the login event will cause the cell host to spit out
// the old record to us, so we will have a new record for the
// player to deal with also.

static SINT32
_playerExiting(SINT32 teulIndex, const playerid_t playerID, const Server::PersonaXfer& xfer)
{
	Player* player = PlayerInterface::Lookup(playerID);
	if ( player == nullptr )
		return 0;

	if ( player->OTP() != xfer.dispatch().m_OTP )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s doesn't match current OTP, ignoring", player->teulIndex(), player->playerID(), player->callsign());
		return 0;
	}

	player->ExitBrigade(PERS_EXITING);

	player->SetSideLock();
	player->SetState(PERS_EXITING);

	delete player;

	return 0;
}

SINT32
mapCreatePlayer(SINT32 teulIndex, PlayerStatusNode* psn, const Server::PersonaXfer& xfer)
{
	const playerid_t playerID = xfer.dispatch().m_customerID;

	dbugLog(_FLI, "t:%d p:%u c:%s PERS: %s (%d)"
			, teulIndex, playerID, xfer.dispatch().m_callsign
			, xferReasons[(UINT32)xfer.reason()]
			, static_cast<int>(xfer.reason()));

	try
	{
		if ( playerID <= 1 )
			throw std::invalid_argument("Illegal playerID");

		switch ( xfer.reason() )
		{
		case Server::Transfer::XferReason::Login:
			assert( psn != nullptr );
			return _playerLoggingIn(teulIndex, playerID, *psn, xfer);

		case Server::Transfer::XferReason::Reassign:
			LogAbort("--- player logging in with PXR-TRANSFER; wtf oliver ---");

		case Server::Transfer::XferReason::Exit:
			return _playerExiting(teulIndex, playerID, xfer);

		default:
			throw std::invalid_argument("unhandled xfer reason");
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "t:%d p:%u c:%s PERS: %s %s", teulIndex, playerID, xfer.dispatch().m_callsign, xferReasons[(UINT32)xfer.reason()], e.what());
		if ( psn != nullptr )
			psn->localInfo = nullptr;
	}

	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
// Called when a player is closing their connection to us, including
// when we transfer them to the cell host during spawning.

SINT32
mapReleasePlayer(SINT32 teulIndex, PlayerStatusNode* psn, Server::ReleaseType releaseType)
{
	if ( psn == nullptr && teulIndex < 0 )
		LogAbort("can't handle NULL player node and teulIndex");
	else if ( psn == nullptr )
	{
		dbugLog(_FLA, "t:%d don't know what to do with NULL player", teulIndex);
		return -EINVAL;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s otp:%u PERS: release %d [%s] p:%p active:%d"
					, teulIndex
					, ((psn)?psn->dispatch().m_customerID:0)
					, ((psn)?psn->dispatch().m_callsign:"???")
					, ((psn)?psn->dispatch().m_OTP:0)
					, static_cast<int>(releaseType)
					, Server::GetReleaseType(releaseType)
					, psn
					, _numTotalPlayers()
					);

	playerid_t playerID = 0;
	try
	{
		playerID = psn->dispatch().m_customerID;
		Player* player = PlayerInterface::Lookup(playerID);
		if ( player == nullptr )
		{
			dbugLog(_FLD, "t:%d p:%u persona already released", teulIndex, playerID);
			return 0;
		}

		dbugLog(_FLD, "t:%d p:%u c:%s PERS: releasing from state %s", player->teulIndex(), player->playerID(), player->callsign(), _pstates[player->state()]);

		psn->localInfo = nullptr;
		if ( teulIndex >= 0 )
			teulSetPersonaData(teulIndex, nullptr);
		else if ( player->teulIndex() >= 0 )
			teulSetPersonaData(player->teulIndex(), nullptr);

		///TODO: Think about this, it doesn't seem like such
		///a great idea. What if we have the wrong teulIndex?
		player->SetTeulIndex(teulIndex);
		assert( releaseType != Server::ReleaseType::Redirected );

		player->SetState(PERS_EXITING);

		// Delete any vehicle reserved to this player
		mmSpawnDeleteVehicle(*player);

		player->SetInstance(Instance::g_invalid);

		player->SetState(PERS_EXITING);

		delete player;
		dbugLog(_FLN,"t:%d p:%u PERS: released persona for %d [%s]"
					, teulIndex, playerID
					, static_cast<int>(releaseType)
					, Server::GetReleaseType(releaseType));
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d p:%u %s(%s#%d): %s"
					, teulIndex, playerID, __FUNCTION__
					, Server::GetReleaseType(releaseType)
					, static_cast<int>(releaseType)
					, e.what());
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// When a cell host crashes, all the players that are spawned on it
// would eventually time out and exit the network. We need to catch
// these exits and make sure we don't leave parked records around
// for the player.

SINT32
mapPlayerExitCluster(PlayerStatusNode& psn)
{
	const playerid_t playerID = psn.dispatch().m_customerID;
	if ( playerID <= DEFAULT_PLAYER_ID )
	{
		dbugLog(_FLW, "p:%u personaExitCluster with invalid playerID", playerID);
		return -EINVAL;
	}

	Player* const player = PlayerInterface::Lookup(playerID);
	if ( player == nullptr )
		return 0;
	if ( player->OTP() != psn.dispatch().m_OTP )
	{
		player->log(_FLM, "otp:%u/%u PERS: otp mismatch, not exiting this persona from cluster."
					, player->OTP(), psn.dispatch().m_OTP);
		return 0;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s otp:%u PERS: exiting cluster", player->teulIndex(), player->playerID(), player->callsign(), psn.dispatch().m_OTP);

	// Don't delete the persona directly
	player->SetState(PERS_EXITING);
	psn.localInfo = player;

	return 0;
}

void
mapSendChatUpdates()
{
	while ( !g_chatSyncQueue.empty() )
	{
		Player* player = g_chatSyncQueue.front();
		g_chatSyncQueue.pop_front();
		player->sendUpdateToChat();
	}
}

void
Player::startLoginSpam(bool clientActivated, bool xferedToChat)
{
	if ( clientActivated )
		m_clientActivated = true;
	if ( xferedToChat )
		m_xferedToChat = true;

	// Both must have occured before we can proceed
	if ( !m_clientActivated || !m_xferedToChat )
		return;

	if ( m_loginState > 0 )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s called %s from state %d", teulIndex(), playerID(), callsign(), __FUNCTION__, m_loginState);
		return;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s requesting login spam", teulIndex(), playerID(), callsign());

	mapRequestTutProgress(playerID(), teulIndex());

	m_loginState = 1;
	m_loginSub = 0;
	m_nextSpam = teulClock64 + 1250;
	g_logins.push_back(this);
}

//////////////////////////////////////////////////////////////////////
// Send a list of HC Units to this persona

bool
_sendHCUnitList(Player* player)
{
	if ( hcUnits.empty() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s no hcunits to send", player->teulIndex(), player->playerID(), player->callsign());
		return true;
	}

	// Find the next available country to send to them
	// and then increment the sub value and find if there
	// are any countries left to send after
	bool finished = false;
	while ( !activeCountries[player->m_loginSub] && player->m_loginSub < MAX_COUNTRIES )
		player->m_loginSub++;
	if ( player->m_loginSub >= MAX_COUNTRIES )
		return true;
	WWII_COUNTRY country = (WWII_COUNTRY)(player->m_loginSub++);
	while ( player->m_loginSub < MAX_COUNTRIES && !activeCountries[player->m_loginSub] )
		player->m_loginSub++;
	if ( player->m_loginSub >= MAX_COUNTRIES )
		finished = true;


	HCUNIT_INDEX::iterator it;
	HCUnitList hcul;

	hcul.m_listEnd = false;

	UINT32 sent = 0;

	for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
	{
		if ( it->second->country() != country )
			continue;

		if ( hcul.m_count == MAX_HCUNITS_PER_LIST )
		{
			if ( hcul.send(player->teulIndex()) < 0 )
				dbugLog(_FLW, "t:%d p:%u c:%s error sending HCUL#%u", player->teulIndex(), player->playerID(), player->callsign(), hcul.m_count);
			hcul.m_listStart = false;
			hcul.m_count = 0;
		}
		hcul.m_units[hcul.m_count] = it->second;
		++hcul.m_count;
		++sent;
	}
	hcul.m_listEnd = true;

	if ( hcul.m_count > 0 && hcul.send(player->teulIndex()) < 0 )
		dbugLog(_FLW, "t:%d p:%u c:%s error closing HCUL list (#%d)", player->teulIndex(), player->playerID(), player->callsign(), hcul.m_count);

	dbugLog(_FLD, "t:%d p:%u c:%s sent %u %s hc units", player->teulIndex(), player->playerID(), player->callsign(), sent, _countryNameAdj[country]);

	return finished;
}

//////////////////////////////////////////////////////////////////////
// Build a HCUnits movements update from a given start point

static void
_buildMovementsUpdate(HC_UNIT_LOCATIONS& packet, const UINT64& start, const UINT64& end)
{
	BRIGADE_MOVEMENTS::iterator it;
	HCUnit* unit;
	HCUnitLocation entry;
	packet.clear();
	for ( it = brigadeMovements.begin(); it != brigadeMovements.end(); ++it )
	{
		if ( it->second <= start || it->second > end )
			continue;
		unit = HCUnit::Lookup(it->first);
		if ( !unit )
			continue;
		entry.m_unitID = ToInt(it->first);
		entry.m_homeCPID = unit->homeCPID();
		packet.add(entry);
	}
}

//////////////////////////////////////////////////////////////////////
// Send HCUnitLocations

static bool
_sendHCUnitMovements(Player* const player)
{
	if ( lastBrigadeMovement == 0 || brigadeMovements.size() == 0 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s no movements to send", player->teulIndex(), player->playerID(), player->callsign());
		return true;
	}

	if ( player->m_lastHCUpdate >= lastBrigadeMovement )
		return true;

	dbugLog(_FLD, "t:%d p:%u c:%s movements %" PR64BitUT "->%" PR64BitUT "", player->teulIndex(), player->playerID(), player->callsign(), player->m_lastHCUpdate, lastBrigadeMovement);

	static UINT64 _lastStart = 0;
	static UINT64 _lastEnd = 0;
	static HC_UNIT_LOCATIONS _packet;

	// Do we have to build this packet?
	if ( player->m_lastHCUpdate != _lastStart || lastBrigadeMovement != _lastEnd )
	{
		_lastStart = player->m_lastHCUpdate;
		_lastEnd = brigadeMovementWatermark;
		_buildMovementsUpdate(_packet, _lastStart, _lastEnd);
	}

	if ( _packet.size() > 0 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s updating with HCMovements %" PR64BitUT "->%" PR64BitUT ""
						, player->teulIndex()
						, player->playerID()
						, player->callsign()
						, _lastStart
						, _lastEnd
						);
		_packet.send(player->teulIndex());
	}

	player->m_lastHCUpdate = _lastEnd;

	return true;
}

//////////////////////////////////////////////////////////////////////
// We send a lot of data to players who are logging in, sweep
// thru the pending logins queue and send them their data.

void
mapProcessLogins()
{
	if ( g_logins.empty() )
		return;

	PLAYER_LIST::iterator it, prev;
	// Increment is done as soon as we obtain
	for ( it = g_logins.begin(); it != g_logins.end(); )
	{
		prev = it;
		Player* player = *(it++);

		if ( player->teulIndex() < 0 )
			player->m_loginState = 255;

		if ( player->m_nextSpam > teulClock64 )
			continue;

		bool advanceState = true;

		switch ( player->m_loginState )
		{
			case 1:
				advanceState = _sendHCUnitList(player);
			break;

			case 2:
				g_arenaDescription.send(player->teulIndex());
			break;

			case 3:
				mapRequestPersonaSummaryList(player);
			break;

			case 4:
				advanceState = _sendHCUnitMovements(player);
			break;

			case 5:
				Map::SendSwingMeter(player->teulIndex());
			break;

			case 6:
				advanceState = mmCheckOpinionPolls(player);
			break;

			default:
				dbugLog(_FLI, "t:%d p:%u c:%s login spam complete", player->teulIndex(), player->playerID(), player->callsign());
				player->m_loginState = 255;
				player->m_loginSub = 0;
				player->m_nextSpam = 0LL;
				advanceState = false;
				g_logins.erase(prev);
			break;
		}

		if ( advanceState )
		{
			player->m_nextSpam = teulClock64 + 500;
			player->m_loginState++;
			player->m_loginSub = 0;
		}
		else
			player->m_nextSpam = teulClock64 + 200;
	}
}

void
Player::debug(const char* text) const
{
	this->log(text);
	message(LSTR_BETA, LSTR_GENERIC, text);
}

void
Player::debug(const char* label, const char* mask, ...) const
{
#ifdef BETA_RELEASE
	static char buf[512];

	va_list args;
	va_start(args, mask);
	vsnprintf(buf, sizeof(buf) - 1, mask, args);
	va_end(args);

	dbugLog(_FLD, "t:%d p:%u c:%s in:%" PR64BitUT " %s: %s"
				, teulIndex(), playerID(), callsign(), instance().id()
				, label, buf);
	chatSendLocalizedMessage(playerID(), LSTR_BETA, LSTR_LABELLED_GENERIC, label, buf);
#endif
}

void
mapSendHCUpdates()
{
	static const TeulTimeMS UPDATE_INTERVAL_MS = 250;
	static TeulTimeMS nextUpdate = teulClock64;

	if ( lastBrigadeMovement == brigadeMovementWatermark )
	{
		return;
	}

	if ( nextUpdate > teulClock64 )
		return;

	PLAYERS::iterator it;
	Player* player;

	brigadeMovementWatermark = lastBrigadeMovement;

	for ( it = g_players.begin(); it != g_players.end(); ++it )
	{
		player = static_cast<Player*>(*it);
		// Don't send updates to players who are still logging in or
		// in a non-campaign instance.
		if ( player->hasCompletedSpam()
			&& player->instance() == Instance::g_campaign
			&& player->m_lastHCUpdate < brigadeMovementWatermark
			)
			_sendHCUnitMovements(player);
	}

	nextUpdate = teulClock64 + UPDATE_INTERVAL_MS;
}

bool
Player::hasAvailableVehicle() const
{
	if ( !hasVehicle() )
		return false;
	if ( country() != weapon().spec.wCountry )
		return false;

	return true;
}

static bool
_isEffectRepair(STRAT_DAMAGE_EFFECT effect)
{
	switch (effect)
	{
	case SDE_REPAIRED:
	case SDE_REBUILT:
		return true;
	default:
		return false;
	}
}

void
Player::addStratHit(const MissionDamageEvent& mde)
{
    dbugLog(_FLD, "t:%d p:%u c:%s received MDE for target %" PR64BitT " type %d"
                    , teulIndex(), playerID(), callsign()
                    , mde.m_targetID
                    , mde.m_targetType
                    );

	// Make sure we understand this packet
	if ( mde.m_effectType <= SDE_INVALID )
	{
        dbugLog(_FLD, " invalid");
		return;
	}
	if ( mde.m_effectType >= SDE_NUM_EFFECTS )
	{
        dbugLog(_FLD, " unknown effect");
		return;
	}

	const bool isRepair = _isEffectRepair(mde.m_effectType);
	auto& record = isRepair ? m_repairRecord : m_damageRecord;
	auto it = record.find(mde.m_targetID);
	if ( it != record.end() && it->second >= teulTime() )
	{
        dbugLog(_FLD, " duplicate");
		return;
	}
	record[mde.m_targetID] = teulTime() + host_config.score.stratDmgInterval_s;

	auto& score = missionPacket().scoring();

	switch ( mde.m_targetType )
	{
	case SDT_AUTOGUNNER:
		if ( isRepair )
			++ score.aiRepairs;
		else
			++ score.aiKills;
	break;

	case SDT_BRIDGE:
		if ( isRepair )
			++ score.bridgeRepairs;
		else
			++ score.bridgeKills;
	break;

	case SDT_FACTORY:
		if ( isRepair )
			break;
		else
			++ score.factoryKills;
	break;

	case SDT_FIREBASE:
		if ( isRepair )
			break;
		else
			++ score.firebaseKills;
	break;

	default:
	break;
	}

	dbugLog(_FLD, " dr#:%" PRSizeT ", rr#:%" PRSizeT ", aiHits:%u aiReps:%u brdgHits:%u brdgReps:%u facHits:%u fbHits:%u"
					, m_damageRecord.size(), m_repairRecord.size()
					, score.aiKills(), score.aiRepairs(), score.bridgeKills(), score.bridgeRepairs(), score.factoryKills(), score.firebaseKills());
}

UINT32
Player::progress() const
{
	UINT32 pctg = points() * 100 / actualRank().points();
	return ( pctg > 100 ) ? 100 : pctg;
}

//////////////////////////////////////////////////////////////////////
// Notification that a user has aborted their app with alt-f4

void
mapUserAbortedApp(Player* player)
{
	dbugLog(_FLI, "t:%d p:%u c:%s player aborted application", player->teulIndex(), player->playerID(), player->callsign());
	player->cancelWeapon(true);
	player->ExitMission();
	player->ExitBrigade(PERS_EXITING);
}

void
mapPlayerInit()
{
	assert( g_players.empty() );
	g_players.clear();
	g_chatSyncQueue.clear();
	g_logins.clear();

	Teul::uWeb::s_debugServer.addVirtualFile("Player counts", "/player-balance.json", _reportPlayerDetails);
}
