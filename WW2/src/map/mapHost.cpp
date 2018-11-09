#include <syscfg.h> // Path is project specific.
// Date: Mon Feb 28 14:23:02 CST 2005
//
// Functions that serve as the backbone of the map host
//
// Copyright (C) 2005 Playnet, Inc. All Rights Reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatHandle.h"
#include "chat/chatGroupPermissionTypes.h"
#include "inst/instHostInstance.h"
#include "map/mapBrigades.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapDB.h"
#include "map/mapMissionList.h"
#include "map/mapPersonaSummary.h"
#include "map/mapPlayer.h"
#include "miss/missionMission.h"
#include "miss/missionServerTypes.h"
#include "miss/missionTypes.h"
#include "mm/mmdbPersonaSelections.h"
#include "mm/mmdbLazyFetchPersona.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHCUnits.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtCPStruct.h"
#include "mm/mmQueries.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiConfig.h"

#include "chat/chatSupportRPCMessages.h"

const int GROUP_SIZE_LIMIT=100;

//////////////////////////////////////////////////////////////////////

SINT32
PersonaSelection::_sendResult(SINT32 teulIndex, SELECTION_STATUS result, RANK_NO rank, UINT32 rankPctg, const ChatPermissions* const m_permissions, UINT32 goodUntil)
{
	m_status = result;
	m_checksum = 0;
	m_rank = rank;
	m_rankPctg = rankPctg;
	m_chatPermissions = *m_permissions;
	m_accountGoodUntil = goodUntil;
	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Receive a persona summaries request

SINT32
PersonaSummaries::receive(SINT32 teulIndex)
{
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d %s: no persona associated with teulIndex", teulIndex, __FUNCTION__);
		return -EINVAL;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s persona summary request", teulIndex, player->playerID(), player->callsign());

	// Persona must be at persona selection, so clobber everything
	player->selectNullPersona();

	mapRequestPersonaSummaryList(player);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Create a new persona.

SINT32
PersonaSelection::_create(Player* const player)
{
	// A rank limit of zero means no access.
	if ( player->chatPermissions()->m_limitRank[m_branchID] == 0 )
	{
		player->debug("Persona creation failed", "subscription limit");
		return _sendResult(player->teulIndex(), PSS_SUBSCRIPTION_LIMIT, player->accountGoodUntil());
	}

	(new CreatePersonaQuery(*this, player))->Queue();

	// return value is ignored by the caller
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Delete a persona.

SINT32
PersonaSelection::_delete(Player* const player)
{
	dbugLog(_FLW, "t:%d p:%u c:%s DELETE not implemented", player->teulIndex(), player->playerID(), player->callsign());
	return _sendResult(player->teulIndex(), PSS_ERROR);
}

//////////////////////////////////////////////////////////////////////
// Select an existing player (or try to create it if absent)

SINT32
PersonaSelection::_select(Player* const player)
{
	// A rank limit of zero means no access.
	if ( player->chatPermissions()->m_limitRank[m_branchID] == 0 )
	{
		player->debug("Persona select failed", "subscription limit");
		return _sendResult(player->teulIndex(), PSS_SUBSCRIPTION_LIMIT);
	}

	if ( m_personaID == 0 )
	{
		if ( m_countryID > WWII_NO_COUNTRY && m_branchID > BRANCH_NOT_USED )
			return _create(player);

		player->debug("Persona select failed", "no personaID specified");
		dbugLog(_FLW, "t:%d p:%u c:%s attempt to select pid 0", player->teulIndex(), player->playerID(), player->callsign());
		return _sendResult(player->teulIndex(), PSS_ERROR);
	}

	// Ok - so the data looks valid
	(new SelectPersonaQuery(*this, player))->Queue();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Process a persona selection (sel/cre/del) request.

SINT32
PersonaSelection::receive(SINT32 teulIndex)
{
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d %s: no persona associated with teulIndex", teulIndex, __FUNCTION__);
		return -EINVAL;
	}

	if ( m_countryID <= WWII_NO_COUNTRY || m_countryID >= MAX_COUNTRIES )
	{
		player->debug("persona selection refused", "invalid country");
		dbugLog(_FLW, "t:%d p:%u c:%s invalid country [%d]", player->teulIndex(), player->playerID(), player->callsign(), m_countryID);
		return _sendResult(player->teulIndex(), PSS_ERROR);
	}
	if ( !activeCountries[m_countryID] )
	{
		player->debug("Inactive Country", _countryName[m_countryID]);
		return _sendResult(player->teulIndex(), PSS_ERROR);
	}
	if ( m_branchID < BRANCH_LAND || m_branchID > BRANCH_SEA )
	{
		player->debug("persona selection refused", "invalid branch");
		dbugLog(_FLW, "t:%d p:%u c:%s invalid branch [%d]", player->teulIndex(), player->playerID(), player->callsign(), m_branchID);
		return _sendResult(player->teulIndex(), PSS_ERROR);
	}

	switch ( m_status )
	{
		case PSS_NEW:
			dbugLog(_FLI, "t:%d p:%u c:%s persona create request: st:%d, pid:%d, c:%d, b:%d"
							, teulIndex, player->playerID(), player->callsign()
							, m_status, m_personaID, m_countryID, m_branchID
							);
		return _create(player);

		case PSS_SELECT:
			dbugLog(_FLI, "t:%d p:%u c:%s persona selection request: st:%d, pid:%d, c:%d, b:%d"
							, teulIndex, player->playerID(), player->callsign()
							, m_status, m_personaID, m_countryID, m_branchID
							);
		return _select(player);

		case PSS_DELETE:
			dbugLog(_FLI, "t:%d p:%u c:%s persona delete request: st:%d, pid:%d, c:%d, b:%d"
							, teulIndex, player->playerID(), player->callsign()
							, m_status, m_personaID, m_countryID, m_branchID
							);
		return _delete(player);

		default:
			dbugLog(_FLW, "t:%d p:%u c:%s invalid request type: %d", teulIndex, player->playerID(), player->callsign(), m_status);
		return _sendResult(teulIndex, PSS_ERROR);
	}
}

//////////////////////////////////////////////////////////////////////

int
BrigadeSummaries::populate(WWII_COUNTRY country, BRANCH_ENUM branch, Player* const player, int groupOffset)
{

	m_error = false;
	m_count = 0;

	if ( g_brigades[country][branch].empty() ) {
		dbugLog(_FLN, "No brigades for this country/branch");
		return m_count;
	}

	// Count up the players in all brigades for this country
	BRIGADES::iterator it;
	UINT32 maxPlayers = 1;
	for ( UINT32 i = 1; i < MAX_BRANCHES; ++i )
	{
		BRIGADES& brigades = g_brigades[country][i];
		for ( it = brigades.begin(); it != brigades.end(); ++it )
		{
			UINT32 players = (*it)->players();
			if ( players > maxPlayers )
				maxPlayers = players;
		}
	}

	int offset = 0;
	BRIGADES& brigades = g_brigades[country][branch];
	for ( it = brigades.begin(); (it != brigades.end()) && (m_count < GROUP_SIZE_LIMIT); ++it, offset++ )
	{
		if (offset < groupOffset) 
			continue;

		Brigade& brigade = *(*it);

		BrigadeSummary& summary = m_summaries[m_count];

		summary = brigade;
		if ( brigade.players() > 1 )
		{
			// Produce a percentage-like number 1-MAX_POPULATION
			summary.m_players = brigade.players() * MAX_POPULATION_RANK / maxPlayers;
			if ( summary.m_players == 0 )
				summary.m_players = 1;
		}
		summary.m_canPostHere = player->ableToPostIn(brigade);

		m_count++;
	}

	return m_count;
}

SINT32
BrigadeSummaries::receive(SINT32 teulIndex)
{

	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d %s: no persona associated with teulIndex", teulIndex, __FUNCTION__);
		return -EINVAL;
	}

	try
	{
		m_group = 0;
		int offset = 0;
		do {
			// -- Check that they have a persona selected
			if ( player->personaID() == 0 )
				throw std::logic_error("brigade request with no persona selected");
	
			// -- And check that we have a valid country/side from the persona
			WWII_COUNTRY country = player->country();
			if ( country == WWII_NO_COUNTRY || country >= MAX_COUNTRIES )
				throw std::logic_error("persona selected with invalid country set");
			if ( !activeCountries[country] )
				throw std::logic_error("persona selected with inactive country");
	
			BRANCH_ENUM branch = player->branch();
			if ( branch == BRANCH_NOT_USED || branch >= MAX_BRANCHES )
				throw std::logic_error("persona selected with invalid branch set");
	
			// Populate the entry
			int groupsAdded = populate(country, branch, player, offset);
			offset += groupsAdded;	
			
			m_lastGroup = (groupsAdded < GROUP_SIZE_LIMIT);

			send(teulIndex);
			m_group++;
		} while (!m_lastGroup);
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Error sending summaries t:%d p:%u c:%s %s", teulIndex, player->playerID(), player->callsign(), e.what());

		m_error = true;
		m_count = 0;

		send(teulIndex);
	}

	return 0;
}

bool
BrigadeSelection::_autoSelectCheck(Player* player, Brigade* brigade)
{
	bool retVal = false;
	// if there is no brigade selected or the selected brigade belongs to the wrong side
	// the server should choose
	if ( brigade == nullptr || (brigade->side() != player->side()))
	{
		dbugLog(_FLN, "Server automatically selecting brigade for player %d", player->playerID());

		// Select the training brigade for the country/branch we started
		WWII_COUNTRY country = (brigade != nullptr)?brigade->m_countryID:player->country();
		BRANCH_ENUM branch = (brigade != nullptr)?brigade->m_branchID:player->branch();
		brigade = (Brigade *)HCUnit::GetTrainingUnit(country, branch);
		if (brigade == nullptr)
		{
			dbugLog(_FLW, "No training unit found for country %d, branch %d", country, branch);
			m_detail.m_unitID = 0;
		}
		else
		{
			dbugLog(_FLN, "Unit ID %d selected for player %d", brigade->m_unitID, player->playerID());
		}
		m_detail = *brigade;
		mmdbSavePersonaSelection(player);
		retVal = true;
	}
	return retVal;
}

SINT32
BrigadeSelection::_query(Player* const player, const Brigade* const brigade)
{
	///TODO: Determine if they have permission to see this brigade
	m_detail = *brigade;

	// in case the brigade is no longer valid ...
	_autoSelectCheck(player, (Brigade *)&m_detail);

	return _sendResult(player->teulIndex(), BRIG_DETAIL);
}

SINT32
BrigadeSelection::_select(Player* player, Brigade* brigade)
{
	if (_autoSelectCheck(player, brigade))
		return _sendResult(player->teulIndex(), BRIG_SELECT);

	if ( !player->SelectBrigade(brigade) )
		return _sendResult(player->teulIndex(), BRIG_RESTRICTED);

	m_detail = *brigade;

	mmdbSavePersonaSelection(player);

	return _sendResult(player->teulIndex(), BRIG_SELECT);
}

SINT32
BrigadeSelection::receive(SINT32 teulIndex)
{
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d %s: no persona associated with teulIndex", teulIndex, __FUNCTION__);
		return _sendResult(teulIndex, BRIG_ERROR);
	}

	if ( m_detail.brigadeID() == 0 && m_status != BRIG_SELECT )
	{
		chatSendLocalizedMessage(player->playerID(), LSTR_CAUTION, LSTR_BDESEL_REFUSED_ID0);
		dbugLog(_FLW, "t:%d p:%u c:%s brigade select request with brigade id 0"
						, teulIndex, player->playerID(), player->callsign());
		return _sendResult(teulIndex, BRIG_MISSING);
	}

	if ( player->personaID() == 0 )
	{
		chatSendLocalizedMessage(player->playerID(), LSTR_CAUTION, LSTR_BDESEL_REFUSED_PERSONA);
		dbugLog(_FLW, "t:%d p:%u c:%s brigade select request with no persona selected", teulIndex, player->playerID(), player->callsign());
		return _sendResult(teulIndex, BRIG_ERROR);
	}

	Brigade* const brigade = mapFindBrigade(m_detail.brigadeID());
	if ( brigade == nullptr && m_status != BRIG_SELECT)
	{
		chatSendLocalizedMessage(player->playerID(), LSTR_NONE, LSTR_BDESEL_REFUSED_BDE);
		dbugLog(_FLW, "t:%d p:%u c:%s brigade select request for non-existant brigade %d"
						, teulIndex, player->playerID(), player->callsign(), m_detail.brigadeID());
		return _sendResult(teulIndex, BRIG_MISSING);
	}

	switch (m_status)
	{
		case BRIG_DETAIL:
			return _query(player, brigade);


		case BRIG_SELECT:
			return _select(player, brigade);


		default:
			dbugLog(_FLW, "t:%d p:%u c:%s invalid selection.m_status: %d"
							, teulIndex, player->playerID(), player->callsign(), m_status);
			return _sendResult(teulIndex, BRIG_ERROR);
	}
}

//////////////////////////////////////////////////////////////////////
// Populate a MissionSynopsis from a Mission

void
MissionSynopsis::populate(const Mission* const mission, bool isCurrent, bool isLeader, UINT32 highestPop)
{
	m_missionID = mission->missionID();
	m_countryID = mission->m_country;
	m_branchID  = mission->m_brigade->branch();
	m_brigadeID = mission->m_brigadeID;

	m_type = mission->missionType();
	m_subType = mission->subType();
	m_origin = mission->summary().origin;
	m_target = mission->summary().target;
	m_hasMobileSpawn = mission->isMobileSpawnAvailable();
	m_poster = mission->posterHandle();
	m_isCurrent = isCurrent;
	m_amLeader = isLeader;
	UINT32 population = 0;
	if ( mission->numTakers() > 1 )
		population = 1 + (14 * mission->numTakers() / highestPop);
	m_population = (population < 15) ? (UCHAR)population : 15;
	m_isOpen = (mission->originFac()->side == mission->side() && mission->originFac()->open);
	m_canMobileSpawn = mission->isMobileSpawnAllowed();
}

//////////////////////////////////////////////////////////////////////
// Receive a request for a mission list

SINT32
MissionList::receive(SINT32 teulIndex) {
	Player* const player = PlayerInterface::Lookup(teulIndex);

	m_count = 0;
	m_listStart = m_listEnd = true;
	m_canPost = false;

	if ( player == nullptr ) {
		dbugLog(_FLA, "t:%d %s: no persona associated with teulIndex", teulIndex, __FUNCTION__);
		return _sendResult(teulIndex, MLR_ERROR);
	}

	if ( player->personaID() == 0 ) {
		dbugLog(_FLW, "t:%d p:%d c:%s mission list without persona selected", player->teulIndex(), player->playerID(), player->callsign());
		return _sendResult(teulIndex, MLR_NO_PERSONA);
	}

	Brigade* const brigade = player->brigade();
	if ( m_status != MLR_NO_BRIGADE && brigade == nullptr ) {
		dbugLog(_FLW, "t:%d p:%d c:%s mission list without brigade selected", player->teulIndex(), player->playerID(), player->callsign());
		return _sendResult(teulIndex, MLR_NO_BRIGADE);
	}

	//------
	// TODO: Cleanup / refactor.

	if ( m_status != MLR_NO_BRIGADE ) {
		const Mission* const activeMission = player->activeMission();
		if ( player->ableToPost() && activeMission == nullptr )
			m_canPost = true;
		m_activeMission.populate(activeMission);

		if ( brigade->actualMissions() == 0 ) {
			player->message(LSTR_NOTE, LSTR_NO_ACTIVE_MISSIONS);
			dbugLog(_FLD, "t:%d p:%d c:%s no missions for brigade", player->teulIndex(), player->playerID(), player->callsign());
			return _sendResult(teulIndex, MLR_OK);
		}

		dbugLog(_FLD, "t:%d p:%u c:%s sending list of %u/%u missions for brigade %u activeMission %u"
		      , player->teulIndex(), player->playerID(), player->callsign(), brigade->missions(), brigade->actualMissions(), brigade->brigadeID(), m_activeMission.m_missionID);

		// Until we exit the for loop, we don't know if we've reached the list end
		m_listEnd = false;

		BrigadeMissions::iterator it;
		static const UINT32 MIN_MISSION_POPULATION_SCALE = 8;
		UINT32 highestCount = MIN_MISSION_POPULATION_SCALE + player->hostInstance()->count(brigade->side()) / 60;
		for ( it = brigade->m_missionList.begin() ; it != brigade->m_missionList.end() ; ++it ) {
			const Mission* const mission = (*it);

			// Ignore squad missions for other squads
			if ( mission->numTakers() > highestCount && mission->accessible(player) )
				highestCount = mission->numTakers();
		}

		UINT32 missionCount = 0;
		for ( it = brigade->m_missionList.begin() ; it != brigade->m_missionList.end() ; ++it ) {
			const Mission* const mission = (*it);

			// Ignore squad missions for other squads
			if ( !mission->accessible(player) )
				continue;

			if ( m_count == MAX_MISSIONS_PER_LIST ) {
				_sendResult(teulIndex, MLR_OK);
				m_listStart = false;
				m_count = 0;
			}

			bool isCurrent = (player->mission() == mission);
			bool isLeader = (isCurrent && player->isMissionLeader());
			m_missions[m_count].populate(mission, isCurrent, isLeader, highestCount);

			++m_count;
			++missionCount;
		}
		// Send the end of the list
		m_listEnd = true;

		if ( missionCount == 0 ) {
			player->message(LSTR_NOTE, LSTR_NO_ACTIVE_MISSIONS);
			dbugLog(_FLD, "t:%d p:%d c:%s no missions for brigade", player->teulIndex(), player->playerID(), player->callsign());
		}

		_sendResult(teulIndex, MLR_OK);
	} else {
		// Missions ranked by attractiveness.
		// TODO: This should probably be at least partially cached.
		std::multimap<int, const Mission*> missions;
		UINT32 highestCount = 0;

		auto add_missions =
		[&] (WWII_COUNTRY country, BRANCH_ENUM branch) {
			for ( BRIGADES::const_iterator brig = g_brigades[country][branch].begin();
			      brig != g_brigades[(size_t)country][(size_t)branch].end() ; ++brig ) {
				for ( BrigadeMissions::const_iterator miss = (*brig)->m_missionList.begin();
				      miss != (*brig)->m_missionList.end() ; ++miss ) {
					const Mission* mission = *miss;

					// If we can't spawn, don't bother.
					if ( ! mission->accessible(player) && mission->accessibility(player) != MC_BRIGADE )
						continue;
					if ( mission->missionType() == RECON || mission->missionType() == RESUPPLY )
						continue;

					float value = static_cast<float>(mission->numTakers()) ;
					// TODO: Move to script to be configured by producers.
					{
						const CPStruct* target = mission->targetCP();

						// NOTE: May make FB attacks work wrong? They're probably poor
						// choices for new players anyway...
						if ( mission->side() != target->ControllingSide() ) {
							// Attacking mission.
							if ( mission->isMobileSpawnAvailable() )
								value *= 2.0f;
						} else {
							value *= 1.25f; // Slight preference for defense.

							if ( target->isContested() )
								value *= 2.5f;
						}

						if ( ! target->hasHeavyInfantryEWS() )
							value *= 0.25f;

						// TODO: Big boost for missions with squadmates.
					}

					missions.insert(std::make_pair((int)value, mission)) ;
					if ( mission->numTakers() > highestCount )
						highestCount = mission->numTakers();
				}
			}
		} ;

		for ( int country = 0 ; country < MAX_COUNTRIES ; ++country ) {
			if ( wwiiSide(country) != player->side() )
				continue;
			add_missions((WWII_COUNTRY)country, player->branch());
		}

		//------

		const size_t max_quick_missions = 16;

		UINT32 missionCount = 0;
		for ( auto it = missions.rbegin() ; it != missions.rend() ; ++it ) {
			if ( missionCount >= max_quick_missions )
				break;

			const Mission* const mission = it->second;

			if ( m_count == MAX_MISSIONS_PER_LIST ) {
				_sendResult(teulIndex, MLR_OK);
				m_listStart = false;
				m_count = 0;
			}

			bool isCurrent = (player->mission() == mission);
			bool isLeader = (isCurrent && player->isMissionLeader());
			m_missions[m_count].populate(mission, isCurrent, isLeader, highestCount);

			++m_count;
			++missionCount;
		}
		// Send the end of the list
		m_listEnd = true;

		if ( missionCount == 0 ) {
			player->message(LSTR_NOTE, LSTR_NO_ACTIVE_MISSIONS);
			dbugLog(_FLD, "t:%d p:%d c:%s no missions found.", player->teulIndex(), player->playerID(), player->callsign());
		}

		_sendResult(teulIndex, MLR_OK);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Populate an active mission object from a mission

void
ActiveMission::populate(const Mission* const mission)
{
	if ( mission == nullptr || mission->missionID() == 0 )
	{
		m_missionID = 0;
		return;
	}

	m_missionID = mission->missionID();
	m_countryID = mission->country();
	m_branchID = mission->branch();
	m_brigadeID = mission->brigade()->brigadeID();
}

