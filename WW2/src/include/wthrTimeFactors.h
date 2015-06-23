#pragma once

//! @file include/wthrTimeFactors.h
//! @author Oliver 'kfs1' Smith
//! @brief Describes the game's time acceleration factors.

//! Number of acceleration factors.
static const size_t NumTimeAccelerations = 2;

//! Assigns enumeration names to each factor.
enum { SlowTimeAccelerationIndex = 0, FastTimeAccelerationIndex = 1 };

//! Describes acceleration rate (in seconds) for each factor.
//! i.e. SlowTime is 3 game-seconds per real-second,
//! and FastTime is 20 game-seconds per real-second.
static const UINT32 TimeAccelerations[NumTimeAccelerations] = { 3, 20 };
