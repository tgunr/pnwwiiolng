/*
 *  SpatialDB.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 5/2/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#ifdef SPATIAL_DB
#ifndef _SPATIALDB_H_
#define _SPATIALDB_H_

class OBJECT;
class Tree;

enum SDB_Flags
{
    SDB_Flags_None=     0,
    SDB_Flags_Tree=     0x0001,
    SDB_Flags_Bush=     0x0002,
    SDB_Flags_Clutter=  0x0004,
    SDB_Flags_Road=     0x0008,
    SDB_Flags_Tile=     0x0010,
    SDB_Flags_OBJECT=   0x0020,
    SDB_Flags_Removed=  0x0040

};

struct SDB_Box2D
{
    FLOAT32 min_x, min_y, max_x, max_y;
    bool inited;

    SDB_Box2D() { inited = false; min_x=min_y=max_x=max_y=0.0;}
    void add(FLOAT32 minx, FLOAT32 miny, FLOAT32 maxx, FLOAT32 maxy)
    {
        if ( inited )
        {
            if ( minx < min_x ) min_x = minx;
            if ( miny < min_y ) min_y = miny;
            if ( maxx > max_x ) max_x = maxx;
            if ( maxy > max_y ) max_y = maxy;
        }
        else
        {
            min_x=minx; min_y=miny; max_x=maxx; max_y=maxy;
            inited = true;
        }
    }
    void offset( FLOAT32 dx, FLOAT32 dy )
    {
        min_x += dx; max_x += dx;
        min_y += dy; max_y += dy;
    }
};

void SDB_Empty_Supercell( TERRAIN_DETAIL* terrainDetail );

void SDB_RemoveObject( OBJECT* obj );
void SDB_RemoveTree( Tree* tre );

void SDB_AddObject( OBJECT* obj, double xmin, double ymin, double xmax, double ymax, double altitude, TERRAIN_DETAIL* terrainDetail, bool isTile );
void SDB_AddTree( Tree* tre, double xmin, double ymin, double xmax, double ymax, double altitude, TERRAIN_DETAIL* terrainDetail, bool isNotTree );

void SDB_DrawDebug();

#endif
#endif
