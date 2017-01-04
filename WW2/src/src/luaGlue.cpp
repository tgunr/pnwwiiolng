#include <syscfg.h> // Path is project specific.
// Created: Thu Sep 23 12:45:00 2004
//
// Generic Lua glue code.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/luaGlue.h"
#include "wwii/wwiiIRC.h"
#include "teul/teulConfigVar.h"
#include "wwii/wwiiHCUnitsIndex.h"

extern std::vector<const char*> script_args;

// The Lua module open routine specified by #defines on command line
extern int tolua_game_open(lua_State*);

// The general-purpose/default _luaVM
lua_State* _luaVM;					// General purpose/default _luaVM
std::string _luaSource = "?";		// Current source for commands/scripts etc
std::string _luaSubSystem = "?";	// Part of the source
Lua::SCRIPT_RESULTS _luaOutput;

extern ConfigPath cv_luaPath;
extern ConfigPath cv_luaRCFile;

//////////////////////////////////////////////////////////////////////
// dbugLog wrapper that takes error level and text string

static inline void
_report(UINT32 level, const char* message, const char* file = _luaSubSystem.c_str(), unsigned int lineNo = 0)
{
	std::string output;

	if ( message == nullptr )
		return;

	// Save the output
	switch (level) {
		case Debug::ERR_WARN:	output += "!: "; break;
		case Debug::ERR_FATAL:	output += "<!>: "; break;
		case Debug::ERR_ABORT: output += "ERR: "; break;
	}
	output += message;
	output += "\n";

	_luaOutput.push_back(output);

	if ( _luaSource.size() > 0 )
		_dbugLog(_luaSubSystem.c_str()		// Source
				, 0						// Line "0" (we don't know)
				, level					// It's only a print
				, "%s: %s"				// We don't want the mask parsing for %s
				, _luaSource.c_str()	// The sub-system if specified
				, message
				);
	else
		_dbugLog(_luaSubSystem.c_str()
				, 0
				, level
				, "%s"
				, message);
}

extern const char* chatGetCallsign(const ChatHandle);
const char* getCallsign(unsigned long long handle) { return chatGetCallsign((ChatHandle)handle); }

//////////////////////////////////////////////////////////////////////
// print/error/warn -- simple scripting dbugLog wrapper

void print(const char* message) { _report(Debug::ERR_INFO, message); }
void warn(const char* message) { _report(Debug::ERR_WARN, message); }
void error(const char* message) { _report(Debug::ERR_ABORT, message); }
void writeToLog(const char* file, const char* func, unsigned int line, const char* level, const char* facil, const char* message)
{
	if ( ((file == nullptr) | (func == nullptr) | (message == nullptr) | (level == nullptr)) || *level == 0 )
		return;

	char levelChar = toupper(level[0]);
	UINT32 levelNo = 0;
	switch ( levelChar )
	{
	case 'D': levelNo = Debug::ERR_DBUG; break;
	case 'N': levelNo = Debug::ERR_NOTE; break;
	case 'M': levelNo = Debug::ERR_MINOR; break;
	case 'W': levelNo = Debug::ERR_WARN; break;
	case 'A': levelNo = Debug::ERR_ABORT; break;
	case 'F': levelNo = Debug::ERR_FATAL; break;
	case 'I':	/*PASSTHRU*/
	default: levelNo = Debug::ERR_INFO; break;
	}

	std::string filename = facil;
	filename += "@";
	filename += file;
	filename += ":";
	filename += func;

	_dbugLog(filename.c_str(), line, levelNo, "%s", message);

	std::string output = "";
	switch (levelNo)
	{
		case Debug::ERR_WARN:	output = "<W>:"; break;
		case Debug::ERR_FATAL:	output = "[F]:"; break;
		default:
			output += levelChar;
			output += ":";
		break;
	}
	output += filename;
	output += ":";
	char lineNo[22];
	strprintf(lineNo, "%u", line);
	output += lineNo;
	output += ": ";
	output += message;
	output += "\n";
	_luaOutput.push_back(output);
}
char* ARGS(int i) { return ( i >= (int)script_args.size() ) ? nullptr : const_cast<char*>(script_args[i]); }

extern bool chatSendAdminMessage(const char* fmt, ...);
void gmMessage(const char* message) { chatSendAdminMessage("%s", message); }

//////////////////////////////////////////////////////////////////////
// Various minor functions.

void
_cantReadReturn(const char* fn)
{
	dbugLog(_FLW, "Incorrect return type to strat.%s: %s", fn, lua_tostring(_luaVM, -1));
	lua_pop(_luaVM, 1);
}

bool
_callFunction(const char* fn, int nargs)
{
	Lua::SetContext(fn);
	bool result = (lua_pcall(_luaVM, nargs, 1, 0) != 0);
	Lua::EndContext();

	if ( result )
	{
		dbugLog(_FLW, "%s failed: %s", fn, lua_tostring(_luaVM, -1));
		lua_pop(_luaVM, 1);
		return false;
	}

	return true;
}

int
_getNumber(const char* function, int argc=0)
{
	if ( !_callFunction(function, argc) )
		return 0;

	if ( !lua_isnumber(_luaVM, -1) )
	{
		_cantReadReturn(function);
		return 0;
	}
	int result = (int)lua_tonumber(_luaVM, -1);
	lua_pop(_luaVM, 1);
	return result;
}

double
_getDouble(const char* function, int argc=0)
{
	if ( !_callFunction(function, argc) )
		return 0;

	if ( !lua_isnumber(_luaVM, -1) )
	{
		_cantReadReturn(function);
		return 0;
	}
	double result = (double)lua_tonumber(_luaVM, -1);
	dbugLog(_FLI, "%s returned %g", function, result);
	lua_pop(_luaVM, 1);
	return result;
}

//////////////////////////////////////////////////////////////////////
// Report a lua error to the log / irc server

void
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

	Lua::Results().clear();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Log it and shut down.
	LogAbort("luaErr: %s", error);
}

//////////////////////////////////////////////////////////////////////
// Report the latest lua error and pop it off the stack

static inline bool
_luaError()
{
	error(lua_tostring(_luaVM, -1));
	lua_pop(_luaVM, 1);
	return false;
}

static void
_luaOpenLib(lua_State* L, const char* name, lua_CFunction fn)
{
	lua_pushcfunction(L, fn);
	lua_pushstring(L, name);
	lua_call(L, 1, 0);
}

static void
_luaOpenLibs(lua_State* L)
{
	luaL_openlibs(L);
	_luaOpenLib(L, "debug", luaopen_debug);
}

//////////////////////////////////////////////////////////////////////
// Set the current lua context and flush the output buffer

void
_luaContext(const char* source = "?", const char* func = "?")
{
	_luaSource = source;
	_luaSubSystem = func;
}

static std::string _prevSource = "";
static std::string _prevSub = "";

void
Lua::SetContext(const char* func)
{
	_prevSource = _luaSource;
	_prevSub = _luaSubSystem;

	_luaSource = "code";
	_luaSubSystem = func;
}

void
Lua::EndContext()
{
	_luaSource = _prevSource;
	_luaSubSystem = _prevSub;
}

//////////////////////////////////////////////////////////////////////
// Innards of script execution for any given lua_State

bool
_executeScript(const char* script)
{
	_luaContext("file", script);	// Start context
	_luaOutput.clear();

	bool success;

	if ( luaL_loadfile(_luaVM, script) || lua_pcall(_luaVM, 0, 0, 0) )
	{
		dbugLog(_FLW, "%s", lua_tostring(_luaVM, -1));
		_luaError();
		success = false;
	}
	else
		success = true;

	_luaContext();					// End of context

	return success;
}

//////////////////////////////////////////////////////////////////////
// execute a script from a file in the global lua state

bool
Lua::RunScript(const char* script)
{
	if ( script == nullptr || *script == 0 )
	{
		dbugLog(_FLW, "%s called with null script name", __FUNCTION__);
		return false;
	}

	return _executeScript(script);
}

//////////////////////////////////////////////////////////////////////
// Compile code and return the lua chunk.

const char*
_luaReader(lua_State* L, void* data, size_t* size)
{
	static const void* lastData = nullptr;
	if ( data == lastData )
	{
		lastData = nullptr;
		return nullptr;
	}

	lastData = data;
	const std::string* str = (const std::string*)data;
	*size = str->length();

	return str->c_str();
}

static int
_luaWriter(lua_State* L, const void* p, size_t sz, void* ud)
{
	std::string* str = (std::string*)ud;
	str->append((const char*)p, sz);
	return 0;
}

std::string
Lua::CompileCode(std::string code, const char* name)
{

	if ( lua_load(_luaVM, _luaReader, &code, name) != 0 )
	{
		_luaError();
		return "";
	}

	std::string result = "";
	if ( lua_dump(_luaVM, _luaWriter, &result) != 0 )
	{
		_luaError();
		return "";
	}

	lua_pop(_luaVM, 1);

	return result;
}

//////////////////////////////////////////////////////////////////////
// runObject provides a means to execute a buffer of lua code
// that has already been loaded, such as from a database object

bool
Lua::RunObject(const char* name, std::string& obj)
{
	if ( obj.size() == 0 )
	{
		dbugLog(_FLW, "%s called with null object", __FUNCTION__);
		return false;
	}

	// Tell ourselves what we're executing
	_luaContext("object", name);	// Start of context
	_luaOutput.clear();

	bool success = true;
	if ( luaL_loadbuffer(_luaVM, obj.c_str(), obj.size(), name)
		|| lua_pcall(_luaVM, 0, 0, 0) )
	{
		success = false;
		_luaError();
	}

	_luaContext();					// End of context

	return success;
}

//////////////////////////////////////////////////////////////////////
// execute an anonymous segment of code
// Invoke with "runCode" which is a wrapper that passes your function

bool
Lua::RunCode(std::string& code, const char* function)
{
	if ( code.size() == 0 )
	{
		dbugLog(_FLW, "%s called with null code", __FUNCTION__);
		return false;
	}
	_luaContext("lua", function);	// start of context
	_luaOutput.clear();

	bool success = true;
	if ( luaL_loadbuffer(_luaVM, code.c_str(), code.size(), "code")
		|| lua_pcall(_luaVM, 0, 0, 0) )
	{
		success = false;
		_luaError();
	}

	_luaContext();					// end of context

	return success;
}

//////////////////////////////////////////////////////////////////////
// Start an instance of a lua state

extern const char* HostLanguage;

static bool
_startInterpreter()
{
	_luaVM = lua_open();
	if ( _luaVM == nullptr )
	{
		dbugLog(_FLA, "Unable to start lua interpreter");
		return false;
	}

	_luaOpenLibs(_luaVM);

	lua_getglobal(_luaVM, "package");
	lua_getfield(_luaVM, -1, "path");
	std::string luaPath = lua_tostring(_luaVM, -1);
	luaPath.append(";");
	luaPath.append(cv_luaPath.GetString());
	luaPath.append("/?.lua");
	lua_pop(_luaVM, 1);
	lua_pushstring(_luaVM, luaPath.c_str());
	lua_setfield(_luaVM, -2, "path");
	lua_pop(_luaVM, 1);

	tolua_game_open(_luaVM);
	dbugLog(_FLI, "started Lua interpreter");

	std::string rcFile = cv_luaPath.GetString();
	rcFile += "/";
	rcFile += cv_luaRCFile.GetString();
	if ( !_executeScript(rcFile.c_str()) || _luaVM == nullptr )
	{
		dbugLog(_FLW, "startup file failed");
		exit(0);
	}

	rcFile = cv_luaPath.GetString();
	rcFile += "/rc-";
	rcFile += HostLanguage;
	rcFile += ".lua";

	_executeScript(rcFile.c_str());

	_luaContext();
	_luaOutput.clear();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Start the global lua interpreter

bool
Lua::Start(const char* const hostName)
{
	if ( _luaVM != nullptr )
	{
		dbugLog(_FLW, "Attempted to restart active global vm");
		return true;
	}

	if ( !_startInterpreter() )
		return false;

	std::string functionName = hostName;
	functionName += "Setup";

	lua_getglobal(_luaVM, functionName.c_str());
	if ( lua_isfunction(_luaVM, -1) != 1 )
		return false;

	_callFunction(functionName.c_str(), 0);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Stop the global interpreter

bool
Lua::Stop()
{
	_luaContext();
	_luaOutput.clear();

	if ( _luaVM == nullptr )
		return false;

	lua_close(_luaVM);
	_luaVM = nullptr;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Return the vector of strings

Lua::SCRIPT_RESULTS&
Lua::Results()
{
	return _luaOutput;
}
