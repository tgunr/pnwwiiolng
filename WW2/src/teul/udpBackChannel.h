#pragma once
// Created: Fri Sep 29 17:53:48 CDT 2006
//
// Defines the class that links a teul persona with a
// raknet backchannel.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "RakNetTypes.h"

class BackChannel
{
public:
	BackChannel(const RakNet::AddressOrGUID& addr, UINT32 personaID) ;
	~BackChannel() ;

	void changeMTU(UINT32 bytes) ;
	UINT32 getAveragePing() ;
	bool sendUpdate(UCHAR* const data, SINT32 bitSize) ;
	void sendRemoveList(struct WORLD_UPDATE& wu) ;
	void sendResupplyIssue(UINT32 fromSerial, const ChatHandle& fromHandle) ;
	void userHasBailed(SINT32 teulIndex) ;

public:
	RakNet::AddressOrGUID	m_address ;
	UINT32					m_personaID ;
	TeulTimeMS				m_nextPingCheck ;
	UINT32					m_averagePing ;
} ;

