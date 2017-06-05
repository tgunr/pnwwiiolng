#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 21 14:55:05 CST 2006
//
// Functions for the server tracked object classes
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>

#include "wwii/wwiiServerTrackedObject.h"                // Our header file
#include "wwii/wwiiPack.h"
#include "teul/teulPackers.h"

#define HOST_STO_SERIAL_START 10000

// On the host we use a serialized object kept in a database to track
// what the next STO UID should be. On the client, which shouldn't
// really be creating STO UIDs except in offline mode,
// it can just be a UINT32
#if platBuild == BUILD_SERVER
# include "wwii/wwiiSerialNumber.h"
  static SerialNumber stoUniqueID("sto_uid", HOST_STO_SERIAL_START, 100, 0x7fffffff);	// max is 2^31 - 1
#else
  static UINT32 stoUniqueID = 0;
#endif

// UID-based lookup table for STOs
using STO_MAP = std::map<UINT32,ServerTrackedObject*>;
static STO_MAP stoMap;

//////////////////////////////////////////////////////////////////////
// STO base class pack routine

SINT32
wwiiPackServerTrackedObjectType(PackMode mode, UCHAR* const buffer, SINT32 offset, UINT16* const type, SINT32 /*size*/)
{
	return teulPackUINT16(mode, buffer, offset, type, 4);
}

// My poor, poor STO packer. I blame Ramp for how complicated and
// messy it has gotten =/
SINT32
ServerTrackedObject::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->offset = wwiiPackServerTrackedObjectType(mode, a->packed, a->offset, &m_type, 0);

	UINT32 mask = 0;
	enum
	{
		LARGE_UID = 0x01
	,	LARGE_SUB = 0x02
	,	HAS_FLAGS = 0x04
	,	LARGE_FLAGS = 0x08
	,	HAS_LIFESPAN = 0x10
	,	LARGE_LIFESPAN = 0x20
	,	LARGE_AGE = 0x40
	,	MASK_BITS = 8
	};

	TeulTimeMS packAge = 0ULL;
	UINT32 packLife = 0;

	static const UINT32 SCALE_FACTOR = 25;
	if ( mode == TEUL_PACK )
	{
		if ( m_uid >= 1 << 24 )
			mask |= LARGE_UID;
		if ( m_subType >= 1 << 8 )
			mask |= LARGE_SUB;
		if ( m_flags > 0 )
			mask |= HAS_FLAGS;
		if ( m_flags >= 1 << 16 )
			mask |= LARGE_FLAGS;
		if ( m_lifespan > 0 )
		{
			mask |= HAS_LIFESPAN;
			packLife = (m_lifespan + SCALE_FACTOR - 1) / SCALE_FACTOR;
			if ( packLife >= 1 << 12 )
				mask |= LARGE_LIFESPAN;
			packAge = (teulClock64 + 1 - m_created) / SCALE_FACTOR;
			if ( packAge >= 1 << 12 )
				mask |= LARGE_AGE;
		}
	}

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &mask, MASK_BITS);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_uid, (mask & LARGE_UID) ? 32 : 24);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_subType, (mask & LARGE_SUB) ? 16 : 8);

	if ( mask & HAS_FLAGS )
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_flags, (mask & LARGE_FLAGS) ? 32 : 16);

	if ( mask & HAS_LIFESPAN )
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &packLife, (mask & LARGE_LIFESPAN) ? 24 : 12);
		a->offset = teulPackUINT64(mode, a->packed, a->offset, &packAge, (mask & LARGE_AGE) ? 24 : 12);
	}

	if ( mode == TEUL_UNPACK )
	{
		m_created = teulClock64 - (packAge * SCALE_FACTOR);
		m_lifespan = packLife * SCALE_FACTOR;
		m_activated = false;
	}

	m_attitude.packer(mode, a);
	m_position.packer(mode, a);

	switch(m_type)
	{
		case _BAILOUT:
			a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_owner, 0);
		break;
		default:
			// do nothing
		break;
	}

	return a->offset;
}


//////////////////////////////////////////////////////////////////////
// Look up an STO by UID

ServerTrackedObject*
ServerTrackedObject::lookup(UINT32 uid)
{
	STO_MAP::iterator it = stoMap.find(uid);
	return ( it != stoMap.end() ) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Default behavior for STO_Implementation activation

bool
ServerTrackedObject::activate()
{
	if ( activated() )
		return false;
	m_activated = true;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Add an STO to the index

int
ServerTrackedObject::addToIndex(ServerTrackedObject* sto)
{
	if ( sto->uid() == 0 )
	{
		sto->m_uid = static_cast<UINT32>(++stoUniqueID);

#if platBuild != BUILD_SERVER
		// local client sto ids range from 1 and 99
		if(Online)
		{
			if(stoUniqueID >= HOST_STO_SERIAL_START)
				stoUniqueID = 0;
		}
#endif
	}

	// Find if this is a duplicate, or where to insert the entry if not
	STO_MAP::iterator it = stoMap.lower_bound(sto->uid());
	if ( it != stoMap.end() && it->first == sto->uid() )
		return ServerTrackedObject::_ADD_DUPLICATE;

	// Until we reach this point, the object should only be "constructed",
	// it shouldn't have affected the game engine. Now we need it to do its stuff
	if ( !sto->activate() )
		return ServerTrackedObject::_ADD_FAIL;

	stoMap.insert(it, std::make_pair(sto->uid(), sto));

	return ServerTrackedObject::_ADD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Base implementation of applyJoules, does nothing

bool
ServerTrackedObject::applyJoules(const SINT64& /*joules*/, Player* const /*player*/)
{
	return false;
}

//////////////////////////////////////////////////////////////////////
// Default behavior for STO_Implementation deactivation

void
ServerTrackedObject::deactivate()
{
	m_activated = false;
}

//////////////////////////////////////////////////////////////////////
// Remove a particular STO

void wwiiRemoveSTO(UINT32 uid)
{
	STO_MAP::iterator it = stoMap.find(uid);

	if( it == stoMap.end() )
	{
		dbugLog(_FLD, "sto:%u not in map, couldn't remove", uid);
		return;
	}

	if( it->second != nullptr )
	{
		// Call the object's destructor
		if ( it->second->activated() )
			it->second->deactivate();

		delete it->second;
	}

	// Remove the map entry
	stoMap.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Is this a valid STO?

bool wwiiValidSTO(UINT32 uid)
{
	STO_MAP::iterator it = stoMap.find(uid);
	return (it != stoMap.end()) ? true : false;
}

//////////////////////////////////////////////////////////////////////
// Get an STO by UID

ServerTrackedObject* wwiiGetSTO(UINT32 uid)
{
	STO_MAP::iterator it = stoMap.find(uid);
	return (it != stoMap.end()) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Patrol the STO map for objects that need to be expired

void wwiiExpireSTOs()
{
	if ( stoMap.empty() )
		return;

	std::vector<UINT32> expired;

	// Get me a list of all the expirable STOs
	STO_MAP::iterator it = stoMap.begin();
	while (it != stoMap.end())
	{
		if (it->second->getCanExpire())
		{
			if (it->second->expired())
				expired.push_back(it->first);
		}
		++it;
	}

	if ( expired.empty() )
		return;

	// Now delete all of those STOs
	size_t numExpired = expired.size();
	for ( size_t i = 0; i < numExpired; ++i )
	{
		it = stoMap.find(expired[i]);
		if ( it == stoMap.end() )
		{
#if platBuild == BUILD_SERVER
			LogAbort("stoMap is corrupt: could not re-find sto#%u", expired[i]);
#else
			continue;
#endif
		}

		ServerTrackedObject* const sto = it->second;
		if ( sto->activated() )
		{
#if platBuild == BUILD_SERVER
			dbugLog(_FLD, "sto:%u expired, deactivating", sto->uid());
#endif
			sto->deactivate();
		}

		stoMap.erase(it);

		delete sto;
	}
}

//////////////////////////////////////////////////////////////////////
// Apply a function across all current STOs.

void
wwiiForEachSTO(STO_ITERATOR func)
{
	STO_MAP::iterator it = stoMap.begin();
	while ( it != stoMap.end() )
	{
		ServerTrackedObject* const sto = it->second;
		++it;
		if ( sto != nullptr )
			func(*sto);
	}
}

//////////////////////////////////////////////////////////////////////
// Apply a function across all current STOs with handler function data

bool
wwiiForEachSTO(STO_COOKIE_ITERATOR func, void* cookie)
{
	STO_MAP::iterator it = stoMap.begin();

	while ( it != stoMap.end() )
	{
		ServerTrackedObject* const sto = it->second;
		++it;
		if ( sto != nullptr && func(*sto, cookie))
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Get a count of type/subtype stos...passing a 0 for either is equivalent to a wildcard
UINT32
wwiiCountSTOs(UINT16 type, UINT16 subtype)
{
	UINT32 count = 0;

	STO_MAP::iterator it = stoMap.begin();
	while ( it != stoMap.end() )
	{
		ServerTrackedObject* const sto = it->second;
		++it;
		if ((type == 0 || sto->type() == type) && (subtype == 0 || sto->subType() == subtype))
			++count;
	}

	return count;
}
