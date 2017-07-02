#pragma once
// Core implementation of dot commands on the strat host;
// this are the various hooks and stubs for supporting
// out of band data commands, primarily from the chat host,
// but potentialy from the other hosts.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

class OOBMessage;
class CPStruct;
class FacilityStruct;

struct DOT_COMMANDS
{
	const char*	command;
	bool (*stub)(char* arguments, OOBMessage& message);
	const char*	description;
};
extern DOT_COMMANDS dotCommands[];

extern std::vector<const char*> script_args;

extern void strtProcessOOBMessage(OOBMessage& message);
extern OOBMessage* script_env;

//tolua_begin
extern void dotApplyConfig();

extern CPStruct* targeted_cp;
extern FacilityStruct* targeted_fac;

//tolua_end
