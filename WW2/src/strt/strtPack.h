#pragma once
// Date: Tue Nov  7 16:33:57 2000
//
// Pack routines for the strat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

extern SINT32 strtPackInit(void);
extern SINT32 strtPackHitList(PackMode mode, UCHAR* data, SINT32 offset, struct CP_HIT_HEADER* hh,  SINT32 size);
extern SINT32 strtPackOwnershipUpdate(PackMode mode, UCHAR* data, SINT32 offset, struct OWNERSHIP_UPDATE* object, SINT32 size);
extern SINT32 strtPackSyncCPs(PackMode mode, UCHAR* message, SINT32 offset, struct STRT_SYNC_CPS* object, SINT32 size);
extern SINT32 strtPackSyncFacilities(PackMode mode, UCHAR* message, SINT32 offset, struct STRT_SYNC_FACILITIES* object, SINT32 size);
extern SINT32 strtPackSyncBuildings(PackMode mode, UCHAR* message, SINT32 offset, struct STRT_SYNC_BUILDINGS* object, SINT32 size);

extern void* strtGetStaticPacketBuffer();
extern struct STRT_SYNC_CPS& strtGetSyncCPs();
extern struct STRT_SYNC_FACILITIES& strtGetSyncFacilities();
extern struct STRT_SYNC_BUILDINGS& strtGetSyncBuildings();

