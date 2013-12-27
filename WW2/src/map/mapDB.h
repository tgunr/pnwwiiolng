#pragma once
// Created: Wed Jan 17 17:23:19 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

extern bool mapInitDB();
extern void mapCloseDB();

//extern LOGICAL mapGetPlayerCallsign(UCHAR* callsign,UINT32 customerID);
extern LOGICAL mapFillPersonaData(class Player* player);
extern bool mapInitPersonaDataForCustomer(Player* player, UINT32 playerID);
extern LOGICAL mapSaveMissionResults(class MissionPacket* packet);
extern void mapSaveErrorInfo(class Player& player, struct WWII_ERROR_INFO& info);

extern void mapSaveRank(const class Player* player, bool immediate);

// Request a persona list for a particular player
extern void mapRequestPersonaSummaryList(const class Player* player);

extern void mapCloseOpenSorties(UINT32 ip);

extern void mapRequestTutProgress(const playerid_t playerID, SINT32 teulIndex);

extern void mapLoadWeaponIndexes();

extern void mapRequestBrigadeDetailsUpdate();
