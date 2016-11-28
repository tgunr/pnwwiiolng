#include <syscfg.h> // Path is project specific.
// Date: Mon May 21 13:59:54 2001
//
// The main update loop of the strat updater host
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "server/serverInterface.h"

#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostPlayers.h"
#include "host/hostVehicle.h"
#include "strt/strtConstants.h"
#include "strt/strtDamageQueueEntry.h"
#include "strt/strtData.h"
#include "strt/strtDataErrors.h"
#include "strt/strtHack.h"
#include "strt/strtIO.h"
#include "strt/strtSyncSupportMessages.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdater.h"
#include "strt/strtUpdaterHostMessages.h"
#include "teul/teulPacket.h"
#include "teul/teulUtils.h"

//Times per second that the strat updater sends updates to the main strat host
#define STRAT_SYNC_FREQUENCY 10

#define MAX_DAMAGE_ENTRIES_PER_MESSAGE 5

#ifdef BETA_RELEASE
	#define DEF_ERR_LEVEL ERR_DBUG
#else
	#define DEF_ERR_LEVEL ERR_INFO
	//#define DEF_ERR_LEVEL ERR_NOTE
#endif

#define DEF_ERR_FLAGS 0

std::vector<DAMAGE_QUEUE_ENTRY> damageQueue;
CPStruct* _cpList;

static void
_flushDamageQueue()
{
	if ( damageQueue.empty() )
		return;

	const size_t dataSize = damageQueue.size() * sizeof(decltype(damageQueue[0]));
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_SYNC_DAMAGE_LIST_Y), &damageQueue[0], dataSize);
	outgoing.setUnencrypted();
	serverSendPacketToType(outgoing, STRAT_HOST);

	damageQueue.clear();
}

void
strtUpdaterInit()
{
	damageQueue.reserve(256);

	//Initialize the strat system
	_cpList = strtImportStratData();

	strtFakeBridges(_cpList);

	strtReportDataErrors(0);
}

DAMAGE_QUEUE_ENTRY&
_getDamageQueueEntry(const UINT64& objectKey, const WEAPON_SERIAL& weapon, const UINT32 playerID)
{
	const size_t entries = damageQueue.size();
	for ( size_t i = 0; i < entries; ++i )
	{
		DAMAGE_QUEUE_ENTRY& entry = damageQueue[i];
		if ( (entry.objectKey == objectKey) & (entry.weapon.serial == weapon.serial) & (entry.playerID == playerID) )
			return entry;
	}

	damageQueue.push_back(DAMAGE_QUEUE_ENTRY(objectKey, weapon, playerID));

	return damageQueue.back();
}

void
strtAccumulateDamageMessage(const FULL_OBJ_ID object, const SINT64 damageJoules, UINT32 playerID, const WEAPON_SERIAL& serial)
{
	ObjectStruct* const currentObject = strtGetObject(object);
	if (currentObject == nullptr)
	{
		dbugLog(_FLD, "p:%u v:%u couldn't find damage object %d:%d:%u"
					, playerID, serial.serial
					, object.cell.ox, object.cell.oy,object.id);
		return;
	}

	//Generate its object ID
	const UINT64 objectKey = object.Key();

	//Since we've found it, we need to add it to our outgoing queue
	DAMAGE_QUEUE_ENTRY& damageEntry = _getDamageQueueEntry(objectKey, serial, playerID);

	// Do the signs differ?
	const SINT64 newDamage = damageEntry.damageJoules + damageJoules;
	if ( damageEntry.damageJoules > 0 )
	{
		if (damageJoules < 0 )
		{
			dbugLog(_FLD, "p:%u v:%u obj:%d,%d.%u ignoring repairs over damage", playerID, serial.serial, object.cell.ox, object.cell.oy, object.id);
			return;
		}
		if ( newDamage < damageJoules )
		{
			dbugLog(_FLD, "p:%u v:%u obj:%d,%d.%u ignoring %" PR64BitT " joules damage - would wrap", playerID, serial.serial, object.cell.ox, object.cell.oy, object.id, damageJoules);
			return;
		}
		damageEntry.damageJoules = newDamage;
	}
	else if ( damageEntry.damageJoules < 0 )
	{
		if ( damageJoules > 0 )
		{
			dbugLog(_FLD, "p:%u v:%u obj:%d,%d.%u overriding repairs with damage", playerID ,serial.serial, object.cell.ox, object.cell.oy, object.id);
			damageEntry.damageJoules = damageJoules;
		}
		else
		{
			dbugLog(_FLD, "p:%u v:%u obj:%d,%d.%u discarding overlapping repairs", playerID ,serial.serial, object.cell.ox, object.cell.oy, object.id);
			return;
		}
	}
	else
	{
		damageEntry.damageJoules = newDamage;
	}

	if ( damageQueue.size() >= MAX_DAMAGE_ENTRIES_PER_MESSAGE )
	{
		//flush damage queue
		_flushDamageQueue();
	}
}

void
strtUpdaterProcessEvents()
{
	static TeulTimeMS nextDamageFlush = 0ULL;
	if ( teulClock64 < nextDamageFlush )
		return;
	nextDamageFlush = teulClock64 + 100;

	_flushDamageQueue();
}

