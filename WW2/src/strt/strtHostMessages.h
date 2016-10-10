#pragma once
// Created: Fri Nov  3 13:54:24 2000
//
// This module contains all the setup and such to make the strat system
// listen for messages from the client or the game
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtMapTypes.h"

// Divided into two sections :: Lua Exposed functions and hidden functions

////    ------------------------------  NON-SCRIPTING FUNCTIONS

//Installs our callbacks
extern SINT32 strtHostInstallCallbacks();

//This function sends a vislevel update to a specific strat Updater.  Used
//	for making sure that the strat updater is aware of the vislevel of every
//	building in th world upon startup
extern SINT32 strtSendVislevelUpdate(UINT32 address, UINT16 port, FULL_OBJ_ID objectID, UCHAR vislevel);

// functions to transmit status info to the network
extern SINT32 strtSendFacilityStatus(UINT32 address, UINT16 port, class FacilityStruct* facility);
extern SINT32 strtSendCPStatus(UINT32 address, UINT16 port, class CPStruct* cp);

// Notify a customer that their post has expired
extern SINT32 strtSendPostExpired(UINT32 customerID, const WEAPON_SERIAL& weaponSerial, bool cancelled);

// Notify a specific connection of a time update
extern SINT32 strtSendMMCrewBailed(UINT32 comanderID, UINT32 crewID);
extern SINT32 strtSendMMJoinDeny(UINT32 crewID,UINT32 reasonCode);
extern SINT32 strtSendMMJoinListResponse(UINT32 commandID, UINT32 crewID, ChatHandle crewHandle);

// Process a capacity request and send the response
extern LOGICAL strtReplyToCapacityRequest(SINT32 teulIndex, const MM_CAPACITY_REQUEST& request, FULL_OBJ_ID destFacility, bool vehicleDelivered);

//A lot smarter than the rest of the send functions, this function actually
// breaks the CP, Facilty, or Building information inside this supercell
// into chunks that can be sent easily
extern void strtAllowSyncing();	// Until first call, sync fns do nothing
extern SINT32 strtSyncCPs(UINT32 address, UINT16 port, Supercell* sc);
extern SINT32 strtSyncFacilities(UINT32 address, UINT16 port, Supercell* sc);
extern SINT32 strtSyncObjects(UINT32 address, UINT16 port, Supercell* sc);

class FacilityStruct;

////    ------------------------------  SCRIPTING-EXPOSED FUNCTIONS
//tolua_begin

extern void strtSendChatInitialUpdates();

// immediately transmit a status update
extern void strtBroadcastStratUpdates();

// Broadcast to the game networks that time or config has changed
extern SINT32 strtBroadcastTimeUpdate(UINT32 gameTime, UINT32 timeFactor, UINT32 weatherPattern, UINT32 fogPattern);
extern SINT32 strtBroadcastConfigUpdate();

// Add an update onto a queue for aggregate transmission
extern void strtQueueVisLevelUpdate(class ObjectStruct*);
extern void strtQueueFacilityStatus(class FacilityStruct*);
extern void strtQueueCPStatus(class CPStruct*);

extern void strtSendCaptureScoreNotice(const FULL_OBJ_ID fid, WWII_SIDE forSide);

extern void strtSendCaptureState(const FacilityStruct* facility, WWII_SIDE forSide, SINT32 rate, bool spawnable);

//tolua_end
