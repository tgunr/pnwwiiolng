#include "syscfg.h" // Path is project specific.
// Date: 11/08/99
//
//	This is where all declarations for commPack routines for universal
//	game specific object structures that will be transmitted
//	across the network must reside or be included into.  All commPack
//	routines must have an entry in this file.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "comm/commTypes.h"
#include "updt/updtVehicleHit.h"
#include "wwii/wwiiPack.h"

SINT32 commPackInit()
{
	return 0;
}

SINT32 commPackVEHICLE_REQUEST(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_REQUEST* request, SINT32 /*size*/)
{
	offset = wwiiPackWeaponSerial(mode,data,offset, &request->weapon, 0);
	offset = teulPackUCHAR(mode, data,offset,&request->country, 0);
	offset = teulPackUCHAR(mode, data, offset, &request->side, 0);
	offset = teulPackUCHAR(mode, data, offset, &request->station, 0);

	return offset;
}

SINT32 commPackVEHICLE_REPLY(PackMode mode, UCHAR* const data, SINT32 offset, VEHICLE_REPLY* reply, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, &reply->hostIndex, 0);
	offset = wwiiPackWeaponSpec(mode, data, offset, &reply->spec, 0);
	offset = wwiiPackFullObjID(mode, data, offset, &reply->source, 0);

	offset = teulPackUCHAR(mode, data, offset, &reply->persVehicleIndex, 0);
	offset = teulPackUCHAR(mode, data, offset, &reply->station, 0);
	offset = wwiiPackRankNo(mode, data, offset, &reply->rankLevel, 0);

	return offset;
}

SINT32 commPackPlayerExit(PackMode mode, UCHAR* const data, SINT32 offset, PLAYER_EXIT* pe, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&pe->destination, 0);
	offset = teulPackUINT32(mode, data, offset, (UINT32*)&pe->exitCode, 0);
	offset = wwiiPackFullObjID(mode, data, offset, (FULL_OBJ_ID*)&pe->facility, 0);

	return offset;
}

SINT32 Vehicle::Hit::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->packer(mode, m_shooter);
	a->packer(mode, m_target);

	// Pack components.
	UINT32 numComponents = m_components.size();
	a->packer(mode, numComponents, 8);
	numComponents = std::min(numComponents, (UINT32)m_components.capacity());
	if ( mode == TEUL_UNPACK )
		m_components.resize(numComponents);

	for ( UINT32 i = 0; i < numComponents; ++i )
	{
		Vehicle::Hit::Component& comp = m_components[i];
		a->packer(mode, comp.compID, 7);
		a->packer(mode, comp.isImportant);
		a->packer(mode, comp.NumHits, 6);
		UCHAR damageWords = 0;
		if (mode == TEUL_PACK)
		{
			for ( damageWords = 0; damageWords < 3; ++damageWords )
			{
				if ( comp.m_damage < (1ULL << ((damageWords + 1) * 16)) )
					break;
			}
		}
		a->packer(mode, damageWords, 2);
		a->packer(mode, comp.m_damage, (damageWords + 1) * 16);
	}

	return a->offset;
}

