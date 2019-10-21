#pragma once
// Date: Tue Nov  7 18:40:05 2000
//
// Constants used by the strat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

namespace Strat
{
	enum { CallbackX = 206 };
}

enum {
	//Messages start here
	STRT_SYNC_DAMAGE_LIST_Y = 0,
	STRT_SUPERCELL_Y,
	STRT_FACILITY_WEAP_LIST_Y,
	STRT_UPDATE_Y,
	STRT_MM_POST_REQ_Y,

	STRT_MM_POST_ACCEPT_Y,		//5
	STRT_MM_POST_DENY_Y,
	STRT_MM_POST_LIST_Y,
	STRT_MM_JOIN_REQ_Y,
	STRT_MM_JOIN_ACCEPT_Y,

	STRT_MM_JOIN_DENY_Y,		//10
	STRT_MM_POST_EXPIRED_Y,
	STRT_MM_SPAWN_GRANT_Y,
	UNUSED_13,
	STRT_MM_POST_CANCEL_Y,

	STRT_SYNC_WEAPONS_RETURN_Y,	//15
	STRT_BROADCAST_TIME_UPDATE_Y,
	STRT_MM_SPAWN_CANCEL_Y,
	STRT_MM_CREW_BAILED_Y,
	STRT_MM_CAPACITY_Y,

	STRT_SYNC_CPS_Y,			//20
	STRT_SYNC_FACILITIES_Y,
	STRT_SYNC_BUILDINGS_Y,
	STRT_MM_POST_INFO_UPDATE_Y,
	STRT_MM_JOIN_LIST_Y,

	STRT_MM_LEAVE_LIST_Y,
	STRT_ENEMY_REPORT_Y,
	STRT_CAPTURE_CONTRIB_Y,
	STRT_DAMAGE_Y,
	STRT_BROADCAST_CONFIG_UPDATE_Y,

	// 30
	STRT_MM_DAMAGE_EVENT_Y,
	STRT_MM_SERIAL_REQUEST_Y,
	STRT_ENEMY_SPOTTED_Y,
	STRT_CP_FACILITIES_Y,
	STRT_POPULATION_NOTICE_Y,			// For chat to send pop notices.
	STRT_ATTACK_ORDER_Y,				// Chat->Strat attack orders.
	STRT_MM_GARRISON_COUNTRY_CHANGE_Y,

	//Add stuff here
	MAX_STRAT_Y
};

//The period, in* MILLISECONDS* that a posted weapon exists on the host
//#define STRAT_POSTED_WEAPONS_EXPIRE_AFTER 300 * 100
//#define STRAT_POSTED_WEAPONS_EXPIRE_AFTER 30 * 100
#define STRAT_POSTED_WEAPONS_EXPIRE_AFTER (8 * 100)
//The period, in* SECONDS* that should be between time updates
#define STRAT_TIME_UPDATE_FREQUENCY (60)

//The period, in* REAL* minutes, that DAWN, DAY, DUSK, and NIGHT should last
#define STRAT_DAWN_TIME		15
#define STRAT_DAY_TIME		240
#define STRAT_DUSK_TIME		15
#define STRAT_NIGHT_TIME	30

//The port upon which the master strat server will broadcast all of its
//	updates
#define STRAT_BROADCAST_ADDRESS	"192.168.1.255"

//The timeout for the strat system on all its connections
#define STRAT_TIMEOUT			500000

//Maximum number of "objects" to send in the sync messages
#define STRT_CPS_PER_SYNC			40
#define STRT_FACILITIES_PER_SYNC	90
#define STRT_BUILDINGS_PER_SYNC		640
