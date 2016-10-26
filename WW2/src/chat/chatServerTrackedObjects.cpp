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
#include "wwii/wwiiConfigVars.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiPPOLoader.h"

extern PPOSpecLoader specLoader;

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
		dbugLog(_FLW, "t:%d p:%u c:%s attempting to delete an STO from another instance. sto:%u pin:%" PR64BitUT " sin:%" PR64BitUT ""
						, a->teulIndex, player->playerID(), player->callsign()
						, uid, sto->instanceID(), player->instance().id());
		return;
	}

	dbugLog(_FLM, "t:%d p:%u c:%s in:%" PR64BitUT " sto:%u destroying STO at GM request"
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
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %" PR64BitT "j damage to same side STO"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), joules);
		return false;
	}

	if ( damage().isDestroyed() )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %" PR64BitT "j ignored to dead msp"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), joules);
#if defined(BETA_RELEASE)
		player->message(LSTR_BETA, LSTR_MSP_DMG_IGNORED);
#endif
		return false;
	}

	if ( damage().applyJoules(joules) == false )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %" PR64BitT "j not applied"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules);
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		char doneJoules[32];
		char threshJoules[32];
		strprintf(doneJoules, "%" PR64BitUT "", joules);
		strprintf(threshJoules, "%" PR64BitUT "", damage().threshold());
		player->message(LSTR_BETA, LSTR_STO_DMG_THRESHOLD, doneJoules, threshJoules);
#endif
		return false;
	}

	if ( damage().isDestroyed() == false )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s side:%u sto:%u side:%u did %" PR64BitT "j. msp at %" PR64BitUT "j/%" PR64BitUT "j total"
						, player->teulIndex(), player->playerID(), player->callsign(), player->side()
						, uid(), side()
						, joules, damage().accumulated(), damage().capacity());
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		PREP_CHAT_MSG(cmsg, "%" PR64BitUT "j to sto (%" PR64BitUT "/%" PR64BitUT ")", joules, damage().accumulated(), damage().capacity());
		player->message(LSTR_BETA, LSTR_GENERIC, cmsg);
#endif
		return false;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u m:%u destroyed [%" PR64BitT "j damage]"
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
			mission->setFRUCooldown(host_config.sto.deathCooldown_s[m_subType-1]);
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
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u %" PR64BitT "j below %" PR64BitT "j threshold"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, damage().threshold());
		char doneJoules[32];
		char threshJoules[32];
		strprintf(doneJoules, "%" PR64BitUT "", joules);
		strprintf(threshJoules, "%" PR64BitUT "", damage().threshold());
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
		dbugLog(_FLI, "t:%d p:%u c:%s sto:%u did %" PR64BitT "j damage. %s at %" PR64BitUT "j/%" PR64BitUT "j total"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, typeName(), damage().accumulated(), damage().capacity());
#if defined(BETA_RELEASE) || defined(TRAINING_SERVER)
		PREP_CHAT_MSG(cmsg, "%" PR64BitUT "j to %s sto (%" PR64BitUT "/%" PR64BitUT ")", joules, typeName(), damage().accumulated(), damage().capacity());
		player->message(LSTR_BETA, LSTR_GENERIC, cmsg);
#endif
		return false;
	}

	// Finally, it's dead-dead jim.
	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u %s destroyed [%" PR64BitT "j -> %" PR64BitUT "j damage]"
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
			threadLog(_FLD, "t:%d p:%u c:%s sto:%u entered %s [%" PR64BitT " damage]"
							, player->teulIndex(), player->playerID(), player->callsign()
							, uid(), typeName(), joules);
			m_occupants.push_back(player);
		}
		else
		{
			threadLog(_FLD, "t:%d p:%u c:%s sto:%u got %s entry message [%" PR64BitT " damage]"
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
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u damage (%" PR64BitT "j) < damage threshold (%" PR64BitUT "j), taking no damage"
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
		dbugLog(_FLI, "t:%d p:%u c:%s sto:%u did %" PR64BitT "j damage. %s at %" PR64BitUT "j/%" PR64BitUT "j total"
						, player->teulIndex(), player->playerID(), player->callsign()
						, uid(), joules, typeName(), damage().accumulated(), damage().capacity());
		return false;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s sto:%u %s destroyed [%" PR64BitT "j -> %" PR64BitUT "j damage]"
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
		dbugLog(_FLD, "t:%d p:%u c:%s sto:%u damage:%" PR64BitT "", teulIndex, player->playerID(), player->callsign(), it->m_uid, it->m_joules);
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
				, "INSERT INTO `audit`.`sto_history`"
					" VALUES (%u,%" PR64BitUT ",'%s',%" PR64BitUT ",%u,%u,%u,%u,%f,%f,%f,%f,%f,%f,%u,%u)"
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
	PPOSpec* config = specLoader.ppoGetSpec(subType);

	// grab damage/capacity first grap from config
	if (config != nullptr)
	{
		if (m_threshold == 0)
			m_threshold = config->damageThreshold;
		if (m_capacity == 0)
			m_capacity = config->damageCapacity;
	}

	// grab damage/capacity override from host config
	if (host_config.sto.damageThreshold[subType-1] > 0)
		m_threshold = host_config.sto.damageThreshold[subType-1];
	if (host_config.sto.damageCapacity[subType-1] > 0)
		m_capacity = host_config.sto.damageCapacity[subType - 1];
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
	, m_damage(m_subType)
{
}

//////////////////////////////////////////////////////////////////////
// Constructor for a PPO STO

PPOSTO::PPOSTO(ACTION_OBJECT* const a)
	: ServerSTO(a)
	, m_damage(m_subType)
	, m_occupants(8)
	, m_maxLife(0ULL)
	, m_lastOccupied(teulClock64)
{
	m_occupants.clear();
	m_lifespan = 0;
	PPOSpec* config = specLoader.ppoGetSpec(m_subType);
	if (config != nullptr)
	{
		m_maxLife = config->timeout;
		dbugLog(_FLW, "PPO max life set to %" PR64BitUT " for subtype %d", m_maxLife, m_subType);
	}
	else
	{
		dbugLog(_FLW, "PPO Spec not found for subtype %d", m_subType);
	}
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
		specLoader.loadSpecMap();
	}
} dotLoadPPOXml;

