#pragma once
// Created: Tue Jan  5 12:05:02 CST 2010
//
// Tracks in-use and free management client player IDs.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class Player;

namespace Chat
{
	class ManagementIDAllocator
	{
	public:
		static void Initialize();			// Initializes data.

		// Request an ID. Throws std::runtime_error if none available.
		static playerid_t AllocateID(Player*);

		// Returns an ID to the free pool.
		static void ReleaseID(const playerid_t playerID);

		// Lookup an ID
		static Player* Lookup(const playerid_t playerID);
	};
}

