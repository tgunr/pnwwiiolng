#pragma once
// Created: Thu Oct 23 15:02:48 CDT 2008
//
// Prototypes for instHostMessages.cpp
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Instance
{
	extern  SINT32 InstallHostCallbacks();
	// Save the population counts periodically to database, only intended to be called
	// by one of the host processes.
	extern	void RecordPopulations();
}
