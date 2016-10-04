#pragma once
// Date: Mon May 21 14:00:39 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

class Player;
struct FULL_OBJ_ID;
struct WEAPON_SERIAL;

//Adds a player to the global send list
extern bool strtAddPlayerToSendList(Player* player, const TeulTimeMS& msTillSend);
extern void strtRemovePlayerFromSendList(Player* player);

//Accumulates the damage messages that come up to it
extern void strtAccumulateDamageMessage(const FULL_OBJ_ID object, const SINT64& damageJoules, UINT32 playerID, const WEAPON_SERIAL& serial);

extern void strtUpdaterInit();
extern void strtUpdaterProcessEvents();
