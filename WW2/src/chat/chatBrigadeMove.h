#pragma once
// Created: Fri Apr 22 19:31:54 CDT 2005
//
// Structure for manipulating a brigade movement
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBrigadeMoveResults.h"
#include "teul/teulNetworkMsg.h"
#include "wwii/wwiiFullObjID.h"

class StratHCUnit;
class CPStruct;

class BrigadeMove
{
public:
	BrigadeMove() : m_action(BMR_FAILED), m_brigadeID(0), m_delay(0), m_actionable(false) {}
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) { return (a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this))); }
	BRIGADE_MOVE_RESULT process(const ChatHandle& mover, const char* callsign, StratHCUnit* unit, CPStruct* cp, bool isGM);
	void process(const ChatHandle& playerHandle) const;

public:
	BRIGADE_MOVE_RESULT	m_action;		// What we're doing
	UINT32		m_brigadeID;			// Brigade being moved
	FULL_OBJ_ID	m_destination;			// Where it moves to
	UINT32		m_delay;				// How long till the command is/can be executed
	bool		m_actionable;			// Set to true if you want the strat host to
										// execute this move if the result is BMR_APPROVED
										// otherwise it will only test the move for validity
};

typedef teul_network_msg<BrigadeMove> BRIGADE_MOVE_MSG;

extern void chatSendBrigadeMoveResult(BRIGADE_MOVE_MSG& outgoing);
