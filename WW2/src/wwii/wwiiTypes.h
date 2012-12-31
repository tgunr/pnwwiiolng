#pragma once
// Created: Thu May 24 14:57:15 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "wwii/wwiiOwnershipEnums.h"

//tolua_begin

#define 	WWII_X 128
#define		SUPERCELL_SIZE_CM	1280000
#define		SUPERCELL_SIZE_M	12800

enum
{
	WWII_DIALOG_Y = 1,
	WWII_BUTTON_REPLY_Y,
	WWII_ACTIVATE_Y,
	WWII_ERROR_INFO_Y,
	_WWII_UNUSED_1_Y,
	WWII_ADDRESS_LIST_Y,
	WWII_PING_Y,
	WWII_USER_ABORTED_APP_Y
};

// Determination of the different infantry types
enum WWII_INF_TYPE
{
	_NON_INF	= 0
,	_INF_RIFLE	= 1
,	_INF_SMG	= 2
,	_INF_ENG	= 3
,	_INF_LMG	= 6
,	_INF_RG		= 7
,	_INF_HC		= 8
,	_INF_ATR	= 9
,	_INF_PARA	= 10
,	_INF_ML		= 11
,	_INF_SNIPER	= 12
,	_INF_MORTAR	= 13
,	MAX_WWII_INF_TYPES
} ;

//Used for a number of FRONT_END requests, probably needs to move to a more central location
struct SUPER_CELL
{
	SINT16 sx;
	SINT16 sy;
} ;

//tolua_end

//This is the full ID of any object in the strat system
struct FULL_ID
{
	SINT16	sx;			// supercell coordinates start at the center of the map
	SINT16	sy;			// and therfore must be signed.
	UINT16	id;
	UINT16	type;
} ;

// supercell relative id
struct REL_OBJ_ID
{
	UINT16		id;
	SINT8		objClass;
	SINT8		objType;

} ;

enum WWII_DIALOG
{
	DLG_INVALID = 0,
	DLG_BASE_CAPTURED,
	MAX_DIALOGS
} ;

struct FULL_OBJ_ID ;

