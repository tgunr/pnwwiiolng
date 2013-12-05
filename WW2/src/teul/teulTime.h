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

extern TeulTimeMS teulClock64;						//!< Miliseconds since epoch as of last teulSyncClock().
extern UINT32 teulClockTime;						//!< Seconds since epoch as of last teulSyncClock().

//tolua_end

static const UINT32	SECOND_MS	=	1000;			//!< Number of seconds in a milliseconds.
static const UINT32 MINUTE_S	=	60;				//!< Number of seconds in a minute.
static const UINT32	HOUR_M		=	60;				//!< Number of minutes in an hour.
static const UINT32 HOUR_S		=	60 * 60;		//!< Number of seconds in an hour.
static const UINT32	DAY_S		=	24 * 60 * 60;	//!< Number of seconds in a day.

static inline UINT32 teulTime() { return teulClockTime; }	//!< Seconds since epoch as of last teulSyncClock().

extern void teulSyncClock(void);					//!< Updates the teulClock64 and teulClockTime values.

//! \deprecated
static inline UINT32 teulClock() { return (teulClock64 & 0x000fffff); } //!< Returns millisecond time in 20 bits (limited accuracy)
