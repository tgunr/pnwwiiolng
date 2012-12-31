#pragma once
// Created: Tue Oct 31 14:40:41 CST 2006
//
// Defines the sub-types required for class Player
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSerial.h"

struct JoinRequestNode
{
	//CL linked list stuff
	JoinRequestNode* prev ;
	JoinRequestNode* next ;

	UINT32	joinRequestID;	//The "key" to our little list
							//	every joinrequest has an ID that is used
							//	to identify it.  That's what the poster
							//	front end gets instead of customerID

	UINT32	customerID;		//The customer ID of the requester.  How we
							//	find them again to let them know the
							//	answer to their request

	WEAPON_SERIAL weaponSerial;

	class Player* poster;   //Pointer back to the poster, just
									//	in case we need it
} ;

enum MM_VEH_STATE
{
///CAVEAT: Add new states in the appropriate section

	//// No vehicle yet
	MM_VEH_NONE,
	MM_VEH_POST_REQ,
	MM_VEH_POST_LIST_REQ,

	//// Vehicle reserved
	MM_VEH_RESERVED,
	MM_VEH_INXFER,
	MM_VEH_WAITING_CREW,
	MM_VEH_HAS_CREW,
	MM_VEH_JOINED,
	MM_VEH_PRERESERVED
///CAVEAT: Add new states in the appropriate section
} ;

enum MM_PERS_STATE
{
	PERS_SELECTING_PERSONA
,	PERS_SELECTING_BRIGADE
,	PERS_SELECTING_MISSION
,	PERS_AT_MISSION
,	PERS_SPAWNING				// Player has gone to cell host
,	PERS_DEBRIEFING				// Back from cell host
,	PERS_EXITING
} ;

