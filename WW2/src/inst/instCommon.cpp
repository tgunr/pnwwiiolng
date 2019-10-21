#include "syscfg.h" // Path is project specific.
// Created: Tue Oct  7 22:26:47 CDT 2008
//
// Instancing code/data common between client/host
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instDescriptor.h"
#include "inst/instTypes.h"

//	------------------------------	Declarations

namespace Instance
{

// The "Global" and "Private" instance descriptors.
const Descriptor g_invalid = Descriptor(_INVALID, 0, 0, 0);
const Descriptor g_campaign = Descriptor(_CAMPAIGN, 0, 0, _CAMPAIGN);
const Descriptor g_tutorial = Descriptor(_TUTORIAL, 0, 0, 0);
const Descriptor g_lobby = Descriptor(_SCENARIO, 0, 0, 0);

//////////////////////////////////////////////////////////////////////
// Operator required for std::map etc to organize us.

bool
Descriptor::operator<(const Instance::Descriptor& rhs) const
{
	if ( category() < rhs.category() ) return true;
	if ( category() == rhs.category() )
	{
		if ( type() < rhs.type() ) return true;
		if ( type() == rhs.type() )
		{
			if ( handle() < rhs.handle() )
				return true;
			if ( handle() == rhs.handle() )
				return id() < rhs.id();
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Convert an instance id-string to an instance id (for Lua)

instanceid_t
StrToInstanceID(const char* instStr)
{
	return strtoull(instStr, nullptr, 10);
}

}	// namespace Instance

