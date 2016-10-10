#include <syscfg.h> // Path is project specific.
// Created: Tue Nov  7 16:34:36 2000
//
// Pack routines for the strat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "mem/mem.h"
#include "strt/strtMapTypes.h"
#include "strt/strtPack.h"
#include "strt/strtSyncData.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdates.h"
#include "teul/teulPackers.h"
#include "teul/teulEndpointDefs.h"
#include "wwii/wwiiPack.h"

static SINT32 _sizeOfObject = 0;
static SINT32 _sizeOfObjectHeader = 0;
static SINT32 _objectsPerSupercellResponse;
SINT32 strtPackInit()
{
	UCHAR dummyBuffer[Teul::MaxPacket];
	UINT16 dummyUINT16=0;
	SINT16 dummySINT16=0;
	UCHAR dummyUCHAR=0;

	//Calculate the size of the header we'll be sending
	_sizeOfObjectHeader = teulPackUCHAR(TEUL_PACK, dummyBuffer, _sizeOfObjectHeader, &dummyUCHAR, 0);
	_sizeOfObjectHeader = teulPackUCHAR(TEUL_PACK, dummyBuffer, _sizeOfObjectHeader, &dummyUCHAR, 0);
	_sizeOfObjectHeader = teulPackUINT16(TEUL_PACK, dummyBuffer, _sizeOfObjectHeader, &dummyUINT16, 0);

	//Calculate the size of the object
	_sizeOfObject = teulPackSINT16(TEUL_PACK, dummyBuffer, _sizeOfObject, &dummySINT16, 0);
	_sizeOfObject = teulPackSINT16(TEUL_PACK, dummyBuffer, _sizeOfObject, &dummySINT16, 0);
	_sizeOfObject = teulPackSINT16(TEUL_PACK, dummyBuffer, _sizeOfObject, &dummySINT16, 0);
	_sizeOfObject = teulPackUCHAR(TEUL_PACK, dummyBuffer, _sizeOfObject, &dummyUCHAR, 0);
	_sizeOfObject = teulPackUCHAR(TEUL_PACK, dummyBuffer, _sizeOfObject, &dummyUCHAR, 0);

	_objectsPerSupercellResponse=(512 - _sizeOfObjectHeader) / _sizeOfObject;

#if platBuild == BUILD_SERVER
#ifdef DEBUG
	dbugLog(_FLD,"Header size:%d\nObject size: %d\nObjects/SupercellResponse: %d\n",_sizeOfObjectHeader,_sizeOfObject,_objectsPerSupercellResponse);
#endif
#endif
	return 0;

}

static SINT32 _packCP_OBJ_HIT(LOGICAL mode, UCHAR* data, SINT32 offset, CP_OBJ_HIT* hit,  SINT32 size)
{
	offset = wwiiPackFullObjID(mode, data, offset, &hit->object, 0);
	offset = teulPackSINT64(mode, data, offset, &hit->damage, 0);

	return offset;
}

SINT32 strtPackHitList(LOGICAL mode, UCHAR* data, SINT32 offset, CP_HIT_HEADER* hh,  SINT32 size)
{
	SINT32 i;

	offset = teulPackUINT32(mode,data,offset, (UINT32*)&hh->numHits,8);

	//dbugLog(_FLA,"recieved %d cp hits offset %d",hh->numHits, offset);

	if (mode == TEUL_UNPACK)
		hh->list = (CP_OBJ_HIT*)memAlloc(hh->numHits * sizeof(CP_OBJ_HIT));

	for (i=0; i < hh->numHits; i++)
		offset = _packCP_OBJ_HIT(mode,data, offset, &hh->list[i],0);

	//dbugLog(_FLA,"recieved %d cp hits offset %d",hh->numHits, offset);

	return offset;

}

SINT32
strtPackOwnershipUpdate(LOGICAL mode, UCHAR* data, SINT32 offset, OWNERSHIP_UPDATE* object, SINT32 size)
{
	UINT16 country;

	offset = teulPackSCHAR(mode,data,offset,&(object->objectID.objType),8);
	offset = wwiiPackFullObjID(mode,data,offset,&(object->objectID),0);

	country = (UINT16) object->country;
	offset = teulPackUINT16(mode,data,offset,&(country),0);
	object->country = (WWII_COUNTRY) country;

	offset = teulPackUINT32(mode, data, offset, &(object->flags), 0);

	//dbugLog(_FLA,"Ownership: %d\n\tx:%d\n\ty:%d\n\tid:%d\n\tcountry:%d\n\n",object->type, object->objectID.sx,object->objectID.sy,object->objectID.id,object->country);
	return offset;
}

SINT32
strtPackSyncCPs(LOGICAL mode, UCHAR* message, SINT32 offset, STRT_SYNC_CPS* object, SINT32 size)
{
	offset = teulPackVoid(mode, message, offset, object, sizeof(STRT_SYNC_DATA));
	offset = teulPackVoid(mode, message, offset, object->info, sizeof(CP_SINFO)*object->numEntries);

	return offset;
}

SINT32
strtPackSyncFacilities(LOGICAL mode, UCHAR* message, SINT32 offset, STRT_SYNC_FACILITIES* object, SINT32 size)
{
	offset = teulPackVoid(mode, message, offset, object, sizeof(STRT_SYNC_DATA));
	offset = teulPackVoid(mode, message, offset, object->uOID, sizeof(object->uOID[0]) * object->numEntries);
	offset = teulPackVoid(mode, message, offset, object->country, object->numEntries);
	offset = teulPackVoid(mode, message, offset, object->captureStage, sizeof(object->captureStage[0]) * object->numEntries);
	offset = teulPackVoid(mode, message, offset, object->captureCounter, sizeof(object->captureCounter[0]) * object->numEntries);
	offset = teulPackVoid(mode, message, offset, object->captureStateChanged, sizeof(object->captureStateChanged[0]) * object->numEntries);

	return offset;
}

SINT32
strtPackSyncBuildings(LOGICAL mode, UCHAR* message, SINT32 offset, STRT_SYNC_BUILDINGS* object, SINT32 size)
{
	offset = teulPackVoid(mode, message, offset, object, sizeof(STRT_SYNC_DATA));
	offset = teulPackVoid(mode, message, offset, object->vislevel, object->numEntries);

	return offset;
}

void*
strtGetStaticPacketBuffer()
{
	static char buffer[Teul::MaxPacket];
	return (void*)buffer;
}

STRT_SYNC_CPS&
strtGetSyncCPs()
{
	static STRT_SYNC_CPS* buffer = (STRT_SYNC_CPS*) strtGetStaticPacketBuffer();
	assert( sizeof(STRT_SYNC_CPS) <= Teul::MaxPacket );
	return *buffer;
}

STRT_SYNC_FACILITIES&
strtGetSyncFacilities()
{
	static STRT_SYNC_FACILITIES* buffer = (STRT_SYNC_FACILITIES*) strtGetStaticPacketBuffer();
	assert( sizeof(STRT_SYNC_FACILITIES) <= Teul::MaxPacket );
	return *buffer;
}

STRT_SYNC_BUILDINGS&
strtGetSyncBuildings()
{
	static STRT_SYNC_BUILDINGS* buffer = (STRT_SYNC_BUILDINGS*) strtGetStaticPacketBuffer();
	assert( sizeof(STRT_SYNC_BUILDINGS) <= Teul::MaxPacket );
	return *buffer;
}
