#include <syscfg.h> // Path is project specific.
// Created: Tue Jan  5 12:05:02 CST 2010
//
// Tracks the free list of available management playerIDs.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatManagementIDs.h"			// Our header file

#include <queue>

//	------------------------------	Declarations

using ID_QUEUE = std::queue<playerid_t>;
static ID_QUEUE freeIDs;

using IN_USE = std::map<playerid_t, Player*>;
static IN_USE inUse;

static const playerid_t MIN_MANAGEMENT_ID = (playerid_t) -100100;
static const playerid_t MAX_MANAGEMENT_ID = (playerid_t)    -100;
//static const playerid_t NUM_MANAGEMENT_IDS = MAX_MANAGEMENT_ID - MIN_MANAGEMENT_ID;

//////////////////////////////////////////////////////////////////////
// Initialize the freelist.

void Chat::ManagementIDAllocator::Initialize()
{
	for ( playerid_t id = MIN_MANAGEMENT_ID; id < MAX_MANAGEMENT_ID; ++id )
	{
		freeIDs.push(id);
	}
}

//////////////////////////////////////////////////////////////////////
// Allocate an ID from the free pool.

playerid_t
Chat::ManagementIDAllocator::AllocateID(Player* player)
{
	if ( freeIDs.empty() )
		throw std::runtime_error("No management connections available");	///Terminal

	// Grab the front entry, pop it from the queue, and return it.
	const playerid_t id = freeIDs.front();
	freeIDs.pop();

	inUse[id] = player;

	return id;
}

//////////////////////////////////////////////////////////////////////
// Release an ID

void
Chat::ManagementIDAllocator::ReleaseID(const playerid_t playerID)
{
	assert( playerID >= MIN_MANAGEMENT_ID && playerID <= MAX_MANAGEMENT_ID );

	inUse.erase(playerID);

	freeIDs.push(playerID);
}

//////////////////////////////////////////////////////////////////////
// Lookup an ID

Player*
Chat::ManagementIDAllocator::Lookup(const playerid_t playerID)
{
	IN_USE::iterator it = inUse.find(playerID);
	return (it == inUse.end()) ? nullptr : it->second;
}

