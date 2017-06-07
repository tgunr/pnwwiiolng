#include "syscfg.h" // Path is project specific.
/*
 *  SpatialDB.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 5/2/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#ifdef SPATIAL_DB

#define xSPATIAL_DB_OFF

class CSpeedWind;

#include "SpatialDB.h"
#include "SpatialDatabase.h"
#include "wwii/wwiiHash.h"
#include <vector>
#include "obmain.h"
#include "treeData.h"
#include "draw3d.h"
#include "trnvars.h"
#include "persmain.h"

bool SDB_DrawDebugFlag=TRUE; // dont release this!

static SpatialDatabase database;

void SDB_Empty_Supercell( TERRAIN_DETAIL* terrainDetail )
{
    database.emptySupercell(terrainDetail);
}

void SDB_RemoveObject( OBJECT* obj )
{
    database.removeObject(obj);
}

void SDB_RemoveTree( Tree* tre )
{
    database.removeTree(tre);
}

void SDB_AddObject( OBJECT* obj, double xmin, double ymin, double xmax, double ymax, double altitude, TERRAIN_DETAIL* terrainDetail, bool isTile )
{
    database.addObject(obj,xmin,ymin,xmax,ymax,altitude,terrainDetail,isTile);
}

void SDB_AddTree( Tree* tre, double xmin, double ymin, double xmax, double ymax, double altitude, TERRAIN_DETAIL* terrainDetail, bool isNotTree )
{
    database.addTree(tre,xmin,ymin,xmax,ymax,altitude,terrainDetail,isNotTree);
}

void SDB_DrawDebug()
{
}

#endif
