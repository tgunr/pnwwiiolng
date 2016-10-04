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

#include "wwii/wwiiOwnershipEnums.h"

// Look for a mission without creating it
ChatMission* chatFindMission(UINT32 missionID);

// Return reference to new or existing mission entry
ChatMission& chatGetMission(UINT32 missionID);

void chatDeleteMission(UINT32 missionID);

void chatMissionChannelEmpty(UINT32 missionID);

void chatSetMissionOrders(UINT32 missionID, const std::string& orders, bool lockLeader);
const char* chatGetMissionOrders(UINT32 missionID);
void chatMessageMissionChannel(UINT32 missionID, Chat::LocalizedMessage& lmsg);
void chatMissionAddPlayer(Player* player, UINT32 missionID, UINT32 sponsor);
void chatMissionRemovePlayer(Player* player, UINT32 missionID, UINT32 sponsor);
void chatMissionOrders(const Player* player);
void chatSendMissionUpdates();
void chatMissionUpdateMember(Player* player);
void chatSetMissionLeader(UINT32 missionID, const ChatHandle leader, WWII_SIDE side, const UINT64& origin, const UINT64& objective);
bool chatShareWaypoint(Waypoint& waypoint, ChatMission* mission, WWII_SIDE side);
