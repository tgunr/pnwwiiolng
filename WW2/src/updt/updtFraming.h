#pragma once
// Created: Mon Mar 20 00:04:12 CST 2006
//
// Update framing mode description
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct UpdateFraming
{
	enum { _SMALL, _MED, _LARGE, _MAX };
	enum { _MODES = 5 };

	UINT32	maxVehicles;				//!< Maximum vehicles player can see: total.
	UINT32	packetSize;				//!< Which packet size to use.
	UINT32	updateInterval;			//!< MS between updates for this rate
	UINT32	vehiclesPerFrame;			//!< Maximum vehicles per update.
	const	char* description;

	size_t maxVehiclesPerFrame() const { return vehiclesPerFrame; }
};

