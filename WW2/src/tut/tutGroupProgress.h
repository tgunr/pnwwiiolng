#ifndef PN_TUT_GROUPPROGRESS_H
#define PN_TUT_GROUPPROGRESS_H 1
// Created: Tue Oct 21 12:28:22 CDT 2008
//
// Structure used to exchange tutorial progress between client
// and host.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

class Player ;

namespace Tutorial
{
	// The grunt that provides the per-group data
	struct GroupProgress
	{
		ChatHandle		m_groupHandle ;
		UINT64			m_flags ;

		GroupProgress(ChatHandle _handle=0LL, UINT64 _flags=0LL) : m_groupHandle(_handle), m_flags(_flags) {}
		GroupProgress(ACTION_OBJECT* const a) ;

		void packer(LOGICAL mode, ACTION_OBJECT* a) ;

		//// CLIENT:
		// Send this progress to the host.
		void send(SINT32 chatConnection) ;

		//// BOTH:
		// Receive this progress; the client
		// can just ignore the 'Player' argument.
		SINT32 receive(Player* player) const ;
	} ;
}

#if platBuild == TEUL_SERVER
#include <vector>
namespace Tutorial
{
	// For the host
	typedef std::vector<GroupProgress> GroupProgressBatch ;
}

namespace Map
{
	// Send a batch or tutorial progress statuses to a player
	extern void SendTutProgressBatch(SINT32 teulIndex, Tutorial::GroupProgressBatch& batch) ;
}
#endif

#endif // PN_TUT_GROUPPROGRESS_H
