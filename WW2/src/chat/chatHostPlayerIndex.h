#pragma once
// Created: Fri Sep 30 16:46:16 CDT 2011
// Originating Programmer: Oliver "kfsone" Smith
// Copyright: (C) Playnet Inc 2011 -- All rights reserved.
// Defines the chat hosts' primary player index type and list.

#include <unordered_map>

class Player;
typedef std::unordered_map<ChatHandle,Player*> PLAYER_INDEX;	// Players indexed by handle
extern PLAYER_INDEX playerIndex;

