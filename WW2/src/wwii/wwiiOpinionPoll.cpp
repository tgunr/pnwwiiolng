#include <syscfg.h> // Path is project specific.
// Created: Fri Jul 29 11:42:03 CDT 2005
//
// Functions for the opinion poll class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiOpinionPoll.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"

//////////////////////////////////////////////////////////////////////
// Return the title of the opinion poll or nullptr

const char*
OpinionPoll::title() const
{
	if ( empty() )
		return nullptr;
	else
		return m_title.c_str();
}

//////////////////////////////////////////////////////////////////////
// Return the text for 0-based opinion i

const char*
OpinionPoll::entry(UINT32 i) const
{
	if ( i >= m_size )
		return nullptr;
	return m_strings[i];
}

//////////////////////////////////////////////////////////////////////
// Return the number of opinion polls

UINT32
OpinionPoll::size() const
{
	if ( empty() )
		return 0;
	else
		return m_size;
}

//////////////////////////////////////////////////////////////////////
// Add a string entry

void
OpinionPoll::addEntry(const char* newEntry)
{
	if ( size() > _MAX_STRINGS )
		return;

	char* dest = m_strings[m_size++];

	if ( newEntry == nullptr )
		*dest = 0;
	else
	{
		strncpy(dest, newEntry, _MAX_STRING_LEN);
		dest[_MAX_STRING_LEN] = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Packer function

SINT32
OpinionPoll::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_pollID, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_size, 8);

	a->offset = wwiiPackString(mode, a->packed, a->offset, m_title, _MAX_TITLE_LEN);

	for ( UINT32 i = 0; i < m_size && i < _MAX_STRINGS; ++i )
	{
		a->offset = wwiiPackText(mode, a->packed, a->offset, m_strings[i], _MAX_STRING_LEN);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Packer function for OpinionChoice

SINT32
OpinionChoice::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_pollID, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_choice, 8);
	return a->offset;
}
