#include <syscfg.h> // Path is project specific.
// Date: Wed Jan 17 17:23:20 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "build_info.hpp"
#include "auth/authPlayerDetails.h"
#include "chat/chatGroupPermissionTypes.h"
#include "chat/chatName.h"
#include "dbse/dbseHandle.h"
#include "include/arenaLockEnums.h"
#include "include/mydbQuery.h"
#include "map/mapArenaDescription.h"
#include "map/mapBrigades.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapChecksums.h"
#include "map/mapData.h"
#include "map/mapDB.h"
#include "map/mapHostMessages.h"
#include "map/mapPersonaSummary.h"
#include "map/mapPlayer.h"
#include "map/mapSwingMeter.h"
#include "miss/mission.h"
#include "miss/missHitArrayNode.h"
#include "mm/mmQueries.h"
#include "sdm/sdmConfigVars.h"
#include "server/serverServerInfo.h"
#include "str/stringLib.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "tut/tutGroupProgress.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiErrorTypes.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiRank.h"
#include "wwii/wwiiThreads.h"

bool activeCountries[MAX_COUNTRIES];

extern void weapLoadIndexes(DB::handle_t* conn);

struct DB_PERM_STRUCT
{
	PermissionMask mask;
	SINT32 id;
	SINT32 owner;
};

//static VEH_LIST_STRUCT _vehList;

extern SwingMeter g_swingMeter;

// Frequency for checking brigade detail updates.
static const SINT32 _STANDARD_BDU_INTERVAL_MS = 1 * 3 * 1000;

static UINT32		s_lastUnitModifiedStamp = 0;
static TeulTimeMS	s_nextBrigadeDetailsQuery = 0;

//////////////////////////////////////////////////////////////////////
// Read the active country list

static bool
_readActiveCountries()
{
	for ( UINT32 i = 0; i < MAX_COUNTRIES; i++ )
		activeCountries[i] = false;

	try
	{
		DBQuery q(dbh, QRY_SELECT_ACTIVE_COUNTRIES);

		if ( q.HasRows() == false )
			throw std::logic_error("No countries active");

		while (q.FetchRow())
		{
			WWII_COUNTRY country = (WWII_COUNTRY)q.Row().NextUInt();
			if ( country > 0 && country < MAX_COUNTRIES )
				activeCountries[country] = true;
		}

		return true;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what());
		return false;
	}
}

//////////////////////////////////////////////////////////////////////
// Insert a description of this host build into the arenavars.

static void _removePrefix(std::string& from, const std::string& prefix)
{
	const size_t matchLen = prefix.length();
	if ( from.compare(0, matchLen, prefix) == 0 )
		from.erase(0, matchLen);
}

static void
_setArenaDescriptionHostRevision()
{
	char hostRevision[200];
	strprintf(hostRevision, "%s (%s)", PN_git_commitish_string, cv_stratLanguage.GetString());

	std::string revision = hostRevision;
	_removePrefix(revision, "branches/");
	_removePrefix(revision, "tags/");
	g_arenaDescription["host_rev"] = revision;

	dbugLog(_FLI, "set arena host revision to '%s'", revision.c_str());
}

//////////////////////////////////////////////////////////////////////
// The brigades will have their home location position but we need
// to actually move them there for various reasons.

static void _initializeBrigadeLocations()
{
	HCUNIT_INDEX::iterator it;
	for (it = hcUnits.begin(); it != hcUnits.end(); ++it) {
		HCUnit* unit = it->second;
		if (!unit->isBrigade() && !unit->isDivision())
			continue;
		if (unit->homeCPID().id == 0)
			continue;
		unit->setHomeCPID(unit->homeCPID());
	}
}

//////////////////////////////////////////////////////////////////////
// Read the current arena description

static bool
_readArenaDescription()
{
	_setArenaDescriptionHostRevision();

	try
	{
		DBQuery q(dbh, QRY_SELECT_ARENA_DESCRIPTION);

		while (q.FetchRow())
		{
			const char* name = q.Row().NextColumn();
			const char* value = q.Row().NextColumn();

			if ( name == nullptr || value == nullptr )
				continue;

			if ( *name == 0 || *value == 0 )
				continue;

			g_arenaDescription[std::string(name)] = std::string(value);
			dbugLog(_FLI, "arena config: %s=%s", name, value);
		}

		return true;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what());
		return false;
	}
}

static void
_connectGameDB()
{
	if ( dbh.connect() == false )
		LogAbort("unable to connect to game database -- aborting");

	DBSimpleQuery(dbh,
			"CREATE TABLE IF NOT EXISTS `wwii`.`client_terminations`"
			"("
			"  `id` int(11) unsigned NOT NULL auto_increment,"
			"  `sessionOTP` int(11) unsigned default '0',"
			"  `eventID` int(11) unsigned default '0',"
			"  `errorType` int(11) unsigned default '0',"
			"  `playerID` int(11) unsigned default '0',"
			"  `whereIP` char(16) default '',"
			"  `clientVersion` char(16) NOT NULL default '0.0.0.0',"
			"  `platform` int(11) unsigned default NULL,"
			"  `rxTime` timestamp NOT NULL,"
			"  `hostTime` timestamp NOT NULL,"
			"  `clientTime` timestamp NOT NULL,"
			"  `archiveCorrupt` tinyint(3) unsigned default '0',"
			"  `piExitLine` int(11) unsigned default '0',"
			"  `piExitFile` char(64) default '',"
			"  `piExitMessage` char(128) default '',"
			"  `exceptionAddr` int(11) unsigned zerofill default '00000000000',"
			"  `exceptionCode` int(11) unsigned zerofill default '00000000000',"
			"  `invalidArchive` char(64) default '0',"
			"  `invalidFileName` char(64) default '0',"
			"  `invalidCRC` int(11) unsigned default '0',"
			"  `invalidMD5` char(64) default '0',"
			"  `stackinfo` char(255) default '',"
			"  `exceptionText` char(255) default '',"
			"  `clientRevision` int(11) unsigned NOT NULL,"
			"  PRIMARY KEY  (`id`),"
			"  KEY (`id`, `clientVersion`),"
			"  KEY `big_cv_idx` (`clientVersion`,`errorType`,`archiveCorrupt`)"
			")"
	);

	dbugLog(_FLI,"connection established to WWIIOL_DB");
}

bool
mapInitDB()
{
	bool retval = true;

	_connectGameDB();

	if ( !_readActiveCountries() )
	{
		dbugLog(_FLA, "failed to read active countries");
		retval = false;
	}

	wwiiReadRanks();

#if !defined(DEV_BUILD) && !defined(FRES)
	mapPollChecksums(true);
#endif

	if ( !wwiiReadHCUnits() )
	{
		dbugLog(_FLA, "Unable to read HCUnits - aborting");
		retval = false;
	}

	_initializeBrigadeLocations();

	_readArenaDescription();

	return retval;
}

void
mapCloseDB()
{
	dbh.disconnect();
}

//////////////////////////////////////////////////////////////////////
// Load the data matching the specified playerID into the persona
// Does not modify any fields that aren't loaded

bool
Player::selectPlayer(const playerid_t forPlayerID)
{
	SetPlayerID(0);

	try
	{
		TeulTimeMS entryTime = teulClock64;

#undef TIME_QUERIES
#ifdef TIME_QUERIES
		TeulTimeMS startTime;

		teulSyncClock();
		startTime = teulClock64;
#endif

		DBQuery q(dbh, "CALL `auth`.`p_get_player_details`(%u,%u)", forPlayerID, Server::myServerInfo.stats.arenaID);

#ifdef TIME_QUERIES
		teulSyncClock();

		if ( teulClock64 - entryTime > 500 || teulClock64 - startTime > 200 )
			dbugLog(_FLW, "`auth`.`p_get_player_details` appears to be taking too long: apparent:%" PR64BitUT "ms, actual:%" PR64BitUT "ms"
							, teulClock64 - entryTime
							, teulClock64 - startTime);
#endif

		if ( q.HasRows() == false )
			throw std::invalid_argument("No such playerID");
		if ( q.Rows() > 1 )
			dbugLog(_FLW, "t:%d p:%u c:%s multiple rows returned", teulIndex(), forPlayerID, callsign());
		if ( !q.FetchRow() )
			throw std::logic_error("failed to fetch row from db result");

		AuthPlayerDetails details(q.Row());
		if ( details.playerID != forPlayerID )
			throw std::logic_error("get player info retrieved wrong player");
		if ( details.callsign[0] == 0 )
			throw std::logic_error("`auth`.`p_get_player_details` returned empty game name");

		SetPlayerID(details.playerID);			// Our playerID
		m_customerID = details.customerID;		// Billing system ID
		m_playerMask = details.permissions;	// Server permissions
		SetGroups(details.groups);				// Load account permissions
		_setCallsign(details.callsign, isPlaynet());

		// Make sure the callsign is valid
		if ( m_matchHandle == 0 )
			throw std::logic_error("player's game name produces null handle");
		if ( Chat::Name(m_chatHandle) != dispatch().m_callsign )
			throw std::logic_error("invalid callsign");

		SetSquad(details.squadID);
		SetSquadHandle(chatGetHandle(details.squadCallsign));
		SetDecal(details.decal);

		m_sideLockTime = (details.lockTime > teulTime() ? details.lockTime : 0);
		m_lastSide = (WWII_SIDE)(m_sideLockTime > 0 ? details.lockSide : 0);
		m_lastPoll = details.lastPoll;

		SetMeteredAccess(details.minuteBalance);

#ifdef TIME_QUERIES
		teulSyncClock();
#endif
		dbugLog(_FLN, "t:%d p:%u c:%s cust:%u found login info perms:%" PRPermMaskT " player:%" PRPermMaskT " mb:%u %s took %" PR64BitUT "ms"
						, teulIndex(), playerID(), callsign(), m_customerID
						, permissionMask(), m_playerMask
						, minuteBalance(), (meteredAccess() ? "metered" : "unmetered")
						, teulClock64 - entryTime);

		return true;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d p:%u c:%s %s: %s", teulIndex(), forPlayerID, callsign(), __FUNCTION__, e.what());
	}

	return false;
}

UINT32
Player::createPersonaRecord(WWII_COUNTRY countryID, BRANCH_ENUM branchID) const
{
	try
	{
		DBQuery q(dbh, QRY_INSERT_NEW_PERSONA_RECORD, playerID(), countryID, branchID);

		static const char* branches[] = { "NONE", "ARMY", "AIR", "NAVY" };
		dbugLog(_FLN, "t:%d p:%u c:%s created new %s %s persona id %d"
						, teulIndex(), playerID(), callsign(), _countryName[countryID], branches[branchID], q.LastInsertID());

		return q.LastInsertID();
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d p:%u c:%s unable to create persona: %s", teulIndex(), playerID(), callsign(), e.what());
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Advance all of this player's personas in a particular branch to
// the lower of either the top of the buddy rank system or the
// current persona

void
mapSaveRank(const Player* player, bool immediate)
{
	static char buffer[181];
	strprintf(buffer, "CALL `wwii`.`p_increase_rank_score`(%u,%u,%u,%u,%u,%u)"
							, (UINT32)player->playerID(), player->country(), player->branch()
							, (UINT32)player->personaID(), (UINT32)player->realRank(), (UINT32)player->points());
	dbugLog(_FLI, "t:%d p:%u c:%s %s", player->teulIndex(), player->playerID(), player->callsign(), buffer);

	if ( immediate )
	{
		DBSimpleQuery(dbh, buffer);
	}
	else
	{
		DBThread::Do(DBThread::Default, "%s", buffer);
	}
}

//////////////////////////////////////////////////////////////////////
// Change branch - also affects what my max rank can be

void
Player::SetBranch(BRANCH_ENUM newBranch)
{
	m_branch = newBranch;
	m_maxRank = m_permissions->m_limitRank[newBranch];
	dbugLog(_FLD, "t:%d p:%u c:%s new branch %u, maxRank now %u"
					, teulIndex(), playerID(), callsign()
					, newBranch, m_maxRank);
}

//////////////////////////////////////////////////////////////////////
// Retrieve persona data

PERSONA_LOAD_STATUS
Player::selectPersona(UINT32 selectedID, WWII_COUNTRY countryID, BRANCH_ENUM branchID)
{
	// Sanity cleanup of various things you may already have
	ExitBrigade(PERS_SELECTING_PERSONA);
	try
	{
		static const char QRY_SELECT_PERSONA_DATA[] = "CALL `wwii`.`p_get_persona_data`(%u, %u, %u)";

		DBQuery q(dbh, QRY_SELECT_PERSONA_DATA, playerID(), countryID, branchID);

		// If there was no data, there is probably no such persona
		if ( q.Rows() == 0 || !q.FetchRow() )
			return PLS_MISSING;

		auto& row = q.Row();

		selectedID = row.NextUInt();								// 0

		// We get a row for all of this personas branch-matching
		// personas. The country-matching one should be first,
		// if it isn't, then they don't have a persona.
		WWII_COUNTRY thisCountry = (WWII_COUNTRY)row.NextUInt();	// 1
		if ( thisCountry != countryID )
			return PLS_MISSING;

		RANK_NO myRankNo = (RANK_NO)row.NextUInt();					// 2
		UINT32 myRankPoints = row.NextUInt();						// 3

		// Determine if the persona is locked
		m_sideLockTime = row.NextUInt();							// 4
		m_lastSide = (WWII_SIDE)row.NextUInt();						// 5

		if ( m_sideLockTime <= teulTime() )
		{
			m_sideLockTime = 0;
			m_lastSide = WWII_NO_SIDE;
		}

		UINT32 lockedFor = isSideLocked(wwiiSide(countryID));
		if ( lockedFor > 0 )
		{
			dbugLog(_FLN, "t:%d p:%u select persona %u:%u/%u is side-locked to %u for %u", teulIndex(), playerID(), selectedID, countryID, branchID, m_lastSide, lockedFor);
			return PLS_LOCKED;
		}

		SetPersonaID(selectedID);
		SetCountry(countryID, wwiiSide(countryID));
		SetBranch(branchID);

		m_playerMask = row.NextUINT64();							// 6
		updateMask();

		SetDecal(row.NextUInt());									// 7

		SetSquad(row.NextUInt());									// 8
		SetSquadHandle(chatGetHandle(row.NextString()));			// 9

		UINT32 playedTime = row.NextUInt();							//10
		UINT32 newMinuteBalance = row.NextUInt();					//11

		SetRank(myRankNo);
		m_rankPointsEarned = myRankPoints;

		if ( useRank().permissions(ALL_SPAWN) )
			dbugLog(_FLD, "t:%d p:%u c:%s persona has ALL_SPAWN", teulIndex(), playerID(), callsign());
		if ( m_playerMask & ALL_SPAWN )
			dbugLog(_FLD, "t:%d p:%u c:%s player has ALL_SPAWN", teulIndex(), playerID(), callsign());

		// Now you need to select a brigade
		SetState(PERS_SELECTING_BRIGADE);

		updateMap();

		updateChat(true);

		// If the player is accessing a persona that might need
		// to be buddied up, check the other personas for higher
		// ranks within the same branch
		const RANK_NO globalShareCeiling = std::min(maxRank(), (RANK_NO)host_config.score.maxGlobalSharedRank);
		const RANK_NO sideShareCeiling = std::min(maxRank(), (RANK_NO)host_config.score.maxSideSharedRank);

		bool changed = false;

		while ( q.FetchRow() )
		{
			auto& altRow = q.Row();
			altRow.NextUInt();	// Skip personaID
			const WWII_COUNTRY altPersCountry = (WWII_COUNTRY) altRow.NextUInt();
			const RANK_NO altPersRankNo = (RANK_NO) altRow.NextUInt();
			const UINT32 altPersPoints = altRow.NextUInt();

			const bool sameSide = (wwiiSide(altPersCountry) == wwiiSide(thisCountry));
			const RANK_NO sharedRank = std::min(altPersRankNo, sameSide ? sideShareCeiling : globalShareCeiling);

			if ( myRankNo > sharedRank )
				continue;
			if ( myRankNo == sharedRank && (!sameSide || myRankPoints >= altPersPoints) )
				continue;

			if ( myRankNo < sharedRank )
				myRankNo = sharedRank;

			myRankPoints = altPersPoints;

			changed = true;
		}

		if ( changed )
		{
			log(_FLI, "applying rank sharing points to %s persona -> rank %u->%u pts %u->%u"
					, _countryName[country()]
					, realRank(), myRankNo
					, m_rankPointsEarned, myRankPoints
					);

			if ( myRankNo > realRank() )
				SetRank(myRankNo);
			m_rankPointsEarned = myRankPoints;

			mapSaveRank(this, false);
		}

		SetMeteredAccess(newMinuteBalance);

		return PLS_LOADED;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "t:%d p:%u select persona %u data failed: %s", teulIndex(), playerID(), selectedID, e.what());
		return PLS_ERROR;
	}
}

void
Player::loadChatMask()
{
	updateMask();
}

bool mapFillPersonaData(Player* const player)
{
	std::this_thread::yield();

	player->SetRank(0);

	// Refresh information when processing a player's mission scoring packet
	static const char QRY_SELECT_PERSONA_REFRESH[] = "CALL `auth`.`p_persona_refresh`(%u, %u)";

	try
	{
		TeulTimeMS entryTime = teulClock64;
#ifdef TIME_QUERIES
		TeulTimeMS startTime;
		teulSyncClock();
		startTime = teulClock64;
#endif

		DBQuery q(dbh, QRY_SELECT_PERSONA_REFRESH, player->personaID(), Server::myServerInfo.stats.arenaID);

#ifdef TIME_QUERIES
		if ( teulClock64 - entryTime > 500 || teulClock64 - startTime > 200 )
			dbugLog(_FLW, "QRY_SELECT_PERSONA_REFRESH taking too long: apparent %" PR64BitUT "ms, actual %" PR64BitUT "ms"
							, teulClock64 - entryTime
							, teulClock64 - startTime
							);
#endif

		if ( q.Rows() == 0 || !q.FetchRow() )
			throw std::logic_error("No rows for this player/persona");
		auto& row = q.Row();

		// Capture the rank they would have incase they
		// don't have priviliges to actually get that rank.
		const RANK_NO newRank = row.NextUInt();

		player->SetRank(newRank);
		player->m_rankPointsEarned = row.NextUInt();
		UINT64 newMask = row.NextUINT64();
		const char* const newGroups = row.NextString();
		const UINT32 newMinuteBalance = row.NextUInt();

		if ( newGroups == nullptr || *newGroups == 0 )
			newMask &= ~SUBSCRIPTION_ACTIVE;
		else
			newMask |= SUBSCRIPTION_ACTIVE;

		player->m_playerMask = newMask;
		player->SetGroups(newGroups);

		player->SetMeteredAccess(newMinuteBalance);

#ifdef TIME_QUERIES
		teulSyncClock();
#endif

		threadLog(_FLD, "t:%d p:%u c:%s filled persona data pers:%d rank:%d gotrank:%d points:%u rankperms:%" PRPermMaskT " tr:%u took %" PR64BitUT "ms"
						, player->teulIndex(), player->playerID(), player->callsign()
						, player->personaID(), newRank, player->realRank(), player->points(), player->useRank().permissions()
						, player->minuteBalance()
						, teulClock64 - entryTime
						);

		return true;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "t:%d p:%u c:%s per:%d %s: %s", player->teulIndex(), player->playerID(), player->callsign(), player->personaID(), __FUNCTION__, e.what());
		player->loadChatMask();
		return false;
	}
}

static void
_errException(const WWII_ERROR_INFO& info, const Player& player, const UINT32 platform)
{
	std::string stackinfo = "";

	for ( UINT32 i = 0; i < WWII_EXCEPTION_INFO::_STACK_LEN; ++i )
	{
		if ( info.data.exceptInfo.stackAddrs[i] != 0 )
		{
			char addr[20];
			sprintf(addr, "%x,", info.data.exceptInfo.stackAddrs[i]);
			stackinfo.append(addr);
		}
	}

	if ( stackinfo.empty() )
		stackinfo = "none";
	else if ( stackinfo.size() > 255 )
	{
		stackinfo.resize(255);
	}

	char whereIP[20];
	char clientVersion[20];
	teulGetAddress(info.hostIP, whereIP, 20);
	teulGetAddress(info.clientVersion, clientVersion, 20);

	player.log(_FLN, "Received ver %s exception info type %u stack %s", clientVersion, info.errorType, stackinfo.c_str());

	try
	{
		DBQuery q(dbh
					, "INSERT DELAYED INTO `wwii`.`client_terminations`"
						" (sessionOTP, eventID, errorType, playerID"
						", whereIP, clientVersion, platform"
						", clientTime, hostTime, rxTime, archiveCorrupt"
						", exceptionCode, exceptionAddr"
						", stackinfo, exceptionText"
						", clientRevision"
						")"
					 " VALUES"
						" (%u, %u, %u, %u"
						", '%s', '%s', %u"
						", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), NOW(), %u"
						", %u, %u"
						", '%s', '%s'"
						", %u"
						")"
					, player.OTP(), info.eventID, info.errorType, player.playerID()
					, whereIP, clientVersion, platform
					, info.clientTime, info.hostTime, /* NOW(), */ info.archiveWasCorrupt
					, info.data.exceptInfo.exceptionCode, info.data.exceptInfo.exceptionAddr
					, stackinfo.c_str(), info.data.exceptInfo.infoText
					, info.clientSourceCommit
		);
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLW, "Unable to save exception report: %s", e.what());
	}
}

static void
_errExit(const WWII_ERROR_INFO& info, const Player& player, const UINT32 platform)
{
	char whereIP[21];
	char clientVersion[21];

	teulGetAddress(info.hostIP, whereIP, sizeof(whereIP) - 1);
	teulGetAddress(info.clientVersion, clientVersion, sizeof(clientVersion) - 1);

	std::string src(info.data.piExitInfo.srcFilename.string);
	auto p = src.rfind("|");
	if ( p != src.npos )
	{
		src.erase(0, p);
	}

	std::string error = info.data.piExitInfo.errorMessage.string;
//	DB::handle_t* conn = threadsOpenDBConnection(false);
	DBThread::EscapeString(dbh, src);
	DBThread::EscapeString(dbh, error);
//	threadsCloseDBConnection(conn);

	player.log(_FLN, "Received PIEXIT ver %s exit info type %u at %s:%u: %s"
						, clientVersion, info.errorType
						, src.c_str()
						, info.data.piExitInfo.srcLine
						, error.c_str());

	try
	{
		DBQuery q(dbh
					, "INSERT DELAYED INTO `wwii`.`client_terminations`"
						" (sessionOTP, eventID, errorType, playerID"
						", whereIP, clientVersion, platform"
						", clientTime, hostTime, rxTime, archiveCorrupt"
						", piExitLine, piExitFile, piExitMessage) "
					 " VALUES"
						" (%u, %u, %u, %u"
						", '%s', '%s', %u"
						", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), NOW(), %u"
						", %u, '%s', '%s'"
						")"
						, player.OTP(), info.eventID, info.errorType, player.playerID()
						, whereIP, clientVersion, platform
						, info.clientTime, info.hostTime, /* NOW(), */ info.archiveWasCorrupt
						, info.data.piExitInfo.srcLine, src.c_str(), error.c_str()
		);
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLW, "Unable to save exception report: %s", e.what());
	}
}

void
mapSaveErrorInfo(Player& player, WWII_ERROR_INFO& info)
{
	char statement[4096+1];

	const UINT32 platform = player.dispatch().m_platform;

	switch (info.errorType)
	{
	case WW_ERR_PIEXIT:
		try
		{
			_errExit(info, player, platform);
		}
		catch ( std::exception& e )
		{
			dbugLog(_FLA, "Error writing bug report to database: %s", e.what());
		}
	return;

	case WW_ERR_EXCEPTION:
		try
		{
			_errException(info, player, platform);
		}
		catch ( std::exception& e )
		{
			dbugLog(_FLA, "Error writing bug report to database: %s", e.what());
		}
	return;

	case WW_ERR_VALIDITY_CHECK:
		player.log(_FLI, "Got validity check bug report");
		strprintf(statement,"INSERT DELAYED INTO `wwii`.`client_terminations` (sessionOTP, eventID, playerID"
						", errorType, whereIP, clientVersion, platform, clientTime, hostTime, rxTime,  archiveCorrupt"
						", invalidArchive, invalidFileName, invalidCRC, invalidMD5) "
						" values (%u,%u,%u"
						",%u,'%s','%s',%d, FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), now(), %u"
						",'%s', '%s', %u, '%s') "
						, player.OTP(), info.eventID, player.playerID()
						, info.errorType,teulGetAddress(info.hostIP), teulGetAddress(info.clientVersion),platform, info.clientTime, info.hostTime, info.archiveWasCorrupt
						, info.data.validityInfo.archive.string,  info.data.validityInfo.filename.string,  info.data.validityInfo.crc, info.data.validityInfo.md5);
	break;

	case WW_ERR_SYSTEM_INFO:
		player.log(_FLW, "Got system info bug report???");
	return;

//		strprintf(statement,"INSERT DELAYED INTO `wwii`.`client_terminations` (sessionOTP, eventID, playerID"
//						", errorType, whereIP, clientVersion, platform,  clientTime, hostTime, rxTime,  archiveCorrupt"
//						", sysNumPackets, sysPacketID, sysInfoText) "
//						" values (%u,%u,%u"
//						",%u,'%s','%s',%d, FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), now(), %u"
//						",%u,%u, '%s') "
//						, player.OTP(), info.eventID, player.playerID()
//						, info.errorType,teulGetAddress(info.hostIP), teulGetAddress(info.clientVersion),platform, info.clientTime, info.hostTime, info.archiveWasCorrupt
//						, info.data.systemInfo.numPackets,  info.data.systemInfo.packetID,  info.data.systemInfo.infoText);

	default:
		return;
	}

	DBSimpleQuery(dbh, statement);
	dbugLog(_FLI, "t:%d p:%u c:%s inserted bug report type %d for version %s", player.teulIndex(), player.playerID(), player.callsign(), info.errorType, teulGetAddress(info.clientVersion));

	return;
}

//////////////////////////////////////////////////////////////////////
// Test if the sides are imbalanced and delays are in effect

static inline bool
_sidesAreImbalanced()
{
	return ( host_config.isArenaOpen() && host_config.balance.personaDelay > 0 && g_swingMeter.inEffect() );
}

UINT32
Player::isSideLocked(WWII_SIDE forSide) const
{
	if ( m_lastSide < NEUTRAL)
	{
		if ( m_lastSide == 0 )
			return 0;
		if ( m_lastSide == forSide )
			return 0;
		if ( teulTime() > m_sideLockTime )
			return 0;
		if ( permissions(FREE_SIDE_SWITCH) )
			return 0;
		if ( _sidesAreImbalanced() && forSide == g_underdog )
			return 0;
	}

	return m_sideLockTime - teulTime();
}

static UINT32
_checkSideLock(Player* player)
{
	// Banned overrides everything
	if ( player->permissions(SUBSCRIPTION_ACTIVE) == false )
		return PersonaSummary::_NOT_SUBBED;

	if ( player->m_sideLockTime <= teulTime() )
	{
		player->m_sideLockTime = 0;
		player->m_lastSide = WWII_NO_SIDE;
	}

	// Banned overrides everything
	if ( player->m_lastSide >= NEUTRAL )
		return PersonaSummary::_BANNED;

	if ( !player->isPlaynet() && host_config.isArenaLocked(AL_ENTRY) )
		return PersonaSummary::_ARENALOCK;

	// FSS rules all
	if ( player->permissions(FREE_SIDE_SWITCH) )
	{
		player->m_sideLockTime = 0;
		player->m_lastSide = WWII_NO_SIDE;
		return PersonaSummary::_AVAILABLE;
	}

	// At this point we know they haven't picked a side

	// If the sides aren't balanced and you were last on the overpop side ...
	if ( _sidesAreImbalanced() && player->m_lastSide != g_underdog )
	{

		// If this is your first look at the list, lets delay you...
		if ( !player->m_hasBalanced )
		{
			// Allow you to pick the other side soonest
			player->m_lastSide = g_underdog;
			player->m_sideLockTime = teulTime() + host_config.balance.personaDelay;
			return PersonaSummary::_DELAYED;
		}

		// Otherwise we let you change sides if this is a return to the screen
		if ( player->m_hasSelectedPersonaOnce )
			player->m_sideLockTime = 0;
		return PersonaSummary::_AVAILABLE;
	}

	return PersonaSummary::_SIDELOCKED;
}

//////////////////////////////////////////////////////////////////////
// Handler that receives back the list of personas for a player so
// that we no-longer need to block on the query

class PersonaSummaryQuery : public DBThreadQuery
{
public:
	PersonaSummaryQuery(const Player* player)
		: DBThreadQuery(DBThread::Default, "PersonaSummaryQuery", DBThreadQuery::_CALLBACK
							, QRY_SELECT_PERSONA_LIST
								, player->playerID(), MAX_PLAYER_PERSONAS)
		, m_playerID(player->playerID())
		, m_teulIndex(player->teulIndex())
		, m_results()
		{
			m_results.reserve(MAX_COUNTRIES * MAX_BRANCHES);
		}

protected:
	// We take both playerID and teulIndex so we can
	// make sure we don't try sending a reply to a
	// player who's relogged
	playerid_t			m_playerID;
	SINT32				m_teulIndex;
	std::vector<PersonaSummary>	m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		PersonaSummary ps;

		while ( Query()->FetchRow() && m_results.size() < MAX_PLAYER_PERSONAS )
		{
			DBRow& row = Query()->Row();
			ps.m_countryID = static_cast<WWII_COUNTRY>(row.NextUInt());
			if ( !activeCountries[ps.m_countryID] )
				continue;

			ps.m_branchID = static_cast<BRANCH_ENUM>(row.NextUInt());
			ps.m_personaID = row.NextUInt();
			ps.m_rankID = static_cast<RANK_NO>(row.NextUInt());
			const UINT32 points = row.NextUInt();
			const UINT32 required = row.NextUInt();
			ps.m_rankPctg = ( required > 0 ? points * 100 / required : 0 );
			ps.m_lastUsed = row.NextUInt();
			ps.m_brigadeID = row.NextUInt();

			m_results.push_back(ps);
		}

		return true;
	}

protected:
	virtual void Success() override
	{
		Player* player = PlayerInterface::Lookup(m_playerID);
		if ( player == nullptr || player->teulIndex() != m_teulIndex || m_results.empty() )
			return;

		PersonaSummaries summary;
		summary._list(player, m_results);
		summary.send(player->teulIndex());
	}

	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_playerID);
		if ( player != nullptr )
			dbugLog(_FLA, "t:%d p:%u error listing personas", m_teulIndex, player->playerID());
	}
};

//////////////////////////////////////////////////////////////////////
// Request a persona list for a particular player

void
mapRequestPersonaSummaryList(const Player* player)
{
	dbugLog(_FLD, "t:%d p:%u c:%s queueing persona summary list query"
					, player->teulIndex(), player->playerID(), player->callsign());
	(new PersonaSummaryQuery(player))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Send a player a list of persona summaries from the query given

bool
PersonaSummaries::_list(Player* player, const std::vector<PersonaSummary>& personas)
{
	static UCHAR availability[MAX_COUNTRIES][MAX_BRANCHES];
	static TeulTimeMS nextRedo = 0ULL;
	if ( teulClock64 >= nextRedo )
	{
		memset(availability, PersonaSummary::_INACTIVE, sizeof(availability));
		for ( auto it = hcUnits.begin(); it != hcUnits.end(); ++it )
		{
			HCUnit& unit = *(it->second);
			if ( unit.isBrigade() == false || unit.isTrainingUnit() == true )
				continue;
			if ( unit.homeCPID().Key() == host_config.routDestination[unit.country()] )
				continue;
			availability[unit.country()][unit.branch()] = PersonaSummary::_AVAILABLE;
		}
		nextRedo = teulClock64 + 5000;
	}
	// Provide the callsign
	m_callsign = player->chatHandle();
	m_error = false;
	if ( player->chatPermissions() == nullptr )
		m_chatPermissions = nullChatPermissions;
	else
		m_chatPermissions = *player->chatPermissions();
	m_accountGoodUntil = player->accountGoodUntil();

	const UINT32 count = m_count = personas.size();
	const UINT32 now = teulTime();

	const UINT32 lockReason = _checkSideLock(player);
	player->m_hasBalanced = true;

	if ( player->m_sideLockTime > 0 )
		m_sidelocked = player->m_lastSide;

	for ( size_t i = 0; i < count; ++i )
	{
		PersonaSummary& ps = m_summaries[i];
		ps = personas[i];

		UINT32 lastUsed = ps.m_lastUsed;
		if ( lastUsed >= now )
			ps.m_lastUsed = 1;
		else if ( lastUsed > 0 && lastUsed < now )
			ps.m_lastUsed = now - lastUsed;
		else
			ps.m_lastUsed = 0;

		if ( lockReason == PersonaSummary::_ARENALOCK )
		{
			ps.m_lockedThru = 60;
			ps.m_availability = lockReason;
		}
		else
		{
			ps.m_lockedThru = player->isSideLocked(wwiiSide(ps.m_countryID));
			if ( ps.m_lockedThru == 0 )
			{
				ps.m_availability = availability[ps.m_countryID][ps.m_branchID];
			}
			else
			{
				betaLog(_FLI, "t:%d p:%u c:%s pers:[%d/%d:%d] locked %d"
								, player->teulIndex(), player->playerID(), player->callsign()
								, ps.m_countryID, ps.m_branchID, ps.m_personaID
								, ps.m_lockedThru);
				ps.m_availability = lockReason;
			}
		}
	}

	m_activeMission.populate(player->activeMission());

	dbugLog(_FLI, "t:%d p:%u c:%s read %u personas am:%u-%u/%u agu:%u(%u)"
					, player->teulIndex(), player->playerID(), player->callsign()
					, count
					, m_activeMission.m_missionID
					, m_activeMission.m_countryID
					, m_activeMission.m_branchID
					, m_accountGoodUntil, (m_accountGoodUntil > teulTime() ? m_accountGoodUntil - teulTime() : 0)
					);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Save the mission packet results

void
Player::saveMissionPacket()
{
	if ( (missionID() <= 0) | (sortieID() == 0) | (missionPacket().m_saved == true) )
		return;

	const auto& mp = missionPacket();
	const auto& vehicle = mp.vehicle();
	const auto& scoring = mp.scoring();
	dbugLog(_FLD, "p:%u m:%u v:%u saving mission results", playerID(), missionID(), sortieID());

	UINT32 rtbFacilityOID = 0;
	const FULL_OBJ_ID rtbFacility = vehicle.rtbFacility();
	if ( rtbFacility.id > 0 )
	{
		const FacilityStruct* const fac = strtGetFacility(rtbFacility);
		if ( fac != nullptr )
		{
			rtbFacilityOID = fac->uOID;
		}
		else
		{
			dbugLog(_FLW, "p:%u m:%u v:%u facility lookup failed: %d,%d.%u"
							, playerID(), missionID(), sortieID()
							, rtbFacility.cell.ox, rtbFacility.cell.oy, rtbFacility.id
							);
		}
	}

	DBThread::Do(DBThread::Default
		, "UPDATE wwii_sortie"
			" SET"
				" spawn_time = IF(spawn_time IS NULL,NOW(),spawn_time)"
				",return_time = NOW()"
				",rtb = %u"
				",rtb_facility_oid = %u"
				",successful = %u"
				",vehicles_hit = %u"
				",kills = %u"
				",captures = %u"
				",capture_assists = %u"
				",resupplies = %u"
				",score = %u"
				",alt_f4ed = %u"
				",guarding_kills = %u"
				",capper_kills = %u"
				",guarding_points = %u"
			" WHERE sortie_id = %u AND player_id = %u"
			, vehicle.rtbState()
			, rtbFacilityOID
			, mp.wasSuccessful
			, scoring.hits()
			, scoring.kills()
			, scoring.captures()
			, 0
			, scoring.resuppliesScored()
			, scoring.points()
			, 0
			, scoring.guardingKills()
			, scoring.cappingKills()
			, scoring.guardingPoints()
			, sortieID()
			, playerID()
	);

	missionPacket().m_saved = true;
}

//////////////////////////////////////////////////////////////////////
// Query the database for a players tutorial progress and when we get
// the result back, send the batch to the player.

class TrainingQuery : public DBThreadQuery
{
public:
	// Probably also need to use a teulPacket to send the actual data.
	// Which is nice, we could build the teulPacket in the thread.
	TrainingQuery(const playerid_t playerID, SINT32 teulIndex)
		: DBThreadQuery(DBThread::Default, "TrainingQuery", DBThreadQuery::_CALLBACK
							,	"SELECT group_name, progress"
								 " FROM `wwii`.`training_progress` p"
								" WHERE p.player_id = %u LIMIT 64", playerID)
		, m_playerID(playerID), m_teulIndex(teulIndex)
		{}
	virtual ~TrainingQuery() {}

protected:
	playerid_t	m_playerID;
	SINT32		m_teulIndex;
	Tutorial::GroupProgressBatch m_results;

	virtual bool Process(const size_t numRows) override
	{
		m_results.clear();

		if ( numRows <= 0 )
			return true;

		const size_t entries = std::min(numRows, (size_t)64);
		m_results.reserve(entries);

		for ( size_t i = 0; i < entries && Query()->FetchRow(); ++i )
		{
			m_results.resize(i + 1);
			m_results[i].m_groupHandle = chatGetHandle(Query()->Row().NextColumn());
			m_results[i].m_flags = Query()->Row().NextUINT64();
		}

		return true;
	}

	virtual void Success() override
	{
		// Make sure we still have the same player on the same index.
		Player* player = PlayerInterface::Lookup(m_teulIndex);
		if ( player == nullptr || m_playerID != player->playerID() || player->teulIndex() != m_teulIndex )
			return;

		player->log(_FLD, "has %u training progress entries", m_results.size());

		Map::SendTutProgressBatch(player->teulIndex(), m_results);
	}
};

void
mapRequestTutProgress(const playerid_t playerID, SINT32 teulIndex)
{
	(new TrainingQuery(playerID, teulIndex))->Queue();
}

void
mapLoadWeaponIndexes()
{
	weapLoadIndexes(dbh);
}

//////////////////////////////////////////////////////////////////////
// When a brigade/division changes CP we need to push updates to the
// players so that they can see the transitions. At the time of
// writing we only update the players who are at the map screen and
// the chat host updates the rest.

void
HCUnit::setHomeCPID(const FULL_OBJ_ID cpID)
{
	CPStruct* oldCP;
	oldCP = strtGetCP(m_homeCPID);
	if (oldCP != nullptr)
		oldCP->m_units.erase(unitID());

	CPStruct* cp = strtGetCP(cpID);
	if (cp)
	{
		m_homeCPID = cpID;
		cp->m_units[unitID()] = this;
	}
	else
		m_homeCPID = nullObjID;

	// Have we moved?
	if (cp != oldCP && nullptr != oldCP)
	{
		Brigade* brigade = mapFindBrigade(unitID());
		if (brigade != nullptr)
			brigade->checkOrigins();
		missionCheckOrigins(oldCP);
		if (brigade != nullptr)
			brigade->updateDestinations();
	}

	brigadeMovements[unitID()] = ++lastBrigadeMovement;

	//	dbugLog(_FLI, "XXX hcu:%u name:%s home:%s units:%u", unitID(), name(), (cp ? cp->name : "offmap"), (cp ? cp->m_units.size() : 0));
}

//////////////////////////////////////////////////////////////////////
// Query brigades that have had a database change we don't know about.

struct BrigadeDetailsUpdate
{
	BrigadeDetailsUpdate() {}

	UINT32			m_brigadeID;
	UINT32			m_lastModified;
	ChatHandle		m_owner;
	RANK_NO			m_ownerRank;
	UINT32			m_ownerID;
	char			m_title[MAX_HCUNIT_NAME_LENGTH + 1];
	char			m_motd[MAX_BRIGADE_NOTES_LENGTH + 1];
	char			m_motto[MAX_BRIGADE_MOTTO_LENGTH + 1];
	char			m_shortName[MAX_BRIGADE_SHORT_LENGTH + 1];
	FULL_OBJ_ID		m_homeCPID;
};

struct BrigadeDetailsQuery : public DBThreadQuery
{
	BrigadeDetailsQuery()
		: DBThreadQuery(DBThread::Default, "BrigadeDetailsUpdate", DBThreadQuery::_CALLBACK
			, "SELECT shu.unit_id"
				", UNIX_TIMESTAMP(shu.modified)"
				", shu.title"
				", shu.motd"
				", shu.motto"
				", shu.nick"
				", home.cp_ox"
				", home.cp_oy"
				", home.cp_id"
				", owner.playerid"
				", IF(owner.isplaynet,UPPER(owner.callsign),owner.callsign)"
				", ownerp.rankid"
			" FROM strat_hc_units shu"
				" LEFT JOIN strat_cp home ON (shu.home_cp_oid = home.cp_oid)"
				" LEFT OUTER JOIN wwii_player owner ON (shu.owner_id = owner.playerid)"
				" LEFT OUTER JOIN wwii_persona ownerp ON (shu.owner_id = ownerp.playerid"
					" and shu.country_id = ownerp.countryid"
					" and shu.branch_id = ownerp.branchid)"
			" WHERE (UNIX_TIMESTAMP(shu.modified) > %u)"
			" ORDER BY 2, 1"
			, s_lastUnitModifiedStamp
			)
	{
	}

private:
	std::vector<BrigadeDetailsUpdate> m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if (numRows <= 0)
			return true;

		m_results.resize(numRows);
		char ownerHandle[MAX_HANDLE_SIZE];
		for (size_t i = 0; i < numRows && Query()->FetchRow(); ++i)
		{
			auto& row = Query()->Row();
			auto& bdu = m_results[i];

			bdu.m_brigadeID = row.NextUInt();
			bdu.m_lastModified = row.NextUInt();
			row.NextString(bdu.m_title, sizeof(bdu.m_title));
			row.NextString(bdu.m_motd, sizeof(bdu.m_motd));
			row.NextString(bdu.m_motto, sizeof(bdu.m_motto));
			row.NextString(bdu.m_shortName, sizeof(bdu.m_shortName));
			bdu.m_homeCPID.cell.ox = (SINT16)row.NextSInt();
			bdu.m_homeCPID.cell.oy = (SINT16)row.NextSInt();
			bdu.m_homeCPID.id = (UINT16)row.NextUInt();
			bdu.m_ownerID = row.NextUInt();
			row.NextString(ownerHandle, sizeof(ownerHandle));
			bdu.m_ownerRank = (UINT8)row.NextUInt();

			bdu.m_owner = chatGetHandle(ownerHandle);
		}

		threadLog(_FLD, "received %u brigade detail updates", numRows);

		return true;
	}

public:
	virtual void Success() override
	{
		for (size_t i = 0; i < m_results.size(); ++i)
		{
			auto& bdu = m_results[i];
			if (bdu.m_lastModified >= s_lastUnitModifiedStamp)
				s_lastUnitModifiedStamp = bdu.m_lastModified;

			//	dbugLog(_FLD, "XXX hcUnit(%u) %s update", bdu.m_brigadeID, bdu.m_title);

			HCUnit* unit = HCUnit::Lookup(bdu.m_brigadeID);
			if (unit == nullptr)
			{
				dbugLog(_FLW, "Apparent new brigade(%u): %s", bdu.m_brigadeID, bdu.m_title);
				continue;
			}

			strsafecpy(unit->m_name, bdu.m_title);
			unit->m_owner = bdu.m_owner;
			unit->m_ownerRank = bdu.m_ownerRank;

			if (bdu.m_homeCPID != unit->homeCPID())
			{
				dbugLog(_FLD, "hcu:%u %s moved", bdu.m_brigadeID, bdu.m_title);
				unit->setHomeCPID(bdu.m_homeCPID);
			}

			Brigade* brigade = mapFindBrigade(bdu.m_brigadeID);
			if (brigade != nullptr)
			{
				strsafecpy(brigade->m_motd, bdu.m_motd);
				strsafecpy(brigade->m_shortName, bdu.m_shortName);
				strsafecpy(brigade->m_motto, bdu.m_motto);
				brigade->m_ownerID = bdu.m_ownerID;
			}
		}
		s_nextBrigadeDetailsQuery = teulClock64 + _STANDARD_BDU_INTERVAL_MS;
	}

	virtual void Failure() override
	{
		dbugLog(_FLW, "brigade details query failed");
		s_nextBrigadeDetailsQuery = teulClock64 + 5000;
	}
};

void mapRequestBrigadeDetailsUpdate()
{
	if (teulClock64 < s_nextBrigadeDetailsQuery)
		return;

	// don't do any queries until this fails or succeeds.
	s_nextBrigadeDetailsQuery = ~0ULL;

	(new BrigadeDetailsQuery())->Queue();
}
