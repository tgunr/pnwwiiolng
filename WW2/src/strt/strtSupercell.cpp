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
		cps = nullptr;
	}
	if ( facilities )
	{
		delete [] facilities;
		facilities = nullptr;
	}
	if ( objects )
	{
		memFree(objects);
		objects = nullptr;
	}
	if ( links )
	{
		memFree(links);
		links = nullptr;
	}
	if ( index )
	{
		memFree(index);
		links = nullptr;
	}
	if ( facilityTable )
	{
		memFree(facilityTable);
		facilityTable = nullptr;
	}
	if ( buildingTable )
	{
		buildingTable->destroy(buildingTable);
		buildingTable = nullptr;
	}
}

