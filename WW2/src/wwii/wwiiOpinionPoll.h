#pragma once
// Created: Fri Jul 29 11:33:24 CDT 2005
//
// Describes the OpinionPoll class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"

//////////////////////////////////////////////////////////////////////
// Class that describes an opinion poll.

class OpinionPoll
{
public:
	// Constructors/destructors
	OpinionPoll() : m_pollID(0), m_size(0), m_title("") {}
	OpinionPoll(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	~OpinionPoll() {}

public:
	// Accessors
	bool empty() const { return (m_title[0] == 0 || m_size <= 0); }
	const char* entry(UINT32 i) const;		// NULL if no such entry
	UINT32 pollID() const { return m_pollID; }
	UINT32 size() const;
	const char* title() const;				// NULL if none set

public:		// Member functions
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
	void addEntry(const char* entry);

private:
	enum { _MAX_TITLE_LEN = 240 };
	enum { _MAX_STRINGS = 8, _MAX_STRING_LEN = 120 };

public:	// Members
	UINT32		m_pollID;
	UINT32		m_size;
	std::string	m_title;
	char		m_strings[_MAX_STRINGS][_MAX_STRING_LEN];
};

typedef teul_network_object<OpinionPoll> OPINION_POLL_MSG;

class OpinionChoice
{
public:
	OpinionChoice(UINT32 withPollID=0, UINT32 withChoice=0) : m_pollID(withPollID), m_choice(withChoice) {}
	OpinionChoice(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
	~OpinionChoice() {}

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

public:
	UINT32 pollID() const { return m_pollID; }
	UINT32 choice() const { return m_choice; }

private:
	UINT32		m_pollID;
	UINT32		m_choice;
};

typedef teul_network_object<OpinionChoice> OPINION_CHOICE_MSG;
