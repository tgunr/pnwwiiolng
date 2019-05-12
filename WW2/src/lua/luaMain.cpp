#include "syscfg.h" // Path is project specific.
// Date: Thu Sep 23 12:45:00 2004
//
// Generic Lua glue code.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <assert.h>
#include <vector>
#include "include/vars.h"
#include "fio/fio.h"
#include "lua/luaMain.h"
#include "tut/tutLua.h"
#include "imap/imapLua.h"
#include "fp/fpLua.h"
#include "cfg/cfgLua.h"
#include "wwii/wwiiLua.h"
#include "weap/weapLua.h"
#include "strt/strtLua.h"
#include "ui/uiLua.h"
#include "uimap/uimapLua.h"
#include "scen/scenLua.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "ui/uiDotCommands.h"
#include "chat/chatDefines.h"
#include "client/GameClient.h"
#include "ui/uiMain.h"
#include "veh/vehType.h"
#include "sdm/StratDB.h"
#include "js/jsGame.h"
#include "pers/persmain.h"
#include "pi/piinfo.h"
#include "include/platforms.h"
#include "fp/fpFPS.h"

#include "pers/persLua.h"
#include "veh/vehLua.h"
#include "trn/trnLua.h"
#include "ob/obLua.h"
#include "ml/mltypes.h"
#include "veh/vehtype.h"
#include "veh/vehcomms.h"

#define LUA_GAME_LIB "game"
#define LUA_GAME_RC "include/common.lua"
#define LUA_SEARCH_MAX 2

#if platOS == OS_WIN32
#endif

static const char* _searchPaths[LUA_SEARCH_MAX] =
{
	"scenarios/",
	"lua/"
};

// prototypes
static bool _initGameLib(lua_State* L);
static int _luaLoadLib(lua_State* L);
static int _luaRunScript(lua_State* L);
static int _luaSendChat(lua_State* L);
static int _luaSpawn(lua_State* L);
static int _luaSpawnAt(lua_State* L);
static int _luaDespawn(lua_State* L);
static int _luaFrametime(lua_State* L);
static int _luaOnline(lua_State* L);
static int _luaIngame(lua_State* L);
static int _luaIsSuperfloat(lua_State* L);
static int _luaIsSupertroop(lua_State* L);
static int _luaOS(lua_State* L);
static int _luaDead(lua_State* L);

// types and locals
std::vector<const char*> script_args;

// game lib names
static const char* _gameLibs[GPT_NUM_TYPES] =
{
	"fp",
	"cfg",
	"ui",
	"tutorial",
	"input",
	"player",
	"vehicle",
	"wwii",
	"strat",
	"weapon",
	"terrain",
	"ob",
	"map",
	"scenario"
};

// The general-purpose/default _luaVM
lua_State* _luaVM;					// General purpose/default _luaVM
std::string _luaSource = "?"; 		// Current source for commands/scripts etc
std::string _luaSubSystem = "?";	// Part of the source
SCRIPT_RESULTS _luaOutput;

//////////////////////////////////////////////////////////////////////
// dbugLog wrapper that takes error level and text string

static inline void _report(UINT32 level, const char* message)
{
	std::string output;

	if ( message == nullptr )
		return;

	// Save the output
	switch (level)
	{
		case Debug::ERR_WARN:	output += "!: "; break;
		case Debug::ERR_FATAL:	output += "<!>: "; break;
		break;
	}

	output += message;
	output += "\n";

	_luaOutput.push_back(output);

	uiChatPrintLine("LUA(%s): %s", _luaSubSystem.c_str(), output.c_str());
	//uiConsoleprintf("LUA(%s): %s", _luaSubSystem.c_str(), output.c_str());
	WriteLog(LOG_UI, LOG_FATAL, "LUA(%s): %s", _luaSubSystem.c_str(), output.c_str());
}

//////////////////////////////////////////////////////////////////////
// print/error/warn -- simple scripting dbugLog wrapper

#define print(m) _report(Debug::ERR_INFO,(m))
#define warn(m) _report(Debug::ERR_WARN,(m))
#define error(m) _report(Debug::ERR_ABORT,(m))

char* luaARGS(int i)
{
	return ( i >= (int)script_args.size() ) ? nullptr : (char*)script_args[i];
}

//////////////////////////////////////////////////////////////////////
// Report the latest lua error and pop it off the stack

static inline bool _luaError(lua_State* L)
{
	error(lua_tostring(L, -1));
	lua_pop(L, 1);
	return false;
}

//////////////////////////////////////////////////////////////////////
// Set the current lua context and flush the output buffer

void _luaContext(const char* source = "?", const char* func = "?")
{
	_luaSource = source;
	_luaSubSystem = func;
}

static std::string _prevSource = "";
static std::string _prevSub = "";

void luaSetContext(const char* func)
{
	_prevSource = _luaSource;
	_prevSub = _luaSubSystem;

	_luaSource = "code";
	_luaSubSystem = func;
}

void luaEndContext()
{
	_luaSource = _prevSource;
	_luaSubSystem = _prevSub;
}

//////////////////////////////////////////////////////////////////////
// execute a script from a file in the global lua state

bool luaRunScript(const char* script, lua_State* L)
{
	std::string path = "";

	bool success = false;

	for(int i = 0; i < LUA_SEARCH_MAX; i++)
	{
		if(strncmp(script, _searchPaths[i], strlen(_searchPaths[i])) == 0)
		{
			path = script;
			break;
		}
	}

	if(path.empty())
	{
		path = "lua/";
		path += script;
	}

	// load up the script
    int fd = fioOpen(path.c_str());
    if(fd > -1)
    {
        size_t size = fioFileSize(path.c_str());
        if(size > 0)
        {
            char* buffer = new char[size+1];
            buffer[size] = '\0';

            if(fioRead(fd, buffer, size) == size)
                success = luaRunObject(path.c_str(), buffer, L);

            delete [] buffer;
        }

        fioClose(fd);
    }

	if(success)
	{
		uiConsoleprintf("LUA RUN SCRIPT: %s", path.c_str());
	}
	else
	{
		char message[255+1];
		snprintf(message, 255, "Unable to run %s", path.c_str());
		error(message);
	}

	return success;
}

//////////////////////////////////////////////////////////////////////
// runObject provides a means to execute a buffer of lua code
// that has already been loaded, such as from a database object

bool luaRunObject(const char* name, const char* obj, lua_State* L)
{
	lua_State* vm = (L) ? L: _luaVM;

	if (!strlen(obj))
	{
		uiConsoleprintf("%s called with null object", __FUNCTION__);
		return false;
	}

	// Tell ourselves what we're executing
	_luaContext("object", name);	// Start of context
	_luaOutput.clear();

	bool success = true;

	if (luaL_loadbuffer(vm, obj, strlen(obj), name) || lua_pcall(vm, 0, 0, 0))
	{
		success = false;
		_luaError(vm);
	}

	_luaContext();					// End of context

	return success;
}

//////////////////////////////////////////////////////////////////////
// execute an anonymous segment of code
// Invoke with "runCode" which is a wrapper that passes your function

bool __luaRunCode(lua_State* L, const char* code, const char* function)
{
//	lua_State* vm = (L) ? L: _luaVM;

	if (!strlen(code))
	{
		uiConsoleprintf("%s called with null code", __FUNCTION__);
		return false;
	}

	return luaRunObject(function, code, L);
}

//////////////////////////////////////////////////////////////////////
// Start an instance of a lua state

lua_State* luaGetVM(bool defaultVM)
{
	if(defaultVM && _luaVM != nullptr)
	{
		uiConsoleprintf("Attempted to restart active default vm");
		return _luaVM;
	}

	if(defaultVM && _luaVM)
		return _luaVM;

	lua_State* vm = lua_open();

	if (vm == nullptr)
	{
		uiConsoleprintf("Unable to start lua interpreter");
		return nullptr;
	}

	// open default libs
	luaL_openlibs(vm);

	// init tolua
	tolua_open(vm);

	// common game tasks
	_initGameLib(vm);

	uiConsoleprintf("started Lua interpreter");

	if (!luaRunScript(LUA_GAME_RC, vm))
		error("startup script failed");

	_luaContext();
	_luaOutput.clear();

	if(defaultVM)
		_luaVM = vm;

	return vm;
}

//////////////////////////////////////////////////////////////////////
// Stop the global interpreter

bool luaStopVM(lua_State* L)
{
	_luaContext();
	_luaOutput.clear();

	uiConsoleprintf("Stopping Lua VM");

	if (!L || L == _luaVM)
	{
		if(_luaVM)
		{
			lua_close(_luaVM);
			_luaVM = nullptr;
			return true;
		}
	}
	else
	{
		lua_close(L);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Return the vector of strings

SCRIPT_RESULTS& luaResults()
{
	return _luaOutput;
}

int luaLoadLib(GAME_PKG_TYPE type, lua_State* L)
{
	switch(type)
	{
		case GPT_FP:			tolua_fp_open(L);			break;
		case GPT_CFG:			tolua_cfg_open(L);			break;
		case GPT_UI:			tolua_ui_open(L);			break;
		case GPT_TUTORIAL:		tolua_tut_open(L);			break;
		case GPT_INPUT:			tolua_imap_open(L);			break;
		case GPT_WWII:			tolua_wwii_open(L);			break;
		case GPT_STRAT:			tolua_strt_open(L);			break;
		case GPT_WEAPON:		tolua_weap_open(L);			break;
		case GPT_PLAYER:		tolua_pers_open(L);			break;
		case GPT_VEHICLE:		tolua_veh_open(L);			break;
		case GPT_TERRAIN:		tolua_trn_open(L);			break;
		case GPT_OB:			tolua_ob_open(L);			break;
		case GPT_MAP:			tolua_uimap_open(L);		break;
		case GPT_SCENARIO:		tolua_scen_open(L);			break;
		default:				return 1;					break; // package type not found
	}

	return 0; // no error
}

// game lib interface

bool _initGameLib(lua_State* L)
{
	tolua_open(L);
	tolua_module(L,nullptr,0);
	tolua_beginmodule(L,nullptr);
	    tolua_module(L,LUA_GAME_LIB,0);
		tolua_beginmodule(L,LUA_GAME_LIB);
			tolua_function(L,"LoadLib",_luaLoadLib);
			tolua_function(L,"RunScript",_luaRunScript);
			tolua_function(L,"SendChat",_luaSendChat);
			tolua_function(L,"Spawn",_luaSpawn);
			tolua_function(L,"SpawnAt",_luaSpawnAt);
			tolua_function(L,"Despawn",_luaDespawn);
			tolua_function(L,"Frametime",_luaFrametime);
			tolua_function(L,"Online",_luaOnline);
			tolua_function(L,"Ingame",_luaIngame);
			tolua_function(L,"SuperFloat",_luaIsSuperfloat);
			tolua_function(L,"SuperTroop",_luaIsSupertroop);
			tolua_function(L,"OS",_luaOS);
			tolua_function(L,"Dead",_luaDead);
		tolua_endmodule(L);
	tolua_endmodule(L);

	return true;
}

int _luaLoadLib(lua_State* L)
{
	const char* lib = tolua_tostring(L,1,0);

	if(lib && *lib)
	{
		for(int i = 0; i < GPT_NUM_TYPES; i++)
		{
			if(!strcasecmp(lib, _gameLibs[i]))
				return luaLoadLib((GAME_PKG_TYPE)i, L);
		}
	}

	return 1;
}

int _luaRunScript(lua_State* L)
{
	return !luaRunScript(tolua_tostring(L,1,0), L);
}

int _luaSendChat(lua_State* L)
{
	if(Online)
	{
		const char* line = tolua_tostring(L,1,0);

		if(line && *line)
		{
			if(uiDotCommandFormat(line))
				if(uiDotCommand(line) <= 0) return 0;

			unsigned int txmask = 0;
			for(int i = 0; i < MAX_CLIENT_CHANNELS; i++)
				if(ChatChannels[i].xmit) txmask |= (1<<i);

			PlayerClient()->SendChatMessage(txmask, line);
		}
	}

	return 1;
}

int _luaSpawn(lua_State* L)
{
	int success = 0;

	if(!uiInGame())
	{
		if(Online)
		{
			// do something
		}
		else
		{
			const vehType* vtype = nullptr;
			const StratDB::StratObject* origin = nullptr;

			if(tolua_isusertype(L, 1, "const vehType", 0, nullptr))
				vtype = ((const vehType*)tolua_tousertype(L, 1, 0));

			if(tolua_isusertype(L, 2, "const StratDB::StratObject", 0, nullptr))
				origin = ((const StratDB::StratObject*)tolua_tousertype(L, 2, 0));

			if(vtype)
			{
				// do old game spawn stuff here
				mlSetSpawnStuff(0, 0, -1, 1);

				if(origin)
				{
					const StratDB::StratObject* spawner = jsGame::GetOfflineSpawn(origin, vtype);
					if(spawner) mlSetSpawnStuff(spawner->SupercellX(), spawner->SupercellY(), spawner->Id(), 1);
				}

				WEAPON_SERIAL blah={0};
				vehcommSendVehicleRequest(vtype->VehicleTypeNum, &blah);

				// call spawner
				success = 1;
			}
		}
	}

	lua_pushboolean(L, success);

	return 1;
}

int _luaSpawnAt(lua_State* L)
{
	int success = 0;

	if(!uiInGame())
	{
		const vehType* vtype = nullptr;
		double x, y, z;
		x = y = z = 0;

		if(tolua_isusertype(L, 1, "const vehType", 0, nullptr))
			vtype = ((const vehType*)tolua_tousertype(L, 1, 0));

		if(tolua_isnumber(L, 2, 0, nullptr)) x = tolua_tonumber(L, 2, 0);
		if(tolua_isnumber(L, 3, 0, nullptr)) y = tolua_tonumber(L, 3, 0);
		if(tolua_isnumber(L, 4, 0, nullptr)) z = tolua_tonumber(L, 4, 0);

		if(vtype)
		{
			if(Online)
			{
				FLOAT64VECTOR at;

				at.x = x;
				at.y = y;
				at.z = z;

				// kill them once the packer does something with at
				//PlayerClient()->SetSpawnOverride(x, y, z);

				// send the request
				PlayerClient()->RequestDirectSpawn(vtype->WeaponSpec, &at);

				/*
				// what to do?
				PlayerClient()->ClearMissionWeap();
				PlayerClient()->RequestMissionWeapPost(nullObjID, vtype->WeaponSpec, false);

				// reserve the weapon
				if(PlayerClient()->WaitForMessage(ww2::Client::WEAP_POST_WAIT, 3) == 0)
				{
					// set spawn override
					PlayerClient()->RequestClientSpawn(PlayerClient()->WeaponSerial(), true);

					// call spawner
					success = 1;
				}
				*/
				success = 1;
			}
			else
			{
				// do old game spawn stuff here
				mlSetSpawnStuff(0, 0, -1, 1);
				mlSetMobileSpawnStuff(0, x, y, z);
				WEAPON_SERIAL blah={0};
				vehcommSendVehicleRequest(vtype->VehicleTypeNum, &blah);
				// call spawner
				success = 1;
			}
		}
	}

	lua_pushboolean(L, success);

	return 1;
}

int _luaDespawn(lua_State* L)
{
	int success = 0;
	lua_pushboolean(L, success);

	return 1;
}

int _luaFrametime(lua_State* L)
{
	tolua_pushnumber(L,(lua_Number)frametime);
	return 1;
}

int _luaOnline(lua_State* L)
{
	lua_pushboolean(L,Online);
	return 1;
}

int _luaIngame(lua_State* L)
{
	lua_pushboolean(L,(int)uiInGame());
	return 1;
}

int _luaOS(lua_State* L)
{
#if platOS == OS_MACOS
	tolua_pushnumber(L,PI_OS_MAC);
#else
	tolua_pushnumber(L,PI_OS_PC);
#endif

	return 1;
}

int _luaDead(lua_State* L)
{
	vehVehicle* v = persGetPersonaVehiclePtr();
	bool dead = true;

	if(v && v->InUse && !v->ModelData->DeclaredKilled)
	{
		if(v->VehicleType->Type == VEH_TYPE_PERSON)
			dead = fpFPSIsDead();
		else if(!v->isHulked())
			dead = false;
	}

	lua_pushboolean(L,(int)dead);

	return 1;
}

int _luaIsSuperfloat(lua_State* L)
{
	lua_pushboolean(L,(SuperFloat || persIsPersonaVehicleType(VEH_TYPE_FLOATER)) ? 1 : 0);
	return 1;
}

int _luaIsSupertroop(lua_State* L)
{
	lua_pushboolean(L,(int)SuperTroop);
	return 1;
}
