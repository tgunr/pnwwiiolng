#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 30 12:13:30 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "miss/missionUtils.h"
#include "teul/teulKit.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiTypes.h"			// For FULL_ID
#include "miss/missionTypes.h"

//Converts a FULL_OBJ_ID to FULL_ID
FULL_ID fullObjIDtoFullID(FULL_OBJ_ID fullObjID)
{
	FULL_ID fullID;
	fullID.sx = fullObjID.cell.ox >> 4;
	fullID.sy = fullObjID.cell.oy >> 4;
	fullID.id = fullObjID.id;
	fullID.type = 0;

	return fullID;
}

FULL_OBJ_ID fudgeFullObj(FULL_ID fullID)
{
	FULL_OBJ_ID foi;

	foi.cell.ox = fullID.sx << 4;
	foi.cell.oy = fullID.sy << 4;
	foi.id = fullID.id;

	return foi;
}

FULL_OBJ_ID* populateFullObjID(FULL_OBJ_ID* foid, SINT32 sx, SINT32 sy, REL_OBJ_ID roid)
{
	if ( foid == nullptr )
	{
#ifdef DEBUG
		dbugLog(_FLW, "called %s with null foid", __FUNCTION__);
#endif
		return nullptr;
	}

	foid->cell.ox = sx << 4;
	foid->cell.oy = sy << 4;
	foid->id = roid.id;
	foid->objClass = roid.objClass;
	foid->objType = roid.objType;

	return foid;
}

void printMissionSummary(MISSION_SUMMARY* summary)
{
#ifdef DEBUG
	dbugLog(_FLWB,"author: %s missionID %d || ", summary->callsign,summary->missionID);
#else
	dbugLog(_FLWB,"missionID %d", summary->missionID);
#endif
}

//this really shouldn't be used.  it's only a placeholder until meat gets the
//strtIO stuff converted over to FULL_OBJ_ID
FULL_ID* convertFullObjToFull(FULL_OBJ_ID* ptr)
{
static FULL_ID full;

	full.sx=ptr->cell.ox;
	full.sy=ptr->cell.oy;
	full.id=ptr->id;
	full.type=ptr->objType;

	return( &full );
}

//this really shouldn't be used.  it's only a placeholder until meat gets the
//strtIO stuff converted over to FULL_OBJ_ID
FULL_OBJ_ID* convertFullToFullObj(FULL_ID* ptr)
{
static FULL_OBJ_ID fullObj;

	fullObj.cell.ox=ptr->sx;
	fullObj.cell.oy=ptr->sy;
	fullObj.id=ptr->id;
	fullObj.objType=(SINT8)ptr->type;

	return( &fullObj);
}

void printFULL_OBJ_ID(FULL_OBJ_ID* fullObjID)
{
#ifdef DEBUG
	dbugLog(_FLWB, "ox: %d, oy: %d, id: %d, objClass: %d, objType: %d"
			, fullObjID->cell.ox, fullObjID->cell.oy, fullObjID->id
			, fullObjID->objClass, fullObjID->objType );
#else
	dbugWrite(_FLWB, "ox: %d, oy: %d, id: %d, objClass: %d, objType: %d"
			, fullObjID->cell.ox, fullObjID->cell.oy, fullObjID->id
			, fullObjID->objClass, fullObjID->objType );
#endif
}
