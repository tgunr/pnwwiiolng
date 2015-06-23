#include <syscfg.h> // Path is project specific.
// Created: Fri Dec  2 18:12:40 CST 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtSyncData.h"
#include "strt/strtServerTypes.h"
#include "strt/strtData.h"
#include "strt/strtSupercell.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"

#if defined(IS_MAP_HOST)
# include "map/mapData.h"
#endif

#undef DEBUG_SYNC

void
strtProcessSyncCPs(const STRT_SYNC_CPS& syncCPs)
{
	Supercell* sc = strtGetSupercell(syncCPs.sx, syncCPs.sy);
	if (sc == nullptr)
	{
		//Log the fact that we have an error.  But we arent' going to return
		//	here.  Why?  We need to go ahead and unpack the rest of the
		//	message, because not unpacking it could screw some stuff up
		dbugLog(_FLA,"received sync cps for supercell %d:%d, but it doesn't exist!"
				, syncCPs.sx, syncCPs.sy);
		return;
	}

#ifdef DEBUG_SYNC
	dbugLog(_FLN,"received sync cps for %d:%d, from cpID %u to cpID %u"
			, syncCPs.sx, syncCPs.sy, syncCPs.startID, syncCPs.startID + syncCPs.numEntries - 1);
#endif

	if ((syncCPs.startID + syncCPs.numEntries) > sc->numCPs)
		LogAbort("Bad strat data");

	UINT32 i, cpID;
	const CP_SINFO* sync;

	//Unpack the data
	for ( i = 0, cpID = syncCPs.startID, sync = &syncCPs.info[0]; i < syncCPs.numEntries; ++i, ++cpID, ++sync )
	{
		CPStruct& cp = sc->cps[cpID];
		cp.uOID = sync->uOID;

#if defined(IS_MAP_HOST)
		mapUpdateCP(&cp, (WWII_COUNTRY)sync->owner, (WWII_COUNTRY)sync->control, sync->flags, __FUNCTION__);
#endif

		cp.country = (WWII_COUNTRY)sync->owner;
		cp.side = wwiiSide(sync->owner);

		cp.controllingCountry = (WWII_COUNTRY)sync->control;
		cp.controllingSide = wwiiSide(sync->control);

		cp.flags = sync->flags;
		cp.m_contention = (sync->flags & CP_STAT_CONTENTION) ? TRUE : FALSE;
		cp.m_enemyLinked = (sync->flags & CP_STAT_ENEMY_LINKED) ? TRUE : FALSE;
	}
}

void
strtProcessSyncFacilities(const STRT_SYNC_FACILITIES& syncFacilities)
{
	//Find the supercell being referenced here
	Supercell* sc = strtGetSupercell(syncFacilities.sx, syncFacilities.sy);

	if (sc == nullptr)
	{
		//Log the fact that we have an error.  But we arent' going to return
		//	here.  Why?  We need to go ahead and unpack the rest of the
		//	message, because not unpacking it could screw some stuff up
		dbugLog(_FLA,"received sync facilities for supercell %d:%d, but it doesn't exist!", syncFacilities.sx, syncFacilities.sy);
		return;
	}

#ifdef DEBUG_SYNC
	dbugLog(_FLD,"received sync facilities for %d:%d, from facilityID %u to facilityID %u"
			, syncFacilities.sx, syncFacilities.sy, syncFacilities.startID
			, syncFacilities.startID + syncFacilities.numEntries - 1);
#endif

	if ((syncFacilities.startID + syncFacilities.numEntries) > sc->numFacilities)
		LogAbort("wrong number of facilities in supercell");

	UINT32 i, facID;

	//Unpack the data
	for ( i=0, facID = syncFacilities.startID; i<syncFacilities.numEntries; ++i, ++facID )
	{
		FacilityStruct* fac = &sc->facilities[facID];
		UINT8 country = syncFacilities.country[i];
		fac->uOID = syncFacilities.uOID[i];
		fac->setOpen((country & FAC_OPEN_FLAG) == FAC_OPEN_FLAG);
		fac->setCountry((WWII_COUNTRY)(country & ~FAC_OPEN_FLAG));
		fac->m_captureStage = (FacilityStruct::CaptureStage)syncFacilities.captureStage[i];
		fac->m_captureCounter = syncFacilities.captureCounter[i];
		if ( fac->m_captureStage == FacilityStruct::_CAP_ENABLED )
			fac->m_captureProgress = (float)fac->m_captureCounter;
		fac->m_captureStateChanged = teulTime() + syncFacilities.captureStateChanged[i];

#ifdef DEBUG_SYNC
		dbugLog(_FLN,"syncing facility %d => %u to country %d open %d"
			, fac->fullObjID.id
			, fac->uOID
			, country
			, fac->open);
#endif
	}
}

extern void mapUpdateBuildingVisLevel(const ObjectStruct* const obj);

void
strtProcessSyncBuildings(const STRT_SYNC_BUILDINGS& syncBuildings)
{
	//Find the supercell being referenced here
	Supercell* sc = strtGetSupercell(syncBuildings.sx, syncBuildings.sy);
	if (sc == nullptr)
	{
		dbugLog(_FLA,"received sync buildings for supercell %d:%d, but it doesn't exist!  Unpacking rest of bogus message"
			, syncBuildings.sx, syncBuildings.sy);
		return;
	}

#ifdef DEBUG_SYNC
	dbugLog(_FLD,"received sync buildings for %d:%d, from buildingID %u to buildingID %u"
		, syncBuildings.sx, syncBuildings.sy, syncBuildings.startID, syncBuildings.startID + syncBuildings.numEntries - 1);
#endif

	if ((syncBuildings.startID + syncBuildings.numEntries) > sc->numObjects)
		LogAbort("Bad strat data");

	UINT32 i, b;

	//Unpack the data
	for (i=0, b = syncBuildings.startID; i<syncBuildings.numEntries; ++i, ++b)
	{
		//Here's the NULL check repeated from above.
		if (sc != nullptr)
		{
			sc->objects[b].setVisLevel(syncBuildings.vislevel[i]);
#if defined(IS_MAP_HOST)
			mapUpdateBuildingVisLevel(&sc->objects[b]);
#endif
		}
	}
}

