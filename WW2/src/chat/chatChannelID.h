#pragma once
// Created: Thu Mar 10 10:50:26 CST 2005
//
// Defines the channelID class and related functions
//
// Copyright (C) 2005 PlayNet, Inc., All rights reserved
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiFullObjID.h"
#include "chat/chatChannelTypes.h"

//////////////////////////////////////////////////////////////////////
// A chat channel is basically an associative container describing
// a list of players who are connected through their mutual interest
// in receiving certain chat messages, such as through subscription
// to the same mission or sharing the same objective CP.
//
// These affiliations are usually through an external (non-chat)
// identifier that is unique within and specific to the channel type.
// All of these identifiers are 48 bits and less:
//  fullObjID -- 48 bits (ox, oy, id)
//  chatHandle -- 48 bits (8 characters at 6 bits each)
//  missionID -- 32 bits
//  brigadeID -- 16 bits
//  squadID   -- 32 bits
// Since we assume there are less than 256 channel types, and since
// some channel types require that IDs be treated uniquely per side,
// and since we know there are less than 256 sides, we can construct
// a 64-bit, sparsely populated channel id that guarantees uniqueness.
//  <type:8> <side:8> <id:48>
// The ChannelID class implements this and provides accessors to
// deconstruct the resulting 64bit value.
//

class ChannelID
{
public:
	// Create a channel ID from a type, side and full object id
	ChannelID(CHANNEL_TYPE _type, UINT32 _side, const FULL_OBJ_ID& _fid, const UINT64 _instID=0ULL)
		: m_instID(_instID), m_type(_type), m_side(_side), m_id(_fid.Key()) {}

	// Create a channel ID from a type, side and 32 bit id
	ChannelID(CHANNEL_TYPE _type, UINT32 _side, UINT32 _id, const UINT64 _instID=0ULL)
		: m_instID(_instID), m_type(_type), m_side(_side), m_id((UINT64)_id) {}

	// Create a channel ID from a type and 32 bit id - i.e. side zero
	ChannelID(CHANNEL_TYPE _type, UINT32 _id)
		: m_instID(0), m_type(_type), m_side(0), m_id((UINT64)_id) {}

	ChannelID(CHANNEL_TYPE _type) : m_instID(0ULL), m_type(_type), m_side(0), m_id(0) {}

	//// Accessors
	// return the instance
	const instanceid_t& InstID() const { return m_instID; }
	// return the channel type
	CHANNEL_TYPE Type() const { return (CHANNEL_TYPE) m_type; }
	// return the side
	WWII_SIDE Side() const { return (WWII_SIDE) m_side; }
	// return the type-specific id as a 64bit value
	const UINT64& SubID() const { return m_id; }

	bool IsPlaceHolder() const { return (m_type > 0 && m_id == 0); }

	// Change the side and id
	ChannelID& Specify(const instanceid_t& instID, const UINT64& id, WWII_SIDE side=WWII_NO_SIDE) { m_instID = instID; m_side = side; m_id = id; return *this; }

	bool operator == (const ChannelID& rhs) const { return (m_instID == rhs.m_instID) & (m_type == rhs.m_type) & (m_side == rhs.m_side) & (m_id == rhs.m_id); }

	bool IsSet() const { return (m_instID | m_type | m_side | m_id) != 0; }

	UINT64 HashValue() const
	{
		return (m_instID << 4 | (UINT64)m_type << 32 | (UINT64)m_side) ^ m_id;
	}

private:
#pragma pack(push, 1)
	instanceid_t		m_instID;
	UINT32				m_type : 28;
	UINT32				m_side : 4;
	UINT64				m_id;
#pragma pack(pop)
};
