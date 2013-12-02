#pragma once
// Created: Thur Dec 17,  2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern LOGICAL chatInitDB();

extern void chatLoadMotds();

LOGICAL chatdbRecordEvent(const SCHAR* category, const SCHAR* callsign, const SCHAR* event, const SCHAR* parameter, const SCHAR* data);

class Property;
extern Property* chatFindProperty(const struct FULL_OBJ_ID& id);

extern bool chatGetWWIIConfig();

extern UINT32 chatGetPermissionNames(const PermissionMask& mask, char* into, UINT32 fromIndex);

