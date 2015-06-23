#pragma once
// Date: Tue May 22 18:19:42 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "weap/weapSerial.h"

struct DAMAGE_QUEUE_ENTRY
{
	DAMAGE_QUEUE_ENTRY(const UINT64& objectKey_, const WEAPON_SERIAL& weapon_, const UINT32 playerID_)
		: objectKey(objectKey_)
		, weapon(weapon_)
		, playerID(playerID_)
		, damageJoules(0ULL)
		{}

	UINT64			objectKey;		// 64-bit encoded full object id.
	WEAPON_SERIAL	weapon;			// Spec and Sortie of the weapon responsible.
	UINT32			playerID;		// ID of the player who did the damage.
	SINT64			damageJoules;	// Damage done.
};
