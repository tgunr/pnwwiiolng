#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechGyros class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "mech/MechModule.h"

class MechGyros : public MechModule
{
public:

	MechGyros(gr2ModuleSpec* spec = NULL);
	virtual ~MechGyros();

	// slow the mech turn as it becomes damaged
	virtual void HandleDamage(gr2DamageComponent* component);

	static gr2GameObject* Create(gr2ModuleSpec* spec);
};

