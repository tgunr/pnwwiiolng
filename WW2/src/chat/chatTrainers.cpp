#include <syscfg.h> // Path is project specific.
// Date: Sun Aug 21 00:49:29 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"

#include "chat/chatTrainers.h"

static TRAINERS s_trainers;

//////////////////////////////////////////////////////////////////////
// Add a trainer to the list if they aren't already in it

static void
_addTrainer(const playerid_t playerID)
{
	// Find the spot that you should be at
	TRAINERS::iterator it = s_trainers.lower_bound(playerID);

	// Nothing to do if you already have an entry in here
	if ( it != s_trainers.end() && *it == playerID )
		return;

	s_trainers.insert(it, playerID);

	dbugLog(_FLI, "p:%u added to trainer list (%" PRSizeT " total)", playerID, s_trainers.size());
}

//////////////////////////////////////////////////////////////////////
// Remove a trainer from the list

static void
_removeTrainer(const playerid_t playerID)
{
	TRAINERS::iterator it = s_trainers.find(playerID);

	// Are you in the list?
	if ( it == s_trainers.end() || *it != playerID )
		return;

	s_trainers.erase(it);

	dbugLog(_FLI, "p:%u removed from trainer list (%" PRSizeT " total)", playerID, s_trainers.size());
}

//////////////////////////////////////////////////////////////////////
// Public entry point -- add or remove a trainer depending on their
// new status

void
chatUpdateTrainerStatus(const playerid_t playerID, bool isTrainer)
{
	if ( isTrainer )
		_addTrainer(playerID);
	else
		_removeTrainer(playerID);
}

//////////////////////////////////////////////////////////////////////
// Return a const reference to the trainers list

const TRAINERS&
chatGetTrainers()
{
	return s_trainers;
}
