#pragma once
// Created: Wed Mar  2 16:05:27 CST 2005
//
// Defines the brigade summary & selection classes
// See below for client usage documentation
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// Client Usage:
//
// - To obtain a list of brigades:
//			BrigadeSummaries request();
//			request.send(hostIndex);
//		Returns a list of brigades that match your currently
//		selected country and branch.
//		 m_count is the number of returned brigades
//			where 0 <= m_count < MAX_LISTED_BRIGADES
//		 m_summaries will have m_count BrigadeSummary objects.
//		 m_summaries[i].isActive() will tell you if a brigade is
//		  currently selectable or not
//
// - To get detail on a particular brigade
//			BrigadeSelection request(BRIG_DETAIL, brigadeID);
//			request.send(Connection);
//		On success you get back a BrigadeSelection(BRIG_DETAIL)
//		with the details filled out
//		otherwise you get BRIG_ERROR or higher
//
// - To select a brigade for your persona
//			BrigadeSelection request(BRIG_SELECT, brigadeID);
//			ps.send(Connection);
//		On success, you will get a BRIG_SELECT via BrigadeSelection::receive()
//		with the details filled out as BRIG_DETAIL
//		otherwise you get BRIG_ERROR or higher
//
// ------------------------------------------------------------------

#include "teul/teulPackers.h"
#include "mm/mmEnums.h"
#include "wwii/wwiiHCUnits.h"

class Player;
class Brigade;

// Sizes
static const size_t MAX_LANGUAGES_PER_BRIGADE = 2;
static const size_t MAX_BRIGADE_NOTES_LENGTH = 400;
static const size_t MAX_BRIGADE_MOTTO_LENGTH = 40;

// Take approx 1078 / sizeof a packed brigade summaries
static const UINT32 MAX_LISTED_BRIGADES = 500;

// The highest population rank a brigade can have - semi arbitrary number,
// but be mindful the number fits inside the number of bits we pack
static const UINT32 MAX_POPULATION_RANK	= 9;

// Class for listing brigade select options
class BrigadeSummary : public HCUnit
{
public:
	//// ---- Class Operators ---- ////
	// Create from scratch
	BrigadeSummary(UINT32 withBrigadeID, HCUNIT_TYPE withTypeID, UINT32 withParentID
					, WWII_COUNTRY withCountryID, BRANCH_ENUM withBranchID, BRANCH_ENUM withMovementType, UCHAR withUnitClass
					, const ChatHandle withHandle, const char* withName, const char* withNick, UINT8 withRank
					, FULL_OBJ_ID withHome=nullObjID, const ChatHandle withOwner=0LL, RANK_NO withOwnerRank=0)
		: HCUnit(withBrigadeID, withTypeID, withParentID, withCountryID, withBranchID, withMovementType, withUnitClass
				, withHandle, withName, withNick, withRank, withHome, withOwner, withOwnerRank)
		, m_active(false), m_missions(0), m_players(0), m_officers(0), m_objectiveCPID(nullObjID), m_mobileSpawns(0), m_canPostHere(false)
		{ m_languages[0] = 0; }

	BrigadeSummary(UINT32 withBrigadeID=0)
		: HCUnit(withBrigadeID, HCU_BRIGADE)
		, m_active(false), m_missions(0), m_players(0), m_officers(0), m_objectiveCPID(nullObjID), m_mobileSpawns(0), m_canPostHere(false)
		{ m_languages[0] = 0; }

	virtual ~BrigadeSummary() {}

	// Create from the network
	explicit BrigadeSummary(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Pack/unpack routine
	virtual SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	//// ---- Member Functions ---- ////

	UINT32 brigadeID() const { return m_unitID; }
	bool active() const { return m_active; }
	UINT16 missions() const { return m_missions; }
	UINT16 players() const { return m_players; }
	UINT16 officers() const { return m_officers; }
	FULL_OBJ_ID objectiveCPID() const { return m_objectiveCPID; }

	// Query: Is this brigade available for use at the moment?
	bool isActive() const { return m_active; }

	//// ---- Class Variables ---- ////
public:
		// Top N languages currently in-use in this brigade (desc order)
	UCHAR			m_languages[MAX_LANGUAGES_PER_BRIGADE];
		// Whether or not the brigade is active
	bool			m_active;
		// Count of missions posted
	UINT16			m_missions;
		// Player population ranking (0 = best)
	UINT16			m_players;
		// Number of officers online
	UINT16			m_officers;
		// Current objective
	FULL_OBJ_ID		m_objectiveCPID;
		// Number of mobile spawns?
	UINT16			m_mobileSpawns;
		// Can I post here right now?
	bool			m_canPostHere;

private:
};

// Wrapper structure for the purposes of transport

class BrigadeSummaries
{
public:
	BrigadeSummaries() : m_error(false), m_lastGroup(false), m_group(0), m_count(0)  { }
	BrigadeSummaries(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

    SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	// Send/Receive functions (defined as needed)
	// Returns: error < 0  /  ok >= 0
	SINT32 send(SINT32 teulIndex);		// host "send" function
	SINT32 receive(SINT32 teulIndex);	// client/host receive function

	// Host function to populate the summaries
	int populate(WWII_COUNTRY country, BRANCH_ENUM branch, Player* player, int groupOffset);

public:
	bool			m_error;							// True error occured
	bool			m_lastGroup;							// True if this is the last group in the list
	UINT32			m_group;							// group id for this summary group
	UINT32			m_count;							// Number of summaries described in group
	BrigadeSummary	m_summaries[MAX_LISTED_BRIGADES];	// Array of summaries
};

// More detail on a brigade

class BrigadeDetail : public BrigadeSummary
{
public:
	BrigadeDetail(UINT32 withBrigadeID, HCUNIT_TYPE withTypeID, UINT32 withParentID
					, WWII_COUNTRY withCountryID, BRANCH_ENUM withBranchID, BRANCH_ENUM withMovementType, UCHAR withUnitClass
					, const ChatHandle withHandle, const char* withName, const char* withNick, UINT8 withRank
					, const FULL_OBJ_ID withHomeCP, const ChatHandle withOwner, RANK_NO withOwnerRank)
		: BrigadeSummary(withBrigadeID, withTypeID, withParentID, withCountryID, withBranchID, withMovementType, withUnitClass, withHandle, withName, withNick, withRank, withHomeCP, withOwner, withOwnerRank)
			{ m_motd[0] = m_motto[0] = 0; }
	BrigadeDetail(UINT32 withBrigadeID=0) : BrigadeSummary(withBrigadeID) { m_motd[0] = m_motto[0] = 0; }
	virtual ~BrigadeDetail() {}

	// Packer
	virtual SINT32 packer(PackMode mode, ACTION_OBJECT* a) override;

	// Helpers
	FULL_OBJ_ID homeCPID() const { return m_homeCPID; }
	ChatHandle officerHandle(UINT32 i) const { return ((i < officers()) ? m_officerHandles[i] : 0); }
	ChatHandle officerRank(UINT32 i) const { return ((i < officers()) ? m_officerRanks[i] : 0); }

public:
	static const size_t _MAX_OFFICERS = 12;

	char			m_motd[MAX_BRIGADE_NOTES_LENGTH+1];
	char			m_motto[MAX_BRIGADE_MOTTO_LENGTH+1];
	ChatHandle		m_officerHandles[_MAX_OFFICERS];
	UCHAR			m_officerRanks[_MAX_OFFICERS];
};


//////////////////////////////////////////////////////////////////////
// Brigade selection request

enum BRIGADE_SELECT_STATUS
{
	BRIG_INVALID = 0			// Should never be zero
,	BRIG_DETAIL					// Detail on a particular brigade
,	BRIG_SELECT					// Brigade selection
,	BRIG_ERROR					// Not available due to unspecified error
,	BRIG_MISSING				// No such brigade
,	BRIG_INACTIVE				// Brigade is not active
,	BRIG_RESTRICTED				// You don't qualify for this brigade (e.g. GM, or event brigades)
};

class BrigadeSelection
{
public:
	// client request constructor
	BrigadeSelection(BRIGADE_SELECT_STATUS status, UINT32 brigadeID) : m_status(status), m_detail(brigadeID) { }
	BrigadeSelection() : m_status(BRIG_INVALID), m_detail(0) {}

	// Construct from network object
	BrigadeSelection(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Network exchange functions
	SINT32 send(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

	// Packer
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	//// ---- Member variables ---- ////
public:
	BRIGADE_SELECT_STATUS	m_status;
		// Recap of the brigade summary
	BrigadeDetail			m_detail;

	// Private host functions
private:
	SINT32 _query(Player* player, const Brigade* brigade);
	SINT32 _select(Player* player, Brigade* brigade);
	SINT32 _sendResult(SINT32 teulIndex, BRIGADE_SELECT_STATUS result) { m_status = result; return send(teulIndex); }
};
