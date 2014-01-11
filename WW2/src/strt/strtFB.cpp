#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 16 13:06:59 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "strt/strtTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtFB.h"
#include "cnvy/cnvyMain.h"
#include "strt/strtSpawn.h"
#include "strt/strtHostMessages.h"
#include "strt/strtData.h"
#include "strt/strtIO.h"
#include "wwii/wwiiConfig.h"

#ifdef IS_STRAT_HOST
#include "strt/strtHostDB.h"
#endif

//////////////////////////////////////////////////////////////////////
// Check if brigade FBs are allowed between two towns

static bool
_allowBrigadeFBs(const CPStruct& left, const CPStruct& right)
{
	if ( host_config.supply.fbsToFrontline <= 0 )
		return false;

	if ( left.side != left.controllingSide )
		return false;
	if ( left.side != right.side )
		return false;
	if ( right.side != right.controllingSide )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Check to see if two towns qualify for a brigade firebase

static bool
_brigadeFirebase(const CPStruct& left, const CPStruct& right)
{
	// Open up a link between brigades
	if ( host_config.supply.fbsToFrontline > 0 && right.isEnemyLinked() == true && left.numBrigades(BRANCH_LAND) > 0 && right.numUnits() == 0 )
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////

static inline bool
_fbsBetweenCPs(const CPStruct& left, const CPStruct& right)
{
	if ( left.isEnemyLinked() == false && right.isEnemyLinked() == false )
		return false;

	if ( left.side != left.controllingSide && right.side != right.controllingSide )
		return false;

	if ( left.side == right.side && left.controllingSide == right.controllingSide )
	{
		if ( _brigadeFirebase(left, right) == true || _brigadeFirebase(right, left) == true )
			return true;

		// Otherwise behave as normal
		return false;
	}

	if ( isValidSide(left.side) == false || isValidSide(right.side) == false )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Process the fireabases on a link - check all conditions that should
// cause it to be open etc.

bool
LinkStruct::processFirebases()
{
	// Not all links have a firebase
	if ( firebase == nullptr )
		return false;

	// Shortcut the return link
	LinkStruct* mate = endDepot->m_outgoingLink;
	if ( mate == nullptr || mate->firebase == nullptr )
	{
		dbugLog(_FLD, "%s: no firebase", name);
		return false;
	}

	CPStruct* leftCP = this->firebase->cp;
	CPStruct* rightCP = mate->firebase->cp;

	// Check if we should be looking the other way around
	bool fbsAllowed = _fbsBetweenCPs(*leftCP, *rightCP);

	if ( fbsAllowed == true && _brigadeFirebase(*rightCP, *leftCP) == true  )
	{
		dbugLog(_FLD, "%s: reversing", name);
		return mate->processFirebases();
	}

	const WWII_COUNTRY country = this->firebase->country;
	const WWII_COUNTRY mateCountry = mate->firebase->country;
	const WWII_COUNTRY newCountry = strtNewFBOwner(*(this->firebase));
	const WWII_COUNTRY mateNewCountry = strtNewFBOwner(*(mate->firebase));

	bool result = false;

	if ( fbsAllowed == false )
	{
		if ( firebase->isOpen() || mate->firebase->isOpen() )
			dbugLog(_FLD, "pfb: no fbs allowed on %s", name);
		if ( firebase->isOpen() == true )
			strtCloseFirebase(*firebase);
		if ( mate->firebase->isOpen() == true )
			strtCloseFirebase(*(mate->firebase));
		return false;
	}

	if ( country != newCountry )
	{
		if ( this->firebase->isOpen() && wwiiSide(country) != wwiiSide(newCountry) )
		{
			dbugLog(_FLI, "pfb: closing %s fb, country change %u->%u", this->firebase->name, country, newCountry);
			strtCloseFirebase(*(this->firebase));
		}
		this->firebase->setCountry(newCountry);
	}
	if ( mateCountry != mateNewCountry )
	{
		if ( mate->firebase->isOpen() && wwiiSide(mateCountry) != wwiiSide(mateNewCountry) )
		{
			dbugLog(_FLI, "pfb: closing mate (%s) fb, country change %u->%u", mate->firebase->name, mateCountry, mateNewCountry);
			strtCloseFirebase(*(mate->firebase));
		}
		mate->firebase->setCountry(mateNewCountry);
	}

	if ( firebase->isOpen() == true && mate->firebase->isOpen() == true )
	{
		result = true;
		dbugLog(_FLI, "pfb: resolving %s by closing %s", name, mate->firebase->name);
		strtCloseFirebase(*(mate->firebase));
	}

	if ( firebase->isOpen() == false && mate->firebase->isOpen() == false )
	{
		result = true;
		dbugLog(_FLI, "pfb: %s empty, opening %s", name, this->firebase->name);
		strtOpenFirebase(*(this->firebase));
	}

	return result;
}

//////////////////////////////////////////////////////////////////////

WWII_COUNTRY
strtNewFBOwner(FacilityStruct& fb)
{
	WWII_COUNTRY newCountry;
	CPStruct& cp = fb.CP();

	CPStruct& mateCP = *fb.link->endDepot->cp;
	// The owner and control of Local and Remote CP

	const WWII_SIDE locOwnSide = cp.side;
	const WWII_SIDE locConSide = cp.controllingSide;
	//const WWII_SIDE remOwnSide = mateCP.side;
	const WWII_SIDE remConSide = mateCP.controllingSide;

	if ( _allowBrigadeFBs(cp, mateCP) )
	{
		// This option enables FBs between two towns where:
		// Town A has 1+ army brigades and Town B is frontline with 0 army brigades
		// The FB from the empty town has to be given an enemy country so that
		// both FBs can be destroyed

		// Are we a town with armybrigades linked to a friendly frontline town with no army brigades?
		bool leftBrigadeFB = _brigadeFirebase(cp, mateCP);
		// If left CP is projecting an FB and we control, use the controlling country.
		if ( leftBrigadeFB )
			return cp.controllingCountry;

		bool rightBrigadeFB = (leftBrigadeFB == false && _brigadeFirebase(mateCP, cp));

		if ( rightBrigadeFB )
		{
			// Now we're looking at the enemy FB that comes back to the bde;
			// we need to make up an owner.
			if ( cp.side != mateCP.side )
			{
				return cp.country;
			}
			if ( cp.side != cp.originalSide )
			{
				return cp.originalCountry;
			}
			if ( mateCP.originalSide != cp.side )
			{
				return mateCP.originalCountry;
			}
			return ( cp.side == ALLIED ) ? WWII_GERMANY : WWII_FRANCE;
		}
	}

	// If the owning CP is sure about its identity
	if ( locOwnSide == locConSide || locConSide != remConSide )
	{
		if ( fb.armyBase && fb.armyBase->side == locConSide )
			newCountry = fb.armyBase->country;
		else
			newCountry = cp.controllingCountry;
	}
	else
	{
		fb.side = locOwnSide;
		if ( fb.armyBase && fb.armyBase->side == locOwnSide )
			newCountry = fb.armyBase->country;
		else
			newCountry = cp.country;
	}

	return newCountry;
}

//////////////////////////////////////////////////////////////////////

static inline void
_initFirebaseObjects(FacilityStruct& firebase)
{
	UCHAR visLevel = (firebase.isOpen()) ? 0 : FB_CLOSED_VISLEVEL;

	for ( ObjectStruct* object = firebase.objectList; object != nullptr; object = object->next )
		object->setVisLevel(visLevel);
}

//////////////////////////////////////////////////////////////////////

static void
_initFirebases(FacilityStruct& depot)
{
	if ( depot.m_outgoingLink == nullptr || depot.m_outgoingLink->firebase == nullptr )
		return;

	FacilityStruct& outFB = *depot.m_outgoingLink->firebase;
	FacilityStruct& inFB = *depot.m_incomingLink->firebase;

	outFB.setCountry(strtNewFBOwner(outFB));
	inFB.setCountry(strtNewFBOwner(inFB));

	_initFirebaseObjects(outFB);
	_initFirebaseObjects(inFB);

	depot.m_outgoingLink->processFirebases();
}

//////////////////////////////////////////////////////////////////////

LOGICAL
strtInitStratSystemFirebases(CPStruct* cpList)
{
	CPStruct* cp;
	FacilityStruct* facility;

	if ( cpList == nullptr )
		return FALSE;

	for ( cp = cpList; cp != nullptr; cp = cp->next )
	{
		for ( facility = cp->facilityTypeIndex[FACILITY_DEPOT]; facility != nullptr; facility = facility->typeNext )
		{
			_initFirebases(*facility);
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//This fucntion opens or closes a firebase at the beginning of the game
//	NOTE: this should NOT be used to open or close a firebase during
//		the running of the strat system
//	if mode is TRUE, then the firebase is opened
//	if mode is FALSE, the firebase is closed
void
strtInitFirebase(FacilityStruct& firebase, bool isOpen)
{
	if (isOpen)
		strtOpenFirebase(firebase);
	else
		strtCloseFirebase(firebase);
}

//////////////////////////////////////////////////////////////////////
//This function opens a firebase that is currently closed in a RUNNING
//	strat system
void
strtOpenFirebase(FacilityStruct& openingFirebase)
{
	dbugLog(_FLN,"Opening firebase %s %d.%d.%u as %s"
					, openingFirebase.name
					, openingFirebase.fullObjID.cell.ox, openingFirebase.fullObjID.cell.oy, openingFirebase.fullObjID.id
					, strtPrintCountry(strtNewFBOwner(openingFirebase))
					);

	//Set open flag
	openingFirebase.setOpen(true);

	//Set capacity to full, since we are starting with a whole firebase
	openingFirebase.capacity = 1000;

	// Always determine whom we should belong to at opening time.
	openingFirebase.setCountry(strtNewFBOwner(openingFirebase));

	//Set every object to fully rebuilt and send the vislevel
	//	update
	ObjectStruct* currentObject = openingFirebase.objectList;
	while (currentObject != nullptr)
	{
		currentObject->setDamageJoules(0);
		currentObject->setCapacity(currentObject->m_maxCapacity);
		currentObject->setVisLevel(VISLEVEL_NORMAL);
		currentObject->setSmokeLevel(SMOKELEVEL_OFF);
		currentObject->percentDamage = 0;

		currentObject = currentObject->next;
	}

	strtQueueFacilityStatus(&openingFirebase);
#ifdef IS_STRAT_HOST
	strtSaveFacility(&openingFirebase);
#endif
}

//////////////////////////////////////////////////////////////////////
//This function closes a firebase that is currently open in a RUNNING
//	strat system
void
strtCloseFirebase(FacilityStruct& closingFirebase)
{
	dbugLog(_FLN,"Closeing firebase %s %d.%d.%u"
					, closingFirebase.name
					, closingFirebase.fullObjID.cell.ox, closingFirebase.fullObjID.cell.oy, closingFirebase.fullObjID.id
					);

	//Set open flag
	closingFirebase.setOpen(false);
	closingFirebase.capacity = 0;

//	closingFirebase.setCountry(closingFirebase.cp->controllingCountry);

	//Set every object to "closed" and send the visLevel update
	ObjectStruct* currentObject = closingFirebase.objectList;
	while (currentObject != nullptr)
	{
		currentObject->setDamageJoules(currentObject->m_maxDamageJoules);
		currentObject->setCapacity(0);
		currentObject->setVisLevel(FB_CLOSED_VISLEVEL);
		currentObject->setSmokeLevel(SMOKELEVEL_LIGHT);
		currentObject->percentDamage = 100;

		currentObject = currentObject->next;
	}

#ifdef IS_STRAT_HOST
	strtQueueFacilityStatus(&closingFirebase);
	strtSaveFacility(&closingFirebase);
#endif
}


//////////////////////////////////////////////////////////////////////
//This function is called to clear the firebases on BOTH ENDS of a PATH
//	This function acts on both link the link its passed, and that links
//	partner

void
strtClearFirebases(LinkStruct& link)
{
	//Check the far end
	if (link.firebase)
	{
		if (link.firebase->isOpen())
			strtCloseFirebase(*link.firebase);
	}

	LinkStruct* mate = link.endDepot->m_outgoingLink;

	//Check the close end
	if (mate->firebase)
	{
		if (mate->firebase->isOpen())
			strtCloseFirebase(*(mate->firebase));
	}
}
