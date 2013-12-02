#pragma once
// Date: Fri Nov  3 13:54:24 2000
//
// This module contains all the setup and such to make the strat system
// listen for messages from the client or the game
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "weap/weapDefs.h"			// For MAX_WEAP_CATEGORY

struct STRAT_ENEMY_REPORT;

//tolua_begin

extern bool strtAddEnemyReport(const STRAT_ENEMY_REPORT& report);

extern UINT32 ewsHighThreshold[MAX_WEAP_CATEGORY];

extern void strtEvaluateEWSUpdates();

//tolua_end

