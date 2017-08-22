// Created: Tue Jan 30 12:13:30 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "syscfg.h"
#include "miss/missionUtils.h"
#include "teul/teulKit.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiTypes.h"			// For FULL_ID
#include "miss/missionTypes.h"

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
