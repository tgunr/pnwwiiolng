#pragma once
// Created: Fri Feb 22 02:11:44 CST 2008
// Copyright (C) PlayNet, Inc 2008-2011 - All rights reserved.
// Not for redistribution.
//
// #defines for arenaLock values
//
// Originating Programmer: Oliver "kfsone" Smith

// Arena Lock enums/bit masks
//tolua_begin
enum
{
	AL_OFFLINE		=	-4
,	AL_LOADING		=	-3
,	AL_INITIALIZING	=	-2
,	AL_SYNCING		=	-1
,	AL_OFF			=	0x00
,	AL_CAPTURE		=	0x01
,	AL_SPAWN		=	0x02
,	AL_KILL			=	0x04
,	AL_ENTRY		=	0x08
,	AL_MAX_BIT		=	0x10

,	FULL_ARENA_LOCK = AL_CAPTURE + AL_SPAWN + AL_KILL + AL_ENTRY
} ;
//tolua_end
