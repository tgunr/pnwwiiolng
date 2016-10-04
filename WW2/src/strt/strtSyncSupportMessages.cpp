#include <syscfg.h> // Path is project specific.
// Created: Mon May 21 16:26:58 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "versionNo.h"

#include "include/wthrTime.h"
#include "server/serverInterface.h"
#include "strt/strtConstants.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtPack.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSyncData.h"
#include "strt/strtTypes.h"				// For VISLEVEL_NORMAL, ugh.
#include "strt/strtUpdates.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPlayer.h"
#include "host/hostTimeUpdate.h"

#ifdef IS_CELL_HOST
#include "host/host.h"
#endif

#if defined(IS_MAP_HOST)
#define _USE_STRAT_EVENTS
#include "strt/strtEvents.h"
#endif

#if defined(IS_MAP_HOST)
#include "miss/mission.h"
#include "map/mapData.h"
#endif

//////////////////////////////////////////////////////////////////////
// Set all of the objects on a facility's object list to a given
// side and country.

static void
_setObjectsOwner(ObjectStruct* const objectList, WWII_SIDE side, WWII_COUNTRY country)
{
	if ( objectList == nullptr )
		return;
	if ( side == 0 || country == 0 )
	{
		dbugLog(_FLA, "setObjectsOwner called with side:%u, ctry:%u", side, country);
		return;
	}

	//Rip through the object list, changing ownership along the way
	for ( auto current = objectList; current != nullptr; current = current->next )
	{
#ifdef HEAVY_DEBUG
		dbugLog(_FLD,"setting object " PRF_FULL_OBJ_ID " to side %d country %d"
					, PRA_FULL_OBJ_ID(current->fullObjID)
					, current->side
					, current->country);
#endif

		current->setCountry(country, side);
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// returns TRUE if facility actually changes state,
// FALSE if it was already that state

static bool
_facilityStatusHandler(FULL_OBJ_ID fullObjID, WWII_COUNTRY country, CP_STATUS_FLAGS flags, UINT32 captureStage, UINT16 captureCounter, UINT32 lastChange)
{
	//get the facility
	FacilityStruct* const facility = strtGetFacility(fullObjID);
	if (facility == nullptr)
	{
		dbugLog(_FLA, "cant find facility for id " PRF_FULL_OBJ_ID "", PRA_FULL_OBJ_ID(fullObjID));
		return false;
	}

	const bool open = (flags & CP_STAT_OPEN);

#ifdef HEAVY_DEBUG
	dbugLog(_FLN,"Facility status: %s %d %s Flags: %x captureStage %u captureCounter %hu lastChange %u"
				, facility->name
				, country
				, (open)?"OPEN":"CLOSED"
				, flags
				, captureStage
				, captureCounter
				, lastChange
				);
#endif

	const WWII_SIDE newSide = wwiiSide(country);

	const bool hasChanged = ( facility->isOpen() != open || facility->country != country || facility->side != newSide );

	//set the ownership
	if ( facility->type == FACILITY_ARMYBASE && facility->side != newSide )
	{
		if ( facility->cp->armyBasesOwned[facility->side] > 0 )
			--facility->cp->armyBasesOwned[facility->side];
		++facility->cp->armyBasesOwned[newSide];
	}

#if defined(IS_MAP_HOST)
	if ( facility->type == FACILITY_FIREBASE && facility->isOpen() && !open )
	{
		missionFirebaseDestroyed(*facility);
	}

	if ( facility->side != newSide )
	{
		missionRevokeMissionsFrom(*facility);
	}
#endif

	if ( hasChanged )
	{
		facility->setOpen(open);
		facility->setCountry(country);
		facility->flags = flags;

		_setObjectsOwner(facility->objectList, facility->side, facility->country);
	}

	// Apply capture stage changes.
	facility->m_captureStage = (FacilityStruct::CaptureStage)captureStage;
	facility->m_captureCounter = captureCounter;
	if ( facility->m_captureStage == FacilityStruct::_CAP_ENABLED )
		facility->m_captureProgress = (float)captureCounter;
	facility->m_captureStateChanged = teulTime() - lastChange;

#if defined(IS_MAP_HOST)
	if ( hasChanged )
	{
		Strat::Events::Add(facility);
	}
#endif

	return hasChanged;
}

//////////////////////////////////////////////////////////////////////
// Handle change of CP state.

static void
_cpStatusHandler(FULL_OBJ_ID fullObjID, WWII_COUNTRY country, WWII_COUNTRY controllingCountry, CP_STATUS_FLAGS flags)
{
	//get the cp
	CPStruct* const cp = strtGetCPorBridge(fullObjID);
	if ( cp == nullptr )
	{
		dbugLog(_FLI, "CP status on unrecognized object: " PRF_FULL_OBJ_ID "", PRA_FULL_OBJ_ID(fullObjID));
		return;
	}

#ifndef BRIDGES
	if ( cp.isBridge() )
		return;
#endif

#if defined(IS_MAP_HOST)
	const bool hasChanged = (cp->country != country || cp->controllingCountry != controllingCountry || cp->flags != flags);
	mapUpdateCP(cp, country, controllingCountry, flags, __FUNCTION__);
#endif

	//actually set the ownership
	cp->country = country;
	cp->side = wwiiSide(country);
	cp->controllingCountry = controllingCountry;
	cp->controllingSide = wwiiSide(controllingCountry);

	cp->flags = flags;
	cp->m_contention = (flags & CP_STAT_CONTENTION) == CP_STAT_CONTENTION;
	cp->m_enemyLinked = (flags & CP_STAT_ENEMY_LINKED) == CP_STAT_ENEMY_LINKED;

#if defined(IS_MAP_HOST)
	if ( hasChanged )
		Strat::Events::Add(cp);
#endif
}

//////////////////////////////////////////////////////////////////////
// Handle change of object/building state.

extern void mapUpdateBuildingVisLevel(const ObjectStruct* const obj);

static void
_buildingStatusHandler(FULL_OBJ_ID fullObjID, UINT32 visLevel, UINT32 damagedBy)
{
	//Find the item and fix its level locally
	ObjectStruct* const object = strtGetObject(fullObjID);
	if (object == nullptr)
	{
		dbugLog(_FLA, "invalid object for vislevel update: " PRF_FULL_OBJ_ID "", PRA_FULL_OBJ_ID(fullObjID));
		return;
	}

	object->m_damagedBy = damagedBy;

	// Is this a change of vis level?
#if defined(IS_MAP_HOST)
	if ( object->m_visLevel != visLevel )
	{
		object->m_visLevel = visLevel;
		mapUpdateBuildingVisLevel(object);
	}
#elif defined(IS_CELL_HOST)
	bool buildingDestroyed = false;
	if ( object->m_visLevel == VISLEVEL_NORMAL && visLevel > 0 )
	{
		dbugLog(_FLD, "%s visLevel changed %u -> %u [norm = %u] by %u", object->name, object->m_visLevel, visLevel, VISLEVEL_NORMAL, damagedBy);
		buildingDestroyed = true;
	}

	if ( object->m_visLevel != visLevel )
	{
		object->m_visLevel = visLevel;
	}
	if ( buildingDestroyed )
		hostBuildingVisLevelChange(object, Instance::g_campaign.id());
#endif

	return;
}

static SINT32
_recvStratUpdate(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;

	UINT32 updates = 0;
	while ( a->canRead<StratUpdates>() )
	{
		++updates;
		auto& entry = a->read<StratUpdates>();
		switch (entry.fullObjID().objClass)
		{
		case CLASS_CP:
			_cpStatusHandler(entry.fullObjID(), entry.owner(), entry.controller(), entry.flags());
		break;
		case CLASS_FACILITY:
#ifdef DEBUG_SYNC
			dbugLog(_FLD, "i:%d fac:" PRF_FULL_OBJ_ID "", i, PRA_FULL_OBJ_ID(entry));
#endif
			_facilityStatusHandler(entry.fullObjID(), entry.owner(), entry.flags(), entry.captureStage(), entry.captureCounter(), entry.lastChange());
		break;
		case CLASS_BUILDING:
#ifdef DEBUG_SYNC
			dbugLog(_FLD, "i:%d obj:" PRF_FULL_OBJ_ID "", i, PRA_FULL_OBJ_ID(entry));
#endif
			_buildingStatusHandler(entry.fullObjID(), entry.flags(), (UINT32)entry.u_extra.m_buildingDamagedBy);
		break;
		default:
			dbugLog(_FLA, "unknown strat element type: %d", entry.fullObjID().objClass);
		break;
		}
	}

	dbugLog(_FLD, "received %u strat updates", updates);

	return a->offset;
}

static SINT32
_recvSyncCPs(ACTION_OBJECT* const a)
{
	STRT_SYNC_CPS& syncCPs = strtGetSyncCPs();

	a->offset = strtPackSyncCPs(TEUL_UNPACK, a->packed, a->offset, &syncCPs, 0);

	strtProcessSyncCPs(syncCPs);

	return a->offset;
}

static SINT32
_recvSyncFacilities(ACTION_OBJECT* const a)
{
	STRT_SYNC_FACILITIES& syncFacilities = strtGetSyncFacilities();

	//Unpack the header
	a->offset = strtPackSyncFacilities(TEUL_UNPACK, a->packed, a->offset, &syncFacilities, 0);

	strtProcessSyncFacilities(syncFacilities);

	return a->offset;
}

static SINT32
_recvSyncBuildings(ACTION_OBJECT* const a)
{
	STRT_SYNC_BUILDINGS& syncBuildings = strtGetSyncBuildings();

	//Unpack the header
	a->offset = strtPackSyncBuildings(TEUL_UNPACK, a->packed, a->offset, &syncBuildings, 0);

	strtProcessSyncBuildings(syncBuildings);

	return a->offset;
}

static SINT32
_recvConfigUpdate(ACTION_OBJECT* const a)
{
#ifdef DEV_BUILD
	dbugLog(_FLD, "receiving config update");
#endif
	return teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &host_config, sizeof(WWII_CONFIG));
}

static SINT32
_recvTimeUpdate(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<Host::TimeUpdate>() == false )
		return -EINVAL;

	auto& incoming = a->read<Host::TimeUpdate>();

	dbugLog(_FLD,"time update: gt:%d, tf:%d, wp:%d, fp:%d"
					, incoming.gametime, incoming.factor, incoming.weather, incoming.rain);

	wthrSetGameTime(incoming.gametime);
	wthrSetWeatherPattern(incoming.weather);
	wthrSetFogPattern(incoming.rain);

#ifdef IS_CELL_HOST
	incoming.Broadcast();
#endif

	return a->offset;
}

static SINT32
_registerStrtXCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
	{
		REGISTER_RECEIVER(STRT_UPDATE_Y, _recvStratUpdate);
		REGISTER_RECEIVER(STRT_SYNC_CPS_Y, _recvSyncCPs);
		REGISTER_RECEIVER(STRT_SYNC_FACILITIES_Y, _recvSyncFacilities);
		REGISTER_RECEIVER(STRT_SYNC_BUILDINGS_Y, _recvSyncBuildings);
		REGISTER_RECEIVER(STRT_BROADCAST_CONFIG_UPDATE_Y, _recvConfigUpdate);
		REGISTER_RECEIVER(STRT_BROADCAST_TIME_UPDATE_Y, _recvTimeUpdate);
	}
	END_CALLBACK_REGISTRATION(Strat::CallbackX);
}

SINT32
strtSyncSupportInstallCallbacks(void)
{
	_registerStrtXCallbacks();

	return 0;
}

