/*
 *  SpatialDatabase.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 5/10/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#ifndef _SPATIALDATABASE_H_
#define _SPATIALDATABASE_H_
#ifdef SPATIAL_DB
#include "sdm.h"
#include "SpatialDB.h"
#include <vector>
#include <utility>
#include "wwii/wwiiHash.h"

struct OBJECT;
struct Tree;
struct TERRAIN_DETAIL;

const int SDM_SUPERCELLS_ACROSS = 5;
const int SDM_SUPERCELLS_DOWN = 5;
const int GRIDS_SIZE = 100; // 100 meters per grid
const int GRIDS_PER_SIDE = 800 /GRIDS_SIZE; // 100 meters per grid
const int RECORDS_PER_CHUNK = 7;

typedef struct SD_Record
{
    int minx, miny, maxx, maxy; // in 1/4 meters global resolution, ie [s]29.2 fixed point
    void* data; // tree or OBJECT or next
    unsigned short flags;
    short alt;    // meters, only used for debug display
} SD_Record;

typedef struct SD_GridChunk
{
    SD_Record* records[RECORDS_PER_CHUNK];
    SD_GridChunk* next;
} SD_GridChunk;

typedef struct SD_Grid
{
    SD_GridChunk* chunkPtr;
} SD_Grid;

typedef struct SD_Octet
{
    SD_Grid grids[GRIDS_PER_SIDE][GRIDS_PER_SIDE];
} SD_Octet;

typedef struct SD_Supercell
{
    short supercellX, supercellY; // current mapping to this data structure
    SD_Octet octets[SDM_SUPERCELL_OCTETS_X][SDM_SUPERCELL_OCTETS_Y];
} SD_Supercell;

typedef std::vector<SD_Record> SD_Record_Vector;
typedef std::vector<SD_Record_Vector*> SD_Record_Vector_Vector;

typedef std::vector<SD_GridChunk> SD_GridChunk_Vector;
typedef std::vector<SD_GridChunk_Vector*> SD_GridChunk_Vector_Vector;

typedef stdext::hash_map<std::pair<int,int>,SD_Supercell*> SD_SuperCellMap;


class SpatialDatabase
{
public:
    SpatialDatabase();

    void removeObject( OBJECT* obj );
    void removeTree( Tree* tre );
    void addObject( OBJECT* obj, double xmin, double ymin, double xmax, double ymax,
                   double altitude, TERRAIN_DETAIL* terrainDetail, bool isTile );
    void addTree( Tree* tre, double xmin, double ymin, double xmax, double ymax,
                 double altitude, TERRAIN_DETAIL* terrainDetail, bool isNotTree );

    void emptySupercell( TERRAIN_DETAIL* terrainDetail );

private:

    SD_SuperCellMap supercells;
    SD_Record_Vector_Vector recordStorage;
    SD_Record* nextFreeRecord;

    SD_GridChunk_Vector_Vector chunkStorage;
    SD_GridChunk* nextFreeChunk;

    SD_Record* newRecord();
    void returnRecord(SD_Record* record);

    SD_GridChunk* newChunk();
    void returnChunk(SD_GridChunk* chunk);

};

#endif
#endif
