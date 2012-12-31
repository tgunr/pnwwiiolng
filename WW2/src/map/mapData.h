#pragma once
// Created: Wed May 31 12:33:44 2000
//
//Copied from strat system with minor changes
//If you are trying to use strtData and mapData, don't, it will suck!
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

extern SINT32 mapInitMapData();
extern class CPStruct* mapGetStratSystem(void);

extern void mapUpdateCP(class CPStruct* cp, WWII_COUNTRY owner, WWII_COUNTRY controller, UINT32 flags, const char* from) ;

