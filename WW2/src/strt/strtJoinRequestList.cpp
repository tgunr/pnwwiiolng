#include <syscfg.h> // Path is project specific.
// Created: Fri Apr 20 17:07:02 2001
//
// list management routines for JoinRequest Lists.  These lists
// are attached to mapPersonas as people request to join thier vehicles
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "strt/strtServerTypes.h"
#include "strt/strtJoinList.h"
#include "mem/mem.h"
#include "cnvy/cnvyMain.h"
#include "strt/strtHostMessages.h"

typedef std::map<UINT32, WeapJoinRequestNode*> JOIN_MAP;
static JOIN_MAP joinMap;

//////////////////////////////////////////////////////////////////////
// Remove all members from a join request list.

void
JOIN_LIST::clear(UINT32 exceptPlayerID, bool announce)
{
	WeapJoinRequestNode* exceptedNode = NULL;
	while ( m_list.empty() == false )
	{
		WeapJoinRequestNode* node = m_list.front();
		if ( exceptPlayerID > 0 && node->m_playerID == exceptPlayerID )
		{
			exceptedNode = node;
			m_list[0] = m_list.back();
			m_list.resize(m_list.size() - 1);
		}
		else
		{
			if ( announce )
				strtSendMMJoinDeny(cnvyGetMapConnection(), node->m_playerID, MM_JOIN_FAIL_POSTER_DENIED_ACCESS);
			// Remember: Delete will extract the node from the list.
			delete node;
		}
	}

	m_list.clear();

	assert( m_list.empty() );

	if ( exceptedNode )
		m_list.push_back(exceptedNode);
}

//////////////////////////////////////////////////////////////////////
// Look up a join node by it's playerID

WeapJoinRequestNode*
WeapJoinRequestNode::Lookup(UINT32 playerID)
{
	JOIN_MAP::iterator it = joinMap.find(playerID);
	return ( it != joinMap.end() ) ? it->second : NULL;
}

//////////////////////////////////////////////////////////////////////
// Ctor for WeapJoinRequestNode

WeapJoinRequestNode::WeapJoinRequestNode(UINT32 playerID, const ChatHandle& handle, UINT32 missionID)
	: prev(NULL), next(NULL)
	, m_joinList(NULL)
	, m_playerID(playerID)
	, m_missionID(missionID)
	, m_capacityTeulIndex(-1)
	, m_handle(handle)
{
	ABORT_IF_EX(m_playerID == 0, "attempted to create WeapJoinRequestNode with playerID of 0");
	ABORT_IF_EX( m_handle == 0, "p:%u attempted to create WeapJoinRequestNode with handle of 0", m_playerID);

	memset(&m_capacityRequest, 0, sizeof(m_capacityRequest));

	if ( g_logDebug )
	{
		dbugLog(_FLD, "p:%u c:%s ptr:%p +ALLOC WeapJoinRequestNode", m_playerID, Chat::Name(m_handle).c_str(), this);
	}

	_register();
}

//////////////////////////////////////////////////////////////////////
// Dtor for a WeapJoinRequestNode

WeapJoinRequestNode::~WeapJoinRequestNode()
{
	if ( m_joinList != NULL )
	{
		dbugLog(_FLD, "p:%u removing node %p from list %p on dtor"
						, m_playerID, this, this->m_joinList
						);
		m_joinList->Extract(this);
	}

	_unregister();

	if ( g_logDebug )
	{
		dbugLog(_FLD, "p:%u c:%s ptr:%p -REL", m_playerID, Chat::Name(m_handle).c_str(), this);
	}

	m_playerID = 0;
	m_handle = 0ULL;
}

//////////////////////////////////////////////////////////////////////
// Register a join node to the global map

void
WeapJoinRequestNode::_register()
{
	if ( WeapJoinRequestNode::Lookup(m_playerID) != NULL )
	{
		LogAbort("p:%u c:%s attempted to allocate additional WeapJoinRequestNode", m_playerID, Chat::Name(m_handle).c_str());
	}

	joinMap[m_playerID] = this;

	dbugLog(_FLD, "p:%u ptr:%p +REG", m_playerID, this);
}

//////////////////////////////////////////////////////////////////////
// Unregister a join node from the global map

void
WeapJoinRequestNode::_unregister()
{
	JOIN_MAP::iterator it = joinMap.find(m_playerID);
	if ( it == joinMap.end() )
	{
		LogAbort("p:%u ptr:%p attempt to free node twice!!", this->m_playerID, this);
	}

	if ( it->second != this )
	{
		LogAbort("p:%u ptr:%p attempting to free unmapped join node (mapped node %p)", this->m_playerID, this, it->second);
	}

	joinMap.erase(it);

	dbugLog(_FLD, "p:%u ptr:%p -REG", m_playerID, this);
}

//////////////////////////////////////////////////////////////////////
// Add this node to a JoinList, automatically extracting ourselves
// from any list we are currently registered to.

bool
WeapJoinRequestNode::AddToList(JOIN_LIST* dest)
{
	if ( dest == NULL )
	{
		LogAbort("p:%u c:%s ptr:%p attempted to add JRN to NULL list", m_playerID, Chat::Name(m_handle).c_str(), this);
	}

	// if the node is currently on a list...
	if ( m_joinList != NULL )
	{
		// Already on the dest list...
		if ( m_joinList == dest )
		{
			dbugLog(_FLD, "p:%u c:%s ptr:%p attempted to re-add to list %p"
							, m_playerID, Chat::Name(m_handle).c_str(), this, m_joinList);
			return true;
		}

		m_joinList->Extract(this);
	}

	dbugLog(_FLI, "p:%u c:%s ptr:%p adding to list %p type %u"
					, m_playerID, Chat::Name(m_handle).c_str(), this
					, dest, dest->m_type);

	dest->Add(this);

	return true;
}

void
JOIN_LIST::Add(WeapJoinRequestNode* node)
{
	assert( node != NULL );

	try
	{
#ifdef DEV_BUILD
		if ( node->m_playerID == 0 ) throw "playerID == 0";
		if ( WeapJoinRequestNode::Lookup(node->m_playerID) != node ) throw "orphaned node";
#endif
		if ( node->m_joinList == this ) throw "unnecessary attempt to re-add node";
		if ( node->m_joinList != NULL ) throw "node is already on another list";
		for ( UINT32 i = 0; i < m_list.size(); ++i )
		{
			if ( m_list[i] == node || m_list[i]->m_playerID == node->m_playerID )
				throw "attempt to re-add orphaned node";
		}

		m_list.push_back(node);
		node->m_joinList = this;

		if ( g_logDebug )
			dbugLog(_FLD, "p:%u c:%s ptr:%p list:%p Added Node (%" PRSizeT ")"
							, node->m_playerID, Chat::Name(node->m_handle).c_str(), node, this, m_list.size());
	}
	catch ( const char* error )
	{
		LogAbort("p:%u c:%s ptr:%p list:%p njl:%p Add failed: %s"
						, node->m_playerID, Chat::Name(node->m_handle).c_str(), node, this, node->m_joinList, error);
	}
}

void
JOIN_LIST::Extract(WeapJoinRequestNode* node)
{
	assert( node != NULL );

	try
	{
		if ( node->m_playerID == 0 ) throw "Node has playerID 0";
		if ( node->m_joinList == NULL ) throw "NULL joinList";
		if ( node->m_joinList != this ) throw "Node not on this list";
		if ( m_list.empty() ) throw "list is empty";

		for ( UINT32 i = 0; i < m_list.size(); ++i )
		{
			if ( m_list[i] != node )
			{
				if ( m_list[i]->m_playerID == node->m_playerID )
					throw "Found orphaned clone";
				continue;
			}

			node->m_joinList = NULL;

			// Swap the last node in to this slot.
			m_list[i] = m_list.back();
			m_list.resize(m_list.size() - 1);

			if ( g_logDebug )
				dbugLog(_FLD, "p:%u c:%s ptr:%p list:%p Removed Node [%" PRSizeT "]"
								, node->m_playerID, Chat::Name(node->m_handle).c_str(), node, this, m_list.size());

			return;
		}
	}
	catch ( const char* error )
	{
		LogAbort("p:%u c:%s ptr:%p join list extract failed from list %p: %s (njl=%p)"
						, node->m_playerID, Chat::Name(node->m_handle).c_str(), node, this, error, node->m_joinList);
	}
}

