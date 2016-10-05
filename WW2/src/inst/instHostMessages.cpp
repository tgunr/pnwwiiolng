#include <syscfg.h> // Path is project specific.
// Created: Thu Oct 23 13:30:45 CDT 2008
//
// Functionality for the authoritative instancing server.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instClientStatus.h"
#include "inst/instDescriptor.h"
#include "inst/instHost.h"
#include "inst/instHostInstanceImpl.h"
#include "inst/instHostTypes.h"
#include "inst/instListing.h"
#include "inst/instTypes.h"
#include "server/serverInterface.h"
#include "map/mapPlayer.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPlayer.h"
#include "wwii/wwiiSerialNumber.h"
#include "wwii/wwiiThreadTextQuery.h"

//	------------------------------	Declarations

static TEUL_CALLBACK* _XCHG_LISTING;
static TEUL_CALLBACK* _XCHG_SELECT;
static TEUL_CALLBACK* _XCHG_CREATE;

// Track instance IDs through wwiionline.wwii_serial.
static SerialNumber instanceSerialNumber("instance", Instance::_TUTORIAL, 10, 0);

using namespace Instance;

//////////////////////////////////////////////////////////////////////
// Process a request for a list of instances from a client.

static SINT32
_recvInstanceRequest(ACTION_OBJECT* const a)
{
	wwiiPlayer* const player = ServerInterface::LookupByTeulIndex(a->teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLD, "t:%d Instance::Listing request with no player", a->teulIndex);
		return -EINVAL;
	}

	// The client actually sends a descriptor describing the
	// listing they want.
	Instance::ListingRequest query(a);

	Instance::Listing result;					// Container for reply
	if ( !result.populate(player, query) )
	{
		return -EINVAL;
	}

	return teulSend(a->teulIndex, _XCHG_LISTING, &result);
}

//////////////////////////////////////////////////////////////////////
// Pack an Instance::Listing to a client.

static SINT32
_sendInstanceListing(ACTION_OBJECT* const a)
{
	return ((Instance::Listing*)(a->localRawData))->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Response to an instance creation request.

static
SINT32 _sendInstanceCreated(ACTION_OBJECT* const a)
{
	return ((Instance::Result*)(a->localRawData))->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Response to an instance selection request.

static
SINT32 _sendInstanceSelect(ACTION_OBJECT* const a)
{
	return ((Instance::Result*)(a->localRawData))->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Assisgn a descriptor a new instanceid_t

static Descriptor
_getNewInstanceDescriptor(const Descriptor& request)
{
	return Descriptor(request.category(), request.type(), request.handle(), instanceSerialNumber.getNext());
}

//////////////////////////////////////////////////////////////////////
// Select a semi-shared tutorial instance.

static Descriptor
_pickTutorialInstance(wwiiPlayer* const player, const Descriptor& request, HOST_INSTANCES::iterator& it)
{
	UINT32 bestScore = 0;
	Descriptor best;
	for (; it != g_hostInstances.end() && it->first.isInstanceOf(request); ++it )
	{
		UINT32 score = 0;
		HostInstance& instance = *it->second;

		// Ignore full instances
		if ( instance.population() >= instance.capacity() )
			continue;

		// But otherwise look for empty slots.
		score += instance.capacity() - instance.population();

		if ( score > bestScore )
		{
			best = instance.descriptor();
			bestScore = score;
		}
	}

	// No winner, create a new instance.
	if ( bestScore == 0 )
		best = _getNewInstanceDescriptor(request);

	return best;
}

//////////////////////////////////////////////////////////////////////
// Select a semi-shared tutorial instance.

static Descriptor
_pickTrainingInstance(wwiiPlayer* const player, const Descriptor& request, HOST_INSTANCES::iterator& it)
{
	UINT32 bestScore = 0;
	bool matched = false;
	Descriptor best;
	for (; it != g_hostInstances.end() && it->first.isInstanceOf(request); ++it )
	{
		UINT32 score = 0;
		HostInstance& instance = *it->second;

		// Ignore full instances
		if ( instance.capacity() != 0 && instance.population() >= instance.capacity() )
			continue;

		if ( instance.capacity() == 0 ) // Just find population
			score += instance.population();
		else
		{
			// Balance
			const UINT32 sideCap = (instance.capacity() / 2);

			// Don't allow > half on one side.
			const UINT32 ourPop = instance.count(player->side());
			if ( ourPop >= sideCap )
				continue;

			const UINT32 theirPop = instance.count(wwiiOtherSide(player->side()));
			if ( ourPop + 1 <= theirPop )
			{
				// We are underpop
				score += (sideCap - ourPop) * 2 + (theirPop - ourPop);
			}
			else
			{
				// We are overpop
				score += (sideCap - ourPop);
			}
		}

		// Look for squad mates.
		score += (SINT32) instance.getSquadMemberCount(player->squadID()) * 2;

		if ( matched == false || score > bestScore )
		{
			matched = true;
			best = instance.descriptor();
			bestScore = score;
		}
	}

	// No winner, create a new instance.
	if ( matched == false )
		best = _getNewInstanceDescriptor(request);

	return best;
}

//////////////////////////////////////////////////////////////////////
// Try to pick the specified instance, or create a new one.

static Descriptor
_pickOtherInstance(const wwiiPlayer* const player, const Descriptor& request, HOST_INSTANCES::iterator& it)
{
	if ( it == g_hostInstances.end() || it->first != request )
		return _getNewInstanceDescriptor(request);
	return it->first;
}

//////////////////////////////////////////////////////////////////////
// Given a descriptor, try to find a suitable instance to place this
// player in.

static Descriptor
_pickInstance(wwiiPlayer* const player, Instance::Descriptor& request)
{
	switch ( request.category() )
	{
	case Instance::_INVALID:
		return Instance::g_invalid;
	break;
	case Instance::_CAMPAIGN:
		return Instance::g_campaign;
	break;
	case Instance::_TUTORIAL:
		request.m_type = player->side();
	break;
	case Instance::_SCENARIO:
		request = Descriptor(request.category(), request.type(), 0, 0);
	break;
	default:
	break;
	}

	// Check to see if any instances of this descriptor are available
	HOST_INSTANCES::iterator it = g_hostInstances.lower_bound(request);
	if ( it == g_hostInstances.end() || it->first.isInstanceOf(request) == false )
		return _getNewInstanceDescriptor(request);

	switch ( request.category() )
	{
	case Instance::_TUTORIAL:
		return _pickTutorialInstance(player, request, it);
	break;
	case Instance::_TRAINING:
		return _pickTrainingInstance(player, request, it);
	break;
	case Instance::_SCENARIO:
		return it->first;
	break;
	default:
	break;
	}

	return _pickOtherInstance(player, request, it);
}

//////////////////////////////////////////////////////////////////////
// Process a client request to select an instance.

static
SINT32 _recvInstanceSelect(ACTION_OBJECT* const a)
{
	Instance::Descriptor request(a);

	if ( a->offset < 0 )
	{
		dbugLog(_FLA, "t:%d bad data during instance request (%d)", a->teulIndex, a->offset);
		Instance::Result result(g_invalid);
		result.m_status = Instance::_BAD_DATA;
		teulSend(a->teulIndex, _XCHG_SELECT, &result);
		return a->offset;
	}

	bool success = false;
	wwiiPlayer* const player = ServerInterface::LookupByTeulIndex(a->teulIndex);
	if ( player != nullptr )
	{
		player->log(_FLD, "received instance request %u.%u.%llu.%llu"
					, request.category(), request.type(), request.handle(), request.id());
		if ( request.category() == Instance::_INVALID )
		{
			player->log(_FLW, "request for instance 0 ignored");
		}
		else if ( player->instance() == request )
		{
			// player->setInstance returns true/false depending
			// on whether any change was successfully made; if
			// the player is already in this instance it would
			// return false, which is not what we want.
			player->log(_FLD, "redundant instance request");
			success = true;
		}
		else
		{
			// Player's supply an ID of 0 when they want
			// to create an instance or have the host
			// select one for them.
			if ( request.id() == 0 )
			{
				request = _pickInstance(player, request);
			}

			success = player->SetInstance(request);
		}
	}
	else
	{
		dbugLog(_FLW, "t:%d instance request from unregistered connection", a->teulIndex);
	}

	Instance::Result result(request);
	if ( success == true )
		result.m_status = Instance::_SUCCESS;

	return teulSend(a->teulIndex, _XCHG_SELECT, &result);
}

//////////////////////////////////////////////////////////////////////
// Process a request to create an instance.

static
SINT32 _recvInstanceCreate(ACTION_OBJECT* const a)
{
	Instance::Descriptor request(a);

	bool success = false;
	wwiiPlayer* const player = ServerInterface::LookupByTeulIndex(a->teulIndex);
	if ( player != nullptr )
	{
		if ( request.id() != 0 )
		{
			player->log(_FLA, "received instance create request with non-zero id: %u.%u.%llu.>%llu<"
					, request.category(), request.type(), request.handle(), request.id());
		}
		else
		{
			player->log(_FLD, "received instance create request %u.%u.%llu.%llu"
						, request.category(), request.type(), request.handle(), request.id());
			request = _pickInstance(player, request);
		}

		success = player->SetInstance(request);
	}
	else
	{
		dbugLog(_FLW, "t:%d instance request from unregistered connection", a->teulIndex);
	}

	Instance::Result result(request);
	if ( success == true )
		result.m_status = Instance::_SUCCESS;

	return teulSend(a->teulIndex, _XCHG_SELECT, &result);
}

namespace Instance
{

//////////////////////////////////////////////////////////////////////
// Populate a Listing based on a player requesting a list of instances
// that match the Descriptor.

bool
Listing::populate(wwiiPlayer* const player, const ListingRequest& request)
{
	// Clients track individual requests so that they can time them out
	// incase they send multiple requests in a lag spike, etc.
	m_requestID = request.m_requestID;

	const Descriptor& query = request.m_descriptor;
	// Some really obvious gruft:
	if ( query == Instance::g_campaign )
	{
		m_result = _SPECIAL_REQUESTED;
		dbugLog(_FLM, "t:%d p:%u c:%s requesting instance category %u listing"
						, player->teulIndex(), player->playerID(), player->callsign()
						, query.category()
						);
		return true;
	}

	if ( query.id() != 0LL )
	{
		m_result = _BAD_DESCRIPTOR;
		dbugLog(_FLM, "t:%d p:%u c:%s invalid descriptor [%u.%u.%llu.>%llu<]"
						, player->teulIndex(), player->playerID(), player->callsign()
						, query.category(), query.type(), query.handle(), query.id());
		return true;
	}

	if ( query.category() >= 500 && (query.category() % 100) == 0 && query.type() == 0 )
	{
		HOST_INSTANCES::const_iterator top = g_hostInstances.lower_bound(Instance::Descriptor(query.category(), 0));
		HOST_INSTANCES::const_iterator bottom   = g_hostInstances.lower_bound(Instance::Descriptor(query.category() + 100, 0));

		for ( auto it = top; it != bottom; ++it )
		{
			const UINT32 squadMates = it->second->getSquadMemberCount(player->squadID());
			ClientStatus status(*it->second, squadMates);
			m_results.push_back(status);
		}
	}
	else if ( query.category() != 0 )
	{
		Instance::Descriptor base(query.category(), query.type());

		for ( auto it = g_hostInstances.lower_bound(base)
			;	it != g_hostInstances.end() && it->first.isInstanceOf(query)
			;	++it
			)
		{
			const UINT32 squadMates = it->second->getSquadMemberCount(player->squadID());
			ClientStatus status(*it->second, squadMates);
			m_results.push_back(status);
		}
	}
	else
	{
		for ( auto it = g_hostInstances.begin(); it != g_hostInstances.end(); ++it )
		{
			const UINT32 squadMates = it->second->getSquadMemberCount(player->squadID());
			ClientStatus status(*it->second, squadMates);
			m_results.push_back(status);
		}
	}

	m_result = _SUCCESS;
	m_complete = true;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set up the callbacks.

SINT32
InstallHostCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Instance::INST_X)
	{
		REGISTER_RECVSEND(Instance::INST_XCHG_LISTING_Y, _recvInstanceRequest, _sendInstanceListing, _XCHG_LISTING);
		REGISTER_RECVSEND(Instance::INST_XCHG_SELECT_Y, _recvInstanceSelect, _sendInstanceSelect, _XCHG_SELECT);
		REGISTER_RECVSEND(Instance::INST_XCHG_CREATE_Y, _recvInstanceCreate, _sendInstanceCreated, _XCHG_CREATE);
	}
	END_CALLBACK_REGISTRATION(Instance::INST_X)
}

//////////////////////////////////////////////////////////////////////
// For whichever host decides to do it, this will generate an audit
// trail of how many players in each instance for each side.

void
RecordPopulations()
{
	///TODO:INST: This data needs shrinking at the end of a
	/// campaign.
	// Record once a minute for fidelity, we'll need something to
	// automatically compress the table at a later point.
	static const UINT32 _RUN_TIME_INTERVAL_S = 1 * 60;
	static UINT32 nextRunTime_s = 0;
	if ( teulTime() < nextRunTime_s )
		return;
	nextRunTime_s = teulTime() + _RUN_TIME_INTERVAL_S;

	// Use a ThreadTextQuery to batch up the inserts and then commit them
	// in the background through a worker thread.
	ThreadTextQuery query(DBThread::Audit, "INSERT IGNORE INTO `audit`.`instance_populations` VALUES","");
	char entry[128];
	for ( auto it = g_hostInstanceUIDs.begin(); it != g_hostInstanceUIDs.end(); ++it )
	{
		Instance::HostInstance* instance = it->second;
		dbugLog(_FLD, "rp:inst:%u.%u.%llu.%llu"
								, instance->descriptor().category()
								, instance->descriptor().type()
								, instance->descriptor().handle()
								, instance->descriptor().id()
								);

		for ( UINT32 side = 0; side < MAX_SIDES; ++side )
		{
			const UINT32 count = instance->count(side);
			if ( count != 0 || count != instance->m_lastRecorded[side] )
			{
				sprintf(entry, "(%u,%u,%llu,%llu,NOW(),%u,%u)"
								, instance->descriptor().category()
								, instance->descriptor().type()
								, instance->descriptor().handle()
								, instance->descriptor().id()
								, side
								, instance->count(side)
								);
				query.addEntry(entry);
				instance->m_lastRecorded[side] = count;
			}
		}
	}

	// Ok - whatever is left, if anything, needs saving.
	query.commit(0);
}

}
