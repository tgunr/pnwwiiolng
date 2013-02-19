#include <syscfg.h> // Path is project specific.
//
// Created: Tue Apr  4 23:09:26 2000
//
// This file contains all the input/output functions associated with
// the Strat System
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Maintaining Programmer: Eric "Meat" Hall
// Supporting Programmer:  Eric "Meat" Hall

#include "dbug/dbug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>

#include "strt/strtProtos.h"
#include "strt/strtServerTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtSupercell.h"
#include "strt/strtSupercells.h"
#include "str/stringLib.h"
#include "strt/strtDTL.h"
#include "strt/strtIO.h"
#include "strt/strtData.h"
#include "strt/strtDTL.h"
#include "sdm/sdmConfigVars.h"
#include "sdm/sdmParse.h"
#include "include/defs.h"
#include "wwii/wwiiConfig.h"

SUPERCELLS g_supercells ;

//This function encapsulates a big case statement that returns the
//string that correstponds to the country owner referenced by the
//COUNTRY parameter
const char* strtPrintCountry(WWII_COUNTRY country)
{
	return _countryName[country];
}

//This function prints out the enumerated Facilty Type that's passed in
const char* strtPrintFacilityType(FACILITY_TYPE facilityType)
{
	switch(facilityType)
	{
		case FACILITY_CITY:return("City");break;
		case FACILITY_FACTORY:return("Factory");break;
		case FACILITY_BASE:return("Base");break;
		case FACILITY_DEPOT:return("Depot");break;
		case FACILITY_OMT:return("Off Map Transfer");break;
		case FACILITY_SHIPYARD:return("Shipyard");break;
		case FACILITY_FIREBASE:return("Firebase");break;
		case FACILITY_AIRBASE:return("Air Base");break;
		case FACILITY_NAVALBASE:return("Naval base");break;
		case FACILITY_ARMYBASE:return("Army Base");break;
		default:return("Unknown");
	}
	return (NULL);
}

//////////////////////////////////////////////////////////////////////
// Report an occurance of a duplicate object id

static void
_reportDuplicate(Supercell* sc, const char* filename, UINT32 index, const char* curName)
{
	const char* name = NULL;
	switch (((FULL_OBJ_ID*)sc->index[index])->objClass)
	{
		case CLASS_CP: name = ((CPStruct*)sc->index[index])->name; break;
		case CLASS_FACILITY: name = ((FacilityStruct*)sc->index[index])->name; break;
		case CLASS_BUILDING: name = ((ObjectStruct*)sc->index[index])->name; break;
		case CLASS_LINK: name = ((LinkStruct*)sc->index[index])->name; break;
		case CLASS_WAYPOINT: LogAbort("don't know what to do with waypoints") ; break;
	}
	if (name == NULL)
		name = "UNKNOWN OBJECT" ;
	dbugLog(_FLA,"multiple objects with same id in file %s id:%d %s and %s",filename, index, name, curName);
}

//////////////////////////////////////////////////////////////////////
// Initialize the CPs within a supercell

static void
_initCPs(Supercell* sc, const char* filename)
{
	for ( UINT32 n = 0 ; n < sc->numCPs ; ++n )
	{
		CPStruct* currentcp = &sc->cps[n];
		CP* tempcp = sdmGetNextCP();

		if ( !tempcp )
			continue ;

		strtInitCPStructwithCP(currentcp, tempcp);
		UINT32 index = currentcp->fullObjID.id;
		if (index >= sc->indexSize)
			fprintf(stderr,"%d:Index too large!!!\n",index);
		else if (sc->index[index] != NULL)
			_reportDuplicate(sc, filename, index, currentcp->name) ;
		else
			sc->index[index] = currentcp;
	}
}

//////////////////////////////////////////////////////////////////////
// Initialize the facilities within a supercell

static void
_initFacilities(Supercell* sc, const char* filename)
{
	//init all facilities and the main index
	for ( UINT32 n = 0 ; n < sc->numFacilities ; ++n )
	{
		FacilityStruct* currentfacility = &sc->facilities[n];
		FACILITY* tempfacility = sdmGetNextFACILITY();
		if ( !tempfacility )
			continue ;

		strtInitFacilityStructwithFACILITY(currentfacility,tempfacility);
		UINT32 index = currentfacility->fullObjID.id;
		if (index >= sc->indexSize)
			fprintf(stderr,"%d:Index too large!!!\n",index);
		else if (sc->index[index] != NULL)
			_reportDuplicate(sc, filename, index, currentfacility->name) ;
		else
			sc->index[index] = currentfacility;
	}
}

//////////////////////////////////////////////////////////////////////
// Initialize the objects (buildings) within a supercell

static void
_initObjects(Supercell* sc, const char* filename)
{
	//init all buildings (objects) and the main index
	for ( UINT32 n = 0 ; n < sc->numObjects ; ++n )
	{
		ObjectStruct* currentobject = &sc->objects[n];
		BUILDING* tempbuilding = sdmGetNextBUILDING();
		if (!tempbuilding)
			continue ;

		strtInitObjectStructwithBUILDING(currentobject,tempbuilding);
		UINT32 index = currentobject->fullObjID.id;
		if (index >= sc->indexSize)
			fprintf(stderr,"%d:Index too large!!!\n",index);
		else if (sc->index[index] != NULL)
			_reportDuplicate(sc, filename, index, currentobject->name) ;
		else
			sc->index[index] = currentobject;
	}
}

//////////////////////////////////////////////////////////////////////
// Initialize the inter-cp links in a particular supercell

static void
_initLinks(Supercell* sc, const char* filename)
{
	//init all links
	for ( UINT32 n = 0 ; n < sc->numLinks ; ++n )
	{
		LinkStruct* currentlink = &sc->links[n];
		DEPOT_LINK* templink = sdmGetNextDEPOT_LINK();
		if ( !templink )
			continue ;

		strtInitLinkStructwithDEPOT_LINK(currentlink,templink);
		UINT32 index = currentlink->fullObjID.id;
		if (index >= sc->indexSize)
			fprintf(stderr,"%d:Index too large!!!\n",index);
		else if (sc->index[index] != NULL)
			_reportDuplicate(sc, filename, index, currentlink->name) ;
		else
			sc->index[index] = currentlink;
	}
}

//////////////////////////////////////////////////////////////////////
//poulates supercell with info from file name filename

bool
Supercell::initialize(SINT16 scx, SINT16 scy, const char* filename)
{
	assert( numCPs == 0 ) ;

	x = scx ;
	y = scy ;

	numCPs = sdmGetNumCP() ;
	numFacilities = sdmGetNumFACILITY() ;
	numObjects = sdmGetNumBUILDING() ;
	numLinks = sdmGetNumDEPOT_LINK() ;
	numWaypoints = sdmGetNumWAYPOINT() ;
	indexSize = sdmGetMaxStratId() + 1 ;

	if ( numCPs > 0 )
		cps = new CPStruct[numCPs] ;
	if ( numFacilities > 0 )
		facilities = new FacilityStruct[numFacilities] ;
	if ( numObjects > 0 )
		objects = (ObjectStruct*)memCalloc(sizeof(ObjectStruct), numObjects) ;
	if ( numLinks > 0 )
		links = (LinkStruct*)memCalloc(sizeof(LinkStruct), numLinks) ;
	if ( indexSize > 0 )
		index = (void**)memCalloc(sizeof(void*), indexSize) ;

	_initCPs(this, filename) ;
	_initFacilities(this, filename) ;
	_initObjects(this, filename) ;
	_initLinks(this, filename) ;

	// Initialize the facility and building tables
	facilityTable = sdmCreateFacilityTable(sdmGetFACILITYArray(), numFacilities, numCPs);
	if ( facilityTable != NULL )
	{
		UINT32 errors = 0 ;
		buildingTable = sdmCreateBuildingTable(facilityTable, sdmGetBUILDINGArray(), sdmGetNumBUILDING(), NULL, &errors);
		if ( NULL == buildingTable )
		{
			dbugLog(_FLA, "Building Table creation failed") ;
			++errors ;
		}

		if ( errors != 0 )
			return false ;
	}

	return true ;
}

void
strtReleaseSupercells()
{
	g_supercells.clear() ;
}

//////////////////////////////////////////////////////////////////////
// Extract the X and Y co-ordinates out of a filename

static bool
_getFilenameXY(const std::string& filename, SINT16* scx, SINT16* scy)
{
	UINT32 n = filename.rfind(PATH_SEPARATOR) + 1 ;
	if ( n >= filename.length() )
		n = 0 ;
	const UINT32 assigned = sscanf(filename.c_str() + n, "%hx_%hx", scx, scy) ;
	return ( assigned == 2 ) ;
}

//////////////////////////////////////////////////////////////////////
// Read the file name vector

static void
_readSupercells()
{
	std::ifstream importList ;
	std::string filename;

	filename = cv_dataDir;
	filename += PATH_SEPARATOR;
	filename += cv_importFile;
	importList.open(filename.c_str(), std::ifstream::in) ;
	if ( !importList.is_open() )
		LogAbort("Unable to open sdm import list: %s", filename.c_str());

	bool errors = false ;

	while ( importList.good() )
	{
		filename = "" ;
		importList >> filename ;
		if ( filename.length() == 0 || filename[0] == '#' )
			continue ;

		const char* cFilename = filename.c_str() ;
		SINT16 scx = 0, scy = 0 ;
		if ( !_getFilenameXY(filename, &scx, &scy) )
			LogAbort("Invalid sdm filename: %s", cFilename) ;

		SINT32 parseResult = sdmOpenFile(cFilename, SDMPARSE_ALL) ;
		if ( parseResult != 0 )
			LogAbort("%s: sdmOpenFile failed %d", cFilename, parseResult) ;

		Supercell& sc = g_supercells[std::pair<SINT16,SINT16>(scx,scy)] ;
		if ( sc.x != 0 || sc.y != 0 )
			LogAbort("Supercell already loaded? %s [%d,%d : %d,%d]", filename.c_str(), scx, scy, sc.x, sc.y) ;

		if ( sc.initialize(scx, scy, cFilename) == false )
			errors = true ;

		sdmCloseFile() ;
	}

	importList.close() ;

	if ( errors == true )
		dbugLog(_FLA, "Errors loading supercell data") ;

	sdmFinish() ;
}

//////////////////////////////////////////////////////////////////////
// Read the supercells listed in filenames

//////////////////////////////////////////////////////////////////////

CPStruct*
strtImportStratData()
{
	try
	{
		_readSupercells() ;

		return strtInitializeStratData() ;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ;
		return NULL ;
	}
}

