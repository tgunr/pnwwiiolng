#pragma once
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	This is where all declarations for pack routines for universal
//	game specific object structures thatwill be transmited
//	accross the network must reside or be included into.  All pack
//	routines must have an entry in this file.
//
//	Originating Programmer : Martin J. Poulin
//
//	Rev 0.1  11/08/99

// x column general category
namespace Comm
{
	enum { CallbackX = 140 } ;
}

enum
{
	GRANT_VEHICLE_JOIN_Y		= 20
,	UPDATE_Y
,	WORLD_UPDATE_Y
,	CLIENT_KILL_Y
,	CLIENT_TEXT_Y
,	HOST_TEXT_Y
,	CLIENT_HIT_Y
,	HOST_HIT_Y
,	VEH_JOIN_REQUEST_Y
,	GAME_TIME_UPDATE_Y

,	PERSONA_EXIT_Y				= 30
,	ACTIVATE_CLIENT_Y
,	PLAYER_EXIT_Y
,	REMOVE_LIST_Y
,	SPAWN_DEPLOYMENT_Y		// For spawn deployment messages
,	KILL_NOTICE_Y			// Player notifying us of their killer
,	RESUPPLY_ISSUE_Y		// Notify a player they got resupplied
,	MOVE_Y					// Forced "move" command from the host
,	DESPAWNED_Y
,	INF_DEATH_Y				// Notification an infantry was killed.


// WWIIOL specific.
,	STRAT_EVENTS_Y	=	128
,	BCAST_VEH_REMOVE_Y		// Vehicle removal
,	HOST_HIT_FWD_Y			// Host forwarding a hit.
,	DEATH_BY_BUILDING_Y		// You've been killed by a building
,	DAMAGE_LIST_Y			// new damage list .. for strat updater
,	CAPTURE_STATE_Y
,	BCAST_VEH_UPDATE_Y
,	RESYNC_VEHICLES_Y
};
