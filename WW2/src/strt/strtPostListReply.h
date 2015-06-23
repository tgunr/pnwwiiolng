#pragma once
// Date: Wed Jul 11 11:30:10 CDT 2007
//
// List of multicrewable vehicles
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//This message contains a list of posted vehicles at a facility
struct PostListReply
{
	PostListReply() {}
	PostListReply(COMMANDED_WEAPON_ACTION action, UINT32 customerID, UINT16 facilityOID)
		: m_action(action), m_customerID(customerID), m_facilityOID(facilityOID)
		{}

	static SINT32 Process(ACTION_OBJECT* const a);

	COMMANDED_WEAPON_ACTION m_action;
	UINT32					m_customerID;
	UINT16					m_facilityOID;
	// Followed by a bunch of MM_POSTED_WEAPON entries.
};

