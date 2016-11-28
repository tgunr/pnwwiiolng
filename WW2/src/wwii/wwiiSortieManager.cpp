#include <syscfg.h> // Path is project specific.
// Created: Wed Jan 16 15:11:09 CST 2008
//
// Functions for managing commits of sorties to the database
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSortieManager.h"				// Our header file
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Take the list of sorties that need updating and commit their state
// change to the database. Set 'fieldToClose' to either 'spawn_time'
// or 'return_time' depending on what you are closing.

bool
SortieManager::Commit()
{
	if ( m_pending.empty() )
		return false;

	static const char leadText[] = "UPDATE wwii_sortie SET %s WHERE %s AND sortie_id IN (";
	static const UINT32 MAX_WRITES = (TEXT_POOL_SIZE - (sizeof(leadText) + 2)) / 12;
	static char* buffer = threadsAllocText();
	static char* p;

	p = buffer + sprintf(buffer, leadText, m_set, m_where);

	for ( UINT32 i = 0; i < m_pending.size() && i < MAX_WRITES; ++i )
	{
		if ( i > 0 )
			*(p++) = ',';
		p += sprintf(p, "%u", m_pending[i]);
	}

	*(p++) = ')';
	*(p++) = 0;

	dbugLog(_FLD, "Updating %s/%s for %" PRSizeT " sorties", m_set, m_where, m_pending.size());

	DBThread::Do(DBThread::Default, buffer);

	// We're done with all of these
	m_pending.clear();

	return true;
}
