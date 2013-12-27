#pragma once
// Created: Wed Jul 26 14:13:03 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Handle misc dot command clean calls
//
// Originating Programmer: Troy "Ramp" Lowe

#include <wchar.h>

enum
{
    NO_OVERRIDE,
    AREA_51_OVERRIDE,
    NOSPAWN_OVERRIDE,
    OLDSPAWN_OVERRIDE,
    BENCHANTWERP_OVERRIDE,
    BENCHRAMAGEN_OVERRIDE,
	SCENARIO_OVERRIDE
};

enum
{
    BENCH_NOTHING,
    BENCH_ANTWERP,
    BENCH_REMAGEN,
    BENCH_PROFILEFLIGHT,
    BENCH_VEHICLES
};

extern void uiDotCommandsInit();
extern void uiDotCommandsInitLoc();
extern int uiDotCommand(const char*);
extern int uiDotCommand(const wchar_t*);
extern bool uiDotCommandFormat(const char* message);
extern bool uiDotCommandFormat(const wchar_t* message);
extern const wchar_t* uiDotCommandMatch(const char* search, bool again);
extern const wchar_t* uiDotCommandMatch(const wchar_t* search, bool again);

extern piVECTOR& uiDotGetPosXYZ();

// returns the last command
extern const wchar_t* uiDotCommandLast();

// vspawn input handler
extern void uiDotCommandAddClone(int id);

// Squad dot commands can only be issued one at a time, unless they timeout
// Whenever we get a result back from the host, the timeout gets cleared, so
// we can continue on without interruption
extern void uiClearSquadCommandTimeout();

typedef int (*dotCmdFunctionA)(const char*);
typedef int (*dotCmdFunctionW)(const wchar_t*);

extern void uiDotCommandAdd(const char* cmdName, dotCmdFunctionA, bool key);
extern void uiDotCommandAdd(const char* cmdName, dotCmdFunctionW, bool key);

#define ADD_DOT_COMMAND(func, key) \
extern int func(const char*); \
	uiDotCommandAdd(#func, dot_##func, key);

#define ADD_HOST_COMMAND(name, key) uiDotCommandAdd(#name, dot_host_command, key);

// handler return values

// handler error out
#define DC_ERROR -1

// handler handled the command
#define DC_HANDLED 0

// handler didn't handle, send to host
#define DC_PASS_TO_HOST 1

// handler didn't handle, send last command to host
#define DC_PASS_USE_LAST 2

