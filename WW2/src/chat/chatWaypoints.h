#pragma once
// Created: Mon Mar 21 19:55:22 CST 2005
//
// Chat host handling of waypoints
//
// Copyright (C) PlayNet, Inc. 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class Waypoint ;

extern void chatSetMissionWaypoint(UINT32 missionID, const Waypoint& rhs) ;
extern void chatDeleteMissionWaypoint(UINT32 missionID, const ChatHandle& handle) ;
extern void chatPurgeMissionWaypoints(UINT32 missionID) ;
