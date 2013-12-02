#pragma once
// Created: Tue Mar 22 12:35:01 CST 2005
//
// Definitions for room lists
//
// Copyright (C) 2005 PlayNet, Inc., All Rights Reserver
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"		// WEAPON_SPEC

enum RMLIST_ACTION
{
	RMLIST_INIT,    		// Clear list
	RMLIST_ADD,     		// Add player to end
	RMLIST_DELETE,  		// Delete player from list
	RMLIST_UPDATE,  		// Refresh specific player
	RMLIST_MAX
};

class RoomListEntry
{
public:
	RoomListEntry() {};
	~RoomListEntry() {};

	SINT32 packer(LOGICAL, ACTION_OBJECT*);

	RMLIST_ACTION		m_action;
	ChatHandle			m_handle;		// "chatHandle" of player's callsign
	WEAPON_SPEC			m_weapon;
	UINT32				m_serial;		// weapon serial
	UINT16				m_flags;		// See #defines below
	RANK_NO				m_rank;
};

#define RMLIST_ROOM_OWNER	0x01		// "Owner" of this room (mission leader for missions)
#define	RMLIST_PILOT		0x02		// solo or pilot of vehicle
#define	RMLIST_WANTS_CREW	0x04		// looking for crew
#define	RMLIST_CREW			0x08		// Crew member
#define RMLIST_SPAWNED		0x10		// Player is out in the world

static const UINT32 MAX_ROOM_LIST_ENTRIES = 63;

class RoomList
{
public:
	RoomList() : m_numEntries(0) {}
	~RoomList() {}

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	UINT32				m_numEntries;
	RoomListEntry		m_entry[MAX_ROOM_LIST_ENTRIES];	// An array of room lists
};
