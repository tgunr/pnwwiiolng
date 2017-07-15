#pragma once
// Created: Sun Feb  7 19:48:32 PST 2010
//
// Update system enumerations and defines.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// These values are arbitrary, and designed to create an initially high
// "covert check" frequency which decreases over time but also
// has a minimum that tries to reduce overhead.
static const unsigned int INIT_UPDATE_PRIME = 127979;
static const unsigned int UPDATE_PRIME_DEC = 311;
static const unsigned int MIN_UPDATE_PRIME = 32801;

enum
{
	VEH_MAX_DAMAGE_OBJECTS = 128,
	VEH_MAX_OBJ_HIT = 16,
	MAX_TURRETS	= 8,
	MAX_GUNS = 16,
	MAX_UPDATE_VEHICLES = 200,
	MAX_VEH_REMOVE = MAX_UPDATE_VEHICLES,
	MAX_VEH_PER_UPDATE = 64,
	FE_UPDATE_INTERVAL = 95,
	FE_UPDATES_PER_SECOND = (1000 / FE_UPDATE_INTERVAL)
};

enum
{
	UPDT_LONE =0,
	UPDT_CONTROL,
	UPDT_OTHER
};
using UPDT_STATION = UCHAR;

enum ONETIME_BITS
{
	OTB_IN_SQUAD		= 0x01		///< Player/Vehicle is in viewer's squad.
,	OTB_HIGH_COMMAND	= 0x02		///< Player/Vehicle is a high commander.
,	OTB_BUILDER			= 0x04		///< Player/Vehicle is a Builder.
,	OTB_USE_DECALS		= 0x08		///< Player/Vehicle has selected a decal.
,	OTB_IN_BRIGADE		= 0x10		///< Player/Vehicle is in viewer's brigade.
,	OTB_MISSION_LEADER	= 0x20		///< Player/Vehicle is viewer's mission leader.
,	OTB_IN_MISSION		= 0x040		///< Player/Vehicle is on viewer's mission.
,	OTB_TRAINER			= 0x080		///< Player/Vehicle is a trainer.
,	OTB_MAX				= 0x100
};

enum VEH_GLOBAL_FLAGS
{
	VEHG_DEAD	=	0x0001			// Vehicle dead
,	VEHG_KILLED	=	0x0002			// Has been killed
,	VEHG_HC		=	0x0004			// Is High Commander
};

enum VEHICLE_STATE
{
	VEH_FREE = 1,
	VEH_RESERVED,
	VEH_IN_USE,
	VEH_DEAD,
	VEH_EXIT,
	VEH_DELETED
};

//! "PResent" flags. Intended to indicate which sections of the world
//! update are being included, they also now include state fields such
//! as "PR_DEAD" (crew has died).

enum
{
	PR_FIRE1	= 0x01,			//!< Low 8-bits of firing message.
	PR_FIRE2	= 0x02,			//!< High 8-bits of firing message.
	PR_STATE0	= 0x04,			//!< First 16 state bits being sent.
	PR_STATE1	= 0x08,			//!< Second 16 state bits being sent.

	PR_TURRET	= 0x10,			//!< Turret data being sent.
	PR_VEH_VIS	= 0x20,			//!< Visible damage bits being sent.
	PR_ONE_TIME	= 0x40,			//!< Host sending "one time" vehicle header.
	PR_POSITION	= 0x80,			//!< Position information updated.

	PR_ATTITUDE	= 0x0100,		//!< One or more attitude flags.
	PR_DEAD		= 0x0200,		//!< Indicates crew has been killed.
	PR_KILLED 	= 0x0400,		//!< Indicates critical damage to vehicle (kill score).
	PR_RIDING	= 0x0800,		//!< Riding information being sent.

	PR_RELPOS	= 0x1000,		//!< Relative position-to-ride being sent.
	PR_FROM_CONTROLLER	= 0x2000,	//!< Indicates message is from controlling player.
	PR_FRAMEHASH = 0x4000       //!< Contains antihack info disguised as frame-timing stuff.
};

enum
{
	PRX_DESPAWN_TIMER	= 0x010000		//!< Client->Host: Requesting despawn timer.
,	PRX_FIRST_TIME		= 0x020000		//!< Host->Host: Includes first time.
,	PRX_REALLY_ACTIVE	= 0x040000		//!< Host->Host: Player is now really active.
};

enum { PR_NUM_BITS = 16, PRX_NUM_BITS = 24 };

static const UINT32 PR_FIRE_FLAGS = PR_FIRE1 | PR_FIRE2;
static const UINT32 FIRSTTIME_PR_FLAGS = PR_STATE0|PR_STATE1|PR_FIRE_FLAGS|PR_POSITION|PR_TURRET|PR_VEH_VIS|PR_ONE_TIME|PR_RIDING|PR_RELPOS;
using PRESENT_FLAGS = UINT32;

#define PR_SET_ALL ( PR_STATE0 | PR_STATE1 | PR_POSITION | PR_TURRET | PR_VEH_VIS | PR_ONE_TIME | PR_DEAD | PR_KILLED | PR_RIDING | PR_RELPOS | PR_FROM_CONTROLLER | PRX_REALLY_ACTIVE | PRX_FIRST_TIME | PR_FIRE_FLAGS )

#define PR_MASK ( PR_RELPOS | PR_RIDING | PR_KILLED | PR_DEAD | PR_VEH_VIS | PR_TURRET | PR_POSITION | PR_STATE1 | PR_STATE0 | PR_FIRE_FLAGS | PR_FROM_CONTROLLER )
