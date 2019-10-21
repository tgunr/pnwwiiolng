#pragma once
// Created: Wed Nov 1 16:55:26 2000
//
//	This is where all declarations for pack routine prototypes for
//	universal game specific object structures thatwill be transmited
//	accross the network must reside or be included into.  All pack
//	routines must have an entry for a reference prototype in this file.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern SINT32	updtPackInit();

extern SINT32 updtPackWORLD_UPDATE(const PackMode mode , UCHAR* data, SINT32 offset, struct WORLD_UPDATE* const wu, SINT32 size);

extern SINT32 updtPackSHARED_VEHICLE_INFO(PackMode mode, UCHAR* data, SINT32 offset, struct VEHICLE_UPDATE_HEADER*	vh,  SINT32 size);

extern SINT32 updtPackCP_HIT_HEADER(PackMode mode, UCHAR* data, SINT32 offset, struct CP_HIT_HEADER* hh,  SINT32 size);

extern SINT32 updtPushCpControlChange(struct OWNERSHIP_UPDATE* update);

extern SINT32 updtPushCpLevelChange(struct VISLEVEL_UPDATE* update);

#if platBuild == BUILD_SERVER
class Player;
extern SINT32 updtPackEvents(PackMode mode, UCHAR* const message, SINT32 offset, class Player* player, SINT32 size);

extern SINT32 updtPackVehicleBroadcast(PackMode mode, UCHAR* data, SINT32 offset, struct VEHICLE_HOLDER* vh, SINT32 size);
#endif

extern UINT32 g_framingMode;
extern UINT32 g_hostUpdateInterval;	// Time in MS between host updates
