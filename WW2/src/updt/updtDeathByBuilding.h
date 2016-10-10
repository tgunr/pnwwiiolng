#pragma once
// Created: Fri Oct  6 17:02:42 CDT 2006
//
// Host message indicating to a player they died
// from a building collapse.
// No data is currently passed, the message itself
// is considered sufficient.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"

class DeathByBuilding
{
public:
	DeathByBuilding(ACTION_OBJECT* /*a*/=nullptr) { }
	~DeathByBuilding() {}

public:
	SINT32 packer(PackMode /*mode*/, ACTION_OBJECT* a) { return a->offset; }
};

typedef teul_network_object<DeathByBuilding> DEATH_BY_BUILDING;
