#include <syscfg.h> // Path is project specific.
// Created: Tue Oct  7 22:26:38 CDT 2008
//
// Instancing API, client functions
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "client/ww2Client.h"
#include "inst/instClient.h"
#include "inst/instClientStatus.h"
#include "inst/instDescriptor.h"
#include "inst/instListing.h"
#include "inst/instTypes.h"
#include "teul/teulKit.h"
#include "wwii/wwiiCallbacks.h"

//	------------------------------	Declarations

static TEUL_CALLBACK* _XCHG_LISTING;
static TEUL_CALLBACK* _XCHG_SELECT;
static TEUL_CALLBACK* _XCHG_CREATE;

namespace Instance
{

//////////////////////////////////////////////////////////////////////
// Request a list of instances.

SINT32
_sendListingRequest(ACTION_OBJECT* const a)
{
	a->offset = ((Instance::ListingRequest*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Send a request to join a specific instance (or create it).

SINT32
_sendSelect(ACTION_OBJECT* const a)
{
	a->offset = ((Instance::Descriptor*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Send a request to create a specific instance.

SINT32
_sendCreate(ACTION_OBJECT* const a)
{
	a->offset = ((Instance::Descriptor*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a list of instances.

SINT32
_recvInstanceListing(ACTION_OBJECT* const a)
{
	static Instance::ClientListing list;

	list.setComplete(false);

	try
	{
		// unpack our new list
		list.packer(TEUL_UNPACK, a);
		list.setComplete(true);

		///TODO: Process receipt of listing
		// Be aware: exceptions may be thrown
		// Check listing.result()
	}
	catch (std::exception& e)
	{
		(void)e;
		///TODO: Handle exception. "e.what()" is a loggable error.
		Write_Debug("Runtime exception processing Instance::Listing: %s", e.what());
	}

	if(list.isComplete())
	{
		ww2::Client* client = ww2::Client::FromTeulIndex(a->teulIndex);
		if(client) client->ProcessInstanceList(&list);
	}
	else
	{
		ww2::Client* client = ww2::Client::FromTeulIndex(a->teulIndex);
		if(client) client->ProcessInstanceList(nullptr);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Process results of an instance select request.

SINT32
_recvSelect(ACTION_OBJECT* const a)
{
	Instance::Result result(a);

	if ( result.status() != Instance::_SUCCESS )
	{
		///TODO: Handle failure
	}
	else
	{
		///TODO: Handle having selected this instance.
		ww2::Client* const client = ww2::Client::FromTeulIndex(a->teulIndex);
		if(client) client->ProcessInstanceResult(result);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Process results of a create request.

SINT32
_recvCreate(ACTION_OBJECT* const a)
{
	Instance::Result result(a);

	if ( result.status() != Instance::_SUCCESS )
	{
		///TODO: Handle failure
	}
	else
	{
		///TODO: Handle having created and being selected to this instance.
		ww2::Client* client = ww2::Client::FromTeulIndex(a->teulIndex);
		if(client) client->ProcessInstanceResult(result);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// This function is provided as a sample of how to request an instancing
// list from the host.

bool
RequestListing(const Instance::Descriptor& query, SINT32 mapConnection)
{
	UINT32 requestID = Instance::Listing::getNextRequestID();
	Instance::ListingRequest request(requestID, query);
	return ( teulSend(mapConnection, _XCHG_LISTING, &request) >= 0 );
}

//////////////////////////////////////////////////////////////////////
// If you don't want to have to set up your own Descriptors...

bool
RequestListing(UINT32 instCategory, UINT32 instType, const UINT64& instOwner, SINT32 mapConnection)
{
	// Form the parameters into a descriptor
	Instance::Descriptor query(instCategory, instType, instOwner);
	return RequestListing(query, mapConnection);
}

//////////////////////////////////////////////////////////////////////
// Join me to a specific instance. Returns true if we were able to
// send the request. Returning true does not mean the instance is
// selected.

bool
RequestSelect(const Instance::Descriptor& instance, SINT32 mapConnection)
{
	return ( teulSend(mapConnection, _XCHG_SELECT, (Instance::Descriptor*)&instance) >= 0 );
}

bool
RequestSelect(UINT32 instCategory, UINT32 instType, const UINT64 instOwner, const instanceid_t id, SINT32 mapConnection)
{
	Instance::Descriptor instance(instCategory, instType, instOwner, id);
	return RequestSelect(instance, mapConnection);
}

//////////////////////////////////////////////////////////////////////
// Create an instance. Supposedly joins you to it as well
// presumably sends down a request select receive

bool
RequestCreate(const Instance::Descriptor& instance, SINT32 mapConnection)
{
	return ( teulSend(mapConnection, _XCHG_CREATE, (Instance::Descriptor*)&instance) >= 0 );
}

bool
RequestCreate(UINT32 instCategory, UINT32 instType, const UINT64 instOwner, SINT32 mapConnection)
{
	Instance::Descriptor instance(instCategory, instType, instOwner);
	return RequestCreate(instance, mapConnection);
}

bool ClientListing::receive() const
{
	if(Listing::receive())
	{
		// do something
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Early setup of all the instancing system needs.

SINT32
InstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Instance::INST_X)
		REGISTER_SENDRECV(Instance::INST_XCHG_LISTING_Y, _sendListingRequest, _recvInstanceListing, _XCHG_LISTING);
		REGISTER_SENDRECV(Instance::INST_XCHG_SELECT_Y, _sendSelect, _recvSelect, _XCHG_SELECT);
		REGISTER_SENDRECV(Instance::INST_XCHG_CREATE_Y, _sendCreate, _recvCreate, _XCHG_CREATE);
	END_CALLBACK_REGISTRATION
}

}
