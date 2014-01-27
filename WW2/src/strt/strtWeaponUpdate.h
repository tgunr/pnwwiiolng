#pragma once
// Date: Sat Apr 23 18:39:28 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"			// For WEAPON_SPEC

class WeaponUpdate
{
public:
	enum _TYPE { DELIVERY, RESUPPLY, THROTTLE, TRANSFER };

public:
	WeaponUpdate*			next;		//!< Linked list of updates
	WeaponUpdate*			prev;		//!< at a given facility

	_TYPE					wuType;	//!< Delivery, Resupply, Throttle or Transfer.

	class FacilityStruct*	facility;	//!< Where this update is for.
	WEAPON_SPEC				weapon;	//!< Which weapon is being updated.

	struct STRAT_UPDATE*	stratUpdate; //!< Pointer to our strat event.

	SINT32					weapValue;	//!< Data value, meaning depends on wuType.
	class WeaponCounter*	counter;	//!< Optional weapon counter.

public:
	SINT32	value() const { return ( wuType == RESUPPLY ) ? 1 : weapValue; }
};
