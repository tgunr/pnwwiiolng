#pragma once
// Created: Mon Jan 21 16:55:26 2001
//
// This module conatains general purpose hash routines.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "mem/mem.h"	// For memFree()

#define CELL_RES_ADJUST 2
#define CELLS_PER_SIDE 4
#define CELLS_PER_SUPER_CELL CELLS_PER_SIDE * CELLS_PER_SIDE
#define OCTETS_PER_CELL	16
#define MAX_BUILDINGS_PER_CELL 256 * OCTETS_PER_CELL
#define MAX_FACILITIES 2048
#define MAX_BUILD_PER_FAC 2048

struct STRAT_BUILDING
{
	LOGICAL	loaded;
	LOGICAL	isFacility;
    LOGICAL isBuilder;
	void*	pobj;
	SINT16	bid;
	UCHAR	owner;
	UCHAR	level;
} ;

struct STRAT_BUILDING_HASH
{
	SINT32 maxBldID;						// max bid in the array
	STRAT_BUILDING* building;			// pointer to an array of buildings
} ;

struct STRAT_BUILDING_ARRAY
{
	SINT32 maxBldID;						// max bid in the array
	STRAT_BUILDING building[60000];			// pointer to an array of buildings
} ;


struct STRAT_BUILDING_LIST
{
	SINT32 numBuildings;				// number of buildings in the array
	SINT16*	bid;						// pointer to an array of building ids

	static void destroy(STRAT_BUILDING_LIST* list)
	{
		if ( list == NULL )
			return ;

		for ( size_t x = 0 ; x < (size_t)CELLS_PER_SIDE ; ++x )
		{
			for ( size_t y = 0 ; y < (size_t)CELLS_PER_SIDE ; ++y )
			{
				STRAT_BUILDING_LIST* cell = &list[(x * CELLS_PER_SIDE) + y] ;
				if ( cell->bid != NULL )
				{
					memFree(cell->bid) ;
					cell->bid = NULL ;
				}
			}
		}

		memFree(list) ;
	}
} ;

struct STRAT_FACILITY_NODE
{
	SINT16	id;							// id of facility
	STRAT_BUILDING_LIST	bidList;		// building id list that belong to facility
} ;

struct STRAT_FACILITY_TABLE
{
	STRAT_FACILITY_NODE* facility;		// points to an allocated array of facility nodes
	SINT32				numFacilities;	// number of facilities
} ;

// allocates a facility nodes for each facility and a building list for each
// facility that is filled out when sdmCreateBuildingTable is called.
extern STRAT_FACILITY_TABLE* sdmCreateFacilityTable(FACILITY* const fac, const size_t numFacilities, const size_t numCPs);

// returns the building list fro the given facility id from the facility table
// null if no found
extern STRAT_BUILDING_LIST * sdmGetFaciltyBidList(STRAT_FACILITY_TABLE* facTable, SINT16 id);

// Creates a cell basesd table with each cell contatining a list of buildings in
// that cell. In addition this function fills out the facility table.
// table is allocated as one chunck so it must be acessed with sdmGetCellList.
//extern STRAT_BUILDING_LIST* sdmCreateBuildingTable(STRAT_FACILITY_TABLE* facTable, BUILDING* bd,SINT32 numBuildings);

extern STRAT_BUILDING_LIST* sdmCreateBuildingTable(STRAT_FACILITY_TABLE* facTable
													, BUILDING* bd
													, SINT32 numBuildings
													, STRAT_BUILDING_HASH* bldHash
													, UINT32* errors = NULL
													);

// x,y are the base octet address of the cell
// table is the building list table from the supercell struct
// returns the cell building list for the location specified by x,y
static inline STRAT_BUILDING_LIST* sdmGetCellListFromOctet(STRAT_BUILDING_LIST* table, SINT16 x, SINT16 y)
{
	x = (x & 0x0f) >> CELL_RES_ADJUST;
	y = (y & 0x0f) >> CELL_RES_ADJUST;
	return &table[(x * CELLS_PER_SIDE) + y];
}

// x,y are the base octet address of the cell
// table is the building list table from the supercell struct
// returns the cell building list for the location specified by x,y
static inline STRAT_BUILDING_LIST* sdmGetCellList(STRAT_BUILDING_LIST* table, SINT16 x, SINT16 y)
{
	return &table[(x * CELLS_PER_SIDE) + y];
}

extern void strtDataErrorParentFacility(const char* buildingName, const char* filename, SINT32 scx, SINT32 scy, UINT16 bid, UINT16 bno, UINT16 fid) ;
