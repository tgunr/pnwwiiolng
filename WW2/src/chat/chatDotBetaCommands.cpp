#include <syscfg.h> // Path is project specific.
// Date: Fri Nov 11 13:17:18 CST 2005
//
// Beta commands
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#ifdef BETA_RELEASE
#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatList.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatHost.h"
#include "teul/teulTime.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreads.h"

class DotNukePersonas : public DotCommand
{
public:
	DotNukePersonas() : DotCommand("nukepersonas", "Ask the host to delete all your personas"
									, NO_MASK, LSTR_NUKEPERSONAS_USAGE, LSTR_NUKEPERSONAS_DETAIL, true) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		DBThread::Do(DBThread::Default, "DELETE FROM wwii_persona WHERE playerid = %u", player.playerID());
		player.message(LSTR_SYSTEM, LSTR_PERSONAS_NUKED);
	}
} dotNukePersonas;

//////////////////////////////////////////////////////////////////////
// Process a profile result command - not really a player command,
// this is used by the client's profres function.
//////////////////////////////////////////////////////////////////////

class DotProfRes : public DotCommand
{
public:
	DotProfRes() : DotCommand("profres", "Record the results of a /profileflight", NO_MASK, LSTR_CAUTION, LSTR_CAUTION, false) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		// Silently say nothing if they are not spawned as a buzzard
		if ( player.isSpawned() == false || player.isBuzzard() == false )
		{
			dbugLog(_FLM, "t:%d p:%u c:%s non-buzzard use of profres", player.teulIndex(), player.playerID(), player.callsign());
			return;
		}

		std::string version = "", profile = "";
		UINT32	totalFrameCnt = 0, framesOver100 = 0, framesOver300 = 0, framesOver500 = 0, framesOver1000 = 0, peakMem = 0;
		FLOAT32	minFramesTime = 0, maxFramesTime = 0, totalFramesTime = 0;
		std::string os = "", cpuName = "";
		UINT32 numCpus = 0, cpuFlags = 0, cpuMhz = 0;

		ARGV argv;
		int numArgs = vectorize(arguments, argv, 16, false);
		if ( numArgs < 12 )
		{
			dbugLog(_FLM, "t:%d p:%u c:%s funny looking profres result [%u arguments]"
							, player.teulIndex(), player.playerID(), player.callsign(), numArgs);
			return;
		}

		// The first 11 arguments are all numeric
		for ( UINT32 i = 0; i < 10; ++i )
		{
			const char* p = argv[i];
			while ( *p )
			{
				if ( *p != '.' && !isdigit(*p) )
				{
					dbugLog(_FLM, "t:%d p:%u c:%s corrupt looking profres result [arg#%u]"
									, player.teulIndex(), player.playerID(), player.callsign(), i);
					return;
				}
				++p;
			}
		}

		UINT32 argn = 0;

		version = argv[argn++];
		totalFrameCnt = (UINT32)atoi(argv[argn++]);
		minFramesTime = (FLOAT32)atof(argv[argn++]);
		maxFramesTime = (FLOAT32)atof(argv[argn++]);
		totalFramesTime = (FLOAT32)atof(argv[argn++]);
		framesOver100 = (UINT32)atoi(argv[argn++]);
		framesOver300 = (UINT32)atoi(argv[argn++]);
		framesOver500 = (UINT32)atoi(argv[argn++]);
		framesOver1000 = (UINT32)atoi(argv[argn++]);
		peakMem = (UINT32)atoi(argv[argn++]);
		profile = argv[argn++];

		if ( strcmp(argv[argn], "Win") == 0 )
		{
			os = argv[argn++];
			// Extended CPU data
			numCpus = (UINT32)atoi(argv[argn++]);
			// cpuFlags are in hex -- because I'm a sadist?
			cpuFlags = (UINT32)strtoul(argv[argn++], nullptr, 16);
			// cpu speed in ~mhx
			cpuMhz = (UINT32)atoi(argv[argn++]);
			// Finally, the last argument should be the name
			cpuName = argv[argn++];
		}
		else if ( strcmp(argv[argn], "Mac") == 0 )
		{
			os = argv[argn++];
			// No other data at this time
		}
		else
		{
			dbugLog(_FLW, "t:%d p:%u c:%s unknown operating system %s"
							, player.teulIndex(), player.playerID(), player.callsign(), argv[argn]);
			return;
		}

		// Make the strings database safe
		wwiiEscapeString(profile);
		wwiiEscapeString(version);
		wwiiEscapeString(os);
		wwiiEscapeString(cpuName);

		// Send a query to the worker threads
		DBThread::Do(DBThread::Audit
				, "CALL audit.p_save_profile_results("
					"%u,'%s'"		// playerid, version number
					",%u,%f,%f,%f"	// #frames, min time, max time, total time
					",%u,%u,%u,%u"	// frames over 100ms, 300ms, 500ms, 1000ms
					",%u"			// peak memory usage
					",'%s'"			// operating system
					",%u,%u,%u"		// #cpus, cpu flags, cpu mhz
					",'%s'"			// CPU class
					",'%s'"			// Profile name
					")"
					, player.playerID(), version.c_str()
					, totalFrameCnt, minFramesTime, maxFramesTime, totalFramesTime
					, framesOver100, framesOver300, framesOver500, framesOver1000
					, peakMem
					, os.c_str()
					, numCpus, cpuFlags, cpuMhz
					, cpuName.c_str()
					, profile.c_str()
		);
	}
} dotProfRes;

//////////////////////////////////////////////////////////////////////

class DotResetStrat : public DotCommand
{
public:
	DotResetStrat() : DotCommand("resetstrat", "Forces a reset of the strat server"
									, NO_MASK, LSTR_RESETSTRAT_USAGE, LSTR_RESETSTRAT_DETAIL, true) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		static UINT32 lastRequest = 0;
		static UINT32 lastRequester = 0;
		static UINT32 lastIssued = 0;

		static const UINT32 REQUEST_INTERVAL = 30;

		if ( teulTime() - lastIssued < 300 )
		{
			player.message(LSTR_CAUTION, LSTR_RESETSTRAT_5MINUTE_LIMIT);
			return;
		}

		if ( !player.isPlaynet() && playerIndex.size() > 1 )
		{
			if ( teulTime() - lastRequest > REQUEST_INTERVAL )
			{
				lastRequest = teulTime();
				lastRequester = player.playerID();

				player.message(LSTR_NONE, LSTR_RESETSTRAT_ACK, command());
				chatSystemMessage(LSTR_RESETSTRAT_SYS, player.callsign());

				return;
			}
			else if ( player.playerID() == lastRequester )
			{
				player.message(LSTR_CAUTION, LSTR_RESETSTRAT_SELF_APPROVAL);
				return;
			}

			chatSystemMessage(LSTR_RESETSTRAT_APPROVED, player.callsign());
		}

		player.sendStratCommand("restart", "", false);
		lastIssued = teulTime();
	}
} dotResetStrat;

#endif //BETA_RELEASE

