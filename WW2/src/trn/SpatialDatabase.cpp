#include "syscfg.h" // Path is project specific.
/*
 *  SpatialDatabase.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 5/10/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#ifdef SPATIAL_DB

#include "SpatialDatabase.h"
#include "obmain.h"

class CSpeedWind;

#include "treeData.h"

const int RECORD_POOL_SIZE = 10000;
const int CHUNK_POOL_SIZE = 5000;

//--------------------------------------------------------

SpatialDatabase::SpatialDatabase()
{
}

//--------------------------------------------------------

void SpatialDatabase::emptySupercell( TERRAIN_DETAIL* terrainDetail )
{
    // go through the supercell, return all SD_Records, return all chunks
}

//--------------------------------------------------------

void SpatialDatabase::removeObject( OBJECT* obj ) // just mark as removed
{
    if ( obj && obj->sdbInfo )
    {
        assert( ((SD_Record*)obj->sdbInfo)->data == (void*) obj );
        ((SD_Record*)obj->sdbInfo)->flags = SDB_Flags_Removed;
    }
}

//--------------------------------------------------------

void SpatialDatabase::removeTree( Tree* tre ) // just mark as removed
{
    if ( tre && tre->sdbInfo )
    {
        assert( ((SD_Record*)tre->sdbInfo)->data == (void*) tre );
        ((SD_Record*)tre->sdbInfo)->flags = SDB_Flags_Removed;
    }
}

//--------------------------------------------------------

void SpatialDatabase::addObject( OBJECT* obj, double xmin, double ymin, double xmax, double ymax,
                                double altitude, TERRAIN_DETAIL* terrainDetail, bool isTile )
{
}

//--------------------------------------------------------

void SpatialDatabase::addTree( Tree* tre, double xmin, double ymin, double xmax, double ymax,
                              double altitude, TERRAIN_DETAIL* terrainDetail, bool isNotTree )
{
}

//--------------------------------------------------------

SD_Record* SpatialDatabase::newRecord()
{
    if ( nextFreeRecord != NULL )
    {
        SD_Record* r = nextFreeRecord;
        nextFreeRecord = (SD_Record*) r->data;
        r->data = NULL;
        return r;
    }

    SD_Record_Vector* v = new SD_Record_Vector();
    v->resize( RECORD_POOL_SIZE );
    recordStorage.push_back(v);

    for(size_t i=0; i<RECORD_POOL_SIZE-1; i++ )
    {
        (*v)[i].data = &(*v)[i+1];
    }
    nextFreeRecord = &(*v)[0];
    (*v)[RECORD_POOL_SIZE-1].data = NULL;

    return newRecord();
}

//--------------------------------------------------------

void SpatialDatabase::returnRecord(SD_Record* record)
{
    record->data = (void*)nextFreeRecord;
    nextFreeRecord = record;
}

//--------------------------------------------------------

SD_GridChunk* SpatialDatabase::newChunk()
{
    if ( nextFreeChunk != NULL )
    {
        SD_GridChunk* r = nextFreeChunk;
        nextFreeChunk = (SD_GridChunk*) r->next;
        r->next = NULL;
        return r;
    }

    SD_GridChunk_Vector* v = new SD_GridChunk_Vector();
    v->resize( CHUNK_POOL_SIZE );
    chunkStorage.push_back(v);

    for(size_t i=0; i<CHUNK_POOL_SIZE-1; i++ )
    {
        (*v)[i].next = &(*v)[i+1];
    }
    nextFreeChunk = &(*v)[0];
    (*v)[CHUNK_POOL_SIZE-1].next = NULL;

    return newChunk();
}

//--------------------------------------------------------

void SpatialDatabase::returnChunk(SD_GridChunk* chunk)
{
    chunk->next = nextFreeChunk;
    nextFreeChunk = chunk;
}

#endif
