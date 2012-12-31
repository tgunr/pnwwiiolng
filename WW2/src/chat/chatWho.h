#pragma once
// Created: Thu Jul  7 10:57:41 CDT 2005
//
// Defines the Chat::Who class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"

// Chat Who Types; 'WHO' is the catch all 'just list me'
enum
{
	// Non-displayable background lists here
	CWT_SQUAD
	// Put displayable "queries" below here
,	CWT_WHO
,	CWT_TRAINERS
,	CWT_FRIENDS
,	CWT_IGNORE
,	CWT_HC
} ;

namespace Chat
{
class Who
{
public:
	Who()
		: m_handle(0LL), m_online(false)
		, m_inSameSquad(false), m_afk(false), m_hc(false)
		, m_extendedInfo(false), m_spawned(false), m_leadingMission(false), m_recruiting(false)
		, m_country(WWII_NO_COUNTRY), m_branch(BRANCH_NOT_USED), m_rank(0)
		, m_hcUnitID(0)
		, m_nearCP(nullObjID)
		, m_squadRank(0)
		, m_missionID(0)
		, m_joinedSquad(0)
		{}
	Who(ACTION_OBJECT* const a) ;
	virtual ~Who() {}

public:
	// Number of seconds in a delta
	static const UINT32 _SECONDS_PER_DAY = 24 * 60 * 60 ;
	// How many days before we switch to a lower granularity
	static const UINT32 _MAX_DELTA_DAYS = 90 ;
	// How many days in a month
	static const UINT32 _DAYS_PER_MONTH = 30 ;

public:
	void setBasics(const ChatHandle& withHandle, bool isOnline, bool isInMySquad, RANK_NO withSquadRank, const ChatHandle& withSquadHandle=0LL, UINT32 withJoinedSquad=0)
	{
		m_handle = withHandle ;
		m_online = isOnline ;
		m_inSameSquad = isInMySquad ;
		m_squadRank = withSquadRank ;
		m_squadHandle = withSquadHandle ;
		m_extendedInfo = false ;
		m_joinedSquad = withJoinedSquad ;
	}

	void setPrimaryDetails(bool withAfk, bool hc, WWII_COUNTRY withCountry, BRANCH_ENUM withBranch, RANK_NO withRank, UINT32 withHcUnitID, UINT32 withMissionID, bool isLeadingMission, bool isRecruiting)
	{
		m_extendedInfo = false ;
		m_hc = hc ;
		m_afk = withAfk ;
		m_country = withCountry ;
		m_branch = withBranch ;
		m_rank = withRank ;
		m_hcUnitID = withHcUnitID ;
		m_missionID = withMissionID ;
		m_leadingMission = isLeadingMission ;
		m_recruiting = isRecruiting ;
	}

	void setExtendedDetails(bool withSpawned, const FULL_OBJ_ID& withNearCP)
	{
		m_extendedInfo = true ;
		m_spawned = withSpawned ;
		m_nearCP = withNearCP ;
	}

public:
	// No network handling (send/receive) - those are done by teulNetworkArray
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;

public:
	// Accessors
	bool afk() const { return m_afk ; }
	BRANCH_ENUM branch() const { return m_branch ; }
	WWII_COUNTRY country() const { return m_country ; }
	bool extendedInfo() const { return m_extendedInfo ; }
	const ChatHandle& handle() const { return m_handle ; }
	UINT32 hcUnitID() const { return m_hcUnitID ; }
	bool highCommander() const { return m_hc ; }
	bool inSameSquad() const { return m_inSameSquad ; }
	bool leadingMission() const { return m_leadingMission ; }
	UINT32 missionID() const { return m_missionID ; }
	const FULL_OBJ_ID& nearCP() const { return m_nearCP ; }
	bool online() const { return m_online ; }
	RANK_NO rank() const { return m_rank ; }
	bool recruiting() const { return m_recruiting ; }
	bool spawned() const { return m_spawned ; }
	const ChatHandle& squadHandle() const { return m_squadHandle ; }
	RANK_NO squadRank() const { return m_squadRank ; }
	UINT32 joinedSquad() const { return m_joinedSquad ; }

private:
	ChatHandle		m_handle ;

	bool			m_online ;					// If false, nothing else will be initialized
	bool			m_inSameSquad ;				// Whether player is in your squad
	bool			m_afk ;						// Whether player is afk
	bool			m_hc ;						// Whether player is acting as a high commander
	bool			m_extendedInfo ;			// Whether we have extended info
	bool			m_spawned ;					// Whether player is spawned
	bool			m_leadingMission ;			// Whether player is leading a mission
	bool			m_recruiting ;				// Whether player is acting as a recruit or not
												// (ONLY sent if they are a squad mate)

	WWII_COUNTRY	m_country ;
	BRANCH_ENUM		m_branch ;
	RANK_NO			m_rank ;
	UINT32			m_hcUnitID ;
	FULL_OBJ_ID		m_nearCP ;					// Only populated if m_inSquad
	RANK_NO			m_squadRank ;				// Player's rank within their squad
	UINT32			m_missionID ;				// Mission they are on
	ChatHandle		m_squadHandle ;				// Name of player's squad (not supplied by host if inSameSquad)

	// If this player is a squad mate, this is an aproximation of when they joined
	// 0-89 days has day-to-day granularity
	// 3 months to 10 years has monthly granularity
	// Anything longer is listed as 11 years ago
	UINT32			m_joinedSquad ;
} ;
} // namespace Chat
