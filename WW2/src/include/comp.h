#pragma once
// Created: Thu Mar 15 13:32:26 2001
//
// This module conatains general purpose hash routines.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "weap/weapDefs.h"			// For MAX_WEAP_STATIONS
#include "weap/weapSpec.h"			// WEAPON_SPEC

#define MAX_COMPONENTS 128
#define MAX_DAMAGE_LEVELS 4
#define MAX_COMP_HITS	16
#define MAX_DAMAGE_PRIORITY 16
#define COMP_INVALID 255

enum COMP_TYPE
{
	COMP_UNDEF,
	COMP_FUEL_TANK,
	COMP_OIL_TANK,
	COMP_GLYCOL_TANK,
	COMP_WEAPON,
	COMP_AMMO,
	COMP_ENGINE,
	COMP_ARMOR,
};

enum MOD_FLAGS
{
	MOD_DAMG_INFLICTED = 1,
	MOD_DAMG_SELF = 2,
	MOD_LEVEL = 4,
	MOD_CONTENTS = 8
};

struct VEH_COMP
{
	MOD_FLAGS	modified;					// has this component been modified?
	COMP_TYPE	type;						// type of object
	LOGICAL		destroyed;					// is the object damageable?
	UINT32 		totalDamage;				// total damage applied to this component
	UINT32		curDamage;					// current damage that needs to be sent
	UINT32		selfDamage;					// keep track of current self damage seperately
	UINT32		latestShooter;				// last vehicle to inflict damage
	UINT32		maxDamage;					// maximum damage that we keep track of

	UINT32		contents;					// contents of this component (e.g. fuel for a feul tank)
	UINT32 		lastContents;				// last contents sent to other players
	UINT32		level[MAX_DAMAGE_LEVELS];	// threshold for each damage level
	UINT16		priority;					// priority of this component
	UINT16		curLevel;					// current damage level
	UINT16		totalHits;					// total hits (does not include self hits)
	UINT16		curHits;					// hits that need to be sent (not self)
};

class COMP_HEAD
{
public:
	VEH_COMP*	comp[MAX_COMPONENTS];		// pointer to each component
	UINT8		inflicted[MAX_COMPONENTS];	// current modified components by priority
	UINT8		self[MAX_COMPONENTS];  		// current modified components by priority
	UINT8		level[MAX_COMPONENTS];  	// current modified components by priority

	void release();
};

struct COMP_HIT
{
	UINT8		compID;
	UINT8		numHits;
	UINT16		damage;
};

struct COMP_HIT_LIST
{
	COMP_HIT 	hits[MAX_COMP_HITS];
	UINT32		numComponents;
};

// apply a hit
class HostVehicle;

extern void compApplyHit(HostVehicle& target, const struct VEH_HIT& hits);

// used only when first initializing
extern void compInit(COMP_HEAD * compHead);

// init the system before any thing else
extern SINT32 compSystemInit();

