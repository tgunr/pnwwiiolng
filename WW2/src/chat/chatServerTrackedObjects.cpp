#include <syscfg.h> // Path is project specific.
// Date: Sat Mar 25 20:37:05 CST 2006
//
// Host functions for manipulating server tracked objects
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBailSTO.h"
#include "chat/chatServerTrackedObjects.h"				// Our header file
#include "wwii/wwiiServerTrackedObject.h"
#include "chat/chatGrid.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatDotCommand.h"
#include "chat/chatPlayer.h"
#include "chat/chatSTOJoules.h"
#include "chat/chatVehicleSTO.h"
#include "chat/chatHost.h"
#include "chat/chatHulkSTO.h"
#include "chat/chatOrdnanceSTO.h"
#include "chat/chatPPOSTO.h"
#include "chat/chatUMSSTO.h"
#include "chat/chatSTODamage.h"
#include "wwii/wwiiConfig.h"
#include "inst/instTypes.h"
#include "chat/chatHostMessages.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiConfigVars.h"

struct PPOConfig
{
	PPOConfig(TeulTimeMS _timeout = 0ULL, UINT64 _threshold = 0ULL, UINT64 _capacity = 0ULL)
		: m_timeout(_timeout)
		, m_damageThreshold(_threshold)
		, m_damageCapacity(_capacity)
		{}

	TeulTimeMS		m_timeout;
	UINT64			m_damageThreshold;
	UINT64			m_damageCapacity;
};

std::vector<PPOConfig> ppoConfigs;

//////////////////////////////////////////////////////////////////////
// Handle a delete request from a client.

void
_processSTOKillRequest(ACTION_OBJECT* const a)
{
	Player* const player = PlayerInterface::Lookup(a->teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLW, "t:%d ignoring sto kill request from unknown connection", a->teulIndex);
		return;
	}

	if ( player->isEffectiveGM() == false )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s ignoring sto kill from non-gm", a->teulIndex, player->playerID(), player->callsign());
		return;
	}

	// Get the type and UID.
	UINT16 type = 0;
	UINT32 uid = 0;
	a->offset = wwiiPackServerTrackedObjectType(TEUL_UNPACK, a->packed, a->offset, &type, 0);
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &uid, 32);

	ServerSTO* const sto = (ServerSTO*)ServerTrackedObject::lookup(uid);
	if ( sto == nullptr )
	{
		dbugLog(_FLM, "t:%d p:%u c:%s ignoring sto kill for non-existant sto:%u", a->teulIndex, player->playerID(), player->callsign(), uid);
		return;
	}

	if ( sto->instanceID() != player->instance().id() )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s attempting to delete an STO from another instance. sto:%u pin:%llu sin:%llu"
						, a->teulIndex, player->playerID(), player->callsign()
						, uid, sto->instanceID(), player->instance().id());
		return;
	}

	dbugLog(_FLM, "t:%d p:%u c:%s in:%llu sto:%u destroying STO at GM request"
					, a->teulIndex, player->playerID(), player->callsign()
					, sto->instanceID(), uid);

	wwiiRemoveSTO(uid);
}

//////////////////////////////////////////////////////////////////////
// Receive an STO packet from a client.

void
chatReceiveSTO(UINT16 type, ACTION_OBJECT* const a)
{
	assert( a != nullptr );

	ServerSTO* sto = nullptr;
	switch ( type )
	{
	case ServerTrackedObject::_REMOVAL:
		_processSTOKillRequest(a);
		return;
	break;

	case ServerTrackedObject::_VEHICLE:
		sto = new VehicleSTO(a);
	break;

	case ServerTrackedObject::_ORDNANCE:
		sto = new OrdnanceSTO(a);
	break;

	case ServerTrackedObject::_EFFECT:
		sto = new OrdnanceSTO(a);
	break;

	case ServerTrackedObject::_HULK:
		sto = new HulkSTO(a);
	break;

	case ServerTrackedObject::_PPO:
		sto = new PPOSTO(a);
	break;

	case ServerTrackedObject::_UMS:
		sto = new UMSSTO(a);
	break;

	case ServerTrackedObject::_BAILOUT:
		sto = new BailSTO(a);
	break;

	default:
		dbugLog(_FLW, "t:%d type:%d unhandled sto type", a->teulIndex, type);
		return;
	break;
	}

	if ( sto == nullptr )
		return;

	Player* const player = (Player*)(a->m_client);
	sto->m_instanceID = (player != nullptr) ? player->instance().id() : (instanceid_t)Instance::_CAMPAIGN;

	switch ( ServerTrackedObject::addToIndex(sto) )
	{
	case ServerTrackedObject::_ADD_SUCCESS:
		dbugLog(_FLD, "t:%d p:%u sto:%u %s:%d.%d added new sto %f/%f/%f creator:%u lifespan:%u flags:%u"
							, a->teulIndex, sto->creatorID(), sto->uid(), sto->typeName(), sto->type(), sto->subType()
							, sto->position().x(), sto->position().y(), sto->position().z()
							, sto->creatorID(), sto->lifespan(), sto->flags()
				);
		return;
	break;
	case ServerTrackedObject::_ADD_FAIL:
	case ServerTrackedObject::_ADD_DUPLICATE:
	default:
		dbugLog(_FLA, "t:%d type:%d sto:%u failed to add to index", a->teulIndex, type, sto->uid());
		delete sto;
	break;
	}
}

bool
chatAddServerTrackedObject(ServerTrackedObject* sto)
{
	if ( ServerTrackedObject::addToIndex(sto) != ServerTrackedObject::_ADD_SUCCESS )
		return false;
	return true;
}

void
chatRemoveServerTrackedObject(UINT32 stoUID)
{
	wwiiRemoveSTO(stoUID);
}

//////////////////////////////////////////////////////////////////////
// Beta command for dropping an MSP at your last location

#ifdef TEMPORARILY_DISABLED
class DotMsp : public DotCommand
{
public:
	DotMsp() : DotCommand("msp", "Deploy MSP at last known location", MISSION_LEADER, LSTR_MSP_USAGE, LSTR_MSP_DETAIL) {}
	virtual void action(Player& player, Chat::ClientMessage* message) const override;
} dotMsp;

void
DotMsp::action(Player& const player, Chat::ClientMessage* const message) const
{
	if ( player->mission() == NULL )
	{
		player->message(LSTR_CAUTION, LSTR_NOT_ON_MISSION);
		return;
	}

	if ( !player->isMissionLeader() )
	{
		player->message(LSTR_CAUTION, LSTR_NOT_MISSION_LEADER);
		return;
	}

	if ( player->lastX() == INVALID_GRID_COORD )
	{
		player->message(LSTR_CAUTION, LSTR_MSP_FROM_MAP, command());
		return;
	}

#ifdef BETA_RELEASE
	player->message(LSTR_BETA, LSTR_MSP_REMINDER, command());
#endif

	if ( ! chatForceMobileSpawn(player->missionID(), player->playerID(), player->matchHandle()
								, player->lastX(), player->lastY(), player->lastZ(), "Placed Mobile Spawn", 0) )
	{
		player->message(LSTR_CAUTION, LSTR_MSP_FAILED);
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// How an STO handles damage

bool
UMSSTO::applyJoules(const SINT64& joules, Player* const player)
{
	if ( player->side() == side() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %lldj damage to same side STO"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), joules);
		return false;
	}

	if ( damage().isDestroyed() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %lldj ignored to dead msp"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), joules);
#if defined(BETA_RELEASE)
		player->message(LSTR_BETA, LSTR_MSP_DMG_IGNORED);
#endif
		return false;
	}

	if ( damage().applyJoules(joules) == false )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %lldj not applied"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules);
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		char doneJoules[32];
		char threshJoules[32];
		strprintf(doneJoules, "%llu", joules);
		strprintf(threshJoules, "%llu", damage().threshold());
		player->message(LSTR_BETA, LSTR_STO_DMG_THRESHOLD, doneJoules, threshJoules);
#endif
		return false;
	}

	if ( damage().isDestroyed() == false )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s side:%u sto:%u side:%u did %lldj. msp at %lluj/%lluj total"
						, player->teulIndex(), player->playerID(), player->callsign(), player->side()
						, uid(), side()
						, joules, damage().accumulated(), damage().capacity());
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		PREP_CHAT_MSG(cmsg, "%lluj to sto (%llu/%llu)", joules, damage().accumulated(), damage().capacity());
		player->message(LSTR_BETA, LSTR_GENERIC, cmsg);
#endif
		return false;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u m:%u destroyed [%lldj damage]"
					, player->teulIndex(), player->playerID(), player->callsign()
					, uid(), missionID(), joules);

#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
	player->message(LSTR_BETA, LSTR_UMS_KILLED);
#endif

	if ( missionID() > 0 )
	{
		betaLog(_FLI, "m:%u sto:%u sending mspw delete", missionID(), uid());
		ChatMission* const mission = chatFindMission(missionID());
		if ( mission )
		{
			mission->setFRUCooldown(host_config.sto.fruDeathCooldown_s);
			mission->removeMSPWaypoint(uid(), true);
		}
	}

	//// CAUTION //// AT THIS POINT THE STO HAS BEEN DESTROYED! ////

	wwiiRemoveSTO(uid());

	// Tell the player's cell host that they need to be
	// credited with a kill.
	chatSendUMSKill(player, uid());

	return true;
}

bool
HulkSTO::applyJoules(const SINT64& joules, Player* const player)
{
	if ( damage().capacity() == 0 )
	{
#if defined(BETA_RELEASE)
		player->message(LSTR_BETA, LSTR_HULK_INVULNERABLE);
#endif
		return false;
	}

#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
	if ( joules < (SINT64)damage().threshold() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %lldj below %lldj threshold"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, damage().threshold());
		char doneJoules[32];
		char threshJoules[32];
		strprintf(doneJoules, "%llu", joules);
		strprintf(threshJoules, "%llu", damage().threshold());
		player->message(LSTR_BETA, LSTR_STO_DMG_THRESHOLD, doneJoules, threshJoules);
		return false;
	}
#endif

	if ( damage().applyJoules(joules) == false )
	{
		return false;
	}

	if ( damage().isDestroyed() == false )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s sto:%u did %lldj damage. %s at %lluj/%lluj total"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, typeName(), damage().accumulated(), damage().capacity());
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		PREP_CHAT_MSG(cmsg, "%lluj to %s sto (%llu/%llu)", joules, typeName(), damage().accumulated(), damage().capacity());
		player->message(LSTR_BETA, LSTR_GENERIC, cmsg);
#endif
		return false;
	}

	// Finally, it's dead-dead jim.
	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u %s destroyed [%lldj -> %lluj damage]"
					, player->teulIndex(), player->playerID(), player->callsign()
					, uid(), typeName(), joules, damage().capacity());

#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
	player->message(LSTR_BETA, LSTR_HULK_DESTROYED);
#endif

	wwiiRemoveSTO(uid());

	return true;
}

//////////////////////////////////////////////////////////////////////
// Player Placed Object damage handling.

bool
PPOSTO::applyJoules(const SINT64& joules, class Player* const player)
{
	// Player leaving a PPO.
	if ( joules == 0 )
	{
		threadLog(_FLD, "t:%d p:%u c:%s sto:%u player leaving sto (0j damage to %s)"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), typeName());
		extractPlayer(player);
		return true;
	}

	// Player entering a PPO.
	if ( joules < 0 )
	{
		// Add them to the occupants list if they're not already in it.
		auto it = std::find(m_occupants.begin(), m_occupants.end(), player);
		if ( it == m_occupants.end() )
		{
			threadLog(_FLD, "t:%d p:%u c:%s sto:%u entered %s [%lld damage]"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), typeName(), joules);
			m_occupants.push_back(player);
		}
		else
		{
			threadLog(_FLD, "t:%d p:%u c:%s sto:%u got %s entry message [%lld damage]"
								, player->teulIndex(), player->playerID(), player->callsign()
								, uid(), typeName(), joules);
		}

		// Update the last occupied time.
		m_lastOccupied = teulClock64;

		return true;
	}

	if ( damage().capacity() == 0 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u capacity 0, taking no damage"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid());
		return false;
	}

	if ( joules < (SINT64)damage().threshold() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u damage (%lldj) < damage threshold (%lluj), taking no damage"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), joules, damage().threshold());
		return false;
	}

	if ( damage().applyJoules(joules) == false )
	{
		return false;
	}

	if ( damage().isDestroyed() == false )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s sto:%u did %lldj damage. %s at %lluj/%lluj total"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, typeName(), damage().accumulated(), damage().capacity());
		return false;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u %s destroyed [%lldj -> %lluj damage]"
					, player->teulIndex(), player->playerID(), player->callsign()
					, uid(), typeName(), joules, damage().capacity());

	wwiiRemoveSTO(uid());

	return false;
}

//////////////////////////////////////////////////////////////////////
// Receive joules of damage to an STO

template<>
SINT32
STO_JOULES::receive(SINT32 teulIndex, STO_JOULES& incoming)
{
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d sto joules from unregistered player", teulIndex);
		return -EINVAL;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s %" PRSizeT " sto joules messages ", teulIndex, player->playerID(), player->callsign(), incoming.data().size());

	for ( auto it = incoming.data().begin(), end = incoming.data().end(); it != end; ++it )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u damage:%lld", teulIndex, player->playerID(), player->callsign(), it->m_uid, it->m_joules);
		ServerTrackedObject* const sto = ServerTrackedObject::lookup(it->m_uid);
		if ( sto == nullptr )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s unknown sto joules target %u", teulIndex, player->playerID(), player->callsign(), it->m_uid);
		}
		else
			sto->applyJoules(it->m_joules, player);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Constructors for generic STO

ServerSTO::ServerSTO(ACTION_OBJECT* const a)
	: ServerTrackedObject(a)
	, m_creatorID(0)
	, m_side(WWII_NO_SIDE)
	, m_missionID(0)
{
	const Player* const player = static_cast<Player*>(a->m_client);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d unregistered player sending %s sto", a->teulIndex, typeName());
		return;
	}

	m_creatorID = player->playerID();
	m_side = player->side();
	m_missionID = player->missionID();
}

//////////////////////////////////////////////////////////////////////
// Record an STO event.

void
ServerSTO::_audit(ServerSTO::EventType eventType)
{
	DBThread::Do(DBThread::Audit
				, "INSERT INTO audit.sto_history"
					" VALUES (%u,%llu,'%s',%llu,%u,%u,%u,%u,%f,%f,%f,%f,%f,%f,%u,%u)"
					, (UINT32)uid()
					, (UINT64)teulClock64
					, (const char* const)(eventType == Creation ? "Creation" : "Destruction")
					, (UINT64)m_instanceID
					, (UINT32)type()
					, (UINT32)subType()
					, (UINT32)creatorID()
					, (UINT32)missionID()
					, (FLOAT32)position().x(), (FLOAT32)position().y(), (FLOAT32)position().z()
					, (FLOAT32)attitude().x(), (FLOAT32)attitude().y(), (FLOAT32)attitude().z()
					, (UINT32)flags()
					, (UINT32)lifespan()
					);
}

//////////////////////////////////////////////////////////////////////
// Populate an STO damage object from a subtype's config.

STODamage::STODamage(UINT16 subType)
	: m_threshold(0ULL), m_capacity(0ULL), m_accumulated(0ULL)
{
	if ( subType <= 0 || subType >= ppoConfigs.size() )
		return;

	const PPOConfig& config = ppoConfigs[subType];
	m_threshold = config.m_damageThreshold;
	m_capacity = config.m_damageCapacity;
}

//////////////////////////////////////////////////////////////////////
// Constructors for a Hulk STO

HulkSTO::HulkSTO(UINT16 withSubType, const WorldPosition_m& withPosition, const AttitudeVector& withAttitude, const Player* player)
	: ServerSTO(ServerTrackedObject::_HULK, withSubType, withPosition, withAttitude, 0, host_config.sto.hulkLifetime_m * 60 * 1000, player ? player->playerID() : 0, player ? player->side() : WWII_NO_SIDE, player ? player->missionID() : 0)
	, m_damage(host_config.sto.hulkDamageThreshold, host_config.sto.hulkDamageCapacity)
{
	dbugLog(_FLD, "Creating HulkSTO");
}

HulkSTO::HulkSTO(ACTION_OBJECT* const a)
	: ServerSTO(a)
	, m_damage(host_config.sto.hulkDamageThreshold, host_config.sto.hulkDamageCapacity)
{
	if ( m_lifespan == 0 )
		m_lifespan = host_config.sto.hulkLifetime_m * 60 * 1000;
}

//////////////////////////////////////////////////////////////////////
// Constructors for an ordnance STO

OrdnanceSTO::OrdnanceSTO(ACTION_OBJECT* const a)
	: ServerSTO(a)
	, m_fromTeulIndex(a->teulIndex)
{
	if ( host_config.sto.smokeLifetimeOverrideS > 0 )
	{
		m_lifespan = (UINT32)host_config.sto.smokeLifetimeOverrideS * 1000;
	}
}

//////////////////////////////////////////////////////////////////////
// Constructors for a UMS STO

UMSSTO::UMSSTO(UINT16 withSubType, const WorldPosition_m& withPosition, const AttitudeVector& withAttitude, Player* const player)
	: ServerSTO(ServerTrackedObject::_UMS, withSubType, withPosition, withAttitude, 0, 0, player->playerID(), player->side(), player->missionID())
	, m_damage(withSubType)
{
}

UMSSTO::UMSSTO(ACTION_OBJECT* const a)
	: ServerSTO(a)
	, m_damage(host_config.sto.damageThreshold, host_config.sto.damageCapacity)
{
	if ( m_subType > 0 && m_subType < ppoConfigs.size() )
	{
		m_damage = STODamage(m_subType);
	}

}

//////////////////////////////////////////////////////////////////////
// Constructor for a PPO STO

PPOSTO::PPOSTO(ACTION_OBJECT* const a)
	: ServerSTO(a)
//	, m_damage(host_config.sto.ppoDamageThreshold, host_config.sto.ppoDamageCapacity)
	, m_damage(m_subType)
	, m_occupants(8)
	, m_maxLife(0ULL)
	, m_lastOccupied(teulClock64)
{
	m_occupants.clear();
	m_lifespan = 0;
	m_maxLife = ppoConfigs[0].m_timeout;
	if ( m_subType > 0 && m_subType < ppoConfigs.size() )
		m_maxLife = ppoConfigs[m_subType].m_timeout;
}

//////////////////////////////////////////////////////////////////////
// Remove a player from inside an STO.

void
PPOSTO::extractPlayer(const Player* const player)
{
	auto it = std::find(m_occupants.begin(), m_occupants.end(), player);
	if ( it == m_occupants.end() )
		return;

	// Tail-swap-trick to remove this entry.
	*it = m_occupants.back();
	m_occupants.pop_back();

	m_lastOccupied = teulClock64;

	threadLog(_FLD, "t:%d p:%u c:%s sto:%u left %s"
					, player->teulIndex(), player->playerID(), player->callsign()
					, uid(), typeName());
}

//////////////////////////////////////////////////////////////////////
// Test if a PPO has expired.

bool
PPOSTO::expired() const
{
	if ( m_occupants.empty() == false )
		return false;
	return ( teulClock64 - m_lastOccupied >= m_maxLife );
}

//////////////////////////////////////////////////////////////////////
// Because some STOs don't time out...

static std::string stoList;

static void
_checkSTO(ServerTrackedObject& sto)
{
	switch ( sto.type() )
	{
	case ServerTrackedObject::_UMS:
	{
		UMSSTO& umssto = (UMSSTO&)sto;
		ChatMission* mission = chatFindMission(umssto.missionID());
		if ( !mission )
		{
#ifdef BETA_RELEASE
			char buf[128];
			sprintf(buf, "ums:%u(m:%u) ", umssto.uid(), umssto.missionID());
			stoList += buf;
#endif

			umssto.deactivate();
		}
	}
	break;
	default:
	break;
	}
}

bool
chatPatrolSTOExpirations()
{
	wwiiForEachSTO(_checkSTO);
	return true;
}

#ifdef BETA_RELEASE
class DotCheckSTOs : public DotCommand
{
public:
	DotCheckSTOs() : DotCommand("checkstos", "Check all current Server Tracked Objects to see if any have leaked.", NO_MASK, LSTR_CHECKSTOS_USAGE, LSTR_CHECKSTOS_DETAIL) {}
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		stoList = "";
		wwiiForEachSTO(_checkSTO);
		if ( stoList.empty() )
			player.message(LSTR_CAUTION, LSTR_CHECKSTOS_OK);
		else
			player.message(LSTR_NOTE, LSTR_CHECKSTOS_CANCELLED, stoList.c_str());
	}
} dotCheckSTOs;
#endif

static void
_dumpSTO(ServerTrackedObject& sto)
{
	char buf[128];

	UINT32 age = (UINT32)(sto.age() / 1000);
	UINT32 life = (UINT32)(sto.lifespan() / 1000);
	const char* type = "UNK";

	switch ( sto.type() )
	{
	case ServerTrackedObject::_VEHICLE:	type = "veh"; break;
	case ServerTrackedObject::_ORDNANCE: type = "ord"; break;
	case ServerTrackedObject::_HULK: type = "hlk"; break;
	case ServerTrackedObject::_EFFECT: type = "efx"; break;
	case ServerTrackedObject::_TERRAIN: type = "ter"; break;
	case ServerTrackedObject::_PPO: type = "ppo"; break;
	case ServerTrackedObject::_UMS: type = "ums"; break;
	case ServerTrackedObject::_BAILOUT: type = "blo"; break;
	default:
	break;
	}

	sprintf(buf, "%s:%u(%u.%u,age:%u,ls:%u) "
					, type, sto.uid(), sto.type(), sto.subType(), age, life);

	stoList += buf;
}

class DotListSTOs : public DotCommand
{
public:
	DotListSTOs() : DotCommand("liststos", "List current STOs", NO_MASK, LSTR_LISTSTOS_USAGE, LSTR_LISTSTOS_DETAIL) {}
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		stoList = "";
		wwiiForEachSTO(_dumpSTO);
		if ( stoList.empty() )
			player.message(LSTR_CAUTION, LSTR_LISTSTOS_NONE);
		else
			player.message(LSTR_NOTE, LSTR_LISTSTOS_LIST, stoList.c_str());
	}
} dotListSTOs;

//////////////////////////////////////////////////////////////////////
// PPO.xml parsing/handling.

#include "tinyxml/tinyxml.h"

//////////////////////////////////////////////////////////////////////
// Get the config parameters we understand.

static PPOConfig
_getConfig(const TiXmlNode* const parent, const PPOConfig defaultConfig)
{
	if ( parent == nullptr )
		return defaultConfig;

	static const char* const timeoutAttributeName = "timeout-minutes";
	static const char* const dmgThresholdName = "damage-threshold";
	static const char* const dmgCapacityName = "damage-capacity";

	PPOConfig config(defaultConfig);

	// Look through the <param>s for <param name='timeout-minutes'/>
	for ( auto	node = parent->FirstChild("param")
		;		node != nullptr
		;		node = node->NextSibling("param")
		)
	{
		auto param = node->ToElement();
		if ( strcasecmp(param->Attribute("name"), timeoutAttributeName) == 0 )
		{
			const char* const valueStr = param->Attribute("value");
			if ( valueStr != nullptr && isdigit(*valueStr) )
			{
				config.m_timeout = (TeulTimeMS)(strtof(valueStr, nullptr) * 60.0 * 1000.0);
			}
		}
		else if ( strcasecmp(param->Attribute("name"), dmgThresholdName) == 0 )
		{
			const char* const valueStr = param->Attribute("value");
			if ( valueStr != nullptr && isdigit(*valueStr) )
			{
				config.m_damageThreshold = strtoull(valueStr, nullptr, 10);
			}
		}
		else if ( strcasecmp(param->Attribute("name"), dmgCapacityName) == 0 )
		{
			const char* const valueStr = param->Attribute("value");
			if ( valueStr != nullptr && isdigit(*valueStr) )
			{
				config.m_damageCapacity = strtoull(valueStr, nullptr, 10);
			}
		}
	}

	return config;
}

//////////////////////////////////////////////////////////////////////
// Set up everything we need for STOs.

void
chatLoadPPOXML()
{
	std::string xmlFile = cv_dataDir.GetString();
	xmlFile += "/ppo.xml";

	try
	{
		TiXmlDocument doc(xmlFile.c_str());
		if ( doc.LoadFile() == false )
			throw std::runtime_error("Unable to load ppo.xml");

		const TiXmlNode* const root = doc.FirstChild("ppos");
		if ( root == nullptr )
			throw std::runtime_error("Unable to find <ppos> container in ppo.xml");

		dbugLog(_FLI, "ppo: reading file %s", xmlFile.c_str());

		// Get the default timeout-minutes value from the <defaults> tag.
		PPOConfig defaultConfig = _getConfig(root->FirstChild("defaults"), PPOConfig());
		dbugLog(_FLIB, " defaults timeout:%llums, dmgThresh:%lluj, dmgCap:%lluj", defaultConfig.m_timeout, defaultConfig.m_damageThreshold, defaultConfig.m_damageCapacity);

		ppoConfigs.clear();

		// Make the array 1-based.
		ppoConfigs.push_back(defaultConfig);

		// Now read through the <ppo> objects in order and determine the timeouts.
		for ( auto node = root->FirstChild("ppo")
			; node != nullptr
			; node = node->NextSibling("ppo")
			)
		{
			PPOConfig config;

			try
			{
				auto ppo = node->ToElement();

				const char* const name = ppo->Attribute("name");
				const char* const desc = ppo->Attribute("description");
				const char* const type = ppo->Attribute("sto-type");

				UINT32 stoType = strtoul(type, nullptr, 10);

				dbugLog(_FLI, "ppo:%u.%u %s %s [sto-type:%s]", stoType, static_cast<UINT32>(ppoConfigs.size() + 1U), name, desc, type);

				if ( stoType == 0 )
				{
					throw std::domain_error("Ignoring type 0");
				}

				// Look up the <param name='timeout-minutes'/> or use the default
				config = _getConfig(ppo, defaultConfig);
				dbugLog(_FLIB, " timeout: %llums dmgThresh:%lluj dmgCap: %lluj", config.m_timeout, config.m_damageThreshold, config.m_damageCapacity);
		}
			catch ( std::domain_error& e )
		{
				dbugLog(_FLIB, " *%s", e.what());
		}

			ppoConfigs.push_back(config);
	}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what());
	}
}

class DotLoadPPOXml : public DotCommand
{
public:
	DotLoadPPOXml() : DotCommand("loadppoxml", "Force reload of ppo.xml", IS_PLAYNET, LSTR_GENERIC, LSTR_GENERIC) {}
	virtual bool help(Player& player) const override
	{
		if ( !allow(player) )
			return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "/loadppoxml");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Does what it says on the box.");
		return true;
	}

	virtual void action(Player& /*player*/, Chat::ClientMessage* /*message*/) const override
	{
		chatLoadPPOXML();
	}
} dotLoadPPOXml;

