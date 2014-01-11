#pragma once
// Created: Mon Sep 19 11:18:52 CDT 2005
//
// Types for manipulating/working with squad lists
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSquads.h"
#include "teul/teulTime.h"

class Player ;
struct FilterFn ;

class SquadMemberEntry
{
public:
//	SquadMemberEntry() {}
	SquadMemberEntry(const ChatHandle& withHandle, Player* withPlayer, RANK_NO withSquadRank, UINT32 withJoined=0)
		: m_handle(withHandle), m_player(withPlayer), m_squadRank(withSquadRank), m_joined(withJoined), m_msstamp(teulClock64), m_recruiting(false) {}
	~SquadMemberEntry() {}

public:
	const ChatHandle& handle() const { return m_handle ; }
	const Player* player() const { return m_player ; }
	Player* player() { return m_player ; }
	RANK_NO squadRank() const { return m_squadRank ; }
	TeulTimeMS msstamp() const { return m_msstamp ; }
	SINT32 teulIndex() const ;
	UINT32 joined() const { return m_joined ; }
	bool recruiting() const { return m_recruiting ; }

protected:
	ChatHandle	m_handle ;
	Player*		m_player ;
	RANK_NO		m_squadRank ;
	UINT32		m_joined ;
	TeulTimeMS	m_msstamp ;
	bool		m_recruiting ;

	friend class SquadMemberQuery ;
	friend class SquadList ;
} ;

typedef std::unordered_map<ChatHandle,SquadMemberEntry> SQUAD_MEMBERS ;

class SquadList
{
public:
	SquadList() : m_squad(), m_pulled(0), m_members(), m_online(0), m_newSquad(false) {}
	SquadList(const Squad& squad) ;
	~SquadList() {}

public:
	const SQUAD_MEMBERS& members() const { return m_members ; }
	SQUAD_MEMBERS::iterator begin() { return m_members.begin() ; }
	SQUAD_MEMBERS::const_iterator begin() const { return m_members.begin() ; }
	SQUAD_MEMBERS::iterator end() { return m_members.end() ; }
	SQUAD_MEMBERS::const_iterator end() const { return m_members.end() ; }
	SquadMemberEntry* find(const ChatHandle& handle)
	{
		SQUAD_MEMBERS::iterator it = m_members.find(handle) ;
		return ( it == m_members.end() ) ? nullptr : &(it->second) ;
	}
	bool isNewSquad() const { return m_newSquad ; }
	TeulTimeMS lastMemberMod() const { return m_lastMemberMod ; }
	TeulTimeMS msstamp() const { return m_msstamp ; }
	UINT32 online() const { return m_online ; }
	UINT32 pulled() const { return m_pulled ; }
	Squad& squad() { return m_squad ; }
	const Squad& squad() const { return m_squad ; }

public:
	void announce() ;
	void appointNewCO(const ChatHandle& newCO, Player* player) ;
	void changeMemberEntry(SquadMemberEntry& member, RANK_NO squadRank, Player* player) ;
	void created() ;
	void electNewCO() ;
	void memberJoin(Player* player, const ChatHandle& handle, RANK_NO atRank, UINT32 joined, const char* recruitedBy, bool announceToSquad) ;
	void memberLeave(bool removeFromDB, SquadMemberEntry* entry, int reasonLstringID, const char* extraText=nullptr) ;
	void removeMember(ChatHandle handle) ;
	void renamed(const ChatHandle& newHandle) ;
	void updateMember(const ChatHandle& handle, RANK_NO squadRank, Player* player, UINT32 joined=0) ;
	void updateRecruiter(const ChatHandle& handle, bool isRecruiting) ;

	void sendMessage(int prefix, int message, const char* m1=nullptr, const char* m2=nullptr) ;
	void changeNotice(Player* player, int message) ;

	void setFullName(const std::string& name) ;
	void setDescription(const std::string& name) ;
	void setMOTD(const std::string& name) ;
	void setHomepage(const std::string& name) ;
	void setLanguage(UINT32 language) ;
	void setRecruiting(bool newFlag, Player* player=nullptr) ;
	void setSlogan(const std::string& text) ;
	void setSquad(const Squad& toSquad) { m_squad = toSquad ; setUpdated() ; }
	void setSquadNight(const std::string& text) ;
	void setUpdated() { m_msstamp = teulClock64 ; }
	void clearUpdated() { m_msstamp = 0LL ; }
	void setVoting(bool newFlag, Player* player=nullptr) ;

private:
	void _addMember(const ChatHandle& handle, RANK_NO squadRank, Player* player, UINT32 joined) ;
	void _updateOnline(int by) ;

private:
	TeulTimeMS		m_msstamp ;				// Modified timestamp
	TeulTimeMS		m_lastMemberMod ;		// Last member modification

public:
	Squad			m_squad ;
//	UINT64			m_lastListedRecruiters ;// Timestamp of when we were last listed for recruiting
//	UINT32			m_lastRecruiter ;		// Whom we last listed

private:
	UINT32			m_pulled ;				// When the squad was last fetched
	SQUAD_MEMBERS	m_members ;
	UINT32			m_online ;
	bool			m_newSquad ;			// Squad in the process of creation

	friend class SquadMemberQuery ;
} ;
