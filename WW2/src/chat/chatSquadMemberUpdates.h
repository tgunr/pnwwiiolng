#pragma once
// Created: Tue Sep 20 16:30:12 CDT 2005
//
// Defines a structure for communicating squad member updates
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatSquadControl.h"

struct SquadMemberUpdate
{
	SquadMemberUpdate() { m_text[0] = 0; }
	SquadMemberUpdate(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	enum { _INVALID, _CREATE_CHECK, _CREATE, _DISBAND, _ADD, _PROMOTE, _DEMOTE, _APPOINT, _REMOVE, _APPROVE, _SET_TEXT, _SET_INTEGER };
	static const UINT32 MAX_TEXT = 400;

	UINT32		m_action;					// One of the enums
	UINT32		m_cmd;						// Command that originated this request
	UINT32		m_squadID;					// Which squad it affects
	ChatHandle	m_handle;					// Packed callsign of affected player (or squad)
	UCHAR		m_rank;						// New rank
	UCHAR		m_prev;						// Old rank
	ChatHandle	m_originator;				// Who asked for this?
	UINT32		m_coID;						// Creator ID for create
	char		m_text[MAX_TEXT+1];			// Squad's name or text notes
	UINT32		m_members[SquadControl::CREATE_PLAYERS_REQUIRED];
	UINT32		m_result;					// Did it work?

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) { a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)); return a->offset; }

	SINT32 receive(SINT32 teulIndex);
};
