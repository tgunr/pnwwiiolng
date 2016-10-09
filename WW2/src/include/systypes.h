#pragma once
//! @file include/systypes.h
//! This module provides all of the common system types
//! for all playnet projects.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

// True and False
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined(WIN32) || platOS == OS_WIN32

	// Logical types
	typedef		unsigned int		LOGICAL;		//!< TRUE or FALSE.

	// character types
	typedef		unsigned char		UCHAR;			//!< 8 bit unsigned value.
	typedef		char				SCHAR;			//!< 8 bit signed value.

	// Signed types
	typedef		char				SINT8;			//!< 8 bit signed value.
	typedef		signed __int16		SINT16;			//!< 16 bit signed value.
	typedef		signed __int32		SINT32;			//!< 32 bit signed value.
	typedef		signed __int64		SINT64;			//!< 64 bit signed value.

	// Unsigned types
	typedef		unsigned char		UINT8;			//!< 8 bit unsigned value.
	typedef		unsigned __int16	UINT16;			//!< 16 bit unsigned value.
	typedef		unsigned __int32	UINT32;			//!< 32 bit unsigned value.
	typedef		unsigned __int64	UINT64;			//!< 64 bit unsigned value.

	// Float types; single (32 bits) and double (64 bits)
	typedef		float				FLOAT32;		//!< 32 bit single-precision float.
	typedef		double				FLOAT64;		//!< 64 bit double-precision float.

	#include <tchar.h>

#elif platOS == OS_MACOS || platOS == OS_LINUX

	#include <sys/types.h>

    #if platOS == OS_MACOS
	#include <malloc/malloc.h>
    #elif platOS == OS_LINUX
    #include <malloc.h>
    #endif

//tolua_begin
	// Logical types
    typedef     uint32_t            LOGICAL;		//!< TRUE or FALSE.

	// character types
	typedef		unsigned char       UCHAR;			//!< 8 bit unsigned value.
	typedef		char				SCHAR;			//!< 8 bit signed value.
	typedef		wchar_t				WCHAR;			//!< UTF character.
	typedef		wchar_t				TCHAR;			//!< Conditional UTF character.
	typedef		char				SINT8;			//!< 8 bit signed value.
	typedef		unsigned char       UINT8;			//!< 8 bit unsigned value.

	// Signed types
	typedef		int16_t				SINT16;			//!< 16 bit signed value.
	typedef		int32_t             SINT32;			//!< 32 bit signed value.
	typedef		int64_t             SINT64;			//!< 64 bit signed value.

	// Unsigned types
	typedef		uint16_t            UINT16;			//!< 16 bit unsigned value.
	typedef		uint32_t            UINT32;			//!< 32 bit unsigned value.
	typedef		uint64_t            UINT64;			//!< 64 bit unsigned value.

	// Float types; single (32 bits) and double (64 bits)
	typedef		float				FLOAT32;		//!< 32 bit single-precision float.
	typedef		double				FLOAT64;		//!< 64 bit double-precision float.

	typedef		void*				LPVOID;
//tolua_end

	#define APIENTRY

	using SYS_DDINITFUNC = LOGICAL(*)(void);
	using SYS_D3DINITFUNC = LOGICAL(*)(void);
	using SYS_DDRENDERFUNC = LOGICAL(*)(void);
	using SYS_D3DRENDERFUNC = LOGICAL(*)(void);
	using SYS_DDKILLFUNC = void(*)(void);
	using SYS_D3DKILLFUNC = void(*)(void);

#else
#error "platform not defined for systypes.h!"
#endif // OS_MACOS

struct FLOAT64VECTOR
{
	FLOAT64 x;
	FLOAT64 y;
	FLOAT64 z;
};

struct FLOAT32VECTOR
{
	FLOAT32 x, y, z;
};

struct FLOAT64ATT
{
	FLOAT64 Roll;
	FLOAT64 Pitch;
	FLOAT64 Yaw;
};

struct FLOAT64QUAT
{
	FLOAT64 x,y,z,w;
};

using RANK_NO = uint8_t;

using teulindex_t = int32_t;

struct ACTION_OBJECT;
namespace Teul { struct PacketHeader; class Packet; }

//! Compressed player gamename/callsign.
using ChatHandle = UINT64;
//! Player permission mask.
using PermissionMask = UINT64;
//! Teul type for millisecond time.
using TeulTimeMS = UINT64;
//! Teul type for a millisecond delta (signed).
using TeulDeltaMS = SINT64;

//! Index for a player ID
using playerid_t = uint32_t;
//! Index for a sortie ID
using sortieid_t = uint32_t;
//! Index for an instance's unique ID
using instanceid_t = UINT64;
//! Index for a persona
using personaid_t = uint32_t;
