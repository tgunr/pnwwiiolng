#pragma once

#include "veh/vehmain.h"		// For VEH_MAX_MOUNTS

#define fpFLAGS_NONE				0x00000001
#define fpFLAGS_NO_MOVEMENT			0x00000002
#define fpFLAGS_FORCE_EXTERNAL		0x00000004
#define fpFLAGS_FALLING				0x00000020

struct fpPersonData
{
	// Enhanced Collision Info
	SINT32			StartFallTime10ms;
	FLOAT64			StartFallHeight;
	SINT32			LastFallTime10ms;
	FLOAT64			heightAboveGround;

	// Weapon Elements
	LOGICAL			IsFiring;
	LOGICAL			Expendable[VEH_MAX_MOUNTS];

	// Dead stuff
	LOGICAL			Flags;

	// Animation Info to be transmitted
	UINT32			AnimationFlags;
} ;
