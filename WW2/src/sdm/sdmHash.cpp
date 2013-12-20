#include <syscfg.h> // Path is project specific.
// Created: Tue Feb 20 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "dbug/dbug.h"

#include <string.h>
#include <map>

#include "mem/mem.h"
#include "strt/strtDTL.h"
#include "sdm/sdmHash.h"

// temp struct for internal preprocessing
typedef struct
{
	SINT32 numBuildings;
	SINT16	bid[MAX_BUILDINGS_PER_CELL];
} TEMP_BUILDING_LIST;

static TEMP_BUILDING_LIST _tempTable[CELLS_PER_SIDE][CELLS_PER_SIDE];

// allocates a facility nodes for each facility and a building list for each
// facility that is filled out when sdmCreateBuildingTable is called.
STRAT_FACILITY_TABLE*
sdmCreateFacilityTable(FACILITY* const fac, const size_t numFacilities, const size_t numCPs)
{
	// Bridges get fake facilities, so we have to look at CPs.
	const size_t potentialFacs = numFacilities + numCPs;
	if ( (fac == NULL) | (potentialFacs <= 0) )
		return NULL;

	const size_t HeaderNodeSize = sizeof(STRAT_FACILITY_TABLE);
	const size_t FacilityNodeListSize = sizeof(STRAT_FACILITY_NODE) * potentialFacs;
	const size_t BuildingTableSize = sizeof(SINT16) * MAX_BUILD_PER_FAC;
	const size_t BuildingTableListSize = BuildingTableSize * potentialFacs;

	const size_t AllocationSize = HeaderNodeSize + FacilityNodeListSize + BuildingTableListSize;

	UCHAR* base = (UCHAR*)memCalloc(1, AllocationSize);
	STRAT_FACILITY_TABLE* table = (STRAT_FACILITY_TABLE*)(base);
	STRAT_FACILITY_NODE* facility = (STRAT_FACILITY_NODE*)(base + HeaderNodeSize);
	UCHAR* bidList = (base + HeaderNodeSize + FacilityNodeListSize);

	table->facility = facility;
	table->numFacilities = numFacilities;

	for ( size_t i = 0; i < numFacilities; ++i )
	{
		facility[i].id = fac[i].oid.id;
		facility[i].bidList.bid = (SINT16*)bidList;
		facility[i].bidList.numBuildings = 0;

		bidList += BuildingTableSize;
	}

	return table;
}

STRAT_BUILDING_LIST * sdmGetFaciltyBidList(STRAT_FACILITY_TABLE* facTable, SINT16 id)
{
	for (SINT32 i = 0; i < facTable->numFacilities; ++i)
	{
		if (id == facTable->facility[i].id)
			return &facTable->facility[i].bidList;
	}

	return NULL;
}

static LOGICAL _addBuildingToFacTable(STRAT_FACILITY_TABLE* facTable, const BUILDING& building)
{
	const SINT16 facID = building.pid.id;
	const SINT32 bid = building.oid.id;
	STRAT_FACILITY_NODE* fac;
	SINT32 i;

	for(i = 0; i < facTable->numFacilities; i++)
	{
		fac = &facTable->facility[i];
		if (facID == fac->id)
		{
			fac->bidList.bid[fac->bidList.numBuildings++] = static_cast<SINT16>(bid);
			return (TRUE);
		}
	}

	return FALSE;
}

// Creates a cell basesd table with each cell contatining a list of buildings in
// that cell. In addition this function fills out the facility table.
STRAT_BUILDING_LIST*
sdmCreateBuildingTable(STRAT_FACILITY_TABLE* facTable, BUILDING* bd,SINT32 numBuildings,STRAT_BUILDING_HASH* bldHash, UINT32* errors)
{
	TEMP_BUILDING_LIST* cell;
	STRAT_BUILDING_LIST* table, *tcell;
	SINT32 i,x,y,sx,sy;

    static const char builder[] = "builder";

	memset(_tempTable,0,sizeof(_tempTable));

	sx = (bd[0].posx/SUPERCELL_SIZE_M);
	sy = (bd[0].posy/SUPERCELL_SIZE_M);

#ifdef SDM_DEBUG_EXTREAM
	dbugLog(_FLW,"adding buildings to sc %d:%d",sx,sy);
#endif

	std::map<SINT16, STRAT_BUILDING> bldgMap;

	for (i = 0; i< facTable->numFacilities; i++)
	{
		const SINT16 bid = facTable->facility[i].id;
		STRAT_BUILDING& bldg = bldgMap[bid];
		bldg.bid = bid;
		bldg.isFacility = TRUE;
		bldg.pobj = (void*) 1;	// Really?
	}

	for(i=0;i<numBuildings;i++)
	{
		if (! _addBuildingToFacTable(facTable, bd[i]))
		{
#if platBuild == TEUL_SERVER
			const BUILDING& building = bd[i];
			strtDataErrorParentFacility(building.name, building.fileName, sx, sy, building.oid.id, static_cast<UINT16>(i), building.pid.id);
#else
			dbugLog(_FLA,"sc:%d,%d can't find facility:%d for building:%d (%s [%s])"
						, sx, sy
						, bd[i].pid.id
						, bd[i].oid.id
						, bd[i].name
						, bd[i].fileName
						);
#endif
			if(errors) (*errors)++;
			continue;
		}

        // HACK! for builder statues.
		const SINT16 bid = bd[i].oid.id;
		STRAT_BUILDING& bldg = bldgMap[bid];

		bldg.bid = bid;
		bldg.pobj = (void*) 2;	// Really?
        if(!strncmp(bd[i].name, builder, 7))
        {
            bldg.isBuilder = TRUE;
        }

		sx = (bd[i].posx / SUPERCELL_SIZE_M);
		sy = (bd[i].posy / SUPERCELL_SIZE_M);
		x = ((bd[i].posx / 800) & 0x0f) >> CELL_RES_ADJUST;
		y = ((bd[i].posy / 800) & 0x0f) >> CELL_RES_ADJUST;
		cell = &_tempTable[x][y];
		cell->bid[cell->numBuildings++] = bd[i].oid.id;
//		dbugLog(_FLW,"adding building %d to pos:%d,%d sc:%d,%d cell:%d,%d"
//					,bd[i].oid.id,bd[i].posx,bd[i].posy,sx,sy,x,y);
	}

	table = (STRAT_BUILDING_LIST*)memAlloc(CELLS_PER_SUPER_CELL * sizeof(STRAT_BUILDING_LIST));
	memset(table,0,(CELLS_PER_SUPER_CELL * sizeof(STRAT_BUILDING_LIST)));

	for(x = 0; x < CELLS_PER_SIDE; x++)
	{
		for(y = 0; y < CELLS_PER_SIDE;y++)
		{
			cell = &_tempTable[x][y];
			tcell = &table[(x * CELLS_PER_SIDE) + y];
			if (cell->numBuildings > 0)
			{
//				Write_Debug("adding %d buildings to sc:%d,%d cell:%d,%d"
//							,cell->numBuildings,sx,sy,x,y);
				tcell->bid = (SINT16*)memAlloc(cell->numBuildings * sizeof(STRAT_BUILDING_LIST));
				memcpy(tcell->bid,cell->bid,(cell->numBuildings * sizeof(STRAT_BUILDING_LIST)));
				tcell->numBuildings = cell->numBuildings;
			}
		}
	}

	if (bldHash)
	{
		const size_t maxBldgId = (size_t)(bldgMap.rbegin()->first) + 1;

		bldHash->maxBldID = maxBldgId;

		bldHash->building = (STRAT_BUILDING*)memAlloc(maxBldgId * sizeof(STRAT_BUILDING));

		for ( std::map<SINT16, STRAT_BUILDING>::iterator it = bldgMap.begin(); it != bldgMap.end(); ++it )
		{
			bldHash->building[it->first] = it->second;
		}
	}

#ifdef HEAVY_DEBUG
	for(x = 0; x < CELLS_PER_SIDE; x++)
	{
		for(y = 0; y < CELLS_PER_SIDE;y++)
		{
			tcell = &table[(x * CELLS_PER_SIDE) + y];
			if (tcell->numBuildings > 0)
			{
				Write_Debug("sucessfully added %d buildings to sc:%d,%d cell:%d,%d"
							,tcell->numBuildings,sx,sy,x,y);
			}

		}
	}
#endif

	return table;
}
