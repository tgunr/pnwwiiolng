#pragma once

#include "gr2/gr2State.h"

//extern bool gr2VehicleStateLookup(GR2_FLAG_TYPE type, const char* name, unsigned int* value);

enum GR2VEHICLE_CONTROL_FLAG
{
	GVCONT_ENGINE				= 1<<0,
	GVCONT_DEPLOY				= 1<<1,
	GVCONT_THROTTLE				= 1<<2,
	GVCONT_STEER_RIGHT			= 1<<3,
	GVCONT_STEER_LEFT			= 1<<4,
	GVCONT_FIRE1				= 1<<5,
	GVCONT_FIRE2				= 1<<6,
	GVCONT_TURRET_TRAVERSE		= 1<<7,
	GVCONT_TURRET_ELEVATE		= 1<<8
};

// 1<<0 to 1<<15 are shared states
enum GR2VEHICLE_ACTION_FLAG
{
	GVACT_IDLE					= 1<<0,
	GVACT_MOVE					= 1<<1,
	GVACT_TURN					= 1<<2
};

// 1<<0 to 1<<15 are shared states
enum GR2VEHICLE_CONDITION_FLAG
{
	GVCOND_ENGINE				= 1<<0,
	GVCOND_LFOOT				= 1<<1,
	GVCOND_RFOOT				= 1<<2,
	GVCOND_TURRET				= 1<<3,
	GVCOND_REVERSE				= 1<<4
};

