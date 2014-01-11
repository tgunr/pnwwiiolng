#include <syscfg.h> // Path is project specific.
// Created: Mon Dec 19 15:01:32 CST 2005
//
// Inter-server common CPStruct functions
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <set>

#include "strt/strtCPFlags.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtData.h"

//////////////////////////////////////////////////////////////////////
// Check to see if the CP has any of a particular facility type
// for a specific country

bool
CPStruct::hasFacilities(UINT32 type, WWII_SIDE forSide) const
{
	FacilityStruct* fac = facilityTypeIndex[type];
	while ( fac != NULL )
	{
		if ( fac->side == forSide )
			return true;
		fac = fac->typeNext;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Function to test if there is a link from my CP to your CP
// Returns NULL if no link, otherwise returns the link struct

LinkStruct*
CPStruct::linkTo(const CPStruct* rhs) const
{
	if ( rhs == NULL || rhs == this )
		return NULL;

	// Look through our facilities and find a link
	for ( auto it = m_links.begin(), end = m_links.end(); it != end; ++it )
	{
		LinkStruct* outLink = *it;
		if ( PN_UNLIKELY(outLink == NULL || outLink->endDepot == NULL) )
			continue;

		if ( outLink->endDepot->cp == rhs )
			return outLink;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////
// Function to test if a CP has a link to a CP with the following fullObjID

LinkStruct*
CPStruct::linkTo(const FULL_OBJ_ID& rhs) const
{
	return linkTo(strtGetCP(rhs));
}

//////////////////////////////////////////////////////////////////////
// Check all of the firebases

#ifdef IS_STRAT_HOST
void
CPStruct::processFirebases(bool incoming)
{
	if ( numLinks <= 0 )
		return;

	FacilityStruct* facility;
	LinkStruct* fbLink;

	for ( facility = facilityTypeIndex[FACILITY_DEPOT]
		; facility != NULL
		; facility = facility->typeNext
		)
	{
		fbLink = incoming ? facility->m_incomingLink : facility->m_outgoingLink;
		if ( fbLink != NULL && fbLink->firebase )
			fbLink->processFirebases();
	}
}
#else
void
CPStruct::processFirebases(bool /*incoming*/)
{
}
#endif

//////////////////////////////////////////////////////////////////////
// Mark a CP as contested

void
CPStruct::setContention(bool toState)
{
	if ( toState )
	{
		flags |= CP_STAT_CONTENTION;
		m_contention = true;
	}
	else
	{
		flags &= ~CP_STAT_CONTENTION;
		m_contention = false;
	}
}

//////////////////////////////////////////////////////////////////////
// Get the flags for a specific side

CP_STATUS_FLAGS
CPStruct::flagsFor(WWII_SIDE forSide, const Instance::Descriptor& instance) const
{
	if ( instance.isCampaign() )
	{
		UINT32 mask = ((side == forSide) ? (CP_STAT_OWN_HAAC|CP_STAT_OVERRUN) : 0);
		if ( forSide != ALLIED ) mask |= CP_ALLIED_OBJECTIVES;
		if ( forSide != AXIS ) mask |= CP_AXIS_OBJECTIVES;
		return flags & ~mask;
	}
	else
	{
		///TODO:INST: Really need to return instanced flags.
		return (flags & (CP_STAT_HAS_NAVYBASE|CP_STAT_HAS_AIRBASE|CP_STAT_MAKES_RDP)) | CP_STAT_OPEN | CP_STAT_IN_SUPPLY;
	}
}

//////////////////////////////////////////////////////////////////////
// Get a particular link

LinkStruct*
CPStruct::link(UINT32 n) const
{
	if ( n >= m_links.size() )
		return NULL;
	LINKS::iterator it = m_links.begin();
	UINT32 i = 0;
	while ( i < n ) {
		++it;
		++i;
	}
	return *it;
}

//////////////////////////////////////////////////////////////////////
// Set the side of a CP and ensure it is in sideCPs

void
CPStruct::setSide(WWII_SIDE newSide)
{
	side = newSide;
}

//////////////////////////////////////////////////////////////////////
// Check whether we're in or out of supply

void
CPStruct::updateFriendlyLinkStatus()
{
	bool isLinked = ( isBridge() == false && (numLinks < 1 || numOpenLinks >= 1) );
	if ( isLinked == m_friendlyLinked )
		return;

	m_friendlyLinked = isLinked;

#ifdef IS_STRAT_HOST
	resetRouteing();
#endif
}

