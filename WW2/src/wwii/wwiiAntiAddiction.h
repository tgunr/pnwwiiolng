#pragma once
// Created: Tue May 27 01:01:48 CDT 2008
//
// Definitions/constants for anti-addiction
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace AntiAddiction
{
	static const UINT32 TIRED_HOUR = 3;
	static const UINT32 PENALTY_HOUR = 5;
	static const UINT32 SLEEP_HOURS = 5;
	static const FLOAT32 TIRED_WARNING_INTERVAL = 0.5f;	// Half hour intervals
	enum
	{
		_STANDARD_TIME
	,	_TIRED_TIME
	,	_PENALTY_TIME
	};
}
