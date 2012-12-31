#pragma once
// Created: Sun Jan 13 17:31:19 CST 2008
//
// Enumerations for the Chat icons
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Chat
{

	enum
	{
		ENGLISH_FLAG = 1
	,	USA_FLAG
	,	FRENCH_FLAG
	,	GERMAN_FLAG
	,	ITALIAN_FLAG
	,	JAPANESE_FLAG
	,	COMMONWEALTH_FLAG
	,	CHINESE_FLAG
	,	RUSSIAN_FLAG
	,	WORLD_ICON			// 10
	,	THEATER_ICON
	,	MISSION_ICON
	,	SQUAD_ICON
	,	ORIGIN_ICON
	,	PRIVATE_ICON
	,	ALLIED_ICON
	,	AXIS_ICON
	,	HQ_ICON
	,	BRIGADE_ICON
	,	HELP_ICON			// 20
	,	OBJECTIVE_ICON
	,	HC_ICON
	,	SAY_ICON
	,	INFO_ICON
	,	ENEMY_SAY_ICON
	,	UNUSED1_ICON
	,	UNUSED2_ICON
	,	UNUSED3_ICON
	,	CAUTION_ICON
	,	WHISPER_ICON		// 30
	,	YELL_ICON			// 31
	} ;

	// How many bits required to describe an icon?
	static const UINT32 ICON_DESCRIPTION_BITS = 5 ;
}
