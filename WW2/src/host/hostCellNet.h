#pragma once
// Created: Sat Dec  9 11:26:10 CST 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// true when there are other cell hosts
extern bool g_clustered;

// Open a connection to a cell host; only takes effect
// if the cell's IP is greater than our IP. Uses the
// private cell-broadcast network for traffic.
extern void hostQueueVehicleRemove(UINT32 vehIndex, UCHAR exitCode);

// Send a vehicle broadcast to the other cellhosts
extern bool hostSendVehicleBroadcast(UCHAR* buffer, SINT32 size);

extern void hostSendResupplyIssue(UINT32 cellIP, const sortieid_t toVehicle, const sortieid_t fromSerial, const ChatHandle fromHandle);
extern void hostSendResupplyAccepted(UINT32 cellIP, const sortieid_t sendTo, const ChatHandle byHandle);

// Determine if a vehicle is in the synced queue.
extern bool hostNeedVehicleFirstTime(UINT32 vehicleID);

// Force me to re-send FIRST_TIMEs for everyone.
extern void hostForceFirstTimeUpdates();

// Push queued vehicle broadcasts to other hosts
extern void hostPushVehicleBroadcasts();
