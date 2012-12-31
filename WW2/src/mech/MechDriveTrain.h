#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechVehicle class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe
//

#include "mech/MechModule.h"

class MechDriveTrain : public MechModule
{
public:

	MechDriveTrain(gr2ModuleSpec* spec = NULL);
	virtual ~MechDriveTrain();

	// slow the mech as it becomes damaged
	virtual void HandleDamage(gr2DamageComponent* component);

	static gr2GameObject* Create(gr2ModuleSpec* spec);
};

