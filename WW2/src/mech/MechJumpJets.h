#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechJumpJets class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "mech/MechModule.h"

// pre declares

class MechVehicle;
struct gr2ModuleSpec;

class MechJumpJets : public MechModule
{
public:

	MechJumpJets(gr2ModuleSpec* spec = NULL);
	virtual ~MechJumpJets();

	virtual UINT32 ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve);
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	// use/disuse this module; this returns true if there was a change
	virtual bool SetActive(bool a);

	// create a new module
	static gr2GameObject* Create(gr2ModuleSpec* spec);

protected:

	UINT32 _flags;

};

