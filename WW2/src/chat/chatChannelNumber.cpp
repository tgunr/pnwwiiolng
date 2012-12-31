#include <syscfg.h> // Path is project specific.
// Date: Fri Mar  9 14:39:13 2001
//
// This module gets channel IDs
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "chat/chatServerTypes.h"
#include "chat/chatChannelNumber.h"
#include "chat/chatChannelID.h"

static ChannelID id_zero((CHANNEL_TYPE)0) ;
static ChannelID id_admin(ADMIN_CHANNEL, 1) ;
static ChannelID id_brigade(BRIGADE_CHANNEL) ;
static ChannelID id_mission(MISSION_CHANNEL) ;
static ChannelID id_squad(SQUAD_CHANNEL) ;
static ChannelID id_gci(GCI_CHANNEL) ;
static ChannelID id_cp(CP_CHANNEL) ;
static ChannelID id_numbered(NUMBERED_CHANNEL) ;
static ChannelID id_side(SIDE_GENERAL_CHANNEL) ;

// Get the ID of the "admin" (GM) channel
const ChannelID&
chatGetAdminChannelID(void)
{
	return id_admin ;
}

// Get the ID of a specific brigade channel
const ChannelID&
chatGetBrigadeChannelID(const instanceid_t& instID, UINT32 brigadeID, WWII_SIDE side)
{
	if ( brigadeID == 0 || !isValidSide(side) )
		return id_brigade.Specify(instID, 0, side) ;
	return id_brigade.Specify(instID, brigadeID, side) ;
}

// Get the ID of a channel corresponding to a specific mission
const ChannelID&
chatGetMissionChannelID(const instanceid_t& instID, UINT32 missionID, WWII_SIDE side)
{
	if ( missionID == 0 || !isValidSide(side) )
		return id_mission.Specify(instID, 0, side) ;
	return id_mission.Specify(instID, missionID, side) ;
}

// Get the channel corresponding to a specific side+squad
const ChannelID&
chatGetSquadChannelID(const instanceid_t& instID, UINT32 squadID, WWII_SIDE side)
{
	if ( squadID == 0 || !isValidSide(side) )
		return id_squad.Specify(instID, 0, side) ;
	return id_squad.Specify(instID, squadID, side) ;
}

// Find the "HQ" channel for a specific side
const ChannelID&
chatGetGCIChannelID(const instanceid_t& instID, WWII_SIDE side)
{
	if ( !isValidSide(side) )
		return id_gci.Specify(instID, 0, side) ;
	// Use the side as an ID so that ID isn't zero
	return id_gci.Specify(instID, (UINT64)side) ;
}

// Translate a CP's full obj ID into a 48-bit key value,
// and generate a side-specific channel ID for that key
const ChannelID&
chatGetCPChannelID(const instanceid_t& instID, const FULL_OBJ_ID& cpID, WWII_SIDE side)
{
	if ( cpID.id == 0 || !isValidSide(side) )
		return id_cp.Specify(instID, 0) ;
	return id_cp.Specify(instID, cpID.Key(), side) ;
}

// Get a channel ID for a numbered radio-station channel, side filtered
const ChannelID&
chatGetNumberedChannelID(const instanceid_t& instID, UINT32 stationID, WWII_SIDE side)
{
	if ( stationID < 1 || stationID >= CHAT_MAX_NUM_STATIONS )
		return id_zero ;

	if ( !isValidSide(side) )
		return id_numbered.Specify(instID, stationID, WWII_NO_SIDE) ;

	return id_numbered.Specify(instID, stationID, side) ;
}

const ChannelID&
chatGetSideChannelID(const instanceid_t& instID, SIDE_SUB_CHANNEL subChannel, WWII_SIDE side)
{
	if ( !isValidSide(side) )
		return id_side.Specify(instID, 0, side) ;

	if ( subChannel == SIDE_SUB_INVALID || subChannel >= MAX_SIDE_SUB )
		return id_side.Specify(instID, 0, side) ;

	return id_side.Specify(instID, subChannel, side) ;
}

