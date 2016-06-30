#pragma once
// Created: Mon Jul 30 11:07:38 CDT 2007
//
// Playnet<->IRC Interface functions and classes
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <libircclient.h>

struct IRC
{
	IRC() {}
	virtual ~IRC() {}
	virtual bool ignoring(const char* channelName) const = 0;
	virtual const std::string& nick() const = 0;

	static const char _SERVER_PREFIX = '^';
};

#include "host/hostCommand.h"

struct IRC_Command : public Host::CommandMsg
{
	IRC_Command(const char* fromNick, const char* reportTo, const char* withInput)
		: Host::CommandMsg(fromNick, reportTo, withInput)
		{}
	virtual void Reply(ResponseType, const char* format, ...) const override;
};

// Incase you need to call these
extern void wwiiIRCDumpEvent(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);
extern void wwiiIRCDefaultConnect(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);
extern void wwiiIRCDefaultJoin(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);
extern void wwiiIRCDefaultNumeric(irc_session_t* session, unsigned int event, const char* origin, const char** params, unsigned int count);
extern void wwiiIRCDefaultPrivMsg(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);

// If this a notice that /we/ joined a channel, handles the message and
// returns true. If no processing of the message is done by the system,
// returns false.
extern bool wwiiIRCHandleJoinMessage(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);

// Operation functions
extern bool wwiiIRCInitialize(UINT32 ip, bool plural=false);
extern void wwiiIRCAddInitialChannel(std::string name, const char* mode, const char* topic);
extern void wwiiIRCAddChannelIgnore(const char* name);
extern void wwiiIRCAddInitialMode(const char* mode);
// IRC Event handling callbacks are defined in this structure, however, you must
// populate it* between* Initialize and Activate
extern irc_callbacks_t ircCallbacks;
extern bool wwiiIRCActivate(bool includeSigHup);
extern void wwiiIRCTerminate(const char* reason);

extern void wwiiIRCChannelMessage(const char* channel, const char* message);
extern void wwiiIRCMessage(const char* channel, const char* fmt, ...) FORMAT_FN(2,3);
extern void wwiiIRCChannelNotice(const char* channel, const char* message);

typedef void(*irc_dotcmd_t)(Host::CommandMsg &);
extern void wwiiIRCRegisterCommand(const char* command, irc_dotcmd_t function);
