#pragma once
// Created: Thur Dec 17,  2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

void chatInitDB(void);
void chatLoadMotds(void);
void chatdbRecordEvent(const SCHAR* category, const SCHAR* callsign, const SCHAR* event, const SCHAR* parameter, const SCHAR* data);

class Property;
Property* chatFindProperty(const struct FULL_OBJ_ID& id);

bool chatGetWWIIConfig(void);

UINT32 chatGetPermissionNames(const PermissionMask& mask, char* into, UINT32 fromIndex);
