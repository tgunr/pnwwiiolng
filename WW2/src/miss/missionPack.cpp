#include <syscfg.h> // Path is project specific.
// Created: Mon Jan 22 10:21:14 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "miss/missionPack.h"
#include "miss/missionTypes.h"
#include "miss/missionResults.h"
#include "mm/mmPack.h"
#include "strt/strtFacilityWeaponList.h"
#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiRoomLists.h"
#include "wwii/wwiiPackBool.h"

static const SINT32 WEAP_COUNT_BITS = 10;
static const SINT32 WEAP_SPAWNABLE_BITS = 3;

#define WEAP_COUNT_MAX_XMIT 999
//(2^(WEAP_COUNT_BITS-1))

static inline SINT32 missPackWeaponCounter(PackMode mode, UCHAR* data, SINT32 offset, WeaponCounter* object, SINT32 /*size*/)
{
	if (mode == TEUL_PACK)
	{
		UINT16 capacity = object->capacity;
		UINT16 current = object->current;

		if (capacity > WEAP_COUNT_MAX_XMIT)
			capacity = WEAP_COUNT_MAX_XMIT;
		if (current > WEAP_COUNT_MAX_XMIT)
			current = WEAP_COUNT_MAX_XMIT;

		offset = teulPackUINT16(mode, data, offset, &capacity, WEAP_COUNT_BITS);
		offset = teulPackUINT16(mode, data, offset, &current, WEAP_COUNT_BITS);
		offset = teulPackUCHAR(mode, data, offset, &object->restricted, 1);
		offset = teulPackUCHAR(mode, data, offset, &object->spawnable, WEAP_SPAWNABLE_BITS);
	}
	else
	{
		object->capacity = object->current = object->reinforcements = object->spawnable = 0;
		offset = teulPackUINT16(mode, data, offset, &object->capacity, WEAP_COUNT_BITS);
		offset = teulPackUINT16(mode, data, offset, &object->current, WEAP_COUNT_BITS);
		offset = teulPackUCHAR(mode, data, offset, &object->restricted, 1);
		offset = teulPackUCHAR(mode, data, offset, &object->spawnable, WEAP_SPAWNABLE_BITS);
	}

	return offset;
}

SINT32
FacilityWeaponList::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&(m_status), 8);
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_fullObjID, 0);

	a->offset = teulPackUINT16(mode, a->packed, a->offset, &(m_numWeapons), 0);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &(m_refreshSeconds), 8);

	//dbugLog(_FLD, "packing List of % weapons",object->numWeapons);

	//Then, pack or unpack it
	for ( UINT32 i = 0; i < m_numWeapons; ++i )
	{
		m_weapons[i].packer(mode, a);
		a->offset = missPackWeaponCounter(mode, a->packed, a->offset, &(m_counters[i]), 0);
	}

	a->offset = teulPackVoid(mode, a->packed, a->offset, m_levels, m_numWeapons);

	if ( m_status <= FacilityWeaponList::_NORMAL )
		a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_nextSupplyM, 0);
	else if ( mode == TEUL_UNPACK )
		m_nextSupplyM = 0;

	return a->offset;
}

SINT32 missPackMISSION_POST_REPLY(PackMode mode, UCHAR* data,
					SINT32 offset , MISSION_POST_REPLY* object,
					SINT32 /*size*/)
{
	if (object == nullptr)
	{
		dbugLog(_FLA,"passed in a null object!");
		return -EINVAL;
	}

	SINT32 temp=object->posted;
	offset = teulPackSINT32(mode,data,offset,&(temp),0);
	object->posted=temp;

	offset = teulPackUINT32(mode,data,offset,&(object->missionID),0);
	offset = teulPackSCHAR(mode,data,offset,&(object->reasonCode),0);

	return offset;
}

// missPackMISSION_POST_REQ()
//
// actually passed a MISSION_SUMMARY, but we only pack the minimum number of
// items in it, to save bandwidth, mostly so we can get as many chars as
// possible for the desc.
SINT32 missPackMISSION_POST_REQ(PackMode mode, UCHAR* data,
					SINT32 offset , MISSION_POST_REQ* object,
					SINT32 /*size*/)
{
	if (object == nullptr)
	{
		dbugLog(_FLA,"passed in a null object!");
		return -EINVAL;
	}

	offset = wwiiPackFullObjID(mode,data,offset,&(object->origin),0);
	offset = wwiiPackFullObjID(mode,data,offset,&(object->target),0);

	offset = teulPackSINT16(mode,data,offset ,&(object->descSize),0);
	offset = wwiiPackText(mode, data, offset, object->desc, MapMiss::MISSION_DESC_LENGTH);

	offset = teulPackSCHAR(mode,data,offset,&(object->missionType),4);
	offset = teulPackSCHAR(mode, data, offset, &(object->subType),4);

	return offset;
}

SINT32 missPackWEP_LIST_REQ(PackMode mode, UCHAR* data, SINT32 offset, WEP_LIST_REQ* object, SINT32 /*size*/)
{
	if (object == nullptr)
	{
		dbugLog(_FLA,"passed in a null object!");
		return -EINVAL;
	}
	offset = wwiiPackFullObjID(mode,data,offset,&(object->facility),0);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&(object->allWeapons), 1);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&(object->isRefresh), 1);

	return offset;
}

SINT32
missPackMISSION_SUMMARY(PackMode mode, UCHAR* data, SINT32 offset, MISSION_SUMMARY* object, SINT32 /*size*/)
{
	if (object == nullptr)
	{
		dbugLog(_FLA,"passed in a null object!");
		return -EINVAL;
	}

	offset = teulPackUINT32(mode,data,offset,&(object->missionID),0);

	offset = wwiiPackFullObjID(mode,data,offset,&(object->origin),0);
	offset = wwiiPackFullObjID(mode,data,offset,&(object->target),0);

	//priority
	offset = teulPackSCHAR(mode,data,offset,&(object->missionType),3);
	offset = wwiiPackBool(mode, data, offset, object->mobileSpawnAvailable);
	offset = teulPackSCHAR(mode, data, offset, &object->subType, 4);

	offset = wwiiPackBool(mode, data, offset, object->m_canMobileSpawn);
	offset = teulPackSecondsDelta(mode, data, offset, &object->timeLaunch, 15);

	offset = teulPackVoid(mode,data,offset,&(object->callsign),MAX_HANDLE_LEN);
	if ( mode == TEUL_UNPACK )
		object->callsign[MAX_HANDLE_LEN] = 0;

	return offset;
}

//MISSION RESULTS are only sent to the FRONT_END
SINT32 missPackMISSION_RESULTS(PackMode mode, UCHAR* packed, SINT32 offset, MISSION_RESULTS* object, SINT32 /*size*/)
{
	if (object == nullptr)
	{
		dbugLog(_FLA,"passed in a null object!");
		return -EINVAL;
	}

	offset = wwiiPackHandle(mode, packed, offset, &object->handleID, 0);
	offset = teulPackUINT32(mode, packed, offset, &object->missionID, 0);

	offset = teulPackUINT32(mode, packed, offset, &(object->elapsedTime), 24);
	offset = teulPackUINT16(mode, packed, offset, &(object->kills), 0);
	offset = teulPackUINT16(mode, packed, offset, &object->m_hitArraySize, 0);

	for ( UINT32 i = 0; i < object->m_hitArraySize; ++i )
	{
		offset = wwiiPackWeaponSpec(mode, packed, offset, &object->hitArray[i].vehType, 0);
		offset = teulPackUINT16(mode, packed, offset, &(object->hitArray[i].numVehiclesHit), 12);	// } 24 bit alignment
		offset = teulPackUINT16(mode, packed, offset, &(object->hitArray[i].numVehiclesKilled), 12);	// }
	}

	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->missionSuccess, 1);	// }
	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->promoted, 1);		// }
	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->missionInvalid, 1);	// } 8 bits
	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->m_side, 2);			// }
	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->rtb, 3);				// }

	offset = wwiiPackFullObjID(mode, packed, offset, &(object->rtbFacility), 0);
	offset = wwiiPackWeaponSerial(mode, packed, offset, &(object->rtbReservedWeapon), 0);

	offset = wwiiPackRankNo(mode, packed, offset, &object->rank, 0);

	offset = teulPackUINT16(mode, packed, offset, &object->captureEvents, 12);
	offset = teulPackUINT32(mode, packed, offset, &object->mobileSpawnees, 12);
	offset = teulPackUINT32(mode, packed, offset, &object->aisHit, 12);
	offset = teulPackUINT32(mode, packed, offset, &object->bridgesHit, 12);
	offset = teulPackUINT32(mode, packed, offset, &object->bridgesRepaired, 12);
	offset = teulPackUINT32(mode, packed, offset, &object->factorysHit, 16);
	offset = teulPackUINT32(mode, packed, offset, &object->firebasesHit, 12);

	offset = teulPackUINT32(mode, packed, offset, (UINT32*)&object->rankIsKOTH, 1);

	offset = teulPackUINT32(mode, packed, offset, &object->pointsScored, 20);
	offset = teulPackUINT32(mode, packed, offset, &object->pointsAccumulated, 20);
	offset = teulPackUINT32(mode, packed, offset, &object->pointsNextRank, 20);

	offset = wwiiPackRankNo(mode, packed, offset, &object->nextRank, 0);

	offset = teulPackUINT32(mode, packed, offset, &object->m_resupplies, 16);
	offset = teulPackUINT32(mode, packed, offset, &object->m_resuppliesScored, 16);

	offset = teulPackUINT16(mode, packed, offset, &object->captureAssists, 12);

	offset = teulPackUINT16(mode, packed, offset, &object->m_guardingKills, 12);
	offset = teulPackUINT16(mode, packed, offset, &object->m_capperKills, 12);

	bool haveKiller = false;
	if ( mode == TEUL_PACK )
		haveKiller = (object->m_killedBy != 0);
	offset = wwiiPackBool(mode, packed, offset, haveKiller);
	if ( haveKiller )
		offset = wwiiPackHandle(mode, packed, offset, &object->m_killedBy, 0);
	else if ( mode == TEUL_UNPACK )
		object->m_killedBy = 0LL;

	// How many points for guarding capturables
	offset = teulPackUINT16(mode, packed, offset, &object->m_guardingPoints, 16);

	// Pack your real rank and your max rank
	offset = wwiiPackRankNo(mode, packed, offset, &object->m_realRank, 0);
	offset = wwiiPackRankNo(mode, packed, offset, &object->m_maxRank, 0);

	// Tell the client how long remaining on the account
	offset = teulPackSecondsDelta(mode, packed, offset, &object->accountGoodUntil, 24);

	ACTION_OBJECT a;
	a.offset = offset;
	a.packed = packed;
	object->chatPermissions.packer(mode, &a);
	offset = a.offset;

	if ( mode == TEUL_UNPACK )
		object->numHits = object->m_hitArraySize;

	return offset;
}

SINT32
RoomListEntry::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_action, 3);	//} 8 bits
	a->offset = teulPackUINT16(mode, a->packed, a->offset, (UINT16*)&m_flags, 5);		//}
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	m_weapon.packer(mode, a);
	if ( m_weapon.wType != 0 )
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_serial, 0);
	else if ( mode == TEUL_UNPACK )
		m_serial = 0;
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_rank, 0);

	return a->offset;
}

SINT32
RoomList::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->packer(mode, m_numEntries, 8);

	for ( UINT32 i = 0; i < m_numEntries && i < MAX_ROOM_LIST_ENTRIES; ++i )
	{
		a->offset = m_entry[i].packer(mode, a);
	}

	return a->offset;
}

SINT32
missPackMISS_LAUNCH_UPDATE(PackMode mode, UCHAR* data, SINT32 offset, MISS_LAUNCH_UPDATE* const object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, &(object->missionID), 0);

	UINT32 delta = 0;
	UINT32 now = teulTime();
	if ( mode == TEUL_PACK )
		delta = (object->timeLaunch > now) ? object->timeLaunch + 1 - now : 0;
	offset = teulPackUINT32(mode, data, offset, &delta, 16);
	if ( mode == TEUL_UNPACK )
		object->timeLaunch = delta + now;
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&object->state, 4);
	offset = wwiiPackBool(mode, data, offset, object->mobileSpawnAvailable);
	offset = wwiiPackBool(mode, data, offset, object->m_canMobileSpawn);

	return offset;
}

