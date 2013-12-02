#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 23 13:21:32 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "strt/strtTypes.h"
#include "strt/strtWeaponEntry.h"
#include "wwii/wwiiConfig.h"
#include "strt/strtHCUnits.h"
#include "strt/strtTOETypes.h"

//	------------------------------	Client Prototypes

// Get the current weapon counter as it applies to this weapon given
// spawn and supply entries. If "into" is non-null, we read into the
// counter pointed to by it; otherwise we use a temporary (static)
// and read your counter into that.
// Returns a pointer to the counter that was populated for you.
WeaponCounter*
WeaponEntry::getCounter(WeaponCounter* into, const TOE_::WeaponInstance* toeEntry) const
{
	static WeaponCounter tempCounter;
	WeaponCounter* counter = (into) ? into : &tempCounter;

	if ( host_config.supply.enabled == false || weapon().isSystemWeapon() )
	{
		counter->current = counter->capacity = 1;
		return counter;
	}

	if ( toeEntry == NULL || toeEntry->capacity() == 0 )
	{
		counter->clear();
		return counter;
	}

	// If the spawn isn't throttled, just give them what they want
	if ( !isThrottled() )
	{
		counter->current = toeEntry->present();
		counter->capacity = toeEntry->capacity();
		return counter;
	}

	// It's throttled, apply the window
	counter->capacity = toeEntry->throttleCapacity();
	SINT32 newCurrent = (SINT32)counter->capacity - (SINT32)toeEntry->unit().checkThrottleWindow(facility(), toeEntry->spec());
	if ( newCurrent <= 0 )
		counter->current = 0;
	else if ( toeEntry->present() < newCurrent )
		counter->current = toeEntry->present();
	else
		counter->current = newCurrent;

	return counter;
}
