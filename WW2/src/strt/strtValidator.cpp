#include <syscfg.h> // Path is project specific.
// Created: Fri Mar 30 16:46:40 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall
// Supporting Programmer:  Marty "Vandal" Poulin

#include "strt/strtData.h"
#include "strt/strtDataErrors.h"
#include "strt/strtIO.h"
#include "strt/strtSpawn.h"
#include "strt/strtFB.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "teul/teulTime.h"

typedef struct
{
	UINT32 RPsProduced, depotStorage, depotRPs, RPs;
	FLOAT32 factoryMultiplier;
	SINT32 RPsUsed;
} WWII_SIDE_ACC;
//Local functions

static inline UINT32
_factor1k(FacilityStruct* const facility, UINT32 amount)
{
	return (UINT32)((FLOAT64)amount * (FLOAT64)facility->capacity / (FLOAT64)1000);
}

// This function actually runs the internal stuff on a CP
static SINT32 _runFacility(FacilityStruct* const currentFacility, WWII_SIDE_ACC* const sa)
{
	if ( currentFacility == nullptr || sa == nullptr)
		return -1;

	//Check to see if facility is under contention and check to
	//	make sure we are on the same side.
	if (! currentFacility->cp->isContested() )
	{
		switch (currentFacility->type)
		{
			//If its a City, do city stuff
			case FACILITY_CITY:
				//Add the RPs to the pool for this CP
				sa->RPsProduced += _factor1k(currentFacility, currentFacility->rpsPerUnitTime);
				break;
			//If its a factory, do factory stuff
			case FACILITY_FACTORY:
				sa->factoryMultiplier+= _factor1k(currentFacility, currentFacility->multiplier);
				break;
			//If its a Depot, do depot stuff
			case FACILITY_OMT:
			case FACILITY_SHIPYARD:
			case FACILITY_DEPOT:
				sa->depotStorage += _factor1k(currentFacility, currentFacility->requestThreshold - currentFacility->storedRPs);

				sa->depotRPs+=currentFacility->storedRPs;
				break;
			//case FACILITY_BASE: deprecated, no longer used
			case FACILITY_AIRBASE:
			case FACILITY_NAVALBASE:
			case FACILITY_ARMYBASE:
			case FACILITY_FIREBASE:
			default:
			break;
		}
	}

	if (currentFacility->cp->originalSide != currentFacility->side)
	{
		sa->RPsProduced/=2;
		sa->factoryMultiplier/=2;
	}

	//Multiply by factory output
//	sa->RPsProduced*=sa->factoryMultiplier;

	//Run the object damage stuff on all objects for this CP
	sa->RPs=sa->RPsProduced+sa->depotRPs;

	return 0;
}


LOGICAL
_calcStratSystem(const SCHAR* filename, CPStruct* cpList)
{
	FILE* fp;
	WWII_SIDE_ACC sa;

	fp = fopen(filename, "w");
	if (fp == nullptr)
	{
		dbugLog(_FLN,"Filename %s invalid", filename);
		return FALSE;
	}

	CPStruct* cp;
	FacilityStruct* facility;
	for ( cp = cpList; cp != nullptr; cp = cp->next )
	{
		if (cp->fullObjID.objType != CP_BRIDGE)
		{
			//Reset all values to 0
			memset(&sa,0,sizeof(WWII_SIDE_ACC));
			sa.factoryMultiplier = 1;

			for ( facility = cp->facilityList; facility != nullptr; facility = facility->next )
			{
				//We don't run firebases
				if (facility->type != FACILITY_FIREBASE)
				{
					_runFacility(facility, &sa);
				}
			}

			sa.RPsProduced = (UINT32)(sa.RPsProduced * sa.factoryMultiplier);
			//printout the totals
			fprintf(fp,"CP %s\n\tRPs Produced: %u\n\tDepot Storage: %d\n\tFactory Multiplier: %5.2f\n\n",cp->name,sa.RPsProduced, sa.depotStorage, sa.factoryMultiplier);

		}
	}

	fclose(fp);

	return TRUE;
}

int
main (int argc, char** argv)
{
	Teul::SetAppName(argv[0]);
	Teul::ConfigVar::InitializeConfigVars();

	static CPStruct* cpList;	//The list of free CPs
	int level = 1;

	if (argc > 1)
		level = atol(argv[1]);

	switch(level)
	{
	case 0:
		dbugInit(LogMode::TO_FILE, "./FatalErrors");
		break;
	case 1:
		dbugInit(LogMode::TO_FILE, "./Warnings");
		break;
	default:
		dbugInit(LogMode::TO_FILE, "./validator");
		break;
	}

	dbugSetLogLevel(Debug::ERR_DBUG);

	//Get the starting time

	cpList = strtImportStratData();
	if ( cpList == nullptr)
	{
		strtReportDataErrors(0);
		dbugLog(_FLA,"[X][X][X] Strat System would not init [X][X][X]");
		exit(1);
	}

	if ( strtVerifyStratSystem(cpList, level) == false )
	{
		dbugLog(_FLA, "[X][X][X] Strat System is not valid [X][X][X]");
		exit(1);
	}


	dbugLog(_FLI, "./ ./ ./ Strat System verified \\. \\. \\.");

	_calcStratSystem("RP_report.txt",cpList);

	//Setup all the spawning stuff
	//strtInitSpawn();

	dbugClose();

	return 0;
}
