#pragma once
// Created: Tue Apr  4 17:10:59 CDT 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace WWII
{
	namespace Net
	{
		namespace Ports
		{
			enum
			{
			// Internal ports
				PrivateBcast = 6666					// server<->server backbone
            ,	PrivateCellBcast = 6667				// cell host<->cell host backbone
			,	InternalAuth = 6002					// Auth<->server interface
			,	DebugWebServer = 6080				// Debugging web interface
			,	ChatManagement = 6066				// For rosterClient
			,	LuaInterface = 6050					// For Lua interface
			,	ServerPeering = 6090				// For server<->server mesh.

			// Public ports (customer facing)
			,	AuthHost = 27015					// Auth Server port
            ,   AltAuthHost = 27014                 // Auth Server port in Super-CiaB (five-host) mode
			,	MapHost = 27015						// Map/Login server
#if defined(PN_DIFFERENT_PORTS)
			,	ChatHost = 27016					// Chat host server
			,	CellHost = 27017					// Cell host (game world) server
#else
			,	ChatHost = MapHost
			,	CellHost = MapHost
#endif

			,	InternalStrat = 6060				// Strat<->server interface
			,	Dbse = 9000							// } Database proxy primary and
			};
		}
	}
}
