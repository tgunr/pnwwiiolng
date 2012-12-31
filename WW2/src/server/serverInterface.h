#pragma once
// Created: Sun Nov 19 15:22:45 CST 2006
//
// Data exposed by the teul server application
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHash.h"
class wwiiPlayer ;
class Player ;

// Map of players by ID
typedef std::unordered_map<playerid_t /*playerID*/, class PlayerStatusNode*> PLAYERID_HASH ;
typedef std::map<UINT32 /*teulIndex*/, class PlayerStatusNode*> PLAYERIDX_MAP ;

extern void* teulGetPersonaData(SINT32) ;

class ServerInterface
{
protected:
	static PLAYERID_HASH	s_byPlayerID ;
	static PLAYERIDX_MAP	s_byTeulIndex ;

public:
	static bool isServerEmpty() { return s_byPlayerID.empty() ; }
	static PlayerStatusNode* PlayerStatusNodeByPlayerID(playerid_t id)
	{
		PLAYERID_HASH::iterator it = s_byPlayerID.find(id) ;
		return ( it != s_byPlayerID.end() ) ? it->second : NULL ;
	}
	static wwiiPlayer* LookupByPlayerID(playerid_t id) ;

	static PlayerStatusNode* PlayerStatusNodeByTeulIndex(SINT32 teulIndex)
	{
		// This has to be different than just teulGetPersonaData
		// because it has to reference our PlayerStatusNode data
		if ( teulIndex < 0 ) return NULL ;
		PLAYERIDX_MAP::iterator it = s_byTeulIndex.find((UINT32)teulIndex) ;
		return ( it != s_byTeulIndex.end() ) ? it->second : NULL ;
	}
	static wwiiPlayer* LookupByTeulIndex(SINT32 teulIndex)
	{
		return (wwiiPlayer*)teulGetPersonaData(teulIndex) ;
	}
} ;

