#include <syscfg.h> // Path is project specific.
// Created: Tue 03 Feb 2004, 12:17:00
//
// All the code to the pack routines for the authple reside here.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "include/infoMessageTypes.h"
#include "teul/teulPackers.h"

SINT32 packCompInfoReq(LOGICAL mode, UCHAR* data, SINT32 offset, COMPUTER_INFO_REQ* object, SINT32 /*size*/)
{

	if (object == nullptr)
		return 0;

	offset = teulPackUINT16(mode,data,offset,&object->size,0);

	if (offset > 0)
		offset = teulPackVoid(mode,data,offset,(void*)object->text,object->size);

	return offset;
}
