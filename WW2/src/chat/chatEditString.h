#pragma once
// Created: Thu Oct 13 12:34:07 CDT 2005
//
// Class for exchanging exchanging text fields without
// other contextual information
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class EditString
{
	// Private enumerations and constants
public:
	enum									// String categories
	{
		_UNDEFINED
	,	_SIDE_MOTD							// Side 0 = CRS, 1 = Allied, 2 = Axis, etc
	,	_HCUNIT_MOTD						// Change motd of a HCunit
	};
	enum
	{
		_NO_ERROR							// No error occured
	,	_ACCEPTED							// Change accepted
	,	_INVALID							// Invalid category/ID
	,	_PERMISSION_DENIED					// You couldn't change it
	,	_TOO_LONG							// Text too long
	};

	// Maximum absolute text length
	static const UINT32 _MAX_TEXT_LENGTH = 1000;

	// Constructors/destructors
public:
	EditString(UINT32 withCategory=_UNDEFINED, UINT64 withId=0LL, std::string const& withText="")
		: m_category(withCategory), m_id(withId), m_text(withText), m_error(_NO_ERROR)
		{}
	EditString(ACTION_OBJECT* const a);

	// Member functions
public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
	SINT32 receive(SINT32 teulIndex);
	void setError(UINT32 toError) { m_error = toError; m_text = ""; }

	// Accessors
public:
	UINT32 category() const { return m_category; }
	UINT64 id() const { return m_id; }
	const std::string& text() const { return m_text; }
	std::string& text() { return m_text; }
	UINT32 error() const { return m_error; }

	// Booleans
public:
	bool isError() const { return m_error > _ACCEPTED; }

	// Member variables
protected:
	UINT32		m_category;					// Which category
	UINT64		m_id;						// Id within the category
	std::string	m_text;
	UINT32		m_error;					// Any error kicked up
};
