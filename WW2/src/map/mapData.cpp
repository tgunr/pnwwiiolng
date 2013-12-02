#include <syscfg.h> // Path is project specific.
// Date: Thu Jun  1 13:45:40 2000
//
// Encapsulation of the strat system data. This allows me to have quick
// access to the pointers for all these strat system objects without having
// to mess with direct access stuff.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "map/mapDB.h"
#include "map/mapData.h"
#include "miss/mission.h"
#include "strt/strtData.h"
#include "strt/strtDB.h"
#include "map/mapBrigades.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtSupercell.h"
#include "strt/strtSupercells.h"
#include "map/mapPlayer.h"

//static CPStruct* stratSystem=NULL;
//static INDEX supercellIndex; //, cpIndex, facilityIndex, objectIndex, linkIndex;

//UINT64 _supercellKey(SINT16 x, SINT16 y);
//LOGICAL _initTheatreMessage(THEATRE_MAP* tm);

void
mapUpdateCP(CPStruct* cp, WWII_COUNTRY owner, WWII_COUNTRY controller, CP_STATUS_FLAGS flags, const char* from)
{
	if ( cp->country == owner && cp->controllingCountry == controller && cp->flags == flags )
		return;

	// Did it's objective status change
	if ( (cp->flags & CP_OBJECTIVES) != (flags & CP_OBJECTIVES) )
	{
		cp->flags = flags;
		missionCheckOrigins(cp);
		mapUpdateBrigadesWithMissionsAt(cp);
	}
	else
		cp->flags = flags;
}

SINT32
mapInitMapData()
{
	SUPERCELLS::iterator it;
	for ( it = g_supercells.begin(); it != g_supercells.end(); ++it )
	{
		Supercell& sc = it->second;
		for ( UINT32 n = 0; n < sc.numCPs; ++n )
		{
			CPStruct& cp = sc.cps[n];

			if ( cp.isContested() )
				cp.flags |= CP_STAT_CONTENTION;
			else
				cp.flags &= ~CP_STAT_CONTENTION;

			mapUpdateCP(&cp, cp.originalCountry, cp.controllingCountry, cp.flags & ~(CP_STAT_CONTENTION|CP_OBJECTIVES), __FUNCTION__);
		}
	}

	return 0;
}

