#include <syscfg.h> // Path is project specific.
// Created: Sun Sep 7 06:24:00 CDT 2017
//
// Code for Deferred/Lazy access for creation and query of Personas
//
// Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Ian "frisbone" Frisbie

#include "dbug/dbug.h"
#include "wwii/wwiiConfig.h"
#include "map/mapDB.h"
#include "map/mapPlayer.h"
#include "map/mapPersonaSummary.h"
#include "mm/mmdbLazyFetchPersona.h"

//////////////////////////////////////////////////////////////////////
// This class allows a lazy selection of the persona when requested (not blocking main thread)
//////////////////////////////////////////////////////////////////////
bool SelectPersonaQuery::Process(const size_t numRows) {

	dbugLog(_FLD, "SelectPersonaQuery::Process(%zu)", numRows);

	BriefPersonaRecord *bpr = nullptr;
	try
	{

		if (numRows == 0 || !Query()->FetchRow() )
		{
			m_status = PLS_MISSING;
			return false;
		}

		do {
			bpr = new BriefPersonaRecord();

			auto& row = Query()->Row();

			bpr->selectedID = row.NextUInt();
			bpr->country = (WWII_COUNTRY)row.NextUInt();
			bpr->rankNo = (RANK_NO)row.NextUInt();
			bpr->rankPoints = row.NextUInt();
       			bpr->sideLockTime = row.NextUInt();
			bpr->lastSide = (WWII_SIDE)row.NextUInt();
			bpr->playerMask = row.NextUINT64();
			bpr->decal = row.NextUInt();
			bpr->squad = row.NextUInt();
			bpr->handle = chatGetHandle(row.NextString());
			bpr->playedTime = row.NextUInt();
			bpr->newMinuteBalance = row.NextUInt();

			personaQueryResult.push_back(bpr);

		} while ( Query()->FetchRow() );

		dbugLog(_FLD, "Found %u personas for player id %u", personaQueryResult.size(), m_pid);

		m_status =  PLS_LOADED;
		return true;

	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "p:%u select persona %u data failed: %s", m_pid, m_ps.m_personaID, e.what());

		// release any records previously allocated
		releasePersonaRecords();

		m_status = PLS_ERROR;
		return false;
	}
}

void SelectPersonaQuery::Success() {

	Player *m_player = PlayerInterface::Lookup(m_pid);
	if (m_player == nullptr) {
		dbugLog(_FLW, "Failed to find player with pid %u, already despawned?", m_pid);
		Failure();
		return;
	}

	UINT32 selectedID = m_ps.m_personaID;
	m_player->ExitBrigade(PERS_SELECTING_PERSONA);

	selectedID = personaQueryResult[0]->selectedID;

	dbugLog(_FLD, "Selected ID: %d", selectedID);

	// We get a row for all of this personas branch-matching
	// personas. The country-matching one should be first,
	// if it isn't, then they don't have a persona.
	WWII_COUNTRY thisCountry = personaQueryResult[0]->country;
	if ( thisCountry != m_ps.m_countryID )
	{
		m_status = PLS_MISSING;
		Failure();
		return;
	}

	dbugLog(_FLD, "Country = %d, requested = %d", thisCountry, m_ps.m_countryID);

	m_rankNo = personaQueryResult[0]->rankNo;
	m_rankPoints = personaQueryResult[0]->rankPoints;

	dbugLog(_FLD, "RankNo = %d, Points = %d", m_rankNo, m_rankPoints);

	// Determine if the persona is locked
       	m_player->m_sideLockTime = personaQueryResult[0]->sideLockTime;
	m_player->m_lastSide = personaQueryResult[0]->lastSide;

	dbugLog(_FLD, "SL time = %d, last side = %d", m_player->m_sideLockTime, m_player->m_lastSide);

	if ( m_player->m_sideLockTime <= teulTime() )
	{
		m_player->m_sideLockTime = 0;
		m_player->m_lastSide = WWII_NO_SIDE;
       	}

	UINT32 lockedFor = m_player->isSideLocked(wwiiSide(m_ps.m_countryID));
	if ( lockedFor > 0 )
	{
		dbugLog(_FLN, "t:%d p:%u select persona %u:%u/%u is side-locked to %u for %u", m_player->teulIndex(), 
			m_player->playerID(), selectedID, m_ps.m_countryID, m_ps.m_branchID, m_player->m_lastSide, lockedFor);
		m_status =  PLS_LOCKED;
		Failure();
		return;
	}

	m_player->SetPersonaID(selectedID);
	m_player->SetCountry(m_ps.m_countryID, wwiiSide(m_ps.m_countryID));
	m_player->SetBranch(m_ps.m_branchID);

	m_player->m_playerMask = personaQueryResult[0]->playerMask;
	m_player->updateMask();

	m_player->SetDecal(personaQueryResult[0]->decal);

	m_player->SetSquad(personaQueryResult[0]->squad);
	m_player->SetSquadHandle(personaQueryResult[0]->handle);

	m_newMinuteBalance = personaQueryResult[0]->newMinuteBalance;

	m_player->SetRank(m_rankNo);
	m_player->m_rankPointsEarned = m_rankPoints;

	if ( m_player->useRank().permissions(ALL_SPAWN) )
		dbugLog(_FLD, "t:%d p:%u c:%s persona has ALL_SPAWN", m_player->teulIndex(), m_player->playerID(), m_player->callsign());
	if ( m_player->m_playerMask & ALL_SPAWN )
		dbugLog(_FLD, "t:%d p:%u c:%s player has ALL_SPAWN", m_player->teulIndex(), m_player->playerID(), m_player->callsign());

	// Now you need to select a brigade
	m_player->SetState(PERS_SELECTING_BRIGADE);

	m_player->updateMap();

	m_player->updateChat(true);

	// If the player is accessing a persona that might need
	// to be buddied up, check the other personas for higher
	// ranks within the same branch
	const RANK_NO globalShareCeiling = std::min(m_player->maxRank(), (RANK_NO)host_config.score.maxGlobalSharedRank);
	const RANK_NO sideShareCeiling = std::min(m_player->maxRank(), (RANK_NO)host_config.score.maxSideSharedRank);

	dbugLog(_FLD, "Fetching additional Rows");

	for (size_t i=1; i < personaQueryResult.size(); i++)
	{
		const WWII_COUNTRY altPersCountry = personaQueryResult[i]->country;
		const RANK_NO altPersRankNo = personaQueryResult[i]->rankNo;
		const UINT32 altPersPoints = personaQueryResult[i]->rankPoints;
			
		const bool sameSide = (wwiiSide(altPersCountry) == wwiiSide(thisCountry));
		const RANK_NO sharedRank = std::min(altPersRankNo, sameSide ? sideShareCeiling : globalShareCeiling);

		if ( m_rankNo > sharedRank )
			continue;
		if ( m_rankNo == sharedRank && (!sameSide || m_rankPoints >= altPersPoints) )
			continue;

		if ( m_rankNo < sharedRank )
			m_rankNo = sharedRank;

		m_rankPoints = altPersPoints;

		dbugLog(_FLD, "rank info changed ...");

		m_changed = true;
	}

	m_status =  PLS_LOADED;
	
	releasePersonaRecords();

        // old

	if ( m_changed )
	{
		dbugLog(_FLI, "applying rank sharing points to %s persona -> rank %u->%u pts %u->%u"
			, _countryName[m_ps.m_countryID]
			, m_player->realRank(), m_rankNo
			, m_player->m_rankPointsEarned, m_rankPoints
			);

		if ( m_rankNo > m_player->realRank() )
			m_player->SetRank(m_rankNo);
		m_player->m_rankPointsEarned = m_rankPoints;

		mapSaveRank(m_player, true); // execute simple query immediately on this thread
	}

	dbugLog(_FLD, "Setting metered access to %d", m_newMinuteBalance);

	m_player->SetMeteredAccess(m_newMinuteBalance);

	if (m_callBack != nullptr) {
		m_callBack(m_callbackData, true);
	} else {
		switch (m_status) {
			case PLS_LOADED:
				dbugLog(_FLI, "t:%d p:%u c:%s loaded persona %d ctry:%d br:%d rk:%u er:%u prog:%u", m_player->teulIndex(), m_player->playerID(), 
					m_player->callsign(), m_ps.m_personaID, m_ps.m_countryID, m_ps.m_branchID, m_player->realRank(), m_player->effectiveRank(), m_player->progress());
				m_player->checksum(m_ps.m_checksum);
       				m_player->m_hasSelectedPersonaOnce = true;
				m_ps._sendResult(m_player->teulIndex(), PSS_SELECT, m_player->effectiveRank(), m_player->progress(), m_player->chatPermissions(), m_player->accountGoodUntil());
				break;
			case PLS_LOCKED:
				m_player->debug("Persona select refused", "side locked");
				dbugLog(_FLN, "t:%d p:%u c:%s persona %d locked", m_player->teulIndex(), m_player->playerID(), m_player->callsign(), m_ps.m_personaID);
				m_ps._sendResult(m_player->teulIndex(), PSS_PERSONA_LOCKED);
				break;

			case PLS_MISSING:
				m_player->debug("Persona select refused", "missing persona");
				dbugLog(_FLW, "t:%d p:%u c:%s no such persona %d:%d/%d", m_player->teulIndex(), m_player->playerID(), m_player->callsign(), m_ps.m_personaID, 
					m_ps.m_countryID, m_ps.m_branchID);
				m_ps._sendResult(m_player->teulIndex(), PSS_MISSING);
				break;

			default:
				m_player->debug("Persona select refused", "internal error");
				dbugLog(_FLW, "t:%d p:%u c:%s unknown error %d loading persona", m_player->teulIndex(), m_player->playerID(), m_player->callsign(), m_status);
				m_ps._sendResult(m_player->teulIndex(), PSS_ERROR);
				break;
		}
	}
}

void SelectPersonaQuery::Failure() {
	dbugLog(_FLD, "SelectPersonaQuery::Failure()");
 
	releasePersonaRecords();

	if (m_callBack != nullptr) {
		m_callBack(m_callbackData, false);
	}	
}

//////////////////////////////////////////////////////////////////////
// This class allows a lazy creation of the persona when requested (not blocking main thread)
//////////////////////////////////////////////////////////////////////
bool CreatePersonaQuery::Process(const size_t numRows) {
	// Insert didn't return an error
	m_ps.m_personaID = Query()->LastInsertID();
	return m_ps.m_personaID != 0;
}

void CreatePersonaQuery::Success() {
	// Insert didn't return an error
	static const char* branches[] = { "NONE", "ARMY", "AIR", "NAVY" };
	dbugLog(_FLN, "t:%d p:%u c:%s created new %s %s persona id %d"
		, m_player->teulIndex(), m_player->playerID(), m_player->callsign(), _countryName[m_ps.m_countryID], branches[m_ps.m_branchID], m_ps.m_personaID);
	m_ps._select(m_player);

	// NOTE: don't call callback because select will do that for us
}

void CreatePersonaQuery::Failure() {
	// Failed insert
	m_player->debug("Persona creation failed", "Could not create new persona");
	if (m_callBack != nullptr) {
		m_callBack(m_callbackData, false);
	} else 
		m_ps._sendResult(m_player->teulIndex(), PSS_ERROR);
}
