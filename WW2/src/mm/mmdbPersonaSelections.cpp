#include <syscfg.h> // Path is project specific.
// Date: Tue Feb 21 16:43:31 CST 2006
//
// Thread for saving persona brigade selections
// XXX Should also save side locks etc, eh?
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <mysql.h>

#include "map/mapPlayer.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHash.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreadTypes.h"
#include "mmdbPersonaSelections.h"

//////////////////////////////////////////////////////////////////////

// PERSONA_SELECTIONS are personaID->brigadeID
typedef std::unordered_map<UINT32,UINT32> _PERSONA_SELECTIONS;
typedef std::unordered_set<UINT32> _PERSONAS;
typedef std::unordered_map<UINT32,_PERSONAS> _BRIGADE_PERSONAS;

static _PERSONA_SELECTIONS _selections;

static std::mutex _mutex;
static volatile UINT32 _selectionRequests = 0;

//////////////////////////////////////////////////////////////////////
// Save a persona selection

void
mmdbSavePersonaSelection(const Player* player)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_selections[player->personaID()] = player->brigadeID();
	++_selectionRequests;
}

//////////////////////////////////////////////////////////////////////
// Critical (locked) section of _saveAllPersonas()
static void
_grabPersonas(_BRIGADE_PERSONAS& personas)
{
	// Do this early to allow the main thread the chance to skip
	// some locking
	std::lock_guard<std::mutex> lock(_mutex);
	
	for ( auto it = _selections.begin(); it != _selections.end(); ++it )
	{
		personas[it->second].insert(it->first);
	}

	_selections.clear();

	// Incase another thread beat us to the lock...
	_selectionRequests = 0;
}

//////////////////////////////////////////////////////////////////////
// Save the current batch of selections

static void
_saveSelections(DB::handle_t* conn)
{
	static _BRIGADE_PERSONAS _byBrigade;
	_byBrigade.clear();

	_grabPersonas(_byBrigade);

	// We're going to build a statement per-brigade with a list of
	// personas that should be set to that particular brigade, on
	// the basis that this may produce a much smaller set of
	// statements and sql operations than individual updating
	// each persona.

	try
	{
		_BRIGADE_PERSONAS::iterator it;
		_PERSONAS::iterator pit;

		// Set up a DBQuery structure once for efficiency
		DBQuery q(conn);

		static const UINT32 _BUF_SIZE = 4015;
		char buf[_BUF_SIZE+1];
		for ( it = _byBrigade.begin(); it != _byBrigade.end(); ++it )
		{
			UINT32 offset = 0;
			buf[0] = 0;

			// Iterate across the personas registered to this brigade,

			for ( pit = it->second.begin();; ++pit )
			{
				bool isLast = (pit == it->second.end());
				// If we fill up buf execute the query and proceed.
				if ( isLast || offset > _BUF_SIZE - 20 )
				{
					buf[offset++] = ')';
					buf[offset] = 0;

					q.Query(offset, buf);

					offset = 0;
					buf[0] = 0;

					// Check if we ran out of personas in this brigade
					if ( isLast )
						break;
				}

				// Add a new persona or start filling buf up from scratch
				if ( offset == 0 )
					offset = strprintf(buf
								, "UPDATE LOW_PRIORITY wwii_persona SET modified = NOW(), brigade_id = %u WHERE personaID IN (%u"
									, (UINT32)(it->first)
									, *pit);
				else
					offset += sprintf(buf+offset, ",%u", (UINT32)*pit);
			}
		}
	}
	catch ( std::exception& e )
	{
		threadLog(_FLA, "Failed saving personas: %s", e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Thread that periodically saves personas

void
PersonaSelectionThread::run()
{
	DB::handle_t* conn = threadsOpenDBConnection(true);
	if ( conn == nullptr )
	{
		LogAbort("spt: Unable to open database connection");
	}

	threadLog(_FLM, "Persona-selections thread active");

	while ( !isCancelled() )
	{
		for (uint32_t i = 0; i < 50; ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if(isCancelled())
				return;
		}

		if ( !_selectionRequests )
			continue;

		// Wake the connection up
		mysql_ping(conn);

		_saveSelections(conn);
	}
}
