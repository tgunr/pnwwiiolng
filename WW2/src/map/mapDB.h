#pragma once
// Created: Wed Jan 17 17:23:19 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

bool mapInitDB();
void mapCloseDB();

bool mapFillPersonaData(class Player* player);
bool mapInitPersonaDataForCustomer(Player* player, UINT32 playerID);
void mapSaveErrorInfo(class Player& player, struct WWII_ERROR_INFO& info);

void mapSaveRank(const class Player* player, bool immediate);

// Request a persona list for a particular player
void mapRequestPersonaSummaryList(const class Player* player);

void mapCloseOpenSorties(UINT32 ip);

void mapRequestTutProgress(const playerid_t playerID, SINT32 teulIndex);

void mapLoadWeaponIndexes();

void mapRequestBrigadeDetailsUpdate();
