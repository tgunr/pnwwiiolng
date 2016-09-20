#pragma once
// Created: Mon Feb  6 15:33:12 CST 2006
//
// Defines the swing meter indicating which side has most players
// A value of 0 indicates balance. -ve indicates allied advantage,
// while +ve indicates axis advantage.
// When SwingMeter.inEffect() is true, then side balancing is
// currently in effect
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"
#include "wwii/wwiiPack.h"

class SwingMeter
{
public:
	SwingMeter() : m_meter(0), m_delays(0), m_inEffect(false) {}

	void setMeter(SINT32 toMeter)
	{
		if ( toMeter > MAX_SWING ) toMeter = MAX_SWING;
		else if ( toMeter < MIN_SWING ) toMeter = MIN_SWING;
		m_meter = toMeter;
		m_inEffect = false;
		m_delays = 0;
	}

	void setInEffect() { m_inEffect = true; m_delays = 0; }
	void setDelays(UINT8 delay) { m_delays = delay; }

	SwingMeter(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

public:
	SINT32 meter() const { return m_meter; }
	SINT32 inEffect() const { return m_inEffect; }
	UCHAR delays() const { return m_delays; }

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_meter, 7);
		a->packer(mode, m_inEffect);
		a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_delays, 0);
		return a->offset;
	}

private:
	SINT32	m_meter;
	UCHAR	m_delays;
	bool	m_inEffect;

public:
	// The meter is expressed as +/- SwingMeter::MAX_SWING
	// 0 indicates balanced
	static const SINT32 MAX_SWING = 3;
	static const SINT32 MIN_SWING = 0-MAX_SWING;
};

typedef teul_network_object<SwingMeter> SWING_METER_MSG;

#if platBuild == BUILD_SERVER
namespace Map
{
	extern void BroadcastSwingMeter();
	extern void SendSwingMeter(SINT32 teulIndex);
}
#endif
