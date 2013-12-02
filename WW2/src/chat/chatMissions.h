#pragma once
// Created: Tue Jun 28 16:11:08 CDT 2005
//
// Interfaces for chatMissions.cpp
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatPrototypes.h"

class ChatMission;			// #include chatMissionTypes.h for full definition
class Player;
class Waypoint;

#include <string>
#include "wwii/wwiiOwnershipEnums.h"

// Look for a mission without creating it
extern ChatMission* chatFindMission(UINT32 missionID);

// Return reference to new or existing mission entry
extern ChatMission& chatGetMission(UINT32 missionID);

extern void chatDeleteMission(UINT32 missionID);

extern void chatMissionChannelEmpty(UINT32 missionID);

extern void chatSetMissionOrders(UINT32 missionID, const std::string& orders, bool lockLeader);
extern const char* chatGetMissionOrders(UINT32 missionID);
extern void chatMessageMissionChannel(UINT32 missionID, Chat::LocalizedMessage& lmsg);
extern void chatMissionAddPlayer(Player* player, UINT32 missionID, UINT32 sponsor);
extern void chatMissionRemovePlayer(Player* player, UINT32 missionID, UINT32 sponsor);
extern void chatMissionOrders(const Player* player);
extern void chatSendMissionUpdates();
extern void chatMissionUpdateMember(Player* player);
extern void chatSetMissionLeader(UINT32 missionID, const ChatHandle& leader, WWII_SIDE side, const UINT64& origin, const UINT64& objective);
extern bool chatShareWaypoint(Waypoint& waypoint, ChatMission* mission, WWII_SIDE side);
