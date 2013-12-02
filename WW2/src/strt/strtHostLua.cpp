#include <syscfg.h> // Path is project specific.
//
// Created: Thu Sep 23 12:45:00 2004
//
// Strat server specific lua wrappers
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Maintaining Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Patrick "thunder" Macgrude

#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string>

#include "dbug/dbug.h"
#include "strt/strtHostLua.h"
#include "wwii/wwiiConfig.h"
#include "include/luaGlue.h"
#include "strt/strtHostDB.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiConfigVars.h"

#include "strt/strtDotCommands.h"
#include "include/arenaLockEnums.h"

extern ConfigPath cv_luaRCFile, cv_luaStratlibFile;

// Lua/Tolua API headers
#ifdef __cplusplus
extern "C"
{
#endif
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
#ifdef __cplusplus
}
#endif
#include <tolua++.h>

extern lua_State* _luaVM;
#define L _luaVM

static void
_cantReadReturn(const char* fn)
{
	dbugLog(_FLW, "Incorrect return type to strat.%s: %s", fn, lua_tostring(L, -1));
	lua_pop(L, 1);
}

static bool
_callFunction(const char* fn, int nargs)
{
	Lua::SetContext(fn);
	bool result = (lua_pcall(L, nargs, 1, 0) != 0);
	Lua::EndContext();

	if ( result )
	{
		dbugLog(_FLW, "%s failed: %s", fn, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}

static int
_getNumber(const char* function, int argc=0)
{
	if ( !_callFunction(function, argc) )
		return 0;

	if ( !lua_isnumber(L, -1) )
	{
		_cantReadReturn(function);
		return 0;
	}
	int result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	return result;
}

static double
_getDouble(const char* function, int argc=0)
{
	if ( !_callFunction(function, argc) )
		return 0;

	if ( !lua_isnumber(L, -1) )
	{
		_cantReadReturn(function);
		return 0;
	}
	double result = (double)lua_tonumber(L, -1);
	betaLog(_FLI, "%s returned %g", function, result);
	lua_pop(L, 1);
	return result;
}

//////////////////////////////////////////////////////////////////////
// Report a lua error to the log / irc server

static void
_reportLuaError(const char* stage, const char* scriptName)
{
	static const char* const toChannels = "-sys,#servers";
	char error[512+1];
	snprintf(error, 512, "*** ERROR: Unable to %s Lua script '%s'", stage, scriptName);

	// IRC it
	wwiiIRCChannelMessage(toChannels, error);

	Lua::SCRIPT_RESULTS& results = Lua::Results();
	for ( size_t i = 0; i < results.size(); ++i )
		wwiiIRCChannelMessage(toChannels, results[i].c_str());

	results.clear();

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Log it and shut down.
	LogAbort("luaErr: %s", error);
}

UINT32
stratBridgeObjectiveLimit()
{
	const char* function = "maxBridgeObjectives";

	lua_getglobal(L, function);
	int result = _getNumber(function, 0);

	if ( result <= 0 )
		return 0;

	return (UINT32) result;
}

UINT32
stratCaptureObjectiveLimit()
{
	static const char* function = "maxCaptureObjectives";

	lua_getglobal(L, function);
	int result = _getNumber(function, 0);

	if ( result <= 0 )
		return 0;

	return (UINT32) result;
}

UINT32
stratEngagementObjectivesQuota(UINT32 attackLimit)
{
	static const char* function = "engagementObjectivesQuota";

	lua_getglobal(L, function);
	tolua_pushnumber(L, (lua_Number)attackLimit);
	int result = _getNumber(function, 1);

	if ( result <= 0 )
		return 0;

	return (UINT32) result;
}

UINT32
stratResupplyTime(const WEAPON_SPEC& weapon, StratHCUnit* unit, FacilityStruct* facility, UINT32 resupplyTimer)
{
	static const char* function = "resupplyTimeFn";

	lua_getglobal(L, function);
	tolua_pushusertype(L, (void*)&weapon, "WEAPON_SPEC");
	tolua_pushusertype(L, (void*)unit, "StratHCUnit");
	tolua_pushusertype(L, (void*)facility, "FacilityStruct");
	tolua_pushnumber(L, (lua_Number)resupplyTimer);

	int result = _getNumber(function, 4);

	if ( result <= 0 )
		return 0;

	return (UINT32) result;
}

double
stratRDPSupplyPenalty(WWII_COUNTRY forCountry)
{
	static const char* function = "rdpPenaltyPercentage";
	double result = 0.0;

	lua_getglobal(L, function);
	tolua_pushnumber(L, (lua_Number)forCountry);

	result = _getDouble(function, 1);

	return ( result > 0 ? result : 0 );
}

//////////////////////////////////////////////////////////////////////
// Run a CP function which returns true if it wants the host to
// abort what its doing.

bool
strtCPEventTrigger(CPStruct& cp, const char* trigger)
{
	lua_getglobal(L, trigger);
	tolua_pushusertype(L, (void*)&cp, "CPStruct");
	if ( !_callFunction(trigger, 1) )
		return false;

	bool result = (lua_toboolean(L, -1) != 0);
	lua_pop(L, 1);

	return result;
}

//////////////////////////////////////////////////////////////////////
// Run a generic, single-argument, game event

bool
strtGameEventTrigger(const char* trigger, WWII_SIDE forSide)
{
	lua_getglobal(L, trigger);
	tolua_pushnumber(L, (lua_Number)forSide);

	if ( !_callFunction(trigger, 1) )
		return false;
	lua_pop(L, 1);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Obtain a value from a simple global variable

/*
static int
_getGlobalNumber(const char* variable, int minimum, int fallback)
{
	int value = fallback;

	lua_getglobal(L, variable);

	if ( lua_isnumber(L, -1) )
	{
		int num = (int)lua_tonumber(L, -1);
		if ( num >= minimum )
			value = num;
	}

	lua_pop(L, 1);

	return value;
}
*/

extern bool strtdbFetchLuaScript(const char* name, std::string& code);

static bool
_execute(const char* name, bool dbOnly)
{
	bool success = false;
	std::string code = "";
	if ( strtdbFetchLuaScript(name, code) )
	{
		success = Lua::RunObject(name, code);
	}
	else if ( !dbOnly )
	{
		code = std::string(cv_luaPath.GetString()) + PATH_SEPARATOR + name + ".lua";
		success = Lua::RunScript(code.c_str());
	}

	return success;
}

bool
stratStartLua()
{
	if ( _luaVM == NULL )
		return false;

	std::string stratLib = cv_luaPath.GetString();
	stratLib += PATH_SEPARATOR;
	stratLib += cv_luaStratlibFile.GetString();

	if ( !Lua::RunScript(stratLib.c_str()) )
		LogAbort("Unable to execute stratlib");

	return true;
}

void
stratProcessLuaEventQueue()
{
//	static std::string code = "if LuaEvent then LuaEvent.ProcessQueue() end";
//	Lua::RunObject("LuaEventQueue", code);
	static std::string code = Lua::CompileCode("if LuaEvent then LuaEvent.ProcessQueue() end", "EventQueue");
	Lua::RunObject("EventQueue", code);
}

extern SINT32 strtBroadcastConfigUpdate();

bool
stratStartIntermission()
{
	if ( host_config.isIntermission() == false )
		return true;

	// BECD is a tool written by xiper for creating map configurations,
	// becdwrapper defines glue functions for reading/loading these.
	char becdScript[1024];
	snprintf(becdScript, sizeof(becdScript), SUB_PATH("%s", "becdwrapper.lua"), cv_luaPath.GetString());
	if ( Lua::RunScript(becdScript) == false )
	{
		dbugLog(_FLA, "Unable to load intermission functions");
		return false;
	}

	if ( host_config.arena.intermission != NEW_INTERMISSION )
		return true;

	dbugLog(_FLN, "Starting intermission script");

	static const char* scriptName = "intermission-setup";
	if ( _execute(scriptName, true) == false )
	{
		_reportLuaError("load", scriptName);
		return false;
	}
	Lua::Results().clear();

	// Bump the intermission value
	host_config.arena.intermission = LIVE_INTERMISSION;

	char value[21];
	snprintf(value, 20, "%u", host_config.arena.intermission);
	strtdbSaveConfigVar("arena.intermission", value);

	// Disable entry to the arena
	if ( host_config.isArenaOpen() )
		host_config.arena.lock = FULL_ARENA_LOCK;
	else if ( !host_config.isArenaLocked(AL_ENTRY) )
		host_config.arena.lock |= AL_ENTRY;

	dbugLog(_FLN, "Intermission script loaded");

	strtBroadcastConfigUpdate();

	return true;
}

void
stratInitializeLua()
{
	const char* function = "stratSetup";

	lua_getglobal(L, function);
	_callFunction(function, 0);
	lua_pop(L, 1);
}

