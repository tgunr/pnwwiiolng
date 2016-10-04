#pragma once
// Created: Thu Sep 23 12:45:00 2004
//
// Generic Lua glue code prototypes
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

// Lua/Tolua API headers
#ifdef __cplusplus
extern "C"
{
#endif
	#include <lualib.h>
#ifdef __cplusplus
}
#endif
#include <tolua++.h>

// Wrappers for dbugLog
//tolua_begin
extern void print(const char* message);
extern void warn(const char* message);
extern void error(const char* message);
extern void writeToLog(const char* file, const char* func, unsigned int line, const char* level, const char* facil, const char* message);
extern char* ARGS(int i);
extern void gmMessage(const char* message);
extern std::string luaToC_string;
extern const char* getCallsign(unsigned long long);

extern void dbDo(const char* msg);
//tolua_end

namespace Instance { class PlayerState; }

// execute a script in the global lua state
namespace Lua
{
	extern bool RunScript(const char* fileName);
	extern bool RunObject(const char* name, std::string& object);
	extern bool RunCode(std::string& code, const char* caller);

	extern void CallInstanceFunction(const char* function, const instanceid_t instID, const char* data);
	extern void CallInstanceFunction(const char* function, const instanceid_t instID, playerid_t playerID_, Instance::PlayerState* state);

	// Pre-compile a lua statement into a compiled chunk.
	extern std::string CompileCode(std::string code, const char* name);

	// Tell me what the last error was
	typedef std::vector<std::string> SCRIPT_RESULTS;
	extern SCRIPT_RESULTS& Results();

	////////// Functions for controlling lua states
	// start the global interpreter
	extern bool Start(const char* const hostName);
	// stop the global interpreter
	extern bool Stop();

	// Set the context
	extern void SetContext(const char* func);
	extern void EndContext();

	// Consume the event queue.
	extern void ProcessEventQueue();

} // namespace Lua

#define tolua_outside

