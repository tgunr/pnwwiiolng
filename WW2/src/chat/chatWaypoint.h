#pragma once
// Created: Fri Mar 18 15:45:40 CST 2005
//
// Defines the underlying waypoint objects
//
// Copyright (C) PlayNet, Inc. 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "str/stringLib.h"

//////////////////////////////////////////////////////////////////////
// Defined waypoint base-types

// tolua_begin
enum WP_TYPE
{
	WPT_INVALID						// Uninitialized/invalid
,	WPT_MARKER						// Purely informational
,	WPT_RALLY						// Gather
,	WPT_ENEMY						// Enemy contact
,	WPT_ATTACK						// Attack here
,	WPT_DEFEND						// Defend here
,	WPT_WAYPOINT					// A real waypoint
,	WPT_PATROL						// Patrol boundary
,	WPT_ORIGIN						// Mission origin
,	WPT_TARGET						// Mission target
,	WPT_MOBILESPAWN					// Mobile spawn
,	WPT_SUPPLY						// Supply Point
,	WPT_AMMO						// Ammo request
,	WPT_MORTAR						// requested mortar attack
,	WPT_EAI							// enemy ai
,	WPT_FAI							// friendly ai
,	WPT_DEATH						// death
,	MAX_WPT_TYPES
};

enum
{
	WPT_ENEMY_INVALID
,	WTP_ENEMY_AIR
,	WTP_ENEMY_LAND
,	WTP_ENEMY_SEA
,	WTP_ENEMY_TROOP
,	WTP_ENEMY_GUN
,	WTP_ENEMY_TRUCK
,	WTP_ENEMY_DANGER
,	WTP_ENEMY_UNKNOWN				// question mark; enemy type unknown
,	WTP_ENEMY_MORTAR				// Mortar barrage icon; aka "Aim mortars here"
};
// tolua_end

enum
{
	WPT_MS_LOCATION					// Where the MS is at
,	WPT_MS_REQUEST					// Where the ML would like the MS at
};

// Max length of a waypoint's text
static const UINT32 MAX_WP_NOTE_LENGTH = 128;

// How many bits are required to communicate waypoint handle at 6 bits a character; try to byte align
static const UINT32 MAX_WP_HANDLE_LEN = MAX_HANDLE_LEN;
static const UINT32 MAX_WP_HANDLE_BITS = GAME_HANDLE_BITS;
static const UINT32 CONTACT_REPORT_LIFETIME_S = 7 * 60;		// 7 minutes

class Waypoint final
{
	// Constructors/destructors
public:
	Waypoint(bool isLocal, const ChatHandle withHandle, const ChatHandle fromPlayerHandle, WP_TYPE withType, UINT32 withSubType, SINT32 atX, SINT32 atY, UINT32 whenPosted, const char* withNotes)
		: m_local(isLocal)
		, m_handle(withHandle)
		, m_playerHandle(fromPlayerHandle)
		, m_type(withType)
		, m_subType(withSubType)
		, m_x(atX)
		, m_y(atY)
		, m_posted((whenPosted) ? whenPosted: (UINT32)teulTime())
		, m_shared(true)
		{
			strsafecpy(m_notes, withNotes);
		}

	Waypoint()
		: m_local(false)
		, m_handle(0)
		, m_playerHandle(0)
		, m_type(WPT_INVALID)
		, m_subType(WPT_INVALID)
		, m_x(0)
		, m_y(0)
		, m_posted((UINT32)teulTime())
		, m_notes{}
		, m_shared(true)
		{
		}

	// Network constructor
	Waypoint(ACTION_OBJECT* const a)
		: m_local(false)
		, m_handle(0)
		, m_playerHandle(0)
		, m_type(WPT_INVALID)
		, m_subType(WPT_INVALID)
		, m_x(0)
		, m_y(0)
		, m_posted((UINT32)teulTime())
		, m_notes{}
		, m_shared(true)
		{
			a->offset = packer(TEUL_UNPACK, a);
		}

	//Operators
public:
	bool operator == (const Waypoint& rhs) const;

	// Functions
public:
	// Pack and unpack routine
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
	void setPosted(UINT32 postedTime) { m_local = false; m_posted = postedTime; }
	void setHandle(const ChatHandle toHandle) { m_handle = toHandle; }
	void setShared(bool toShared) { m_shared = toShared; }

	// 'true' if anyone can post this type
	bool leaderOnlyPosting() const
	{
		return ( !((type() == WPT_ENEMY) | (type() == WPT_AMMO)) );
	}

	// 'true' if this waypoint should not be retained by the host
	// or clients after being sent (client may continue to display
	// notice of the waypoint afterwards though)
	bool isTransient() const
	{
		return ( type() == WPT_AMMO );
	}

	// Host functions
	void processIncomingFrom(class Player* player);

	// Accessors
public:
	UINT32 local() const { return m_local; }
	ChatHandle handle() const { return m_handle; }
	ChatHandle playerHandle() const { return m_playerHandle; }
	WP_TYPE type() const { return m_type; }
	UINT32 subType() const { return m_subType; }
	SINT32 x() const { return m_x; }
	SINT32 y() const { return m_y; }
	UINT32 posted() const { return m_posted; }
	const char* notes() const { return m_notes; }
	UINT32 shared() const { return m_shared; }

protected:
	bool		m_local;			// FALSE: Host sent it to me and manages it
	ChatHandle	m_handle;			// "handle" of the waypoint
	ChatHandle	m_playerHandle;		// "chatHandle" of the poster
	WP_TYPE		m_type;
	UINT32		m_subType;			// type-specific extended info
	SINT32		m_x, m_y;			// Co-ordinates
	UINT32		m_posted;			// Time it was posted
	char		m_notes[MAX_WP_NOTE_LENGTH+1];
	bool		m_shared	;		// do we shared this waypoint with others?
};
