#pragma once
// Created: Mon Mar  7 11:59:10 CST 2005
//
// Mission list routines
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// client Usage:
//
//   To request a list of missions:
//	  first select a persona and brigade.
//	  Then send a MissionList.send() to the map host.
//	  Result will be returned via MissionList.receive()
//
//	 Results:
//	  If m_error is true, the rest of the data is invalid
//	  If m_count is zero then no missions were available
//	  m_listStart and m_listEnd let you know where in a
//	  possible sequence of multiple packets you are.
//	  NOTE: The client should try to avoid sending
//	  requests while it is between m_listStart and m_listEnd

#include "teul/teulPackers.h"
#include "miss/missionTypes.h"
#include "miss/missActiveMission.h"

class Mission;

/// @class MissionSynopsis
/// @brief Minimal information about a* single* mission

class MissionSynopsis
{
public:
	//// ---- Class Operators ---- ////
	/// Default constructor.
	MissionSynopsis()
		: m_missionID(0)
		, m_countryID(WWII_NO_COUNTRY), m_branchID(BRANCH_NOT_USED), m_brigadeID(0)
		, m_type(RECON), m_subType(0)
		, m_origin(nullObjID), m_target(nullObjID)
		, m_hasMobileSpawn(false)
		, m_canMobileSpawn(false)
		, m_poster(0LL)
		, m_isCurrent(false)
		, m_amLeader(false)
		, m_population(0)
		, m_isOpen(false)
		{}
	/// Network packer constructor.
	MissionSynopsis(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	/// [host] Populate from a host mission.
	void populate(const Mission*, bool, bool, UINT32);

	/// Pack/unpack routine.
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	//// ---- Member Functions ---- ////

	bool canMobileSpawn() const { return m_canMobileSpawn; }
	bool hasMobileSpawn() const { return m_hasMobileSpawn; }

	//// ---- Class Variables ---- ////
public:
	// TODO: 1.35 rearranged this, so we should probably consider memory layout.
	UINT32       m_missionID;      ///< Host's campaign-unique Mission ID.
	WWII_COUNTRY m_countryID;      ///< The country the mission is for.
	BRANCH_ENUM  m_branchID;       ///< The branch the mission uses.
	UINT32       m_brigadeID;      ///< The brigade the mission is attached to.

	MISSION_TYPE m_type;           ///< Main type (attack, defend, etc).
	SINT8        m_subType;        ///< Mission sub-type.
	FULL_OBJ_ID  m_origin;         ///< Facility.
	FULL_OBJ_ID  m_target;         ///< Facility* or* CP.
	bool         m_hasMobileSpawn; ///< True when mobile spawn is active>
	bool         m_canMobileSpawn; ///< True if this mission supports mobile spawning.
	ChatHandle   m_poster;         ///< compressed "chatHandle" of poster.
	bool         m_isCurrent;      ///< If you are on this mission.
	bool         m_amLeader;       ///< If you are the leader.
	UCHAR        m_population;     ///< Scaled population counter.
	bool         m_isOpen;         ///< False if the mission cannot spawn.
};

// Wrapper structure for the purposes of transport

// This is the maximum number of missions that can be sent
// in each MissionList, but there can be multiple MissionLists
// sent to the client.
static const UINT32 MAX_MISSIONS_PER_LIST = 38;

enum ML_RESULT
{
	MLR_OK				// No errors
,	MLR_ERROR			// Unknown error/uninitialized
,	MLR_NO_PERSONA		// No persona selected
,	MLR_NO_BRIGADE		// No brigade selected
};

// List of MissionSynopsis
class MissionList
{
public:
	MissionList() : m_status(MLR_ERROR), m_listStart(true), m_listEnd(true), m_count(0) { }
	MissionList(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
	~MissionList() { }
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	// Send/Receive functions (defined as needed)
	// Returns: error < 0  /  ok >= 0
	SINT32 send(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

	/// Send quick mission list, which ignores current brigade.
	SINT32 sendQuick(SINT32 teulIndex);

	ML_RESULT		m_status;							// Status of the list
	LOGICAL			m_listStart;						// True if this is the start of list
	LOGICAL			m_listEnd;							// True if this is the end of the list
	MissionSynopsis	m_missions[MAX_MISSIONS_PER_LIST];	// Array of missions
	LOGICAL			m_canPost;							// Can I post here?
	ActiveMission	m_activeMission;					// Details what mission, if any, you have active
	UINT8			m_count;							// Number of missions described

private:
	SINT32 _sendResult(SINT32 teulIndex, ML_RESULT result) { m_status = result; return send(teulIndex); }
};

