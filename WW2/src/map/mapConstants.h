#pragma once
// Created: Tue Jan 23 13:09:55 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

namespace Map
{
	enum
	{
		CallbackX = 208
	};

	enum
	{
		XCHG_HCUNIT_LIST_Y			// Exchange a list of HC Units
	,   XCHG_PERSONA_LIST_Y			// Callback to send/receive a persona list
	,   XCHG_PERSONA_SELECT_Y		// Persona selection callbacks
	,   XCHG_BRIGADE_LIST_Y			// Brigade list request/reply
	,   XCHG_BRIGADE_SELECT_Y		// Brigade selection
	,	XCHG_MISSION_LIST_Y			// List mission request/reply

	,   BUILDING_VISLEVELS_Y		// Big packet full of vislevels.
	,   BUILDING_VISLEVEL_Y			// FULL_OBJ_ID + vislevel.
	,   ARENA_DESCRIPTION_Y			// Arena Description variables
		, _SPARE_5_Y
		, _SPARE_6_Y
		,   EXIT_WWIIOL_Y
	,	_SPARE_1_Y
	,	HC_UNIT_LOCATIONS_Y

	,	OPINION_POLL_Y
	,	POLL_RESULT_Y
	,	TUT_PROGRESS_Y

	,	SWING_METER_Y
	,   HC_UNIT_COUNTRY_CHANGE_Y
	};
}
