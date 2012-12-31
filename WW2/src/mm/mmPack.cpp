#include <syscfg.h> // Path is project specific.
// Date: Tue Apr 17 14:42:44 2001
//
// Pack functions for the merged mmhost, including the things necessary to
// posting and spawning "weapons" in the new and improved multi-crewed
// vehicle world
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "miss/missionTypes.h"
#include "mm/mmDirectSpawn.h"
#include "mm/mmPack.h"
#include "mm/mmSpawnTypes.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"

SINT32
mmPackPostedWeapon(LOGICAL mode, UCHAR* data, SINT32 offset, MM_POSTED_WEAPON* object, SINT32 /*size*/)
{
	offset = wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
//	offset = teulPackUINT32(mode,data, offset, &(object->posted), 0);
	offset = teulPackSINT16(mode,data, offset, &(object->portal), 0);
	offset = teulPackUINT32(mode,data, offset, &(object->commander), 0);
	offset = teulPackUINT32(mode,data, offset, &(object->crew), 0);

	return offset;
}

SINT32
mmPackFEPostRequest(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_REQUEST* object, SINT32 /*size*/)
{

	offset = wwiiPackFullObjID(mode, data, offset, &(object->facility), 0);
	offset = wwiiPackWeaponSpec(mode,data, offset, &(object->weapon), 0);

	offset = teulPackUINT32(mode,data, offset, (UINT32*)&(object->immediateSpawn), 0);

	return offset;
}

SINT32
mmPackFEPostAccept(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_ACCEPT* object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode,data, offset, (UINT32*)&(object->newVehicle), 0);
	offset = wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
	offset = teulPackSINT16(mode, data, offset, &(object->portal), 0);
	offset = teulPackUINT32(mode, data, offset, &(object->mobileSpawnAvailable), 1);
	return offset;
}

SINT32
mmPackFEPostDeny(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_DENY* object, SINT32 /*size*/)
{
	offset = wwiiPackWeaponSpec(mode,data, offset, &(object->weapon), 0);
	offset = teulPackUINT32(mode, data, offset, &(object->reasonCode), 0);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&(object->loseWeapon), 1);

	return offset;
}

SINT32
mmPackFEPostListRequest(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_LIST_REQUEST* object, SINT32 /*size*/)
{
	return wwiiPackFullObjID(mode, data, offset, &(object->facility), sizeof(FULL_OBJ_ID));
}

SINT32
mmPackFEPostListResponse(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_LIST_RESPONSE* object, SINT32 /*size*/)
{
	SINT16 i;

	offset = wwiiPackFullObjID(mode, data, offset, &(object->facility), 0);
	offset = teulPackSINT16(mode,data, offset, &(object->numWeapons), 0);

	if ((mode==TEUL_UNPACK) && (object->numWeapons > 0))
	{
		object->commandedWeapon = (COMMANDED_WEAPON*)memAlloc(sizeof(COMMANDED_WEAPON) * object->numWeapons);
	}

	for (i=0; i<object->numWeapons; i++)
	{
		offset = mmPackCommandedWeapon(mode, data, offset, &(object->commandedWeapon[i]), 0);
	}

	return offset;
}

SINT32
mmPackFEJoinRequest(LOGICAL mode, UCHAR* data, SINT32 offset, FE_JOIN_REQUEST* object, SINT32 /*size*/)
{
	return wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
}

SINT32
mmPackFEJoinAccept(LOGICAL mode, UCHAR* data, SINT32 offset, FE_JOIN_ACCEPT* object, SINT32 /*size*/)
{
	return mmPackPostedWeapon(mode, data, offset, &(object->postedWeapon), 0);
}

SINT32
mmPackFEJoinDeny(LOGICAL mode, UCHAR* data, SINT32 offset, FE_JOIN_DENY* object, SINT32 /*size*/)
{
	return teulPackUINT32(mode, data, offset, &(object->reasonCode), 0);
}

SINT32
mmPackFEPostExpired(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_EXPIRED* object, SINT32 /*size*/)
{
	return wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
}

SINT32
mmPackFEJoinPermRequest(LOGICAL mode, UCHAR* data, SINT32 offset, FE_JOIN_PERM_REQUEST* object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, &(object->joinRequestID), 0);
	offset = wwiiPackHandle(mode, data, offset, &object->chatHandle, 0) ;

	return offset;
}

SINT32
mmPackFECrewRequest(LOGICAL mode, UCHAR* data, SINT32 offset, FE_CREW_REQUEST* object, SINT32 /*size*/)
{
	offset = wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&(object->numCrewNeeded), 0);

	return offset;
}

SINT32
mmPackFECrewAccept(LOGICAL mode, UCHAR* data, SINT32 offset, FE_CREW_ACCEPT* object, SINT32 /*size*/)
{
	return teulPackUINT32(mode, data, offset, &(object->joinRequestID), 0);
}

SINT32
mmPackCommandedWeapon(LOGICAL mode, UCHAR* data, SINT32 offset, COMMANDED_WEAPON* object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&(object->action), 0);
	offset = wwiiPackHandle(mode, data, offset, &(object->handle), 0);
	offset = wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);

	return offset;
}

SINT32
mmPackFEPostCancel(LOGICAL mode, UCHAR* data, SINT32 offset, FE_POST_CANCEL* object, SINT32 /*size*/)
{
	return wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
}

SINT32
mmPackFEJoinPermKill(LOGICAL mode, UCHAR* data, SINT32 offset, FE_JOIN_PERM_KILL* object, SINT32 /*size*/)
{
	return teulPackUINT32(mode, data, offset, &(object->joinRequestID), 0);
}

SINT32
mmPackFEClientSpawn(LOGICAL mode, UCHAR* data, SINT32 offset, FE_CLIENT_SPAWN* object, SINT32 /*size*/)
{
	offset = wwiiPackWeaponSerial(mode, data, offset, &(object->weaponSerial), 0);
	offset = teulPackUCHAR(mode, data, offset, &(object->wantMobileSpawn), 1) ;
	return offset;
}

FESpawnNotify::FESpawnNotify(
		const WEAPON_SERIAL& _ws
		, UINT32 _vehicle
		, FLOAT64 _x, FLOAT64 _y, FLOAT64 _z
		, UCHAR _decal, bool _isHighCommander, bool _isBuilder, bool _isMissionLeader)
	: weaponSerial(_ws), spawnVehicleID(_vehicle), x(_x), y(_y), z(_z), decal(_decal)
	, isHighCommander(_isHighCommander), isBuilder(_isBuilder), isMissionLeader(_isMissionLeader)
{
}

// Network-based constructor for FESpawnNotify
FESpawnNotify::FESpawnNotify(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a) ;
}

SINT32
FESpawnNotify::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &spawnVehicleID, 0) ;
	if ( spawnVehicleID != 0 )
	{
		a->packer(mode, x);
		a->packer(mode, y);
		a->packer(mode, z);
	}
	else if ( mode == TEUL_UNPACK )
	{
		x = y = z = 0.0f ;
	}

	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &decal, 0) ;

	UCHAR flags = 0 ;
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &isHighCommander, 1) ;
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &isBuilder, 1) ;
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &isMissionLeader, 1) ;
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &flags, 5) ;

	a->offset = wwiiPackWeaponSerial(mode, a->packed, a->offset, &weaponSerial, 0) ;

	return a->offset;
}

SINT32
mmPackFECrewBailed(LOGICAL mode, UCHAR* data, SINT32 offset, FE_CREW_BAILED* object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, &(object->joinRequestID), 0);
	offset = teulPackUINT32(mode, data, offset, &(object->reasonCode), 0);

	return offset;
}

SINT32
Map::DirectSpawn::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	m_weapon.packer(mode, a) ;
	a->packer(mode, &m_location.x) ;
	a->packer(mode, &m_location.y) ;
	a->packer(mode, &m_location.z) ;
	a->packer(mode, m_useLocation) ;
	return a->offset ;
}
