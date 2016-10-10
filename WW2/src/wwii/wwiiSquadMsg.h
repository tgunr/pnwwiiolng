#pragma once
// Created: Fri Sep 16 17:16:03 CDT 2005
//
// Message for transporting squad structures
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSquads.h"
#include "teul/teulNetworkObject.h"
#include "wwii/wwiiPack.h"

class SquadMsg
{
public:
	// Status codes: Order is significant, see chatSquadList.cpp:sendSquadInfo
	enum { NO_SQUAD, NO_CHANGES, LIST_ONLY, INFO_ONLY, BOTH };

public:
	SquadMsg(UCHAR withStatus=NO_SQUAD) : m_squad(), m_status(withStatus), m_inviter(0LL) {}
	SquadMsg(const Squad& withSquad) : m_squad(withSquad), m_status(INFO_ONLY), m_inviter(0LL) {}
	SquadMsg(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a)
	{
		a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_status, 0);
		if ( m_status > LIST_ONLY )
			a->offset = m_squad.packer(mode, a);
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_inviter, 0);
		if ( isInvite() )
			a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_myRank, 0);
		else if ( mode == TEUL_UNPACK )
			m_myRank = 0;
		return a->offset;
	}

public:
	const ChatHandle& inviter() const { return m_inviter; }
	RANK_NO inviteRank() const { return m_myRank; }
	UCHAR status() const { return m_status; }
	const Squad& squad() const { return m_squad; }
	Squad& squad() { return m_squad; }

	void setStatus(UCHAR newStatus) { m_status = newStatus; }
	void setSquad(const Squad& toSquad) { m_squad = toSquad; }
	void setInviter(const ChatHandle& from) { m_inviter = from; }
	void setRank(RANK_NO rank) { m_myRank = rank; }

public:
	bool isInvite() const { return (m_inviter > 0LL); }
	bool isCreationInvite() const { return (m_inviter > 0LL && m_squad.squadID() == 0); }

private:
	Squad		m_squad;
	UCHAR		m_status;
	ChatHandle	m_inviter;					// 0LL unless this is an invite stub
											// And if this is non-null then:
	RANK_NO		m_myRank;					// 0 to indicate "invite"
											// >0 to indicate squad creation rank
};

typedef teul_network_object<SquadMsg> SQUAD_MSG;
