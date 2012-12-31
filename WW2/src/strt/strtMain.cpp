#include <syscfg.h> // Path is project specific.
//
// Created: Wed Apr  5 04:33:15 2000
//
// These are all the main functions that comprise the Strat System
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Maintaining Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "dbug/dbug.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "strt/strtIO.h"
#include "strt/strtCP.h"
#include "strt/strtFB.h"
#include "strt/strtMain.h"
#include "strt/strtData.h"
#include "strt/strtHostMessages.h"
#include "strt/strtSupply.h"
#include "strt/strtSpawn.h"
#include "strt/strtHack.h"
#include "strt/strtHostDB.h"
#include "wwii/wwiiConfig.h"
#include "cnvy/cnvyMain.h"
#include "versionNo.h"
#include "strt/strtCPClass.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHCUnits.h"
#include "strt/strtSupercell.h"
#include "strt/strtWeaponEntry.h"
#include "strt/strtHostLua.h"
#include "teul/teulTime.h"
#include "wwii/wwiiThreads.h"
#include "strt/strtCaptureContribution.h"

#ifdef WEAP_TEST
//Weapon testing stuff
static void _weapTest(UCHAR * tag);
#endif

static CPStruct* _cpList;	//The list of free CPs
static Supercell* _supercellList = NULL;
static CPStruct* _currentCP = NULL;
static bool _initialized = false ;
static UINT32 _starttime=0;

#ifdef BETA_RELEASE
// Variables for debugger assistance purposes only

 CPStruct*			dbgCP ;
 FacilityStruct*	dbgFacil ;
 ObjectStruct*		dbgObj ;
 void*				dbgPtr ;
 FILE*				dbgFP ;
 UINT32				uint32 ;

#endif // BETA_RELEASE

FULL_OBJ_ID		g_fullObjID ;

//////////////////////////////////////////////////////////////////////
// Build a map of all the pockets

#ifdef POCKET_MAPS
extern UINT32 g_maxCPOID ;

static void
_buildPocketMap()
{
	CPStruct** visitMap = (CPStruct**)alloca(sizeof(CPStruct*) * g_maxCPOID) ;
	CPStruct** newLinks = (CPStruct**)alloca(sizeof(CPStruct*) * g_maxCPOID) ;

	memset(visitMap, 0, sizeof(visitMap)) ;

	std::vector<CPStruct*> pockets ;

	CPStruct* cp = _cpList ;
	CPStruct* linked ;
	CPStruct* neighbor ;
	CPStruct::LINKS::iterator it ;
	WWII_SIDE forSide ;
	UINT32 linkNo ;

	while ( cp != NULL )
	{
		if ( visitMap[cp->uOID] != NULL || cp->numLinks == 0 || isValidSide(cp->side) == false )
			continue ;
		pockets.push_back(cp) ;

		linked = cp ;
		forSide = cp->side ;

		do
		{
			for ( it = linked->m_links.begin() ; it != linked->m_links.end() ; ++it )
			{
				neighbor = (*it)->endDepot->cp ;
				if ( visitMap[neighbor->uOID] == NULL && neighbor->side == forSide )
				{
					visitMap[neighbor->uOID] = cp ;
					newLinks[linkNo++] = neighbor ;
				}
			}

			if ( linkNo > 0 )
			{
				linked = newLinks[--linkNo] ;
			}
		}
		while ( linked != NULL ) ;
	}
}
#endif

// At the start of each strat cycle we check table "ext_vehicle_adjust" for
// new vehicle level adjustments. We find all the current vehicle adjustments
// outstanding and we then apply the group of adjustments to each CP as we
// process them through the strat cycle.

extern int inUse, allocations ;

LOGICAL
strtMain(UINT32 currentTime)
{
	UINT32 i, toRun, leftToRun;
	static UINT32 CPsRan = 0;

	if (!_initialized)
		strtInitializeStratSystem() ;

	FacilityStruct* fac = NULL ;
	while ( (fac = Strat::CaptureContribution::GetNextActiveFacility(fac)) )
	{
		if ( fac->isCapturePending() && fac->getCaptureRemaining() <= 0 )
			fac->setCaptureEnabled() ;
	}

	leftToRun = (strtNumCPs() - CPsRan);
	toRun = (CPSPERCALL < leftToRun) ? CPSPERCALL : leftToRun ;

	//Run the iteration on it
	for ( i = 0 ; i < toRun && _currentCP != NULL ; ++i, _currentCP = _currentCP->next )
	{
		// Run the normal CP processes
		strtRunCP(*_currentCP);

		++CPsRan ;
	}

#ifdef HEAVY_DEBUG
	dbugLog(_FLD, "%d cps ran so far this cycle",CPsRan);
#endif

	if ( _currentCP == NULL )
	{
		//Set the currentCP to the beginning of the list
		_currentCP = _cpList;
		CPsRan = 0;
		dbugLog(_FLI,"starting a new strat cycle.");

#ifdef POCKET_MAPS
		TeulTimeMS start, end ;
		teulSyncClock() ;
		start = teulClock64 ;
		_buildPocketMap() ;
		teulSyncClock() ;
		end = teulClock64 ;
		LogAbort("build pocket map too %llums", end - start) ;
#endif

		return FALSE;
	}

	return TRUE;
}

LOGICAL
strtBroadcastCPs(void)
{
	CPStruct* cp ;

	if ( _cpList == NULL )
		return FALSE;

	for ( cp = _cpList ; cp != NULL ; cp = cp->next )
	{
		//Send the CP status
		strtQueueCPStatus(cp);
	}

	return TRUE;
}

CPStruct*
strtGetCPList(void)
{
	return _cpList;
}

Supercell*
strtGetCellList(void)
{
	return _supercellList ;
}

//////////////////////////////////////////////////////////////////////
//

#ifdef WEAP_TEST
static void _weapTest(UCHAR * tag)
{
WEAPON_SPEC weapSpec;
WEAPON_INDEX weapIndex;
WEAPON* weapon;
const SCHAR weapString[] = "UK_AC_fighter.1";

	weapSpec = weapCreateSpec(weapString);

	weapIndex = weapLookupIndex(weapSpec);

	weapon = weapLookup(weapIndex);

	if (weapon == NULL)
	{
		dbugLog(_FLA,"weapon test failed %s", tag);
		exit(1);
	}

	dbugLog(_FLD,"weapon name is %s", weapon->name);

}
#endif

void
strtInitializeStratSystem()
{
	dbugLog(_FLN,"Initializing Strat System");

	//Get the starting time
	_starttime = teulTime() ;

	// Parse the Wor Files and load terrain data.
	_cpList = strtImportStratData() ;
	if (_cpList == NULL)
	{
		strtReportDataErrors() ;
		fprintf(stderr,"Strat system failed to initialize, /tmp/strt*.log\n");
		LogAbort("Strat System failed to initialize");
		exit(1);
	}

	//Hack to get bridges working
	strtFakeBridges(_cpList);

	// Make sure what we've loaded makes sense
	if (strtVerifyStratSystem(_cpList,0) == false)
	{
		LogAbort("Strat System is not valid");
		exit(1);
	}

	stratInitializeLua() ;

	dbugLog(_FLN,"Strat System verified");
	threadsLogging() ;

	// Make sure all the global spawn data is ready;
	// this doesn't actually initialize spawn lists tho.
	strtInitSpawn();
	threadsLogging() ;

	//Turn on all the bases
	// (which includes creating all the spawn lists)
	strtInitBases(_cpList);
	threadsLogging() ;

	//Init all the firebases
	strtInitStratSystemFirebases(_cpList);
	threadsLogging() ;

#if defined(STRAT_HOST)
	strtdbSaveGameWorld(_cpList) ;
	threadsLogging() ;
#endif // STRAT_HOST

	// Put brigades onto the map
	strtInitializeHCUnits() ;
	threadsLogging() ;

	//Tell everyone the status of the CPs
	strtBroadcastCPs();

	_currentCP = _cpList;

	_initialized = true ;
}

void
strtShutdownStratSystem()
{
	strtReleaseSupercells() ;
}

bool
strtIsStratInitialized()
{
	return _initialized ;
}
