#pragma once
// Created: Tue Feb 22 14:11:32 CST 2005
//
// Defines the persona summary & selection classes
// See below for client usage documentation
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// client Usage:
//
// - To obtain a list of personas:
//			PersonaSummaries request();
//			request.send(hostIndex);
//		List of personas will be received in PersonaSummaries::receive()
//		 m_count is the number of returned personas
//			where 0 <= m_count < MAX_PLAYER_PERSONAS
//		 m_summaries will have m_count PersonaSummary objects.
//		 m_summaries[i].isLocked() will tell you if a persona is not
//		  currently selectable
//		 m_summaries[i].isNewPersona() returns true for personas which
//		  have not yet been initialized
//
// - To create a new persona
//			PersonaSelection ps(PSS_NEW, 0, country, branch);
//			ps.send(Connection);
//		On success, you will get a PSS_NEW via PersonaSelection::receive()
//		otherwise you will get PSS_ERROR or higher
//
// - To select an existing persona
//			PersonaSelection ps(PSS_SELECT, personaID, country, branch);
//			ps.send(Connection);
//		On success, you will get a PSS_SELECT via PersonaSelection::receive()
//		otherwise you get PSS_ERROR or higher
//
// - To reset/delete an existing persona
//			PersonaSelection ps(PSS_DELETE, personaID, country, branch);
//			ps.send(Connection);
//		On success, you will get a PSS_DELETE via PersonaSelection::receive
//		otherwise you get PSS_ERROR or higher
//
// ------------------------------------------------------------------

#include <vector>
#include <string>

#include "mm/mmEnums.h"
#include "miss/missActiveMission.h"
#include "chat/chatGroupPermissionTypes.h"
#include "wwii/wwiiPermissions.h"

class Player;

// Class for listing persona select options
class PersonaSummary
{
public:
	//// ---- Class Operators ---- ////
	// Create from scratch
	PersonaSummary() : m_personaID(0) { }

	// Create from the network
	PersonaSummary(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Pack/unpack routine
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	//// ---- Member Functions ---- ////

	// Query: Is this persona available for use at the moment?
	bool isLocked() const { return m_lockedThru > 0; }
	// Query: has this one been initialized yet?
	bool isNewPersona() const { return m_personaID > 0; }

	bool operator==(const PersonaSummary& rhs) const
	{
		return (m_countryID == rhs.m_countryID &&  m_branchID == rhs.m_branchID);
	}

	bool operator!=(const PersonaSummary& rhs) const
	{
		return !operator==(rhs);
	}

public:
	enum { _AVAILABLE, _SIDELOCKED, _BANNED, _DELAYED, _NOSWAP, _ARENALOCK, _NOT_SUBBED, _INACTIVE };

	const char* availability() const
	{
		static const char* _text[] = { "available", "onleave", "banned", "delayed", "noswap", "arenalock", "notsubbed", "inactive" };
		return ((UINT32)m_availability <= _INACTIVE) ? _text[m_availability] : "inactive";
	}

	//// ---- Class Variables ---- ////
public:
	UINT32			m_personaID;
	WWII_COUNTRY	m_countryID;
	BRANCH_ENUM		m_branchID;
	RANK_NO			m_rankID;
	UINT8			m_rankPctg;		// progress through rank as pctg
	UINT32			m_lastUsed;
	UINT32			m_lockedThru;		// Time until it can be used again or 0
	UINT32			m_brigadeID;		// Last used brigadeID
	UINT32			m_availability;	// Is this persona available?
};

static const UINT32 MAX_COUNTRY_BRANCHES = 3;			// Max branches per country.
static const UINT32 MAX_PLAYER_COUNTRIES = 4;			// Max countries player can have personas in.
static const UINT32 MAX_PLAYER_PERSONAS = MAX_PLAYER_COUNTRIES * MAX_COUNTRY_BRANCHES;		// Maximum personas a player can have.

// Wrapper structure for the purposes of transport

class PersonaSummaries
{
public:
	PersonaSummaries() : m_error(false), m_count(0), m_callsign(0LL), m_chatPermissions(nullChatPermissions) { }
	PersonaSummaries(ACTION_OBJECT* const a) : m_error(false), m_count(0), m_callsign(0LL), m_chatPermissions(), m_accountGoodUntil(0) { a->offset = packer(TEUL_UNPACK, a); }

    SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	// Send/Receive functions (defined as needed)
	// Returns: error < 0  /  ok >= 0
	SINT32 send(SINT32 teulIndex);		// host "send" function
	SINT32 receive(SINT32 teulIndex);	// client/host receive function

	LOGICAL			m_error;							// True error occured
	UINT32			m_count;							// Number of summaries described
	ChatHandle		m_callsign;							// Player's handle
	PersonaSummary	m_summaries[MAX_PLAYER_PERSONAS];	// Array of summaries
	WWII_SIDE		m_sidelocked;						// Which side you're locked to
	ActiveMission	m_activeMission;
	ChatPermissions	m_chatPermissions;					// Chat permissions
	UINT32			m_accountGoodUntil;				// teulTime() + account time remaining

	bool isPlaynet() const { return ((m_chatPermissions.m_permissionMask & IS_PLAYNET) != 0); }

	// Host function: retrieve the actual list
	bool _list(class Player* player, const std::vector<PersonaSummary>& personas_);
};

//////////////////////////////////////////////////////////////////////
// Persona selection request

enum SELECTION_STATUS
{
	PSS_INVALID = 0				// Should never be zero
,	PSS_NEW						// Create a new persona
,	PSS_SELECT					// Persona is selected / ok
,	PSS_DELETE					// Request for deletion/reset
,	PSS_ERROR					// Not available due to unspecified error
,	PSS_DUPLICATE				// Persona exists (bounce for 'NEW')
,	PSS_MISSING					// No such persona
,	PSS_PERSONA_LOCKED			// Not available due to sidelock
,	PSS_SUBSCRIPTION_LIMIT		// Subscription does not provide access
};

class PersonaSelection
{
public:
	// client request constructor
	PersonaSelection(PersonaSummary const& s)
		: m_status(PSS_SELECT), m_personaID(s.m_personaID), m_countryID(s.m_countryID), m_branchID(s.m_branchID), m_checksum(0), m_rank(0), m_rankPctg(0), m_chatPermissions(nullChatPermissions)
		{ }
	PersonaSelection()
	 	: m_status(PSS_INVALID), m_personaID(0), m_countryID(WWII_NO_COUNTRY), m_branchID(BRANCH_NOT_USED), m_checksum(0), m_rank(0), m_rankPctg(0), m_chatPermissions(nullChatPermissions)
	 	{ }
	PersonaSelection(SELECTION_STATUS s, UINT32 pid, WWII_COUNTRY cid, BRANCH_ENUM bid)
		: m_status(s), m_personaID(pid), m_countryID(cid), m_branchID(bid), m_checksum(0), m_rank(0), m_rankPctg(0), m_chatPermissions(nullChatPermissions)
		{ }

	// Construct from network object
	PersonaSelection(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Network exchange functions
	SINT32 send(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

	// Packer
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	//// ---- Member variables ---- ////
public:
	SELECTION_STATUS m_status;
	UINT32			m_personaID;
	WWII_COUNTRY	m_countryID;
	BRANCH_ENUM		m_branchID;
	UINT64			m_checksum;		// client checksum
	RANK_NO			m_rank;
	UINT32			m_rankPctg;
	ChatPermissions	m_chatPermissions;	// Permission details
	UINT32			m_accountGoodUntil; // teulTime() + seconds remaining on acct

	// Private host functions
private:
#if platBuild == TEUL_SERVER

	SINT32 _create(Player*);
	SINT32 _delete(Player*);
	SINT32 _select(Player*);
	SINT32 _sendResult(SINT32 teulIndex, SELECTION_STATUS result, RANK_NO rank=0, UINT32 rankPctg=0, const ChatPermissions* perms=&nullChatPermissions, UINT32 goodUntil=0);
#endif
};

