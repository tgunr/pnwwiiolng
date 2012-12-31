#pragma once
// Created: Sun Jan 21 23:15:25 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "mm/mmEnums.h"
#include "wwii/wwiiRoomLists.h"
#include "wwii/wwiiRank.h"

class Mission ;

extern bool activeCountries[MAX_COUNTRIES] ;

#define USE_TIMEOUTS

static const SINT32 NUM_CLOCK_BITS = 31 ;	// Default number of clock bits to suse

static const UINT32 MINIMUM_TIME_FOR_MISSION_SUCCESS = 10 * 60 ;	// 10 minutes

extern FLOAT32 RTBModifier[2][4];
// Read from config { {.5, 1,1.5, 2 } , {2, 1.5, 1,.5} };

