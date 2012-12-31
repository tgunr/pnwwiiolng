#pragma once
// Date: Sat Dec 16 02:11:58 CST 2006
//
// The exchange of orders for an attack/cancellation
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum ORDER_TYPE { ORD_ATTACK, ORD_DEFEND, ORD_CLEAR } ;

#pragma pack(push, 1)
struct AttackOrder
{
	AttackOrder(const ChatHandle& requester, UINT32 brigadeID, const FULL_OBJ_ID& target, ORDER_TYPE orderType, bool isExecute)
		: m_requester(requester), m_brigadeID(brigadeID), m_target(target), m_orderType((UCHAR)orderType), m_execute(isExecute)
		, m_result(0), m_delay(0)
	{
	}

	void process() ;

	ChatHandle	m_requester ;
	UINT32		m_brigadeID ;
	FULL_OBJ_ID	m_target ;
	UCHAR		m_orderType ;
	bool		m_execute ;
	UINT32		m_result ;
	UINT32		m_delay ;
} ;
#pragma pack(pop)

