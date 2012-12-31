#pragma once
// Created: Thu Sep  1 16:44:49 CDT 2005
//
// Types used by chatMissions
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLStringIDs.h"
#include "chat/chatWaypoint.h"
#include "wwii/wwiiHash.h"
#include "chat/chatMissionMemberUpdates.h"
#include "omg/omgMissionStatus.h"

class Player ;

class ChatMission : public Chat::MissionStatus
{
public:
	ChatMission() ;
	~ChatMission() ;

public:
	typedef std::unordered_map<UINT32,Player*> MEMBERS ;
	typedef std::unordered_map<ChatHandle,Waypoint> WAYPOINTS ;
	typedef std::unordered_map<ChatHandle,Player*> MEMBER_UPDATES ;
	typedef std::unordered_set<ChatHandle> MEMBER_REMOVES ;

public:
	void addMemberRemoval(Player* player) ;
	void addMemberUpdate(Player* player) ;

	//! Add a waypoint to the list
	//! @return true if the waypoint was added/updated
	bool addWaypoint(Waypoint& waypoint) ;
	void addMSPWaypoint(const ChatHandle& ownercall, FLOAT64 x, FLOAT64 y, FLOAT64 z, UINT32 stoUID) ;

	//! Transmit any outstanding Status changes to all members.
	void broadcastStatusChanges() ;

	//! Check the displaced leadership fields.
	void checkDisplacedLeader() const ;
	void lockLeader() ;

	void removeMSPWaypoint(UINT32 uid, bool byPlayer) ;

	void expireWaypoints() ;

	//! Mark mission as requiring an immediate status update.
	void flagForStatusUpdate() ;

	//! Send a message to everyone on this mission, not channel restricted
	void message(LSTRING_ID prefix, LSTRING_ID message, const char* arg1=NULL, const char* arg2=NULL) ;

	//! Remove the STO attached to a mission (if any)
	void removeSTO() ;

	//! Remove a waypoint from the list
	//! Returns true if the waypoint was removed
	bool removeWaypoint(const ChatHandle& handle, bool byPlayer) ;

	//! Send a client their first update
	void sendFirstUpdate(const Player* player) ;

	//! Send a mobile spawn request to the mmhost
	void sendMobileSpawn(UINT32 playerID, const ChatHandle& handle, FLOAT64 x, FLOAT64 y, FLOAT64 z, bool deployed, UINT32 stoUID) ;

	//! Set the fallback leader.
	void setBackupLeader(const ChatHandle& newLeader) { lead().backup(newLeader) ; }

	//! Set the FRU shared cooldown timer.
	void setFRUCooldown(UINT32 cooldown_s) ;

	//! Set the leader of the mission.
	void SetLeader(const ChatHandle& handle) ;

	//! Choose a new backup leader.
	bool electBackupLeader() ;

	//! Change the mission orders.
	void SetOrders(LSTRING_ID message, const std::string& text1="", const std::string& text2="") ;

	//! Change the mission's origin.
	void setOrigin(const UINT64& newOrigin) ;

	//! Change the mission's target.
	void setTarget(const UINT64& newTarget) ;

	// Send any updates pending on this mission, return number sent
	UINT32 sendUpdates() ;

	////////// Accessors.
	UINT32 missionID() const { return header().missionID() ; }
	const UINT64& originCP() const { return m_originCP ; }
	const UINT64& targetCP() const { return m_targetCP ; }
	const ChatHandle& leader() const { return lead().current() ; }
	const ChatHandle& backupLeader() const { return lead().backup() ; }

	SINT32 ordersMsg() const { return m_ordersMsg ; }
	const std::string& ordersText1() const { return m_ordersText1 ; }
	const std::string& ordersText2() const { return m_ordersText2 ; }
	const WAYPOINTS& waypoints() const { return m_waypoints ; }
	const MEMBERS& members() const { return m_members ; }
	const MEMBER_UPDATES& updates() const { return m_updates ; }
	const MEMBER_REMOVES& removes() const { return m_removes ; }
	WWII_SIDE side() const { return m_side ; }
	UINT32 stoUID() const { return m_stoUID ; }
	const ChatHandle& matchLeader() const { return m_matchLeader ; }
	UINT32 backupCandidate() const { return m_backupCandidate ; }
	const ChatHandle& displacedLeader() const { return m_displacedLeader; }
	UINT32 leaderLockFrom() const { return m_leaderLockFrom ; }
	UINT32 leaderLockFor() const { return m_leaderLockFor ; }
	UINT32 leaderDisplaced() const { return m_leaderDisplaced ; }
	bool electing() const { return m_electing ; }

public:
	SINT32			m_ordersMsg ;
	std::string		m_ordersText1 ;
	std::string		m_ordersText2 ;
	WAYPOINTS		m_waypoints ;
	MEMBERS			m_members ;
	MEMBER_UPDATES	m_updates ;					// Members with updates
	MEMBER_REMOVES	m_removes ;					// Members to be removed
	WWII_SIDE		m_side ;					// Which side this mission is
private:
	UINT64			m_originCP ;				// Coming from
	UINT64			m_targetCP ;				// Going to
public:
	UINT32			m_stoUID ;					// MSP sto

	// For leadership control
	ChatHandle		m_matchLeader ;				// Lower callsign of leader
	UINT32			m_backupCandidate ;			// Auto-selected backup candidate playerID
	mutable ChatHandle	m_displacedLeader ;			// Previous (if any) player who was
												// automatically displaced from leadership

	UINT32			m_leaderLockFrom ;			// When a leader performs an action
	UINT32			m_leaderLockFor ;			// (contact report, orders) he's safe
												// aproximately until from+for
	mutable UINT32	m_leaderDisplaced ;			// When previous leader was displaced

	bool			m_electing ;				// TRUE when leader elections are open

private:
	TeulTimeMS		m_lastUpdateSent ;			//! When last update was sent to members.

private:
	MISSION_MEMBER_UPDATES m_mmu ;
} ;
