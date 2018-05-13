#pragma once
// Created: Fri Mar  9 13:34:59 2001
//
// This module provides the interface for locating the* ID* of a
// channel for a certain players side, country, etc.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatDefines.h"			// For SIDE_SUB_CHANNEL
#include "wwii/wwiiOwnershipEnums.h"	// For WWII_SIDE

// Assumptions
#define CHAT_MAX_NUM_STATIONS	100

struct FULL_OBJ_ID;

class ChannelID;

const ChannelID& chatGetAdminChannelID();
const ChannelID& chatGetBrigadeChannelID(const instanceid_t instID, UINT32 brigadeID, WWII_SIDE side);
const ChannelID& chatGetMissionChannelID(const instanceid_t instID, UINT32 missionID, WWII_SIDE side);
const ChannelID& chatGetSquadChannelID(const instanceid_t instID, UINT32 squadID, WWII_SIDE side);
const ChannelID& chatGetGCIChannelID(const instanceid_t instID, WWII_SIDE side);
const ChannelID& chatGetCPChannelID(const instanceid_t instID, const FULL_OBJ_ID cpID, WWII_SIDE side);
const ChannelID& chatGetNumberedChannelID(const instanceid_t instID, UINT32 stationID, WWII_SIDE side);
const ChannelID& chatGetSideChannelID(const instanceid_t instID, SIDE_SUB_CHANNEL, WWII_SIDE);

