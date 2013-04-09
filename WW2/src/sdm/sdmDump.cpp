#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 13 11:58:25 2001
//
// Dump strat contents of an SDM file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "sdm/sdmParse.h"
#include "include/defs.h"
#include "strt/strtDTL.h"
#include "wwii/wwiiTypes.h"

static void
_dumpFile()
{
	const STRT_DTL_HEADER& header = sdmGetDtlHeader() ;
	printf("CPs: %u, Facilities: %u, Buildings: %u, Links: %u, Waypoints: %u\n"
			, header.numCPs
			, header.numFacilities
			, header.numBuildings
			, header.numLinks
			, header.numWaypoints
			) ;

	for ( UINT32 i = 0 ; i < header.numCPs ; ++i )
	{
		const CP* const entry = sdmGetNextCP() ;
		printf("CP relObjID %hu.%d.%d serial %u x,y (%d,%d) sub %d side %d ctry %d flgs %d name \"%s\" filename \"%s\"\n"
				, entry->oid.id, (int)entry->oid.objClass, (int)entry->oid.objType
				, entry->serial
				, entry->posx, entry->posy
				, (int)entry->subType
				, (int)entry->side, (int)entry->country
				, (int)entry->stratFlags
				, entry->name
				, entry->fileName
				) ;
	}

	for ( UINT32 i = 0 ; i < header.numFacilities ; ++i )
	{
		const FACILITY* const entry = sdmGetNextFACILITY() ;
		printf("FAC relObjID %hu.%d.%d serial %u cpid (%d,%d).%hd.%d.%d x,y (%d,%d) sub %d side %d ctry %d flags %d name \"%s\" filename \"%s\"\n"
				, entry->oid.id, (int)entry->oid.objClass, (int)entry->oid.objType
				, entry->serial
				, entry->cpid.cell.ox, entry->cpid.cell.oy
				, entry->cpid.id, (int)entry->cpid.objClass, (int)entry->cpid.objType
				, entry->posx, entry->posy
				, (int)entry->subType
				, (int)entry->originalSide, (int)entry->originalCountry
				, (int)entry->stratFlags
				, entry->name
				, entry->fileName
				) ;
	}

}

static void
_processFile(std::string filename)
{
	size_t pos = filename.rfind('/') ;
	if ( pos != std::string::npos )
		filename.erase(0, pos+1) ;

	printf("%s: ", filename.c_str()) ;

	filename.insert(0, "/terrain/") ;

	bool success = false ;
	SINT32 retval = sdmOpenFile(filename.c_str(), 0) ;
	switch (retval)
	{
		case SDMFILE_READ_ERR:
			printf("FAILED: READ ERROR.\n") ;
		break ;
		case 0:		// No Error
			printf("Loaded: OK.\n") ;
			success = true ;
		break ;
		case SDMFILE_TRANS_ERR: // Translation error, carry on.
			printf("Loaded: TransErr.\n") ;
			success = true ;
		break ;
		case SDMNO_STRAT_DATA:
			printf("Loaded: No strat data.\n") ;
		break ;

		default:
			printf("FAILED: Unknown result.\n") ;
		break ;
	}

	if ( success != false )
		_dumpFile() ;

	sdmCloseFile() ;
}

int
main(int argc, char** argv)
{
	if ( argc < 2 )
	{
		printf("Usage: %s <list of sdm files>\n", argv[0]) ;
		return 1 ;
	}

	Teul::ConfigVar::InitializeConfigVars();

	for ( int i = 1 ; i < argc ; ++i )
	{
		_processFile(argv[i]) ;
	}

	return 0 ;
}

