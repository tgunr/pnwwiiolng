#include <syscfg.h> // Path is project specific.
// Created: Tue Oct  7 22:26:43 CDT 2008
//
// Instancing API, host functions
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <mutex>

#include "inst/instStateEnums.h"
#include "inst/instHost.h"
#include "inst/instHostInstance.h"
#include "inst/instHostInstanceImpl.h"
#include "inst/instHostTypes.h"
#include "inst/instScenario.h"
#include "inst/instTypes.h"
#include "teul/teulMicroWebServer.h"
#include "teul/teulTime.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiPlayer.h"
#include "wwii/wwiiThreads.h"

namespace Lua { extern void ProcessEventQueue(); }

//  ------------------------------  Declarations

HOST_INSTANCES g_hostInstances;
INSTANCE_UIDS g_hostInstanceUIDs;

static std::mutex s_instanceMutex;

//////////////////////////////////////////////////////////////////////
// The HostInstanceImpl generator.

static Instance::HostInstanceImpl*
_defaultHostInstanceImplFactory(Instance::HostInstance& _inst)
{
	return new Instance::HostInstanceImpl(_inst);
}

Instance::HostInstanceImplFactory* Instance::s_implFactory = _defaultHostInstanceImplFactory;

//////////////////////////////////////////////////////////////////////
// Function to register a derived HostInstanceImpl's instead of the
// base class new function.

void
Instance::RegisterHostInstanceImplFactory(Instance::HostInstanceImplFactory* factory)
{
	Instance::s_implFactory = factory;
}

//////////////////////////////////////////////////////////////////////
// HostInstance destructor.

Instance::HostInstance::~HostInstance()
{
	HostInstanceImpl* himpl = m_impl;
	m_impl = nullptr;
	if ( himpl == nullptr )
		return;

	himpl->PreDestructionHandler();

	delete himpl;
}

//////////////////////////////////////////////////////////////////////
// Static definitions for instance-type parameters.

Instance::Scenario
_getScenario(const Instance::Descriptor& descriptor)
{
	switch ( descriptor.category() )
	{

	case Instance::_TUTORIAL:
	{
		Instance::Scenario
				scenario(descriptor, "Tutorial Instance"
					, 256, 0
					, 0, 256
					, 0, 0, 0
					, 12 * 60
					, 0, 5
					, true
					);
		return scenario;
	}
	break;

	case Instance::_CAMPAIGN:
	{
		static Instance::Scenario
			scenario(Instance::g_campaign, "Campaign Scenario"
					, ~0, 0
					, 0, ~0
					, 0, 0, 0
					, 0
					, 0, ~0
					, true
					);
		return scenario;
	}
	break;

	case Instance::_SCENARIO:
	{
		static Instance::Scenario
			scenario(Instance::g_lobby, "Lobby"
					, ~0, 0
					, 0, ~0
					, 0, 0, 0
					, 0
					, 0, ~0
					, true
					);
		return scenario;
	}

	default:
	break;
	}

	static Instance::Scenario
		scenario(Instance::g_lobby, "Default Scenario"
				, 64, 0
				, 0, 64
				, 0, 0, 0
				, 12 * 60
				, 0, ~0
				, true
				);

	return scenario;
}

static Instance::HostInstance*
_getHostInstance(const Instance::Descriptor& descriptor)
{
	Instance::HostInstance* instance = Instance::HostInstance::LookupByUID(descriptor.id());
	if ( instance != nullptr )
		return instance;

	// Get the data on this scenario.
	Instance::Scenario scenario = _getScenario(descriptor);

	dbugLog(_FLI, "in:%u.%u.%llu.%llu creating new %s instance with capacity %u / gametime %u:%u"
					, descriptor.category(), descriptor.type(), descriptor.handle(), descriptor.id()
					, scenario.m_internalDescription.c_str()
					, scenario.m_capacity, (scenario.m_gameTime_m / 60), (scenario.m_gameTime_m % 60)
					);

	instance = new Instance::HostInstance(0, descriptor, scenario.m_capacity, scenario.m_gameTime_m, teulTime(), teulTime());

	g_hostInstances[descriptor] = instance;
	g_hostInstanceUIDs[descriptor.id()] = instance;

	return instance;
}

static Instance::HostInstance&
_campaignInstance()
{
	static Instance::HostInstance& campaign = *_getHostInstance(Instance::g_campaign);
	return campaign;
}

static void
_deleteInstance(const Instance::HostInstance* instance)
{
	dbugLog(_FLI, "in:%u.%u.%llu.%llu deleting instance"
					, instance->descriptor().category()
					, instance->descriptor().type()
					, instance->descriptor().handle()
					, instance->descriptor().id()
					);

	g_hostInstances.erase(instance->descriptor());
	g_hostInstanceUIDs.erase(instance->descriptor().id());

	delete instance;
}

namespace Instance
{

HostInstance*
HostInstance::LookupByUID(const instanceid_t& uid)
{
	INSTANCE_UIDS::iterator it = g_hostInstanceUIDs.find(uid);
	return ( it != g_hostInstanceUIDs.end() ) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Get the number of squad members in a squad.

UINT32
HostInstance::getSquadMemberCount(UINT32 squadID) const
{
	// Don't bother counting for someone not in a squad.
	if ( squadID == 0 )
		return 0;

	UINT32 members = 0;
	for ( auto it = m_players.begin(), end = m_players.end(); it != end; ++it )
	{
		if ( (*it)->inSquad(squadID) )
			++members;
	}
	return members;
}

//////////////////////////////////////////////////////////////////////
// Report on the adjustment of an instance count

void
_adjustmentTrack(wwiiPlayer* const player, const char* const action, const HostInstance* const inst)
{
	player->log(_FLD, "%s to in:%u.%u.%llu.%llu side:%u"
						" mps:%u counts:%u/%u/%u/%u"
					, action
					, inst->descriptor().category(), inst->descriptor().type(), inst->descriptor().handle(), inst->descriptor().id()
					, player->side()
					, inst->players().size()
					, inst->neutral(), inst->allies(), inst->axis(), inst->banned()
					);
}

//////////////////////////////////////////////////////////////////////
// Increment/decrement the player counters

void
HostInstance::incrementCount(UINT8 forSide)
{
#if defined(DEBUG)
	assert( forSide < MAX_SIDES );
#endif
	m_count[forSide]++;
#if defined(DEBUG)
	assert( m_count[forSide] <= m_players.size() );
	const size_t tally = neutral() + allies() + axis() + banned();
	assert( tally == m_players.size() );
#endif
}

void
HostInstance::decrementCount(UINT8 forSide)
{
#if defined(DEBUG)
	assert( forSide < MAX_SIDES );
	assert( m_count[forSide] > 0 );
#endif
	m_count[forSide] --;
#if defined(DEBUG)
	const size_t tally = neutral() + allies() + axis() + banned();
	assert( tally == m_players.size() );
#endif
}

//////////////////////////////////////////////////////////////////////
// Because of the asserts, we need a dedicated function in order
// to move a player between counters.

void
HostInstance::swapPlayerSide(wwiiPlayer* const player, UINT8 fromSide)
{
#if defined(DEBUG)
	assert( fromSide < MAX_SIDES && player->side() < MAX_SIDES );
	assert( m_count[fromSide] > 0 );
#endif
	{
		std::lock_guard<std::mutex> lock(s_instanceMutex);
		--m_count[fromSide];
		++m_count[player->side()];
	}
	_adjustmentTrack(player, "moved", this);
#if defined(DEBUG)
	assert( m_count[player->side()] <= m_players.size() );
	const size_t tally = neutral() + allies() + axis() + banned();
	assert( tally == m_players.size() );
#endif
}

//////////////////////////////////////////////////////////////////////
// Join a player to an instance

bool
HostInstance::addPlayer(wwiiPlayer* const player)
{
	PLAYERS::iterator it = m_players.upper_bound(player);
	if ( it != m_players.end() && *it == player )
		return false;

	// Add me and bump the counters
	{
		std::lock_guard<std::mutex> lock(s_instanceMutex);
		m_players.insert(it, player);
		incrementCount(player->side());
	}

	_adjustmentTrack(player, "added", this);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Remove a player from an instance

bool
HostInstance::removePlayer(wwiiPlayer* const player)
{
	PLAYERS::iterator it = m_players.find(player);
	if ( it == m_players.end() )
		return false;

	{
		std::lock_guard<std::mutex> lock(s_instanceMutex);
		m_players.erase(it);
		decrementCount(player->side());
	}

	_adjustmentTrack(player, "removed", this);

#if defined(DEBUG)
	assert( m_players.find(player) == m_players.end() );
#endif

	if ( m_players.empty() && descriptor().category() != Instance::_CAMPAIGN )
		_deleteInstance(this);

	return true;
}

} // namespace Instance

//////////////////////////////////////////////////////////////////////
// Add a new player to the instancing system, which means that they
// go into the campaign instance by default

bool
wwiiPlayer::SetInstance(const Instance::Descriptor& toInstance)
{
	if ( toInstance == instance() )
		return false;
	if ( m_hostInstance )
	{
		// Remove player from their old instance
		m_hostInstance->removePlayer(this);
	}

	m_hostInstance = nullptr;
	m_instance = Instance::g_invalid;
	dbugLog(_FLD, "t:%d p:%u c:%s switching to in:%u.%u.%llu.%llu", teulIndex(), playerID(), callsign(), toInstance.category(), toInstance.type(), toInstance.handle(), toInstance.id());

	if ( toInstance.isValid() == true )
	{
		// Add player to their new instance
		m_instance = toInstance;
		m_hostInstance = _getHostInstance(toInstance);
		m_hostInstance->addPlayer(this);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Change a player's side

void
wwiiPlayer::SetCountry(WWII_COUNTRY toCountry, WWII_SIDE toSide)
{
	if ( g_logDebug )
	{
		log(_FLD, "setCountry([%u,%u]=>[%u,%u])", country(), side(), toCountry, toSide);
	}

	const bool swapInstanceSide = ( toSide != side() && instance().isValid() && m_hostInstance );
	if ( swapInstanceSide )
	{
		log(_FLD, "changing side (%u => %u)", side(), toSide);
		assert( m_hostInstance->players().find(this) != m_hostInstance->players().end() );
		const WWII_SIDE fromSide = side();
		m_side = toSide;
		m_country = toCountry;
		m_hostInstance->swapPlayerSide(this, fromSide);
		return;
	}

	m_country = toCountry;
	m_side = toSide;
}

//////////////////////////////////////////////////////////////////////
// Return the number of players on a particular side in the campaign.

UINT32
instGetCampaignSideCount(UINT8 forSide)
{
	return _campaignInstance().count(forSide);
}

//////////////////////////////////////////////////////////////////////
// Get the game-time of a specific instance

UINT32
instGetInstanceTime(const Instance::Descriptor& descriptor)
{
	const Instance::HostInstance* instance = Instance::HostInstance::LookupByUID(descriptor.id());
	const UINT32 minuteTime = ( instance ) ? instance->gameTime_m() : 12 * 60;
	return minuteTime * 60;
}

//////////////////////////////////////////////////////////////////////
// Return the total number of people /playing/ in the campaign instance

UINT32
instGetCampaignPopulation()
{
	return _campaignInstance().allies() + _campaignInstance().axis();
}

