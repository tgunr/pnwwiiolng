#pragma once
// Created: Wed Nov  1 15:17:21 CST 2006
//
// All spawned weapons receive a serial number; this class
// encompasses the origin, campaign-unique ID and spec of
// that weapon as a serial number.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.

#include "wwii/wwiiFullObjID.h"
#include "weap/weapSpec.h"

//tolua_begin

struct WEAPON_SERIAL
{
	FULL_OBJ_ID		source;		// source spawn location
	UINT32			serial;		// serial number
	WEAPON_SPEC		spec;		// specific type of weapon
};

extern const WEAPON_SERIAL NULL_WEAP_SERIAL;

//tolua_end

#define PRF_WEAPON_SERIAL "%u@(" PRF_FULL_OBJ_ID ") " PRF_WEAPON_SPEC ""
#define PRA_WEAPON_SERIAL(s) (s).serial, PRA_FULL_OBJ_ID((s).source), PRA_WEAPON_SPEC((s).spec)

static inline const char* weapPrintSerialC(const WEAPON_SERIAL* weap)
{
	if (PN_UNLIKELY(weap == nullptr))
		return "<nullSER>";

	static char buffer[128];
	sprintf(buffer, "Serial: %u Source: %hd.%hd.%hu Spec: " PRF_WEAPON_SPEC ""
				, weap->serial
				, weap->source.cell.ox, weap->source.cell.oy, weap->source.id
				, PRA_WEAPON_SPEC(weap->spec)
				);
	return buffer;
}
