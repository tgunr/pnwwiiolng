#pragma once
// Created: Mon Apr 30 14:16:28 CDT 2007
//
// Enumeration of chat channel types
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//The types of channels there are
// Host: Add transient channels to the list in chatDestroyChannel
enum CHANNEL_TYPE
{
	NO_CHANNEL
,	BRIGADE_CHANNEL
,	MISSION_CHANNEL
,	SQUAD_CHANNEL
,	GCI_CHANNEL
,	CP_CHANNEL
,	NUMBERED_CHANNEL
,	PRIVATE_CHANNEL
,	ADMIN_CHANNEL
,	ORIGIN_CP_CHANNEL
,	OBJECTIVE_CP_CHANNEL
,	SIDE_GENERAL_CHANNEL
,	SIDE_HELP_CHANNEL
,	SIDE_OPS_CHANNEL
,	MAX_CHANNEL_TYPES
};

