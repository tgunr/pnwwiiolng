#pragma once
// Date: Tue Dec 26 12:33:50 CST 2006
//
// Table and Order of Equipment Entries for an HCUnit
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct WEAPON_SPEC;

//tolua_begin

extern void strtLoadTOEWeaponTemplateHeader(const char* id, const char* name, UINT32 country, UINT32 branch);
extern void strtLoadTOEWeaponInstance(const char* id, const WEAPON_SPEC& spec, UINT16 capacity, UCHAR access, UINT16 resupplyTime_m);

// Save all modified TOEWeaponEntries. Must be called once before
// TOEWeaponEntries will begin saving - this prevents the initial
// loading sequence from saving everything.
extern void strtSaveTOESupplies();

//tolua_end
