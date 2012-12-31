#pragma once
// Created: Thu 2001-08-30 10:49:23
//
// enum for ranks used by the map/mission host
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.

#include "wwii/wwiiBranchEnums.h"

//used to track what state the mmHost was in when it sent a strtMMC
enum CAPACITY_REQUEST_STATE
{

	CAPACITY_BEFORE_MISSION = 0,
	CAPACITY_AFTER_MISSION
} ;

enum MISSION_TYPE
{
	POINT_ATTACK, 	//0
	POINT_DEFENSE, 	//1
	AREA_ATTACK,	//2
	AREA_DEFENSE,	//3
	RECON,			//4
	RESUPPLY,		//5
	SYSTEM_MISSION,	//6
	MAX_MISSION_TYPES
} ;

extern const char* _missionTypes[MAX_MISSION_TYPES] ;

// MISSION_CODES
//
// reasons why posting a mission might get rejected
// should these be moved to dbugExtErrors.h?
enum MISSION_CODE
{
	MC_DETAIL,						// Information only
	MC_JOIN,						// Mission join
	// All error codes below here
	MC_ERROR,						// Unknown error
	MC_NO_PERMISSION,				// they don't have high enough rank to post
	MC_BY_USER_EXISTS,				// they already posted a mission here
	MC_EXPIRED,						// a mission has expired
	MC_CAPTURED,					// facility has been captured
	MC_BRIGADE,						// Doesn't belong to your brigade
	MC_BRANCH,						// Doesn't belong to your branch
	MC_BRIGADE_INACTIVE				// No officers present to allow posting
} ;

// Result codes from attempting to load a persona
enum PERSONA_LOAD_STATUS
{
	PLS_LOADED			// Persona loaded ok
,	PLS_LOCKED			// Persona is locked
,	PLS_MISSING			// No such persona
,	PLS_ERROR			// Failed
} ;

static const UINT32 MIN_MISSION_ID = 10000U ;
static const UINT32 MAX_MISSION_ID = 4000000000U ;	// Just shy of 2^32

