#pragma once
// Created: Thu Oct 13 12:50:32 CDT 2005
//
// Extension to Edit String for requesting
// a particular string
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatEditString.h"

class GenericString : public EditString
{
	// Constructors
public:
	GenericString(UINT32 withCategory=_UNDEFINED, UINT64 withID=0LL, UINT32 withModified=0
					, std::string const& withText="", ChatHandle withAuthor=0LL, bool withEditable=false)
					: EditString(withCategory, withID, withText)
					, m_modified(withModified), m_author(withAuthor), m_editable(withEditable)
					{}
	GenericString(ACTION_OBJECT* const a);
	virtual ~GenericString() {}

	// Member functions
public:
	SINT32 packer(UINT32 mode, ACTION_OBJECT* a);
	SINT32 receive(SINT32 teulIndex);

	// Accessors
public:
	UINT32 modified() const { return m_modified; }
	ChatHandle author() const { return m_author; }
	bool editable() const { return m_editable; }

	// Member variables
protected:
	UINT32		m_modified;				// Unix-timestamp of last mod
	ChatHandle	m_author;					// Packed callsign of author
	bool		m_editable;				// Whether or not you can edit it
};

