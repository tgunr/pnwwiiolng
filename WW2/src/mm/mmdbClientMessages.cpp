#include <syscfg.h> // Path is project specific.
// Date: Thu May 10 11:15:52 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "mm/mmdbClientMessages.h"
#include "miss/mission.h"
#include "map/mapBrigades.h"
#include "strt/strtData.h"
#include "strt/strtCPStruct.h"
#include "wwii/wwiiString.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

// By default, check once every 3 seconds.
static const SINT32 _STANDARD_BDU_INTERVAL_MS = 1 * 3 * 1000;

static const char* QRY_SELECT_BRIGADE_DETAILS =
	"SELECT shu.unit_id"
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
	;

static UINT32		s_lastUnitModifiedStamp = 0;
static TeulTimeMS	s_nextBrigadeDetailsQuery = 0;

void
mmdbInitializeBrigadeLocations()
{
	HCUNIT_INDEX::iterator it;
	for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
	{
		HCUnit* unit = it->second;
		if ( !unit->isBrigade() && !unit->isDivision() )
			continue;
		if ( unit->homeCPID().id == 0 )
			continue;
		unit->setHomeCPID(unit->homeCPID());
	}
}

//////////////////////////////////////////////////////////////////////
// When a brigade/division changes CP we need to push updates to the
// players so that they can see the transitions. At the time of
// writing we only update the players who are at the map screen and
// the chat host updates the rest.

void
HCUnit::setHomeCPID(const FULL_OBJ_ID& cpID)
{
	CPStruct* oldCP;
	oldCP = strtGetCP(m_homeCPID);
	if ( oldCP != NULL )
		oldCP->m_units.erase(unitID());

	CPStruct* cp = strtGetCP(cpID);
	if ( cp )
	{
		m_homeCPID = cpID;
		cp->m_units[unitID()] = this;
	}
	else
		m_homeCPID = nullObjID;

	// Have we moved?
	if ( cp != oldCP && NULL != oldCP )
	{
		Brigade* brigade = mapFindBrigade(unitID());
		if ( brigade != NULL )
			brigade->checkOrigins();
		missionCheckOrigins(oldCP);
		if ( brigade != NULL )
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
	char			m_title[MAX_HCUNIT_NAME_LENGTH+1];
	char			m_motd[MAX_BRIGADE_NOTES_LENGTH+1];
	char			m_motto[MAX_BRIGADE_MOTTO_LENGTH+1];
	char			m_shortName[MAX_BRIGADE_SHORT_LENGTH+1];
	FULL_OBJ_ID		m_homeCPID;
};

class BrigadeDetailsQuery : public DBThreadQuery
{
public:	BrigadeDetailsQuery()
			: DBThreadQuery(DBThread::Default, "BrigadeDetailsUpdate", DBThreadQuery::_CALLBACK
							, QRY_SELECT_BRIGADE_DETAILS
								, s_lastUnitModifiedStamp
							)
		{
		}

private:
	std::vector<BrigadeDetailsUpdate> m_results;

protected:
	virtual bool Process(const size_t numRows)
	{
		if ( numRows <= 0 )
			return true;

		m_results.resize(numRows);
		char ownerHandle[MAX_HANDLE_SIZE];
		for ( size_t i = 0; i < numRows && Query()->FetchRow(); ++i )
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
	virtual void Success()
	{
		for ( size_t i = 0; i < m_results.size(); ++i )
		{
			auto& bdu = m_results[i];
			if ( bdu.m_lastModified >= s_lastUnitModifiedStamp )
				s_lastUnitModifiedStamp = bdu.m_lastModified;

			//	dbugLog(_FLD, "XXX hcUnit(%u) %s update", bdu.m_brigadeID, bdu.m_title);

			HCUnit* unit = HCUnit::Lookup(bdu.m_brigadeID);
			if ( unit == NULL )
			{
				dbugLog(_FLW, "Apparent new brigade(%hu): %s", bdu.m_brigadeID, bdu.m_title);
				continue;
			}

			strncpy(unit->m_name, bdu.m_title, MAX_HCUNIT_NAME_LENGTH);
			unit->m_name[MAX_HCUNIT_NAME_LENGTH] = 0;
			unit->m_owner = bdu.m_owner;
			unit->m_ownerRank = bdu.m_ownerRank;

			if ( bdu.m_homeCPID != unit->homeCPID() )
			{
				dbugLog(_FLD, "hcu:%u %s moved", bdu.m_brigadeID, bdu.m_title);
				unit->setHomeCPID(bdu.m_homeCPID);
			}

			Brigade* brigade = mapFindBrigade(bdu.m_brigadeID);
			if ( brigade != NULL )
			{
				strlcpy(brigade->m_motd, bdu.m_motd, sizeof(brigade->m_motd));
				strlcpy(brigade->m_shortName, bdu.m_shortName, sizeof(brigade->m_shortName));
				strlcpy(brigade->m_motto, bdu.m_motto, sizeof(brigade->m_motto));
				brigade->m_ownerID = bdu.m_ownerID;
			}
		}
		s_nextBrigadeDetailsQuery = teulClock64 + _STANDARD_BDU_INTERVAL_MS;
	}

	virtual void Failure()
	{
		dbugLog(_FLW, "brigade details query failed");
		s_nextBrigadeDetailsQuery = teulClock64 + 5000;
	}
};

void
mmdbRequestBrigadeDetailsUpdate()
{
	if ( teulClock64 < s_nextBrigadeDetailsQuery )
		return;

	// don't do any queries until this fails or succeeds.
	s_nextBrigadeDetailsQuery = ~0ULL;

	(new BrigadeDetailsQuery())->Queue();
}

void
mmdbForceBrigadeDetailsUpdate()
{
	s_nextBrigadeDetailsQuery = 0ULL;
	mmdbRequestBrigadeDetailsUpdate();
}
