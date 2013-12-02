#pragma once
// Created: Wed Oct  8 00:06:42 CDT 2008
//
// Defines a set of Player* pointers.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <set>
typedef std::set<class wwiiPlayer*> PLAYERS;
extern PLAYERS g_players;

