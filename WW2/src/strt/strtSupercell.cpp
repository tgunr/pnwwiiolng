#include <syscfg.h> // Path is project specific.
// Destructor for supercell.

#include "strt/strtSupercell.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "sdm/sdmHash.h"

Supercell::~Supercell()
{
	if ( cps )
	{
		delete [] cps;
		cps = NULL;
	}
	if ( facilities )
	{
		delete [] facilities;
		facilities = NULL;
	}
	if ( objects )
	{
		memFree(objects);
		objects = NULL;
	}
	if ( links )
	{
		memFree(links);
		links = NULL;
	}
	if ( index )
	{
		memFree(index);
		links = NULL;
	}
	if ( facilityTable )
	{
		memFree(facilityTable);
		facilityTable = NULL;
	}
	if ( buildingTable )
	{
		buildingTable->destroy(buildingTable);
		buildingTable = NULL;
	}
}

