#pragma once
// Created: Thu Apr  7 12:55:22 CDT 2005
//
// Forward declarations of MapPersona types for
// headers etc that don't need the full definitions
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <list>
using PLAYER_MAP = std::map<playerid_t, class Player*>;
using PLAYER_LIST = std::list<class Player*>;
