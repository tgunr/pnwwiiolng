#pragma once
// Created: Wed Jul 26 16:52:31 2000
//
// List management for the chat System.  Lists are assumed to be circularly
// linked, and
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "host/hostPlayers.h"
#include "chat/chatHostPlayerIndex.h"

extern PLAYERS commandList;
extern PLAYERS sidePlayers[MAX_SIDES];

extern UINT32 _sideCount[];

void chatInitPlayerLists(void);

//This is the logic that is run whenever a logout is received on a teulIndex
void chatLogoutPlayer(SINT32 teulIndex);

namespace Chat { struct PlayerInfo; }
void chatCreatePlayer(class PlayerStatusNode* psn, Chat::PlayerInfo* playerInfo);

void chatPlayerDeactivate(Player* player);
void chatFreePlayer(Player* player);
