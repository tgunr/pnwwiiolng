#include <syscfg.h> // Path is project specific.
// Created: Fri Sep  8 15:08:23 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "teul/teulPackers.h"
#include "dbse/dbseTypes.h"

SINT32
dbsePackHandshake(LOGICAL mode, UCHAR* data,SINT32 offset, HANDSHAKE* object, SINT32 /*size*/)
{
	SINT32 len = 0;
	SINT32 tempStatus;

	offset = teulPackUINT32(mode,data,offset,&(object->host),0);

	tempStatus = object->status;
	offset = teulPackSINT32(mode,data,offset,&(tempStatus),0);
	object->status = (CONNECTION_STATUS)tempStatus;

	offset = teulPackUINT32(mode,data,offset,&object->version,len);

	return offset;
}

