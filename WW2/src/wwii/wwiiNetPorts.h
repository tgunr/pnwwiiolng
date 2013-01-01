#pragma once
// Created: Tue Apr  4 17:10:59 CDT 2006
//
// Defines the parameters for best connection test to the
// map/ping host
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
			,	ClusterBoss = 6001					// Main port for cluster boss.
			,	InternalAuth = 6002					// Auth<->server interface
			,	DebugWebServer = 6080				// Debugging web interface
			,	ChatManagement = 27019				// For rosterClient
			,	LuaInterface = 6050					// For Lua interface
			,	ServerPeering = 6090				// For server<->server mesh.

			// Public ports (customer facing)
			,	StandardPublicPort = 27015			// Standard game server port

			,	AuthHost = 27015					// Auth Server port
			,	MapHost = 27015						// Map/Login server
			,	ChatHost = 27015					// Chat host server
			,	CellHost = 27015					// Cell host (game world) server

			,	InternalStrat = 6060				// Strat<->server interface
			,	DbsePri = 9000						// } Database proxy primary and
			,	DbseSec = 9099						// } secondary interfaces.
			,	ConnectionList = 27016				// } Find best connection
			,	ConnectionTest = 27017				// } algorithm ports
			} ;
		}
	}
}
