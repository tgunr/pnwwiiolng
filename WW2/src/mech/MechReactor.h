#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechWeapon class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "ml/mltypes.h"
#include "veh/vehmain.h"
#include "weap/weapons.h"
#include "mech/MechModule.h"

class gr2Vehicle;
struct gr2ModuleSpec;

class MechReactor : public MechModule
{
public:

	MechReactor(gr2ModuleSpec* spec);
	virtual ~MechReactor();

	virtual void AddConfigParam(std::string name, std::string value);

	static gr2GameObject* Create(gr2ModuleSpec* spec);

protected:

	int _power;
	int _maxPower;
};

