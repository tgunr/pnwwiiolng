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

			,	GameBase	=	27020				// Assign ports here and up.
			,	PeerBase	=	6100				// Peer listening ports.
			,	WebBase		=	6200				// Listen for web connections here.

			,	InternalStrat = 6060				// Strat<->server interface
			,	Dbse = 9000							// } Database proxy primary and
			,	ConnectionList = 27016				// } Find best connection
			,	ConnectionTest = 27017				// } algorithm ports
			} ;
		}
	}
}
