#include <syscfg.h> // Path is project specific.
// Created: Tue Feb 20 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include <map>
#include <array>

#include "mem/mem.h"
#include "strt/strtDataErrors.h"
#include "strt/strtDTL.h"
#include "sdm/sdmHash.h"

// allocates a facility nodes for each facility and a building list for each
// facility that is filled out when sdmCreateBuildingTable is called.
STRAT_FACILITY_TABLE*
sdmCreateFacilityTable(FACILITY* const fac, const SINT32 numFacilities, CP* const cps, const SINT32 numCPs)
{
	// Bridges get fake facilities, so we have to look at CPs.
	const size_t potentialFacs = numFacilities + numCPs;
	if ( (fac == nullptr) | (potentialFacs <= 0) )
		return nullptr;

	const size_t HeaderNodeSize = sizeof(STRAT_FACILITY_TABLE);
	const size_t FacilityNodeListSize = sizeof(STRAT_FACILITY_NODE) * potentialFacs;
	const size_t BuildingTableSize = sizeof(STRAT_BUILDING_LIST::bid) * MAX_BUILD_PER_FAC;
	const size_t BuildingTableListSize = BuildingTableSize * potentialFacs;

	const size_t AllocationSize = HeaderNodeSize + FacilityNodeListSize + BuildingTableListSize;

	UCHAR* base = (UCHAR*)memCalloc(1, AllocationSize);
	STRAT_FACILITY_TABLE* table = (STRAT_FACILITY_TABLE*)(base);
	STRAT_FACILITY_NODE* facility = (STRAT_FACILITY_NODE*)(base + HeaderNodeSize);
	UCHAR* bidList = (base + HeaderNodeSize + FacilityNodeListSize);

	table->facility = facility;
	table->numFacilities = numFacilities + numCPs;

	for ( SINT32 i = 0; i < numCPs; ++i ) {
		facility[i].id = cps[i].oid.id;
		facility[i].bidList.bid = (SINT16*)bidList;
		facility[i].bidList.numBuildings = 0;

		bidList += BuildingTableSize;
	}

	for ( SINT32 i = 0, j = numCPs; i < numFacilities; ++i, ++j ) {
		facility[j].id = fac[i].oid.id;
		facility[j].bidList.bid = (SINT16*)bidList;
		facility[j].bidList.numBuildings = 0;

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

	return nullptr;
}

static bool _addBuildingToFacTable(STRAT_FACILITY_TABLE* facTable, const BUILDING& building)
{
	const SINT16 facID = building.pid.id;
	const SINT32 bid = building.oid.id;

	for (auto it = facTable->facility, end = it + facTable->numFacilities; it < end; ++it)
	{
		if (facID == it->id)
		{
			it->bidList.bid[it->bidList.numBuildings++] = static_cast<SINT16>(bid);
			return true;
		}
	}

	return false;
}

struct BuildingsByCell
{
	using Cell = std::vector<UINT16>;
	using Grid = std::array<Cell, CELLS_PER_SIDE * CELLS_PER_SIDE>;
	Grid m_grid;

	BuildingsByCell() noexcept {}

	Cell& GetCell(size_t x, size_t y) noexcept
	{
		return m_grid[(x * CELLS_PER_SIDE) + y];
	}
};

// Creates a cell based table with each cell containing a list of buildings in
// that cell. In addition this function fills out the facility table.
STRAT_BUILDING_LIST*
sdmCreateBuildingTable(STRAT_FACILITY_TABLE* facTable, BUILDING* bd, SINT32 numBuildings, STRAT_BUILDING_HASH* bldHash, UINT32* errors)
{
    static const char builder[] = "builder";

	BuildingsByCell bbc;

	SINT32 sx = (bd[0].posx/SUPERCELL_SIZE_M);
	SINT32 sy = (bd[0].posy/SUPERCELL_SIZE_M);

#ifdef SDM_DEBUG_EXTREAM
	dbugLog(_FLW,"adding buildings to sc %d:%d",sx,sy);
#endif

	// TODO: Is there any reason this isn't a vector?
	// kfsone: It's sparse because of the terrain editor
	std::map<SINT16, STRAT_BUILDING> bldgMap;

	for (SINT32 i = 0; i< facTable->numFacilities; ++i)
	{
		const SINT16 bid = facTable->facility[i].id;
		STRAT_BUILDING& bldg = bldgMap[bid];
		bldg.bid = bid;
		bldg.isFacility = TRUE;
		bldg.pobj = (void*) 1;	// Really?
	}

	for(SINT32 i = 0; i < numBuildings; ++i)
	{
		if (! _addBuildingToFacTable(facTable, bd[i]))
		{
#if platBuild == BUILD_SERVER
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
			if(errors)
				(*errors)++;
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
		size_t x = ((bd[i].posx / 800) & 0x0f) >> CELL_RES_ADJUST;
		size_t y = ((bd[i].posy / 800) & 0x0f) >> CELL_RES_ADJUST;
		auto& cell = bbc.GetCell(x, y);
		if (cell.empty())
			cell.reserve(MAX_BUILDINGS_PER_CELL);
		cell.push_back(bd[i].oid.id);
//		dbugLog(_FLW,"adding building %d to pos:%d,%d sc:%d,%d cell:%d,%d"
//					,bd[i].oid.id,bd[i].posx,bd[i].posy,sx,sy,x,y);
	}

	STRAT_BUILDING_LIST* table = (STRAT_BUILDING_LIST*)memCalloc(CELLS_PER_SUPER_CELL, sizeof(STRAT_BUILDING_LIST));

	for(size_t x = 0; x < CELLS_PER_SIDE; x++)
	{
		for (size_t y = 0; y < CELLS_PER_SIDE; y++)
		{
			auto& src = bbc.GetCell(x, y);
			if (!src.empty())
			{
				auto& dst = table[(x * CELLS_PER_SIDE) + y];
//				Write_Debug("adding %d buildings to sc:%d,%d cell:%d,%d"
//							,cell->numBuildings,sx,sy,x,y);
				dst.numBuildings = src.size();
				const size_t dataSize = sizeof(*dst.bid) * dst.numBuildings;
				dst.bid = static_cast<decltype(dst.bid)>(memAlloc(dataSize));
				memcpy(dst.bid, src.data(), dataSize);
			}
		}
	}

	if ( bldHash ) {
		const size_t maxBldgId = (size_t)(bldgMap.rbegin()->first) + 1;

		bldHash->maxBldID = maxBldgId;

		delete[] bldHash->building;
		bldHash->building = new STRAT_BUILDING[maxBldgId];

		for ( auto it = bldgMap.begin(), end = bldgMap.end(); it != end; ++it ) {
			assert ( 0 < it->first && (size_t)it->first < (size_t)maxBldgId );
			bldHash->building[it->first] = it->second;
		}
	}

#ifdef HEAVY_DEBUG
	for(size_t x = 0; x < CELLS_PER_SIDE; x++)
	{
		for(size_t y = 0; y < CELLS_PER_SIDE;y++)
		{
			auto& cell = table[(x * CELLS_PER_SIDE) + y];
			if (cell.numBuildings > 0)
				Write_Debug("successfully added %d buildings to sc:%d,%d cell:%" PRSizeT ",%" PRSizeT "", cell.numBuildings, sx, sy, x, y);
		}
	}
#endif

	return table;
}
