#pragma once
// Created: Sun Jan 21 23:06:08 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

// Prep and install the Raknet interface and RPCs

// Install all of the teulKit callbacks
extern SINT32 missionHostInstallCallbacks(void);

extern SINT32 sendToClientMissionPostReply(MISSION_POST_REPLY reply, SINT32 connection);

extern SINT32 sendToClientMissTimeUpdate(SINT32 connection, UINT32 missionID, UINT32 launchTime, bool mobileSpawnAvailable, bool isMobileSpawnAllowed, bool isActive, bool isOngoing);
class RoomList;
extern SINT32 sendToClientRoomList(SINT32 connection, RoomList& roomList);

//send a mission to everybody & their mother
extern SINT32 mmBroadcastMission(Mission* mission);
//send a mission to a specific server
extern SINT32 mmSendExistingMissionTo(Mission* mission, UINT32 server);
extern SINT32 mmSendMissionResults(Player& player);

