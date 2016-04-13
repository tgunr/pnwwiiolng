#pragma once
// Created: Tue Jan 15 13:59:57 CST 2008
//
// Packers for wwiiErrorTypes
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiErrorTypes.h"
#include "str/stringLib.h"

static inline SINT32 wwiiPackERR_STRING(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_ERR_STRING* object, SINT32 size)
{
	if (mode == TEUL_PACK)
		object->length = strnlen(object->string, WWII_ERR_STRING::_MAX_LEN) +1;

	offset = teulPackSINT32(mode,data,offset,&(object->length), 12);

	offset = teulPackVoid(mode,data,offset,&(object->string), object->length);

	return offset;
}

static inline SINT32 wwiiPackPIEXIT_INFO(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_PIEXIT_INFO* object, SINT32 size)
{
	offset = teulPackSINT32(mode,data,offset,&(object->srcLine), 0);

	offset = wwiiPackERR_STRING(mode,data,offset,&(object->srcFilename), 0);
	offset = wwiiPackERR_STRING(mode,data,offset,&(object->errorMessage), 0);

	return offset;
}

static inline SINT32 wwiiPackEXCEPTION_INFO(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_EXCEPTION_INFO* object, SINT32 size)
{
	offset = teulPackUINT32(mode,data,offset,&(object->exceptionCode), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->exceptionAddr), 0);

	for ( UINT32 i = 0; i < WWII_EXCEPTION_INFO::_STACK_LEN; ++i )
   {
	   offset = teulPackUINT32(mode, data, offset, &(object->stackAddrs[i]), 0);
   }

	offset = teulPackVoid(mode, data, offset, object->infoText, WWII_EXCEPTION_INFO::_STR_LEN + 1);

	return offset;
}

static inline SINT32 wwiiPackVALIDITY_INFO(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_VALIDITY_INFO* object, SINT32 size)
{
	offset = wwiiPackERR_STRING(mode,data,offset,&(object->archive), 0);
	offset = wwiiPackERR_STRING(mode,data,offset,&(object->filename), 0);

	offset = teulPackUINT32(mode,data,offset,&(object->crc), 0);

	offset = teulPackVoid(mode,data,offset,&(object->md5), WWII_VALIDITY_INFO::_MD5_LEN);
	if ( mode == TEUL_UNPACK )
		object->md5[WWII_VALIDITY_INFO::_MD5_LEN] = 0;

	return offset;
}

static inline SINT32 wwiiPackSYSTEM_INFO(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_SYSTEM_INFO* object, SINT32 size)
{
	offset = teulPackUINT32(mode,data,offset,&(object->numPackets), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->packetID), 0);

	if (mode == TEUL_PACK)
		object->length = strnlen( object->infoText, WWII_SYSTEM_INFO::_MAX_LEN) +1;

	offset = teulPackSINT32(mode,data,offset,&(object->length), 0);

	offset = teulPackVoid(mode,data,offset,&(object->infoText), object->length);

	return offset;
}

static inline SINT32 wwiiPackERROR_INFO(LOGICAL mode, UCHAR* data, SINT32 offset, WWII_ERROR_INFO* object, SINT32 size)
{
	offset = teulPackUINT32(mode,data,offset,(UINT32*)&(object->errorType), 0);

	offset = teulPackUINT32(mode,data,offset,&(object->eventID), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->clientVersion), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->clientTime), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->hostTime), 0);
	offset = teulPackUINT32(mode,data,offset,&(object->hostIP), 0);

	offset = teulPackUINT32(mode,data,offset,(UINT32*)&(object->archiveWasCorrupt), 8);

	switch (object->errorType)
	{
		case WW_ERR_EXCEPTION:
			offset = wwiiPackEXCEPTION_INFO(mode,data,offset,&object->data.exceptInfo, 0);
			break;
		case WW_ERR_PIEXIT:
			offset = wwiiPackPIEXIT_INFO(mode,data,offset,&object->data.piExitInfo, 0);
			break;
		case WW_ERR_VALIDITY_CHECK:
			offset = wwiiPackVALIDITY_INFO(mode,data,offset,&object->data.validityInfo, 0);
			break;
		case WW_ERR_SYSTEM_INFO:
			offset = wwiiPackSYSTEM_INFO(mode,data,offset,&object->data.systemInfo, 0);
			break;
		case WW_ERR_UNKNOWN:
		default:
			break;
	}

	offset = teulPackUINT32(mode, data, offset, &object->clientSourceCommit, 0);

	return offset;
}
