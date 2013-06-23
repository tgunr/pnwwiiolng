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

#include <stddef.h>
#include <cstdint>
#include <thread>
#include <chrono>

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

#elif platOS == OS_MACOS

	#include <sys/types.h>
	#include <malloc/malloc.h>

	// Logical types
	typedef		unsigned int		LOGICAL;		//!< TRUE or FALSE.

	// character types
	typedef		unsigned char		UCHAR;			//!< 8 bit unsigned value.
	typedef		char				SCHAR;			//!< 8 bit signed value.
	typedef		wchar_t				WCHAR;			//!< UTF character.
	typedef		wchar_t				TCHAR;			//!< Conditional UTF character.
	typedef		char				SINT8;			//!< 8 bit signed value.
	typedef		unsigned char		UINT8;			//!< 8 bit unsigned value.

	// Signed types
	typedef		short				SINT16;			//!< 16 bit signed value.
	typedef		signed int	 		SINT32;			//!< 32 bit signed value.
	typedef		long long			SINT64;			//!< 64 bit signed value.

	// Unsigned types
	typedef		unsigned short		UINT16;			//!< 16 bit unsigned value.
	typedef		unsigned int		UINT32;			//!< 32 bit unsigned value.
	typedef		unsigned long long	UINT64;			//!< 64 bit unsigned value.

	// Float types; single (32 bits) and double (64 bits)
	typedef		float				FLOAT32;		//!< 32 bit single-precision float.
	typedef		double				FLOAT64;		//!< 64 bit double-precision float.

	typedef		void*				LPVOID;

	#define APIENTRY
	#define PN_MAX_PATH		260

	typedef LOGICAL (*SYS_DDINITFUNC)(void);
	typedef LOGICAL (*SYS_D3DINITFUNC)(void);
	typedef LOGICAL (*SYS_DDRENDERFUNC)(void);
	typedef LOGICAL (*SYS_D3DRENDERFUNC)(void);
	typedef void (*SYS_DDKILLFUNC)(void);
	typedef void (*SYS_D3DKILLFUNC)(void);

#elif platOS == OS_LINUX
//tolua_begin

	// Logical types
	typedef unsigned int			LOGICAL;		//!< TRUE or FALSE.

	// character types
	typedef	unsigned char			UCHAR;			//!< 8 bit unsigned value.
	typedef	char					SCHAR;			//!< 8 bit signed value.

	// Signed types
	typedef char					SINT8;			//!< 8 bit signed value.
	typedef short					SINT16;			//!< 16 bit signed value.
	typedef int						SINT32;			//!< 32 bit signed value.
	typedef long long				SINT64;			//!< 64 bit signed value.

	// Unsigned types
	typedef unsigned char			UINT8;			//!< 8 bit unsigned value.
	typedef unsigned short			UINT16;			//!< 16 bit unsigned value.
	typedef unsigned int			UINT32;			//!< 32 bit unsigned value.
	typedef unsigned long long		UINT64;			//!< 64 bit unsigned value.

	// Float types; single (32 bits) and double (64 bits)
	typedef float					FLOAT32;		//!< 32 bit single-precision float.
	typedef double					FLOAT64;		//!< 64 bit double-precision float.

//tolua_end
#else
#error "platform not defined for systypes.h!"
#endif // OS_MACOS

struct FLOAT64VECTOR
{
	FLOAT64 x;
	FLOAT64 y;
	FLOAT64 z;
} ;

struct FLOAT32VECTOR
{
	FLOAT32 x, y, z;
};

struct FLOAT64ATT
{
	FLOAT64 Roll;
	FLOAT64 Pitch;
	FLOAT64 Yaw;
} ;

struct FLOAT64QUAT
{
	FLOAT64 x,y,z,w;
};

typedef UINT8 RANK_NO ;

typedef SINT32 teulindex_t;

struct ACTION_OBJECT ;
namespace Teul { struct PacketHeader ; class Packet ; }

//! Compressed player gamename/callsign.
typedef UINT64 ChatHandle ;
//! Player permission mask.
typedef UINT64 PermissionMask ;
//! Teul type for millisecond time.
typedef	UINT64 TeulTimeMS ;
//! Teul type for a millisecond delta (signed).
typedef SINT64 TeulDeltaMS	;

//! Index for a player ID
typedef unsigned int playerid_t ;
//! Index for a sortie ID
typedef unsigned int sortieid_t ;
//! Index for an instance's unique ID
typedef unsigned long long instanceid_t ;
