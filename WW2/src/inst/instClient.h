#pragma once
// Created: Thu Oct  9 17:13:54 CDT 2008
//
// Prototypes for client-side wrapper functions
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instTypes.h"
#include "inst/instListing.h"

namespace Instance
{
	// Request a list of instances from the host that match this descriptor. If you
	// don't want to build your own descriptor, call the second variant which will
	// do it for you. Response comes back via Instance::Listing::receive() which
	// you will need to implement.
	// Returns 'true' to indicate the request was sent OK.
	extern bool RequestListing(const Instance::Descriptor& query, SINT32 mapConnection) ;
	extern bool RequestListing(UINT32 instCategory, UINT32 instType, const UINT64& instOwner, SINT32 mapConnection) ;

	// Select a specific instance. Note that returning true only indicates the
	// request was sent. See instClient.cpp/_recvSelect for the OK/fail.
	extern bool RequestSelect(const Instance::Descriptor& instance, SINT32 mapConnection) ;
	extern bool RequestSelect(UINT32 instCategory, UINT32 instType, const UINT64& instOwner, const instanceid_t& id, SINT32 mapConnection) ;

	// Ask the host to create a specific instance for me. Note: if this completes
	// the host will automatically select into it for you.
	extern bool RequestCreate(const Instance::Descriptor& instance, SINT32 mapConnection) ;
	extern bool RequestCreate(UINT32 instCategory, UINT32 instType, const UINT64& instOwner, SINT32 mapConnection) ;

	extern SINT32 InstallClientCallbacks() ;

	// client listing receiver class
	class ClientListing : public Listing
	{
	public:
		ClientListing() : Listing()
		{
		}

		virtual ~ClientListing()
		{
		}

		// client receive implementation
		virtual bool receive() const;
	};
} ;
