#pragma once
// Created: Fri Sep  2 14:12:57 CDT 2005
//
// Constant definitions (chatConstants.h already taken)
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
//tolua_begin

// How long can a message be in bytes?
#define MAX_MESSAGE_LENGTH	600
// How many bits does it take to describe that?
#define MAX_MESSAGE_LENGTH_BITS	10
// Make sure MAX_IRC_LEN is less thatn MAX_MESSAGE_LENGTH
#define MAX_IRC_LEN         512

#if (MAX_IRC_LEN >= MAX_MESSAGE_LENGTH)
#error MAX_IRC_LEN not compatible with MAX_MESSAGE_LENGTH
#endif

#define MAX_PRINTABLE_LENGTH 4096
#define MAX_CHANNEL_NAME_LENGTH	80
#define CHAT_NO_SQUAD 0
#define CHAT_NO_MISSION 0
#define MAX_CLIENT_CHANNELS	6

enum SIDE_MESSAGE_TYPE
{
	SM_UNDEFINED = 0
,	SM_SIDE							// Pure and simple side message
,	SM_HC							// High command message
};

// Sub channels of the SIDE channel
enum SIDE_SUB_CHANNEL
{
	SIDE_SUB_INVALID = 0
,	SIDE_SUB_GENERAL
,	SIDE_SUB_HELP
,	SIDE_SUB_OPS
,	MAX_SIDE_SUB
};

//tolua_end

