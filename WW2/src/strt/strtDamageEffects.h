#pragma once
// Date: Tue Jul 12 15:33:35 CDT 2005
//
// Enumerators for the various damage effects
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// this enum describes the type of damage that resulted to an object
// due to an incoming damage packet ..
enum STRAT_DAMAGE_EFFECT
{
	SDE_INVALID = 0,		// These must ordered by {type,subtype} ascending order
							// i.e. increasing order of significant after type
	SDE_DAMAGED,
	SDE_DESTROYED,
	SDE_DEMOLISHED,
	SDE_REPAIRED,
	SDE_REBUILT,
	SDE_NUM_EFFECTS
} ;

enum STRAT_DAMAGE_TARGET
{
	SDT_INVALID = 0
,	SDT_STRAT_OBJECT
,	SDT_AUTOGUNNER
,	SDT_BRIDGE
,	SDT_FIREBASE
,	SDT_FACTORY
,	SDT_NUM_TARGET_TYPES
} ;
