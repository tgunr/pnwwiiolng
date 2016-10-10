#pragma once
// Created: Tue Mar 28 18:36:38 CST 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"
#include "weap/weapSpec.h"

struct SerialNumberRequest
{
	UINT32		playerID;
	WEAPON_SPEC	weapon;
	UINT32		serial;
};

using SERIAL_NUMBER_REQUEST = teul_network_object<teul_void_obj<SerialNumberRequest>>;
