#include <syscfg.h> // Path is project specific.
// Created: Fri Jan 15 16:26:00 CST 2010
//
// Implement the contribution class and queue
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatHandle.h"
#include "chat/chatName.h"
#include "strt/strtCaptureContribution.h"
#include "strt/strtCP.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtHostMessages.h"
#include "wwii/wwiiHCUnits.h"

#include <set>
#include <queue>

using ACTIVE_FACILITIES = std::set<FacilityStruct*>;
using CONTRIBUTIONS = std::queue<Strat::CaptureContribution>;

static ACTIVE_FACILITIES	activeFacilities;
static CONTRIBUTIONS		contributions;

//	------------------------------	Declarations

Strat::CaptureContribution::CaptureContribution(Strat::CaptureContributionEvent::Status withStatus
					, FacilityStruct& forFacility, const ChatHandle forHandle, UINT32 forPlayerID, UINT32 forPlayerCountry, UINT32 forSortieID, UINT32 forMissionID, const HCUnit& forUnit)
	: CaptureContributionEvent(withStatus, forFacility.uOID, forHandle, forPlayerID, forPlayerCountry, forSortieID, forMissionID, forUnit.unitID())
	, m_facility(&forFacility), m_unit(&forUnit)
{
}

//////////////////////////////////////////////////////////////////////
// Alter the current capturable status of a particular facility.

void
FacilityStruct::_setCaptureState(CaptureStage toStage, UINT16 toCounter)
{
	if ( toStage == m_captureStage && toCounter == m_captureCounter )
		return;

	switch ( toStage )
	{
	case _CAP_DISABLED:
		assert( toCounter == 0 );
		dbugLog(_FLI, "fac:%s:cap setting disabled", this->name);
		m_captureProgress = 0;
		m_captureRate = 0;
	break;
	case _CAP_PENDING:
		assert( toCounter > 0 );
		dbugLog(_FLI, "fac:%s:cap setting pending in %us", this->name, toCounter);
		m_captureProgress = 0;
		m_captureRate = 0;
	break;
	case _CAP_ENABLED:
		assert( toCounter < 100 );
		dbugLog(_FLI, "fac:%s:cap setting enabled with %u%% initial progress", this->name, toCounter);
		m_captureProgress = (FLOAT32)toCounter;
		m_captureRate = 0;
	break;
	default:
		dbugLog(_FLA, "unknown capture state");
	break;
	}

	UINT32 oldStage = m_captureStage;

	m_captureStage = toStage;
	m_captureCounter = toCounter;
	m_captureStateChanged = teulTime();

	strtSendCaptureState(this, this->side, (oldStage == _CAP_ENABLED ? 100 : 0), false);

	strtQueueFacilityStatus(this);

	// End of processing for disabled captures.
	if ( _CAP_DISABLED == m_captureStage )
	{
		activeFacilities.erase(this);
	}
	else
	{
		// Make sure we're in the active captures list.
		activeFacilities.insert(this);

		// Captured depots open once
		if ( this->type == FACILITY_DEPOT && !this->isOpen() )
			strtOpenBase(*this);
	}

}

//////////////////////////////////////////////////////////////////////
// Update the capture progress for this facility.
// Claimant is the individual who is "winning" the capture war.

void
FacilityStruct::setCaptureProgress(FLOAT32 newValue, const Strat::CaptureContribution& claimant)
{
	if ( false == this->isCapturable() )
	{
		dbugLog(_FLW, "f:%s:cap not capturable", name);
		return this->setCaptureDisabled();
	}

	if ( this->isCaptureEnabled() == false )
	{
		dbugLog(_FLW, "f:%s:cap not enabled", name);
		return;
	}

	if ( newValue < 0.0 )
		newValue = 0.0;
	else if ( newValue > 100.0 )
		newValue = 100.0;

	const WWII_SIDE forSide = wwiiSide(claimant.playerCountry());
	const FLOAT32 delta = newValue - this->m_captureProgress;

	if ( delta != 0 )
	{
		// It went up or down.
		this->m_captureCounter = (UINT16)newValue;
		this->m_captureProgress = newValue;
		this->m_captureRate = delta;

		strtQueueFacilityStatus(this);

		if ( newValue <= 0.0 || newValue >= 100.0 )
		{
			dbugLog(_FLD, "f:%s:cap capping", name);
			strtCaptureAndLog(this
								, claimant.handle()
								, claimant.playerID()
								, claimant.playerCountry()
								, claimant.missionID()
								, &claimant.unit()
								, teulTime() - this->m_captureStateChanged
								);

			if ( this->m_captureBuilding )
				strtSendCaptureScoreNotice(this->m_captureBuilding->fullObjID, forSide);
		}
		else
		{
			SINT32 rate = 1;
			FLOAT32 absDelta = std::abs(delta);
			if ( absDelta > 5 )
				rate = 3;
			else if ( absDelta >= 0.5 )
				rate = 2;
			strtSendCaptureState(this, forSide, rate, false);
		}
	}
	else
	{
		dbugLog(_FLD, "f:%s:cap old value [%f]", name, newValue);
		strtSendCaptureState(this, forSide, 0, false);
	}
}

//////////////////////////////////////////////////////////////////////
// Add a capture contribution to a facility

void
Strat::CaptureContribution::Queue(Status withStatus, FacilityStruct& forFacility, const ChatHandle forHandle, UINT32 forPlayerID, UINT32 forPlayerCountry, UINT32 forSortieID, UINT32 forMissionID, const HCUnit& forUnit)
{
	if ( forFacility.isCaptureEnabled() == false )
	{
		dbugLog(_FLD, "p:%u c:%s fac:%s:cap ignoring contribution for uncappable facility"
						, forPlayerID, Chat::Name(forHandle).c_str(), forFacility.name);
		return;
	}

	Strat::CaptureContribution contrib(withStatus, forFacility, forHandle, forPlayerID, forPlayerCountry, forSortieID, forMissionID, forUnit);
	contributions.push(contrib);
}

//////////////////////////////////////////////////////////////////////
// Pop the next capture contribution for a facility off the queue

Strat::CaptureContribution
Strat::CaptureContribution::GetQueueFront()
{
	return contributions.front();
}

void
Strat::CaptureContribution::QueuePop()
{
	contributions.pop();
}

bool
Strat::CaptureContribution::IsQueueEmpty()
{
	return contributions.empty();
}

//////////////////////////////////////////////////////////////////////
// Get the next active facility

FacilityStruct*
Strat::CaptureContribution::GetNextActiveFacility(FacilityStruct* current)
{
	ACTIVE_FACILITIES::iterator it = activeFacilities.upper_bound(current);
	if ( it == activeFacilities.end() || *it == current)
	{
		return nullptr;
	}

	return *it;
}

