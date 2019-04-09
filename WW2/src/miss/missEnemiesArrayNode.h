#pragma once
// Created: Mon Apr 08 20:00:00 CST 2019
//
// Structure used by the host to tell the client which
// units, with vehicle spec and callsign, that they killed during their sortie.
//
// Copyright (C) PlayNet, Inc 1999-2019 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Caleb "sniper62" Luehrs

#include "weap/weapSpec.h"
#include "chat/chatHandle.h"

// The ENEMIES_ARRAY is returned with each mission packet
// providing a list of all units that have been killed by vehicle spec and callsign
struct ENEMIES_ARRAY_NODE
{
	WEAPON_SPEC	vehSpec;	// The enemy's type of unit
	ChatHandle	callsignId;	// The enemy's callsign
};

