#include <syscfg.h> // Path is project specific.
// Date: Thu May 10 11:15:52 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "map/mapPlayer.h"
#include "mm/mmdbPack.h"
#include "mm/mmPack.h"
#include "mm/mmdbClientMessages.h"
#include "dbse/dbseClient.h"
#include "dbse/dbseConstants.h"
#include "teul/teulKit.h"
#include "miss/mission.h"
#include "mm/mmdbTypes.h"
#include "miss/missionExtMission.h"
#include "wwii/wwiiCallbacks.h"
#include "map/mapBrigades.h"
#include "map/mapDB.h"
#include "strt/strtData.h"
#include "strt/strtCPStruct.h"
#include "wwii/wwiiString.h"

static	TEUL_CALLBACK*	_XCHG_BRIGADE_DETAILS ;

static UINT32 _lastbduModified ;
static UINT32 _lastbduUnit ;
static TeulTimeMS _lastbduMark ;
static UINT32 _lastbduInterval ;

// By default, check once every minute
static const SINT32 _STANDARD_BDU_INTERVAL_MS = 1 * 3 * 1000 ;

// Whenever we receive data, park another request 200ms from now
static const SINT32 _RAPID_BDU_INTERVAL_MS = 20 ;

static SINT32
_sendBrigadeDetailsRequest(ACTION_OBJECT* const a)
{
	UINT32* data = (UINT32*)(a->localRawData) ;
	// We're passed two UINT32s
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &data[0], 0) ;
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &data[1], 0) ;

	return a->offset ;
}

void
mmdbInitializeBrigadeLocations()
{
	HCUNIT_INDEX::iterator it ;
	for ( it = hcUnits.begin() ; it != hcUnits.end() ; ++it )
	{
		HCUnit* unit = it->second ;
		if ( !unit->isBrigade() && !unit->isDivision() )
			continue ;
		if ( unit->homeCPID().id == 0 )
			continue ;
		unit->setHomeCPID(unit->homeCPID()) ;
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
	CPStruct* oldCP ;
	oldCP = strtGetCP(m_homeCPID) ;
	if ( oldCP != NULL )
		oldCP->m_units.erase(unitID()) ;

	CPStruct* cp = strtGetCP(cpID) ;
	if ( cp )
	{
		m_homeCPID = cpID ;
		cp->m_units[unitID()] = this ;
	}
	else
		m_homeCPID = nullObjID ;

	// Have we moved?
	if ( cp != oldCP && NULL != oldCP )
	{
		Brigade* brigade = mapFindBrigade(unitID()) ;
		if ( brigade != NULL )
			brigade->checkOrigins() ;
		missionCheckOrigins(oldCP) ;
		if ( brigade != NULL )
			brigade->updateDestinations() ;
	}

	brigadeMovements[unitID()] = ++lastBrigadeMovement ;

//	dbugLog(_FLI, "XXX hcu:%u name:%s home:%s units:%u", unitID(), name(), (cp ? cp->name : "offmap"), (cp ? cp->m_units.size() : 0)) ;
}

static SINT32
_recvBrigadeDetailsResponse(ACTION_OBJECT* const a)
{
	BrigadeDetailsUpdate bdu(a) ;

	if ( bdu.m_lastModified >= _lastbduModified )
	{
		_lastbduModified = bdu.m_lastModified ;
		_lastbduUnit = bdu.m_brigadeID ;
	}

//	dbugLog(_FLD, "XXX hcUnit(%u) %s update", bdu.m_brigadeID, bdu.m_title) ;

	HCUnit* unit = HCUnit::Lookup(bdu.m_brigadeID) ;
	if ( unit == NULL )
	{
		dbugLog(_FLW, "Apparent new brigade(%hu): %s", bdu.m_brigadeID, bdu.m_title) ;
		return a->offset ;
	}

	strncpy(unit->m_name, bdu.m_title, MAX_HCUNIT_NAME_LENGTH) ;
	unit->m_name[MAX_HCUNIT_NAME_LENGTH] = 0 ;
	unit->m_owner = bdu.m_owner ;
	unit->m_ownerRank = bdu.m_ownerRank ;

	if ( bdu.m_homeCPID != unit->homeCPID() )
	{
		dbugLog(_FLD, "hcu:%u %s moved", bdu.m_brigadeID, bdu.m_title) ;
		unit->setHomeCPID(bdu.m_homeCPID) ;
	}

	Brigade* brigade = mapFindBrigade(bdu.m_brigadeID) ;
	if ( brigade != NULL )
	{
		strlcpy(brigade->m_motd, bdu.m_motd, sizeof(brigade->m_motd)) ;
		strlcpy(brigade->m_shortName, bdu.m_shortName, sizeof(brigade->m_shortName)) ;
		strlcpy(brigade->m_motto, bdu.m_motto, sizeof(brigade->m_motto)) ;
		brigade->m_ownerID = bdu.m_ownerID ;
	}

	_lastbduMark = teulClock64 ;
	_lastbduInterval = _RAPID_BDU_INTERVAL_MS ;

	return a->offset ;
}

void
mmdbForceBrigadeDetailsUpdate()
{
	_lastbduMark = 0 ;
	mmdbRequestBrigadeDetailsUpdate() ;
}

void
mmdbRequestBrigadeDetailsUpdate()
{
	if ( _lastbduMark != 0 && teulClock64 - _lastbduMark < _lastbduInterval )
		return ;

	_lastbduMark = teulClock64 ;
	_lastbduInterval = _STANDARD_BDU_INTERVAL_MS ;

	UINT32 prior[2] ;
	prior[0] = _lastbduModified ;
	prior[1] = _lastbduUnit ;
	dbseSend(_XCHG_BRIGADE_DETAILS, prior) ;
}

SINT32
mmdbInstallClientCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(MM_DBSE_X)

		REGISTER_SENDRECV(MMDB_XCHG_BRIGADE_DETAILS_Y, _sendBrigadeDetailsRequest, _recvBrigadeDetailsResponse, _XCHG_BRIGADE_DETAILS) ;

	END_CALLBACK_REGISTRATION
}
