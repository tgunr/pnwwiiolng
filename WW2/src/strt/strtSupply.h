#pragma once
// Date: Fri 29 Aug 11:30:00 2003
//
// Funcitons dealing with supply
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

class FacilityStruct;

//tolua_begin

// Turn on supply debugging
extern void strtEnableDebugSupply();

// Turn off supply debugging
extern void strtDisableDebugSupply();

// Get the current supply debug state
extern bool strtGetDebugSupply();

extern HCUnitID strtGetSupplyingUnit(const FacilityStruct& facility, HCUnitID brigadeID);

// Process the resupply queues
extern void strtProcessResupply();

//tolua_end
