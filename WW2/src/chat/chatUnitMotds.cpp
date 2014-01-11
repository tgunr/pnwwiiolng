#include <syscfg.h> // Path is project specific.
// Date: Wed Oct  5 14:33:48 CDT 2005
//
// Get/Set MOTDs for system, side and HCUnits
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHash.h"

typedef std::unordered_map<UINT32,std::string> MOTD_INDEX;
static MOTD_INDEX motds;

//////////////////////////////////////////////////////////////////////
// Set a particular unit's MOTD

void
chatSetUnitMotd(UINT32 unit, const std::string& motd)
{
	motds[unit] = motd;
}

//////////////////////////////////////////////////////////////////////
// Retrieve the c_str of a particular unit's MOTD

const char*
chatGetUnitMotd(UINT32 unit)
{
	MOTD_INDEX::iterator it = motds.find(unit);
	if ( it == motds.end() || it->second.empty() )
		return nullptr;
	return it->second.c_str();
}

