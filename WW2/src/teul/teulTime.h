#pragma once
// Created: Mon Aug 29 16:22:15 CDT 2005
//
//! @file teul/teulTime.h
//! @brief Minimal include file for defining teulTime and teulClock64
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

extern TeulTimeMS teulClock64 ;						//!< Miliseconds since epoch as of last teulSyncClock().
extern UINT32 teulClockTime ;						//!< Seconds since epoch as of last teulSyncClock().

//tolua_end

static const UINT32	SECOND_MS	=	1000 ;			//!< Number of seconds in a milliseconds.
static const UINT32 MINUTE_S	=	60 ;			//!< Number of seconds in a minute.
static const UINT32	HOUR_M		=	60 ;			//!< Number of minutes in an hour.
static const UINT32 HOUR_S		=	60 * 60 ;		//!< Number of seconds in an hour.
static const UINT32	DAY_S		=	24 * 60 * 60 ;	//!< Number of seconds in a day.

static inline UINT32 teulTime() { return teulClockTime ; }	//!< Seconds since epoch as of last teulSyncClock().

extern SINT32 teulSyncClock(void);					//!< Updates the teulClock64 and teulClockTime values.

//! \deprecated
extern UINT32 teulClock(void);						//!< Returns milliseconds time in 20 bits (limited accuracy).

// Adds  an unsigned 20 bit time values with a signed 20bit delta .
// Returns an unsigned 20 bit timeval, - val indicates error.
// Truncates overflows (e.g. 0x0fffff + 1 = 0).
extern SINT32 teulAddClock( UINT32 time, SINT32 delta );

// Returns a signed 20 bit delta taking into account clock rollover.
// Assumes no real negative time diff larger than 1/2 of the max time of 0xfffff.
// Adjusts for clock overflow (e.g. 0 - 0x0fffff = 1 or 0x0fffff - 0 = -1)
extern SINT32 teulDeltaClock( UINT32 newTime, UINT32 oldTime );

// same as teulDeltaClock but newTime = teulClock()
extern SINT32 teulElapsedClock(UINT32 oldTime);

// teulClockBits functions are the same as above excepting that
// the number of bits are specified and may be between 2 and 31.
// allways assumes same number of bits for all parameters.

extern SINT32 teulClockBits(UINT32 bits);
extern SINT32 teulDeltaClockBits( UINT32 bits, UINT32 newTime, UINT32 oldTime );

