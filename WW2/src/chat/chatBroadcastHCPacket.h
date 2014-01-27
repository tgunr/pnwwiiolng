#pragma once
// Created: Wed Nov 22 15:04:00 CST 2006
//
// Functions for broadcasting an object to all of the players within
// a high command hierachy (brigade, division, etc) and all active
// HC officers for that HC.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//#include "chat/chatList.h"
#include "teul/teulTransmission.h"

class wwiiPlayer;
class HCUnit;
struct TEUL_CALLBACK;

struct HCUnitBroadcast : public FilterFn
{
public:
	static void sendToOfficers(const HCUnit* unit, void* object, TEUL_CALLBACK* cb);
	// Send an object to all players within a unit and below it,
	// and all same-side active officers
	static void send(const HCUnit* toUnit, void* object, TEUL_CALLBACK* cb);
	bool operator()(wwiiPlayer* const& player) const;

private:
	static void buildUnitList(const HCUnit* unit);
};

