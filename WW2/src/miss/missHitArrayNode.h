#pragma once
// Created: Thu Jan 17 22:23:13 CST 2008
//
// Structure used by the host to tell the client which
// vehicles they hit during their sortie.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"

// The HIT_ARRAY is returned with each mission packet
// providing a list of all vehicle that have been hit by weapon spec
struct HIT_ARRAY_NODE
{
	WEAPON_SPEC	vehType;			// What type of vehicle
	UINT16		numVehiclesHit;		// How many you fired at and hit
	UINT16		numVehiclesKilled;	// How many you were credited kills on
} ;

