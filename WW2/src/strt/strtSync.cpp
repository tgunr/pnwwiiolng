#include <syscfg.h> // Path is project specific.
// Class and functions for synchronizing other servers with our data.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Oliver "kfsone" Smith

#include <list>
#include <algorithm>
#include <map>

#include "strt/strtSync.h"
#include "teul/teulBasic.h"
#include "strt/strtSupercell.h"
#include "strt/strtSupercells.h"
#include "strt/strtHostMessages.h"
#include "strt/strtMain.h"
#include "teul/teulTime.h"

//////////////////////////////////////////////////////////////////////
class ServerSync
{
private:
	UINT32			m_address;				// IP of server
	SUPERCELLS::iterator m_it;
	bool			m_started;				// True when we start
	TeulTimeMS		m_startAt;

public:
	// Constructor
	ServerSync(UINT32 server)
		: m_address(server)
		, m_it(g_supercells.end())
		, m_started(false)
		, m_startAt(teulClock64 + 2000)
	{
		if ( m_address == 0 )
			dbugLog(_FLA, "Tried to create ServerSync for address 0.0.0.0");
	}

	// Get the address of this server
	UINT32 address() const { return m_address; }

	bool atEnd() const { return m_it == g_supercells.end(); }

	// Tests if the synchronization is complete
	bool complete() const { return ((m_started && atEnd()) || m_address == 0); }

	// Return the* name* of the server's IP address
	const char* ip() const;

	// Send the current supercell and advance
	void sync();
};

//////////////////////////////////////////////////////////////////////
using SYNC_LIST = std::list<ServerSync>;
static SYNC_LIST synclist;

//////////////////////////////////////////////////////////////////////
// Return the IP address in textual format for the server

const char*
ServerSync::ip() const
{
	return teulGetAddress(m_address);
}

//////////////////////////////////////////////////////////////////////
// SyncServer function that sends the current supercell
void
ServerSync::sync()
{
	if ( m_startAt > teulClock64 )
		return;

	UINT32 i = 1;
	while ( !complete() && i-- > 0 )
	{
		if ( !m_started )
		{
			m_it = g_supercells.begin();
			if ( atEnd() )
				return;
			m_started = true;
		}

		strtSyncCPs(m_address, 0, &m_it->second);
		strtSyncFacilities(m_address, 0, &m_it->second);

		strtSyncObjects(m_address, 0, &m_it->second);

		++m_it;
	}
}

//////////////////////////////////////////////////////////////////////
// Find a server in the sync list

SYNC_LIST::iterator
_findServerSync(UINT32 address)
{
	for ( auto it = synclist.begin(), end = synclist.end(); it != end; ++it )
	{
		if ( it->address() == address )
		{
			dbugLog(_FLI, "%s is already on the synclist", teulGetAddress(address));
			return it;
		}
	}

	return synclist.end();
}

//////////////////////////////////////////////////////////////////////
// Add a new host to the list of servers needing to be sync'd

void
strtAddSyncPeer(UINT32 address)
{
	SYNC_LIST::iterator it = _findServerSync(address);
	if ( it != synclist.end() )
	{
		dbugLog(_FLI, "%s is already on the synclist", teulGetAddress(address));
		return;
	}

	synclist.push_back(ServerSync(address));

	dbugLog(_FLN, "Queued %s for synchronization. %" PRSizeT " servers on queue"
					, teulGetAddress(address)
					, synclist.size()
					);
}

//////////////////////////////////////////////////////////////////////
// Remove a host from the sync list

void
strtRemoveSyncPeer(UINT32 address)
{
	SYNC_LIST::iterator it = _findServerSync(address);
	if ( it != synclist.end () )
	{
		dbugLog(_FLI, "removing %s from the synclist", it->ip());
		synclist.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////
// Check the synclist for servers waiting on another supercell

UINT32
strtSyncPeers()
{
	// Purge any completed entries from the front of the list
	while ( !synclist.empty() && synclist.front().complete() )
	{
		dbugLog(_FLI, "completed synchronization of %s", synclist.front().ip());
		synclist.pop_front();
	}

	if ( synclist.empty() )
		return 0;

	// Tell the head of the list to send its next supercell.
	synclist.front().sync();

	return synclist.size();
}

