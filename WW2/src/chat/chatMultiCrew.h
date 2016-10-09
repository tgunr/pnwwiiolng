#pragma once
// Created: Wed Jul 19 10:21:57 CDT 2006
//
// Multi-crew classes and messages
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkArray.h"
#include "weap/weapSpec.h"

// MultiCrewEntry class encapsulates maintenance of the current crew/pilot list
class MultiCrewEntry
{
public:
	MultiCrewEntry()
		: m_handle(0LL), m_weapon(), m_serial(0), m_attachedTo(0LL)
		, m_isRemoval(false), m_isPilot(false), m_isAvailable(false), m_visibility(_MISSION) {}
	MultiCrewEntry(const ChatHandle withHandle)			// Removal
		: m_handle(withHandle), m_weapon(), m_serial(0), m_attachedTo(0LL)
		, m_isRemoval(true), m_isPilot(false), m_isAvailable(false), m_visibility(_PRIVATE) {}
	MultiCrewEntry(const ChatHandle withHandle, bool withIsPilot, bool withIsAvailable, UCHAR withVisibility
					, const WEAPON_SPEC withWeapon, UINT32 withSerial, const ChatHandle withAttachedTo)
		: m_handle(withHandle), m_weapon(withWeapon), m_serial(withSerial), m_attachedTo(withAttachedTo)
		, m_isRemoval(false), m_isPilot(withIsPilot), m_isAvailable(withIsAvailable), m_visibility(withVisibility)
		{}
	MultiCrewEntry(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

public:
	bool isRemoval() const { return m_isRemoval; }
	bool isPilot() const { return m_isPilot; }
	bool isAvailable() const { return m_isAvailable; }
	UCHAR visibility() const { return m_visibility; }
	ChatHandle handle() const { return m_handle; }
	WEAPON_SPEC weapon() const { return m_weapon; }
	UINT32 serial() const { return m_serial; }
	ChatHandle attachedTo() const { return m_attachedTo; }

public:
	// True if the player has a vehicle reserved
	bool hasWeapon() const { return weapon().wType != 0 && serial() != 0; }
	// True if the player is attached to someone
	bool isAttached() const { return m_isAvailable == false && m_attachedTo != 0LL; }

protected:
	ChatHandle	m_handle;						// Handle of player
	WEAPON_SPEC	m_weapon;						// Weapon (if any)
	UINT32		m_serial;						// Serial # of weapon
	ChatHandle	m_attachedTo;					// Player attached

	// isUpdate: True if this is a removal, otherwise this is an add/update
	bool		m_isRemoval;

	// isPilot: True if player is looking to be a pilot
	bool		m_isPilot;

	// isAvailable: Whether this user is idling/lfc
	// This may become false without the player being attached
	// (indicating they are in mid-request)
	// To verify a player is actually unavailable AND attached
	// check isAttached()
	bool		m_isAvailable;

	// Visibility:
	//	SQUAD = Visible to anyone on my squad (regardless of brigade/mission)
	//	BRIGADE = Visible to anyone in brigade
	//  MISSION = Visible to anyone on my mission
	//	PRIVATE = Not visible to others (but can do joins etc)
	// Note: implemented by host, sent to client for informational purposes only
	enum { _MISSION=0, _BRIGADE, _SQUAD, _PRIVATE, _VISIBILITY_BITS = 3 };
	UCHAR		m_visibility;
};

using MULTI_CREW_LIST = teul_network_array<MultiCrewEntry>;
