#pragma once
// Date: Fri Dec  2 18:31:16 CST 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin
enum
{
	CP_STAT_NONE				= 0,
	CP_STAT_OPEN 				= 0x00000001,
	CP_STAT_IN_SUPPLY			= 0x00000002,	// Has supply connection
	CP_STAT_HAS_NAVYBASE		= 0x00000004,	// Has a docks/harbor
	CP_STAT_SURRENDER_CANDIDATE	= 0x00000008,	// Currently a candidate for surrender.
	CP_STAT_HAS_AIRBASE			= 0x00000010,	// Has an airfield
	CP_STAT_CONTENTION 			= 0x00000020,	// Enemy holds facilities
	CP_STAT_ENEMY_ARMOR			= 0x00000040,	// }
	CP_STAT_ENEMY_INFANTRY		= 0x00000080,	// }
	CP_STAT_ENEMY_SEACRAFT		= 0x00000100,	// } EWS
	CP_STAT_ENEMY_AIRCRAFT		= 0x00000200,	// }
	CP_STAT_HAS_RARE_INF		= 0x00000400,	// Infantry deployments
	CP_STAT_HAS_RARE_GROUND		= 0x00000800,	// Armored vehicle deployments
	CP_STAT_HAS_RARE_NAVY		= 0x00001000,	// Has naval deployments
	CP_STAT_HAS_RARE_AIR		= 0x00002000,	// Has bomber deployments
	CP_STAT_ENEMY_LINKED		= 0x00004000,	// Frontline town
	CP_STAT_OWN_HAAC			= 0x00008000,	// Owner holding at all costs
	CP_STAT_FALLBACK			= 0x00010000,	// Fallback pending
	CP_STAT_OVERRUN				= 0x00020000,	// "Overrun" report
	CP_STAT_ALLIED_ATTACK_OBJ	= 0x00040000,	// Attack objective
	CP_STAT_ALLIED_DEFEND_OBJ	= 0x00080000,	// Defend objective
	CP_STAT_AXIS_ATTACK_OBJ 	= 0x00100000,	// Attack objective
	CP_STAT_AXIS_DEFEND_OBJ 	= 0x00200000,	// Defend objective
	CP_STAT_HEAVY_ENEMY_ARMOR 	= 0x00400000,
	CP_STAT_HEAVY_ENEMY_INF 	= 0x00800000,
	CP_STAT_HEAVY_ENEMY_SEA 	= 0x01000000,
	CP_STAT_HEAVY_ENEMY_AIR 	= 0x02000000,
	CP_STAT_MAKES_RDP			= 0x04000000,	// RDP Production Center

	CP_STAT_RESERVED			= 0x80000000	// Reserved for packers that use 31 bits.
};
//tolua_end
using CP_STATUS_FLAGS = UINT32;
//tolua_begin

// Mask for excluding EWS when promoting flags from facility to town
#define CP_NORM_EWS_FLAG_MASK (CP_STAT_ENEMY_ARMOR | CP_STAT_ENEMY_INFANTRY | CP_STAT_ENEMY_SEACRAFT | CP_STAT_ENEMY_AIRCRAFT)
#define CP_HEAVY_EWS_FLAG_MASK (CP_STAT_HEAVY_ENEMY_ARMOR | CP_STAT_HEAVY_ENEMY_INF | CP_STAT_HEAVY_ENEMY_SEA | CP_STAT_HEAVY_ENEMY_AIR)
#define CP_EWS_FLAG_MASK (CP_NORM_EWS_FLAG_MASK | CP_HEAVY_EWS_FLAG_MASK)
#define CP_ALLIED_OBJECTIVES (CP_STAT_ALLIED_ATTACK_OBJ | CP_STAT_ALLIED_DEFEND_OBJ)
#define CP_AXIS_OBJECTIVES (CP_STAT_AXIS_ATTACK_OBJ | CP_STAT_AXIS_DEFEND_OBJ)
#define CP_OBJECTIVES (CP_ALLIED_OBJECTIVES | CP_AXIS_OBJECTIVES)
#define BRIDGE_STAT_MASK (CP_EWS_FLAG_MASK | CP_STAT_CONTENTION | CP_OBJECTIVES)

//tolua_end
