#include <syscfg.h> // Path is project specific.
// Created: Thu May 17 14:04:31 2001
//
// Necessary code additions required to get bridges working in the strat
// system.  However, due to thier hackish nature, we don't want other things
// (namely the mmHost) to use it without extreme caution
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Maintaining Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "dbug/dbug.h"
#include "mem/mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strt/strtTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtSupercell.h"
#include "strt/strtProtos.h"
#include "strt/strtData.h"
#include "strt/strtHack.h"
#include "wwii/wwiiString.h"

static LOGICAL _fakeStructure(CPStruct* cp);
typedef std::vector< std::pair<FacilityStruct*, ObjectStruct*> > FAKES ;
static FAKES _fakes ;

static void
deleteFakes()
{
	for ( auto it = _fakes.begin() ; it != _fakes.end() ; ++it )
	{
		delete it->first ;
		memFree(it->second) ;
	}

	_fakes.clear() ;
}

//Takes the cp list and runs through it all, turning each standalone
//	bridge CP into one with a faked facility and object to take damage
LOGICAL
strtFakeBridges(CPStruct* cpList)
{
	if (cpList == NULL)
		return FALSE;

	dbugLog(_FLI, "Initializing bridges") ;

	//Go through every cp
	for ( CPStruct* cp = cpList ; cp != NULL ; cp = cp->next )
	{
		if (cp->fullObjID.objType == CP_BRIDGE)
		{
#if defined(DEV_BUILD)
			dbugLog(_FLN,"CP %s is a bridge CP, building a facility and an object for it", cp->name);
#endif

			_fakeStructure(cp);
		}
	}

	atexit(deleteFakes) ;

	return TRUE;
}

//Allocates the fake CP structure underneath bridge CPs
static LOGICAL _fakeStructure(CPStruct* cp)
{
	if (cp == NULL)
		return FALSE;

	//Get memory for our darling little hack
	FacilityStruct* const facility = new FacilityStruct ;
	//Our hack is hungry!  I needs more memory!
	ObjectStruct* const object = (ObjectStruct*)memCalloc(sizeof(ObjectStruct), 1);

	_fakes.push_back(std::make_pair(facility, object)) ;

	// Make sure we never try to save these.
	facility->modified = object->modified = true ;

	//The facility is no more than a placeholder, so we only need to set its
	//	type (I think)

	cp->setSide((WWII_SIDE)BRIDGE_SIDE) ;
	cp->country = (WWII_COUNTRY)BRIDGE_BUILT_COUNTRY ;
	cp->controllingSide = cp->side ;
	cp->controllingCountry = cp->country ;

	//Set its type to base, so it won't do anything in the main strat loop
	facility->fullObjID = cp->fullObjID ;
	facility->fullObjID.objType = facility->type = FACILITY_BASE;

	cp->facilityList = facility;
	facility->cp = cp;
	//Setup the side

	facility->setCountry(cp->country) ;

	strlcpy(facility->name, cp->name, sizeof(facility->name)) ;
	facility->filename = cp->filename ;

	//Setup the fake FULL_OBJ_ID
	object->fullObjID = cp->fullObjID ;
	object->fullObjID.objClass = CLASS_BUILDING ;
	object->fullObjID.objType = BUILDING_GENERIC ;
	object->type = BUILDING_BRIDGE ;

	facility->objectList = object;
	object->facility = facility;
	object->setCountry(facility->country, facility->side) ;

	//Initialize the object
	//Fake an object name
	snprintf(object->name, sizeof(object->name) - 1, "BRIDGE _ %s", cp->name) ;

	//copy over the object filename, so when we lookup its information in the
	//	WOR file, we actually can find something
	object->filename = cp->filename ;

	//Initialize the object from the WOR file
	strtInitObjectfromWOR(*object);

	//Changes the SC's index to point at the bridge object as opposed to the
	//	Bridge CP.  This is the big scary hack part.  What it does is
	//	allow damage lookups on the bridge to find out fake object
	Supercell* const sc = strtGetSupercellFULL_OBJ_ID(cp->fullObjID);
	if ( sc != NULL && sc->index[cp->fullObjID.id] == (void*) cp)
	{
		sc->index[cp->fullObjID.id] = (void*) object;
	}

	return TRUE;
}

