#pragma once
// Created: Fri Apr 13 15:10:58 CDT 2007
//
// Simple class that lets you limit the occurances of somethign based on a repeater
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Teul
{

// Implements a heartbeat type that will only return true a
// maximum of once per time period specified in the constructor.
//
// Typical usage:
//   static TeulTimeMS BLIP_INTERVAL_MS = 5 * 1000; // 5 second intervals
//   static Teul::BlipRepeater nextBlip(BLIP_INTERVAL_MS);
//   if ( nextBlip )
//   {
//     // some work to do every 5 seconds
//   }

class BlipRepeater
{
public:
	BlipRepeater(TeulTimeMS interval) : m_interval(interval), m_nextBlip(0LL) {}
	bool Ready()
	{
		if ( teulClock64 < m_nextBlip )
			return false;
		m_nextBlip = teulClock64 + m_interval;
		return true;
	}

private:
	TeulTimeMS	m_interval;
	TeulTimeMS	m_nextBlip;
};

}

