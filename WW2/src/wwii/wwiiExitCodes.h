#pragma once
// Created: Sun Feb  7 19:14:44 PST 2010
//
// Codes describing whether a player has exited yet.
// These are 1-off from the RTB codes. *Sigh*
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin
enum PLAYER_EXIT_CODE
{
	PEXIT_UNINITIALIZED = 0,
	PEXIT_RTB = 1,
	PEXIT_DEAD,
	PEXIT_OVER_ENEMY_TERRITORY,
	PEXIT_OVER_FRIENDLY_TERRITORY
};
//tolua_end
