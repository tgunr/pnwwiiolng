#include <syscfg.h> // Path is project specific.
// Created: Fri Jan 25 19:13:44 CST 2008
//
// Implementation of the CaptureAction class.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatConstants.h"
#include "chat/chatDefines.h"
#include "chat/chatGCITypes.h"
#include "chat/chatSupportRPCMessages.h"
#include "strt/strtCaptureAction.h"
#include "strt/strtCPClass.h"
#include "strt/strtCP.h"
#include "chat/chatGCIMessage.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFB.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostLua.h"
#include "strt/strtHostMessages.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtTypes.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiConfig.h"

//////////////////////////////////////////////////////////////////////
//Originally used to set objects owner on takeover.  Now its also going to
//	destroy any autogunners that happen to be connected to the strat system

static bool
_setObjectsOwner(ObjectStruct* objectList, WWII_SIDE side, WWII_COUNTRY country)
{
	if (objectList == nullptr || side == 0 || country == 0)
		return false;

	ObjectStruct* current;

	//Rip through the object list, changing ownership along the way
	for ( current = objectList; current != nullptr; current = current->next )
	{
		//If autogunner, destroy the object
		if ( current->m_side != side && current->type == BUILDING_AUTOGUNNER )
		{
			dbugLog(_FLD, "destroying AI pit %s", current->name);
			///TODO: We should forward these kills too
			current->applyDamage(current->m_maxDamageJoules, 0);
		}

		current->setCountry(country, side);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Find the majority owning country of a town based on ownership of ABs and
// in the case of a tie, airfields

static WWII_COUNTRY
_findMajorityOwner(const CPStruct& cp, WWII_SIDE side)
{
	SINT32 owned[MAX_COUNTRIES];
	SINT32 maxOwned = 0;
	bool matchedOwnership = false;
	WWII_COUNTRY maxOwner = WWII_NO_COUNTRY;
	FacilityStruct* ab = nullptr;

	// A single AB means there's no contest
	if ( cp.numArmyBases == 1 )
		return cp.facilityTypeIndex[FACILITY_ARMYBASE]->country;

	memset(owned, 0, sizeof(owned));

	for ( ab = cp.facilityTypeIndex[FACILITY_ARMYBASE]; ab != nullptr; ab = ab->typeNext )
	{
		// Redundant, but double check that its owned by whom it should be
		// since we're dealing with control...
		if ( ab->side != side )
		{
			dbugLog(_FLA, "Control of %s by side %d but AB %s is owned by side %d"
							, cp.name
							, side
							, ab->name
							, ab->side
							);
			continue;
		}

		owned[ab->country]++;
		if ( owned[ab->country] > maxOwned )
		{
			maxOwner = ab->country;
			maxOwned = owned[ab->country];
			matchedOwnership = false;
		}
		else if ( owned[ab->country] == maxOwned )
		{
			matchedOwnership = true;
		}
	}

	// If we found a clear match
	if ( maxOwned > 0 && matchedOwnership == false )
		return maxOwner;

	// We have tied ownership; use AFs to try and tie-off
	for ( ab = cp.facilityList; ab != nullptr; ab = ab->next )
	{
		if ( ab->type != FACILITY_AIRBASE && ab->type != FACILITY_NAVALBASE )
			continue;

		owned[ab->country]++;
		if ( owned[ab->country] > maxOwned )
		{
			maxOwner = ab->country;
			maxOwned = owned[ab->country];
		}
		else if ( owned[ab->country] == maxOwned )
			matchedOwnership = true;
	}

	if ( matchedOwnership && cp.originalSide == side )
		return cp.originalCountry;

	return maxOwner;
}

//////////////////////////////////////////////////////////////////////
// Decrement the facilities that <side> holds in this CP

static inline void
_decFacilities(CPStruct& cp, WWII_SIDE side)
{
	if ( side > 0 && side < MAX_SIDES )
	{
		if ( cp.facilitiesOwned[side] > 0 )
			cp.facilitiesOwned[side]--;
		else
			cp.facilitiesOwned[side] = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Increment the facilities that <side> holds in this CP

static inline void
_incFacilities(CPStruct& cp, WWII_SIDE side)
{
	if ( side > 0 && side < MAX_SIDES )
	{
		if ( cp.facilitiesOwned[side] < cp.ownableFacilities )
			cp.facilitiesOwned[side]++;
		else
			cp.facilitiesOwned[side] = cp.ownableFacilities;
	}
}

//////////////////////////////////////////////////////////////////////
// Update the numbers of ABs each side has

static bool
_updateABCounts(CPStruct& cp, WWII_SIDE newSide, WWII_SIDE oldSide)
{
	if ( cp.armyBasesOwned[oldSide] > 0 )
		cp.armyBasesOwned[oldSide]--;
	else
		cp.armyBasesOwned[oldSide] = 0;

	if ( cp.armyBasesOwned[newSide] < cp.numArmyBases )
		cp.armyBasesOwned[newSide]++;
	else
		cp.armyBasesOwned[newSide] = cp.numArmyBases;

	//If the "bad guys" have taken all the army bases
	if ( newSide != cp.controllingSide && (cp.armyBasesOwned[cp.controllingSide] == 0 || cp.armyBasesOwned[newSide] >= cp.numArmyBases) )
	{
		cp.controllingSide = newSide;
		cp.controllingCountry = _findMajorityOwner(cp, newSide);

		if ( cp.controllingCountry == 0 )
		{
			dbugLog(_FLA, "-- FIND MAJORITY OWNER RETURNED 0--");
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Set FBs of an armybase to match ownership

static void
_setFBsToMatch(FacilityStruct& ab)
{
	FacilityStruct* fb;

	for ( fb = ab.CP().facilityTypeIndex[FACILITY_FIREBASE]; fb != nullptr; fb = fb->typeNext )
	{
		if ( fb->armyBase->uOID == ab.uOID && fb->country != ab.country && fb->side == ab.side )
		{
			fb->setCountry(ab.country);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Constructor and processor for a capture action.

CaptureAction::CaptureAction(FacilityStruct& withFacility, WWII_COUNTRY withNewCountry, bool withQuietly)
	: m_facility(withFacility), m_cp(m_facility.CP())
	, m_newCountry(withNewCountry), m_newSide(wwiiSide(withNewCountry))
	, m_oldCountry(m_facility.country), m_oldSide(m_facility.side)
	, m_quietly(withQuietly)
	, m_enteredContention(false), m_leftContention(false), m_setControl(false), m_setOwner(false)
	, m_reclamation(m_newSide == m_cp.side && m_newSide != NEUTRAL && m_oldSide != NEUTRAL)
{
	assert( m_facility.type != FACILITY_FIREBASE );
}

void
CaptureAction::Execute()
{
	// Preparation
	for ( int i = 1; i < MAX_SIDES; i++ )
	{
		m_cp.ownMessage[i] = "";
	}

	if ( m_facility.m_spawnable )
		strtCloseBase(m_facility);

	// Do the conversion
	ChangeOwner();

	// Let People know
	Announce();

	// Update the CP etc based on the effects of this action
	ApplyEffects();
}

//////////////////////////////////////////////////////////////////////
// Execute the change of ownership of the facility and counts within the CP

void
CaptureAction::ChangeOwner()
{
	if ( m_facility.type == FACILITY_FIREBASE )
		return;

	if ( m_facility.canBeCaptured() )
	{
		if ( m_cp.isObjective() )
			m_facility.setCapturePending();
		else
			m_facility.setCaptureDisabled();
	}
	else
		dbugLog(_FLA, "fac:%s no capture object", m_facility.name);

	//Set the side correctly for the facility and all its objects
	m_facility.setCountry(m_newCountry);

	// Set the owner of all objects of this facility to this country
	_setObjectsOwner(m_facility.objectList, m_newSide, m_newCountry);

	_decFacilities(m_cp, m_oldSide);
	_incFacilities(m_cp, m_newSide);

	if ( m_cp.facilitiesOwned[m_oldSide] <= 0 || m_cp.facilitiesOwned[m_newSide] == m_cp.ownableFacilities )
	{
		m_leftContention = true;

		//if ( m_cp.side != m_newSide )
		//{
			m_setOwner = true;
		//}
	}
	else if ( m_cp.isContested() == false && m_cp.facilitiesOwned[m_newSide] == 1 )
	{
		m_enteredContention = true;
	}

	if ( m_facility.type == FACILITY_ARMYBASE )
	{
		// When an AB changes hands, check all the FBs to see if they need to match country
		if ( m_oldSide != m_newSide )
			_setFBsToMatch(m_facility);

		m_setControl = _updateABCounts(m_cp, m_newSide, m_oldSide);
	}
	if ( m_leftContention == true )
	{
		// Since we are exiting contention, the owner and controller must be the same.
		m_cp.side = m_cp.controllingSide = m_newSide;
		m_cp.country = m_cp.controllingCountry = _findMajorityOwner(m_cp, m_newSide);

		// Only one side owns, reset all ownership counters and then give
		// full ownership to us.
		memset(m_cp.facilitiesOwned, 0, sizeof(m_cp.facilitiesOwned));
		m_cp.facilitiesOwned[m_newSide] = m_cp.ownableFacilities;
	}
	else if ( m_setControl == true )
	{
		m_cp.controllingSide = m_newSide;
		m_cp.controllingCountry = _findMajorityOwner(m_cp, m_newSide);
	}
}

//////////////////////////////////////////////////////////////////////
// Send a GCI Message to the chat host.

void
chatSendGCIMessage(GCI_TYPE gci_, WWII_SIDE side_, const CPStruct& cp_, const FULL_OBJ_ID facID_)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::GAME_GCI_MESSAGE_Y));
	outgoing.construct<Chat::GCIMessage>(gci_, side_, cp_.fullObjID, facID_, cp_.name);
	serverSendPacketToType(outgoing, CHAT_HOST);
}

//////////////////////////////////////////////////////////////////////
// Send GCI messages for this capture action

void
CaptureAction::Announce() const
{
	dbugLog(_FLI, "cp:%s %s %s from %s to %s. con=%s, control=%s, owner=%s"
					, m_cp.name
					, (m_reclamation ? "retaking" : "taking")
					, m_facility.name
					, _countryName[m_oldCountry], _countryName[m_newCountry]
					, (m_leftContention ? "left" : (m_enteredContention ? "entered" : "yes"))
					, (m_setControl ? "changed" : "same")
					, (m_setOwner ? "changed" : "same")
					);

	// If we've been told not to report to the players, do this quietly.
	if ( m_quietly == true )
		return;

	GCI_TYPE newSideGCI = GCI_NONE;
	GCI_TYPE oldSideGCI = GCI_NONE;

	if ( m_reclamation == true )
	{
		//// Current CP owner retaking a facility

		// If the facility is changing country but not side, do nothing.
		if ( m_newSide == m_oldSide )
			return;

		if ( m_leftContention == true )
		{
			// Occurs when they retake the only facility keeping them contested
			newSideGCI = GCI_CP_LIBERATED;
			oldSideGCI = GCI_CP_DELIBERATED;
		}
		else
		{
			// Otherwise it's normal.
			newSideGCI = GCI_FACILITY_LIBERATED;
			oldSideGCI = GCI_FACILITY_DELIBERATED;
		}
	}
	else
	{
		// "Attacker" mode - capture by side that does not yet own the CP.
		if ( m_leftContention == true )
		{
			//Let the winners know they have captured a CP, and let
			//	the losers know they have lost one
			if ( m_newSide != m_oldSide )
				oldSideGCI = GCI_CP_CAPTURED;

			newSideGCI = GCI_CP_ACQUIRED;
		}
		else
		{
			//Let the losers know they lost a facility
			if ( m_newSide != m_oldSide )
			{
				// First facility takes us into contention
				oldSideGCI = m_enteredContention ? GCI_CP_UNDER_ATTACK : GCI_FACILITY_CAPTURED;
			}

			//Let the winners know they've gained a facility
			newSideGCI = GCI_FACILITY_ACQUIRED;
		}
	}

	if ( oldSideGCI != GCI_NONE )
		chatSendGCIMessage(oldSideGCI, m_oldSide, m_cp, m_facility.fullObjID);
	if ( newSideGCI != GCI_NONE )
		chatSendGCIMessage(newSideGCI, m_newSide, m_cp, m_facility.fullObjID);
}

//////////////////////////////////////////////////////////////////////
//Called when a CP enters contention

void
CaptureAction::EnterContention() const
{
	//Set contention flag
	m_cp.setContention(true);

	dbugLog(_FLI, "cp %s %d:%d:%d has entered contention", m_cp.name, m_cp.fullObjID.cell.ox, m_cp.fullObjID.cell.oy, m_cp.fullObjID.id);

	m_cp.setTableStates();

	strtQueueCPStatus(&m_cp);

	//save changes to DB
	strtSaveCP(&m_cp);
}

//////////////////////////////////////////////////////////////////////
// SetControl is called when a side takes control of a CP
//	NOTE: control is defined as owning ALL army bases

void
CaptureAction::SetControl() const
{
	strtEnemyLink(m_cp, false);
//	cp.numOpenLinks = 0;

	dbugLog(_FLI,"cp:%s is now controlled by side %u, country %u", m_cp.name, m_cp.controllingSide, m_cp.controllingCountry);

	//The bad guys have captured the CP, we need to
	//	open the opposing firebases
	//Run through all the depots and check all the links
	//	to see where we need to open firebases
	FacilityStruct* facility;
	for ( facility = m_cp.facilityList; facility!=nullptr	; facility = facility->next)
	{
		switch ( facility->type )
		{
			case FACILITY_DEPOT:
				if ( facility->m_outgoingLink != nullptr )
				{
					facility->updateSupplyLinks();
				}
				break;
			case FACILITY_FIREBASE:
				break;
			case FACILITY_AIRBASE:
			case FACILITY_NAVALBASE:
				if ( facility->side == m_cp.controllingSide && facility->isOpen() == false )
				{
					strtCloseBase(*facility);
					strtOpenBase(*facility);
				}
			break;
			case FACILITY_ARMYBASE:
				//Close all armybase spawning for the non-controlling side
				if ( facility->side != m_cp.controllingSide || facility->side != m_cp.side )
				{
					dbugLog(_FLW,"Army Base %s %d is prevented from spawning, as it's not owned by %d."
							, facility->name, facility->uOID
							, m_cp.controllingSide);
					strtCloseBase(*facility);

					// Disable the Garrison
					m_cp.disableAllGarrisons();
				}
				else
				{
					//Open all armybase spawning for the controlling side
					dbugLog(_FLI,"Army Base %s %d will spawn for side %d"
							, facility->name, facility->uOID, m_cp.controllingSide);

					// It's already open, force some of the opening process to execute,
					// but don't flush the spawn lists if we're already open
					if ( facility->isOpen() == false )
					{
						strtCloseBase(*facility);
						strtOpenBase(*facility);
						// Enable the Garrison
						m_cp.enableAllGarrisons();
					}
				}

				break;
			default:
				break;
		}
	}

	m_cp.updateFriendlyLinkStatus();

	strtQueueCPStatus(&m_cp);
	strtSaveCP(&m_cp);
}

//////////////////////////////////////////////////////////////////////
//Called when a CP leaves contention - i.e. one side holds ALL facilities.

void
CaptureAction::LeaveContention() const
{
	// Clear out running flags
	m_cp.endContention();

	dbugLog(_FLI, "cp %s %d:%d:%u has left contention"
				, m_cp.name
				, m_cp.fullObjID.cell.ox
				, m_cp.fullObjID.cell.oy
				, m_cp.fullObjID.id);

	for ( FacilityStruct* fac = m_cp.facilityList; fac != nullptr; fac = fac->next )
	{
		//Turn on spawning at all non-army bases
		if ( fac->side != m_cp.side && fac->type != FACILITY_FIREBASE )
		{
			dbugLog(_FLW, "cp %s exiting contention, forcing side match of %s"
							, m_cp.name
							, fac->name
					);

			fac->setCountry(m_cp.country);

			strtCloseBase(*fac);

			if ( m_cp.side != NEUTRAL )
				strtOpenBase(*fac);
		}

		if ( fac->type == FACILITY_DEPOT && fac->m_outgoingLink != nullptr )
		{
			fac->updateSupplyLinks();
			LinkStruct* link = fac->m_outgoingLink;
			if ( link != nullptr && link->firebase != nullptr && link->firebase->isOpen() && link->endDepot->cp->side == m_newSide )
				strtCloseFirebase(*link->firebase);
		}

		if ( fac->isMilitaryFacility() && fac->isOpen() == false )
		{
			strtOpenBase(*fac);
		}
	}

	strtQueueCPStatus(&m_cp);
	strtSaveCP(&m_cp);
}

void HCUnit::GetGarrisonUnits(std::list<HCUnit>& unitsList, UINT16 cpID)
{
	for (auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it)
	{
		if (it->second->isGarrison())
		{
			if (it->second->homeCPID().id == cpID)
			{
				unitsList.push_back(*it->second);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////
// Finish the capture event.

void
CaptureAction::ApplyEffects() const
{
	if ( m_enteredContention == true )
		EnterContention();

	// Must leave contention before we mess with control
	if ( m_leftContention == true )
		LeaveContention();

	// Assign control to the new controller, if neccessary
	if ( m_setControl == true )
		SetControl();

	// If we've changed the owner of the town, or we've changed
	// the controller of the town, or we're coming out of
	// contention, then we may have changed the factory link
	// status of neighboring towns. Force a rebuild of the
	// factory-link supply status chains.
	if ( m_setOwner || m_setControl || m_leftContention )
	{
		strtBuildSupplyChains(true);
	}

	// Bounce any brigades that should no-longer be here.
	m_cp.checkHCUnits();

	// When we take ownership of a new town, destroy any outgoing
	// firebases this would give us to enemy towns.
	if ( m_leftContention == true && m_setOwner == true )
	{
		CPStruct::LINKS::iterator it;
		for ( it = m_cp.m_links.begin(); it != m_cp.m_links.end(); ++it )
		{
			LinkStruct* link = *it;
			if ( link->firebase && link->firebase->side == m_newSide && link->endDepot->CP().side != m_newSide )
			{
				dbugLog(_FLD, "Closing leapfrog fb: %s", link->firebase->name);
				strtCloseFirebase(*(link->firebase));
			}
		}

		// Change the country of all Garrisons in this cp
		// Sends it to chat and map hosts, map sends it to all clients.
		m_cp.changeAllGarrisonCountries(m_newCountry, false);

	}

	// Now that we are clear of brigades, we need to process the
	// firebases - since brigade deployment controls firebases.
	if ( m_leftContention == true || m_setControl == true )
	{
		m_cp.processFirebases(true);
		// If we advanced the frontline, make sure we shutdown any non-frontline
		// Brigade FB projections.
		CPStruct::LINKS::iterator it;
		for ( it = m_cp.m_links.begin(); it != m_cp.m_links.end(); ++it )
		{
			(*it)->endDepot->cp->processFirebases(false);
		}
	}

	// Military facilities open immediately.
	if ( m_facility.m_spawnable && m_facility.type != FACILITY_DEPOT )
	{
		if ( m_facility.side == m_cp.side && m_facility.side == m_cp.controllingSide )
		{
			strtOpenBase(m_facility);
		}
	}

	if ( m_cp.isObjective() )
		m_facility.setCapturePending();
	else
		m_facility.setCaptureDisabled();

	strtQueueFacilityStatus(&m_facility);

	strtSaveFacility(&m_facility);

	if ( m_enteredContention )
		strtCPEventTrigger(m_cp, "cpEnteredContention");
	if ( m_leftContention )
		strtCPEventTrigger(m_cp, "cpLeftContention");
	if ( m_setControl )
		strtCPEventTrigger(m_cp, "cpChangedControl");
	if ( m_setOwner )
		strtCPEventTrigger(m_cp, "cpChangedOwner");
}

