#pragma once

/// @file	updt/updtTurrets.h
/// @author	Oliver 'kfs1' Smith
///	@brief	Provides definitions for updating turrets.

// Define HIRES_TURRETS for 16-bit turrets.
#define ULTRA_HIRES_TURRETS
#define HIRES_TURRETS

#if defined(ULTRA_HIRES_TURRETS)
	typedef float turret_t;
# if defined(TURRET_GRANULARITY)
#  undef TURRET_GRANULARITY
# endif
#elif defined(HIRES_TURRETS)
	typedef UINT16 turret_t;
	#define TURRET_GRANULARITY 64800.0f		// 360 * 90 * 2 (i.e. 180th degree precision)
#else
	typedef UCHAR turret_t;
	#define TURRET_GRANULARITY 255.0f
#endif
