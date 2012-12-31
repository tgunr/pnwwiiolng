#pragma once
// Created: Sun Jan 21 23:15:25 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "wwii/wwiiFullObjID.h"
#include "mm/mmEnums.h"
#include "chat/chatHandle.h"

namespace MapMiss
{
	enum
	{
		MAX_SHORT_TITLE = 16
	,	MAX_LONG_TITLE = 64
	,	MAX_ABV_TITLE = 5
	,	MAX_HIT_ARRAY = 100
	,	MISSION_DESC_LENGTH = 480
	};
}

enum CREATOR_TYPE
{
	PLAYER_GENERATED = 0,
	COMPUTER_GENERATED,
	EXTERNALLY_GENERATED
} ;

static inline const char*
_EXPAND_CREATOR_TYPE(CREATOR_TYPE x)
{
	if ( x == PLAYER_GENERATED ) return "PLAYER_GENERATED" ;
	if ( x == COMPUTER_GENERATED ) return "COMPUTER_GENERATED" ;
	if ( x == EXTERNALLY_GENERATED ) return "EXTERNALLY_GENERATED" ;
	return "_UNKNOWN_CREATOR_TYPE_" ;
}

extern const char* _missionTypes[MAX_MISSION_TYPES] ;

//WEP_LIST_REQ
//
//the client sends this up to the mmhost to request a list of weapons
//presumably, this means they are not going to take a mission, but just
//spawn at a facility
struct WEP_LIST_REQ
{
	FULL_OBJ_ID facility;
	LOGICAL allWeapons;
	LOGICAL isRefresh ;					// True if this is a refresh request
} ;

struct MISSION_SUMMARY
{
	UINT32		missionID;
	FULL_OBJ_ID		origin;
	FULL_OBJ_ID		target;
	SINT8			missionType;  		//from MISSION_TYPE
	SINT8			subType;			// whatever you want it to be.
	bool			mobileSpawnAvailable ;	// Whether mobile spawning is enabled or not
	bool			m_canMobileSpawn ;	// Whether mobile spawning is possible.
	UINT32			timeLaunch;
	char			callsign[MAX_HANDLE_SIZE];
} ;

struct MISSION_POST_REQ
{
	FULL_OBJ_ID		origin;
	FULL_OBJ_ID		target;
	SINT8			missionType;  	//from MISSION_TYPE
	SINT8			subType;
	SINT16			descSize;		//num chars being sent in desc
	char			desc[MapMiss::MISSION_DESC_LENGTH+1];	//the text description of the mission
} ;

struct MISSION_POST_REPLY
{
	LOGICAL 		posted;		//true or false, whether the mission was posted
	UINT32			missionID;
	SINT8			reasonCode;	//if posted was false, this
								//is the reasonCode why not
} ;

struct MISS_LAUNCH_UPDATE
{
	enum STATE { INVALID, ACTIVE, INACTIVE, CLOSED } ;
	UINT32	missionID;
	UINT32	timeLaunch;
	STATE	state ;
	bool	mobileSpawnAvailable ;			// Does this mission have mobile spawning just now?
	bool	m_canMobileSpawn ;				// Mobile spawning allowed?
	bool	m_isOpen ;						// Is mission still open?
} ;

#ifndef FRONT_END
	#include "miss/missionServerTypes.h"
#endif

