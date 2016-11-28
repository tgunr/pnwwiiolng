#pragma once
// Date: Thu Sep 23 12:45:00 2004
//
// Generic Lua glue code prototypes
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Troy Lowe
//
// Code derived from Oliver's luaGlue.h/.cpp host code

#include <vector>
#include <string>
#include <math.h>

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

// tolua_begin
enum GAME_PKG_TYPE
{
	GPT_FP,
	GPT_CFG,
	GPT_UI,
	GPT_TUTORIAL,
	GPT_INPUT,
	GPT_PLAYER,
	GPT_VEHICLE,
	GPT_WWII,
	GPT_STRAT,
	GPT_WEAPON,
	GPT_TERRAIN,
	GPT_OB,
	GPT_MAP,
	GPT_SCENARIO,
	GPT_NUM_TYPES
};
// tolua_end

extern char* luaARGS(int i);

// execute some lua code
// from a file; fio aware
extern bool luaRunScript(const char* fileName, lua_State* L = nullptr);

// from a string buffer
extern bool luaRunObject(const char* name, const char* object, lua_State* L = nullptr);

// run a code snippet; pass nullptr for the state to run in the global state
extern bool __luaRunCode(lua_State* L, const char* code, const char* caller);
#define luaRunCode(l,c) __luaRunCode(l,c,__FUNCTION__)

// Tell me what the last error was
using SCRIPT_RESULTS = std::vector<std::string>;
extern SCRIPT_RESULTS& luaResults();

////////// Functions for controlling lua states
// start a interpreter
// if its the default VM, starts it if needed
extern lua_State* luaGetVM(bool defaultVM = true);

// stop a interpreter; pass nullptr for the global VM
extern bool luaStopVM(lua_State* L = nullptr);

// Set the context
extern void luaSetContext(const char* func);
extern void luaEndContext();

// function for initing lua game packages
extern int luaLoadLib(GAME_PKG_TYPE type, lua_State* L);

