#include <syscfg.h> // Path is project specific.
// Created: Thu Aug 25 22:41:31 CDT 2005
//
// Class for pooling datagrams and buffers
// to reduce the amount of mallocing they use
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulDatagram.h"

#if platOS == OS_MACOS
typedef std::set<DATA_GRAM*> DATAGRAMS;
#else
typedef UNORDERED_SET<DATA_GRAM*> DATAGRAMS;
#endif

static DATAGRAMS s_datagrams;
static size_t maxGrams = 0;

//////////////////////////////////////////////////////////////////////
// Given a requested data size, work out the best buffer size for it

static UINT32
_getAllocationSize(const UINT32 size)
{
	return (size + 1024) & ~1023;
}

DATA_GRAM*
teulGetDatagram(const UINT32 size)
{
	const UINT32 allocSize = _getAllocationSize(sizeof(DATA_GRAM) + size);
	DATA_GRAM* gram = (DATA_GRAM*)memCalloc(1, allocSize);
	// Point contents to memory immediately AFTER the gram header.
	gram->contents = (Teul::PacketHeader*)(gram + 1);
	gram->m_size = size;		// The actual requested size.

	s_datagrams.insert(gram);

	if ( s_datagrams.size() > maxGrams )
		maxGrams = s_datagrams.size();

	return gram;
}

void
teulReleaseDatagram(DATA_GRAM* const gram)
{
	DATAGRAMS::iterator it = s_datagrams.find(gram);
	if ( it == s_datagrams.end() )
	{
		LogAbort("trying to free unallocated datagram");
	}

	s_datagrams.erase(it);

	memFree(gram);
}

//////////////////////////////////////////////////////////////////////
// Shutdown the datagram pool and free everything up

void
teulFreeDatagramPool()
{
	dbugLog(_FLN, "Releasing datagram pool. size:%" PRSizeT " maxSize:%" PRSizeT, s_datagrams.size(), maxGrams);

	for ( DATAGRAMS::iterator it = s_datagrams.begin(); it != s_datagrams.end(); ++it )
	{
		memFree(*it);
	}

	s_datagrams.clear();
}

