#pragma once

/// @file	updt/updtTurrets.h
/// @author	Oliver 'kfs1' Smith
///	@brief	Provides definitions for updating turrets.

// Define HIRES_TURRETS for 16-bit turrets.
//#define ULTRA_HIRES_TURRETS
#define HIRES_TURRETS

#if defined(ULTRA_HIRES_TURRETS)
	using turret_t = float;
# if defined(TURRET_GRANULARITY)
#  undef TURRET_GRANULARITY
# endif
#elif defined(HIRES_TURRETS)
	using turret_t = UINT16;
    using TURRET_TYPE = UINT16;
	#define TURRET_GRANULARITY 64800.0f		// 360 * 90 * 2 (i.e. 180th degree precision)
#else
	using turret_t = UCHAR;
    using TURRET_TYPE = UCHAR;
	#define TURRET_GRANULARITY 255.0f
#endif

