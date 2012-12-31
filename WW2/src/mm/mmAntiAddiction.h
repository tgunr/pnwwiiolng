#pragma once
// Created: Tue May 27 01:36:16 CDT 2008
//
// Prototypes for anti-addiction routines
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Check thru the minor's list for any players who need updates.
extern void mmCheckAddictionTimers() ;

// Add a minor to the anti-addiction list
extern void mmAddMinor(class Player* player, UINT32 playedTime) ;

// Remove a minor from the anti-addiction list and save their played time
extern void mmRemoveMinor(class Player* player) ;

// Test if a player is in the penalty time
extern FLOAT32 mmAntiAddictionScoringMultiplier(const Player* player) ;

