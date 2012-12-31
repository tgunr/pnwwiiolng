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
UINT32 Listing::s_listingID = 0 ;

//////////////////////////////////////////////////////////////////////
// Network constructor for a Descriptor

Descriptor::Descriptor(ACTION_OBJECT* const a)
	: m_id(0ULL), m_handle(0ULL), m_category(0), m_type(0)
{
	this->packer(TEUL_UNPACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a Descriptor

SINT32
Descriptor::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->packer(mode, &m_category) ;
	a->packer(mode, &m_type) ;
	a->packer(mode, &m_handle) ;
	a->packer(mode, &m_id) ;

	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a BasicStatus

SINT32
BasicStatus::packer(const unsigned int mode, ACTION_OBJECT* const a)
{
	m_descriptor.packer(mode, a) ;
	a->packer(mode, (void*)&m_count[0], sizeof(m_count)) ;
	a->packSecondsDelta(mode, m_createdAt, 24) ;
	a->packSecondsDelta(mode, m_startedAt, 24) ;
	a->packer(mode, m_hasPassword) ;		//}
	a->packer(mode, m_gameTime_m, 11) ;		//}
	a->packer(mode, m_capacity, 12) ;		//} 24 bits
	a->packer(mode, (UINT32*)&m_state, 8) ;

	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Network constructor for a Status object

ClientStatus::ClientStatus(ACTION_OBJECT* const a)
{
	this->packer(TEUL_UNPACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a Status object

SINT32
ClientStatus::packer(const unsigned int mode, ACTION_OBJECT* const a)
{
	BasicStatus::packer(mode, a) ;

	a->packer(mode, &m_squadMates, 16) ;

	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Network constructor for a Listing

Listing::Listing(ACTION_OBJECT* const a)
	: m_results(), m_requestID(0), m_result(_BAD_DATA), m_complete(false)
{
	if ( packer(TEUL_UNPACK, a) > 0 )
		m_complete = true ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a Listing object

SINT32
Listing::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->packer(mode, &m_requestID) ;
	a->packer(mode, &m_result, 8) ;

	if ( m_result != _SUCCESS )
	{
		m_results.clear() ;
		return a->offset ;
	}

	UINT32 numResults = (UINT32)m_results.size() ;
	a->packer(mode, &numResults, 16) ;
	if ( mode == TEUL_UNPACK )
	{
		m_results.clear();
		m_results.resize(numResults) ;
	}

	// Now unpack all of the results
	for ( UINT32 i = 0 ; i < numResults ; ++i )
	{
		m_results[i].packer(mode, a) ;
	}

	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Network Ctor for an instance listing request

ListingRequest::ListingRequest(ACTION_OBJECT* const a)
	: m_requestID(0), m_descriptor()
{
	this->packer(TEUL_UNPACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a ListingRequest

SINT32
ListingRequest::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->packer(mode, &m_requestID) ;
	m_descriptor.packer(mode, a) ;
	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Network ctor for an Instance::Result

Result::Result(ACTION_OBJECT* const a)
{
	this->packer(TEUL_UNPACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Packer for a Result

SINT32
Result::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->packer(mode, (UINT32*)(&m_status), 8) ;
	m_descriptor.packer(mode, a) ;
	return a->offset ;
}

}
