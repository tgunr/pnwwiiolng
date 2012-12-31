#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechTurretMotor class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "mech/MechModule.h"

class MechTurretMotor : public MechModule
{
public:

	MechTurretMotor(gr2ModuleSpec* spec = NULL);
	virtual ~MechTurretMotor();

	// capture which turret to monitor
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	// slow the mech turret as it becomes damaged
	virtual void HandleDamage(gr2DamageComponent* component);

	static gr2GameObject* Create(gr2ModuleSpec* spec);

private:

	// which turret we're interested in
	int _turretIndex;

};

