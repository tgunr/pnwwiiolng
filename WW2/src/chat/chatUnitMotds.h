#pragma once
// Created: Wed Oct  5 14:33:48 CDT 2005
//
// MOTDs for various arbitrary "units"
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Special unit IDs
enum { SYSTEM_MOTD_UNIT = 0, ALLIED_MOTD_UNIT = 1, AXIS_MOTD_UNIT = 2, CRS_MOTD_UNIT = 8, GM_MOTD_UNIT = 9 };

extern void chatSetUnitMotd(UINT32 unit, const std::string& motd);
extern const char* chatGetUnitMotd(UINT32 unit);
