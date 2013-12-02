#pragma once
// Created: Mon Sep 12 14:34:49 CDT 2005
//
// Classes and definitions for describing squads
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy "Ramp" Lowe

#include <map>
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatWho.h"
#include "teul/teulTime.h"

typedef std::map<const ChatHandle, Chat::Who> SquadMembers;
typedef std::vector<Recruiter> RecruiterList;

#define SQUAD_INVITATION_TIMEOUT 120

//////////////////////////////////////////////////////////////////////
// Squad list includes its own integrated index
class ClientSquad : public Squad
{
public:
	ClientSquad();
	ClientSquad(const Squad&);
	virtual ~ClientSquad() {}

	UINT32 size() { return m_members.size(); }
	void addMember(const Chat::Who& who);
	void addMembers(const SquadMembers& members);
	void removeMember(const Chat::Who& who);
	bool isMember(const ChatHandle handle) const;
	bool isSquadBrigade(const UINT32 brigadeID, bool excludeMe = true) const; // is the squad represented here?
	bool isSquadMission(const UINT32 missionID, bool excludeMe = true) const; // is the squad represented here?
	const SquadMembers& members(bool reset = false) const;
	bool modified() const { return m_modified; }

protected:

	// has member been added or removed
	mutable bool m_modified;

	// list of squad members
	SquadMembers m_members;

};

class SquadInvite
{
public:
	SquadInvite(): m_invited(0) {}
	SquadInvite(const SquadMsg& m): m_invited((UINT32)teulTime()), m_invitation(m)  {}
	virtual ~SquadInvite() {}

	SquadMsg const& msg() const { return m_invitation; }
	const wchar_t* inviter(){ return chatGetWideCallsign(m_invitation.inviter()); }
	RANK_NO inviteRank() const { return m_invitation.inviteRank(); }
	UINT32 invited() const { return m_invited; }
	bool valid() const { return ((UINT32)teulTime() < (m_invited + SQUAD_INVITATION_TIMEOUT)); }
	void Invalidate() { m_invited = 0; }

protected:

	// creation invite
	// when the invitation came in
	UINT32 m_invited;

	// invite message
	SquadMsg m_invitation;
};

// Is the host supposed to do something with this?
struct SquadRecruitment
{
	SquadRecruitment() :
		modified(false),
		recruiter(Recruiter()),
		recruiters(RecruiterList())
	{
	}

	bool modified;
	Recruiter recruiter;			// who recruited me to my currect squad
	RecruiterList recruiters;		// list of available recruiters
};
