#pragma once
// Created: Sat Jan 23 21:37:16 CST 2010
// Copyright (C) PlayNet, Inc 2010-2011 - All rights reserved.
// Not for redistribution.
//
// Auth host login queue function prototypes
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Auth
{
struct ClientData ;

	namespace Queue
	{
		extern bool AddPlayer(UINT32 arenaID, ClientData* cdata, bool premium) ;
		extern bool RemovePlayer(UINT32 arenaID, ClientData* cdata, bool premium, bool enteredGame) ;

		extern ClientData* GetNextPlayer(UINT32 arenaID) ;

		extern void ProcessQueues() ;
	}
}
