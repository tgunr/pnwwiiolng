#include <syscfg.h> // Path is project specific.
// Created: Tue Oct  7 16:59:22 CDT 2008
//
// Pack routines for instancing
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instBasicStatus.h"
#include "inst/instClientStatus.h"
#include "inst/instDescriptor.h"
#include "inst/instListing.h"
#include "inst/instTypes.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"

namespace Instance
{

// Used to track the current listing request, for some reason.
UINT32 Listing::s_listingID = 0;

//////////////////////////////////////////////////////////////////////
// Network constructor for a Descriptor

    Descriptor::Descriptor(ACTION_OBJECT* const a)
    {
        a->offset = this->packer(TEUL_UNPACK, a);
    }

//////////////////////////////////////////////////////////////////////
// Packer for a Descriptor

    SINT32
    Descriptor::packer(PackMode mode, ACTION_OBJECT* a)
    {
        static const SINT32 _ID_BITS = 48;
        static const SINT32 _OWNER_BITS = 64;
        static const SINT32 _CATEGORY_BITS = 12;
        static const SINT32 _TYPE_BITS = 12;

        if ( mode == TEUL_PACK )
        {
            // Make sure everything is as expected.
            assert( sizeof(ChatHandle)  <= sizeof(UINT64) );
            assert( sizeof(m_id) <= _OWNER_BITS / 8 );
            assert( m_category < (1 << _CATEGORY_BITS) );
            assert( m_type < (1 << _TYPE_BITS) );
        }

        a->packer(mode, &m_category, _CATEGORY_BITS);
        if ( m_category > 1 )
        {
            a->packer(mode, &m_type, _TYPE_BITS);
            a->packer(mode, &um_ownerID, _OWNER_BITS);
            a->packer(mode, &m_id, _ID_BITS);
        }

        if ( mode == TEUL_UNPACK )
        {
            if ( m_category == _INVALID )
                *this = g_invalid;
            else if ( m_category == _CAMPAIGN )
                *this = g_campaign;

            // Make sure everything is as expected.
            assert( sizeof(ChatHandle) <= sizeof(UINT64) );
            assert( sizeof(m_id) <= _OWNER_BITS / 8 );
            assert( m_category < (1 << _CATEGORY_BITS) );
            assert( m_type < (1 << _TYPE_BITS) );
        }

        return a->offset;
    }


    //////////////////////////////////////////////////////////////////////
    // Network constructor for a Basic Status object

    BasicStatus::BasicStatus(ACTION_OBJECT* const a)
    {
        this->packer(TEUL_UNPACK, a);
    }

    //////////////////////////////////////////////////////////////////////
    // Packer for a BasicStatus

    SINT32
    BasicStatus::packer(PackMode mode, ACTION_OBJECT* const a)
    {
        static const SINT32 _SIDE_BITS = 12;

        m_descriptor.packer(mode, a);
        a->packer(mode, &m_count[0], _SIDE_BITS);
        a->packer(mode, &m_count[1], _SIDE_BITS);
        a->packer(mode, &m_count[2], _SIDE_BITS);
        a->packer(mode, &m_count[3], _SIDE_BITS);
        a->packSecondsDelta(mode, m_createdAt, 24);
        a->packSecondsDelta(mode, m_startedAt, 24);
        a->packer(mode, &m_gameTime_m, 11);
        a->packer(mode, &m_capacity, 13);

        return a->offset;
    }

//////////////////////////////////////////////////////////////////////
// Network constructor for a Status object

ClientStatus::ClientStatus(ACTION_OBJECT* const a)
{
	this->packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer for a Status object

SINT32
ClientStatus::packer(PackMode mode, ACTION_OBJECT* const a)
{
	BasicStatus::packer(mode, a);

	a->packer(mode, &m_squadMates, 16);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Network constructor for a Listing

Listing::Listing(ACTION_OBJECT* const a)
    : m_results(), m_requestID(0), m_complete(false)
{
	if ( packer(TEUL_UNPACK, a) > 0 )
		m_complete = true;
}

//////////////////////////////////////////////////////////////////////
// Packer for a Listing object

SINT32
Listing::packer(PackMode mode, ACTION_OBJECT* const a)
{
    a->packer(mode, &m_result, 6);
    a->packer(mode, &m_requestID);

    if ( m_result != _SUCCESS )
    {
        m_results.clear();
        return a->offset;
    }

    UINT32 numResults = (UINT32)m_results.size();
    a->packer(mode, &numResults, 10);
    if ( mode == TEUL_UNPACK )
    {
        m_results.clear();
        m_results.resize(numResults);
    }

    // Now unpack all of the results
    for ( UINT32 i = 0; i < numResults; ++i )
    {
        m_results[i].packer(mode, a);
    }

    return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Network Ctor for an instance listing request

ListingRequest::ListingRequest(ACTION_OBJECT* const a)
{
	this->packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer for a ListingRequest

SINT32
ListingRequest::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->packer(mode, &m_requestID);
	m_descriptor.packer(mode, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Network ctor for an Instance::Result

Result::Result(ACTION_OBJECT* const a)
{
	this->packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer for a Result

SINT32
Result::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->packer(mode, (UINT32*)(&m_status), 8);
	m_descriptor.packer(mode, a);
	return a->offset;
}

}
