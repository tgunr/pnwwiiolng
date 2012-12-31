#if !defined(GRANNY_TYPES_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_types.h $
// $DateTime: 2010/03/26 14:38:57 $
// $Change: 25899 $
// $Revision: #28 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

#if !defined(GRANNY_COMPILER_H)
#include "granny_compiler.h"
#endif

#if !defined(GRANNY_PROCESSOR_H)
#include "granny_processor.h"
#endif

#if !defined(GRANNY_TELEMETRY_H)
#include "granny_telemetry.h"
#endif

BEGIN_GRANNY_NAMESPACE;

//
// Unsigned integers
//
#if COMPILER_MSVC
// TODO: How do I get an actual unsigned 64 bit integer in MSVC?
typedef unsigned _int64 uint64;
uint64 const UInt64Maximum = 0xffffffffffffffffui64;

#elif PLATFORM_PS2

#if COMPILER_GCC
typedef unsigned long uint64 __attribute__((mode(DI)));
uint64 const UInt64Maximum = 0xffffffffffffffffLL;
#else
// ...there is some doubt...
typedef unsigned long uint64;
uint64 const UInt64Maximum = 0xffffffffffffffffL;
#endif

#elif PLATFORM_PSP

#if COMPILER_GCC
typedef unsigned long uint64 __attribute__((mode(DI)));
uint64 const UInt64Maximum = 0xffffffffffffffffLL;
#else
    // ...there is some doubt...
typedef unsigned long long uint64;
uint64 const UInt64Maximum = 0xffffffffffffffffL;
#endif


#elif PLATFORM_PS3

typedef unsigned long long uint64;
uint64 const UInt64Maximum = 0xffffffffffffffffLL;

#elif COMPILER_GCC || COMPILER_METROWERKS
typedef long long unsigned int uint64;
uint64 const UInt64Maximum = 0xffffffffffffffffLL;
#endif

typedef unsigned int uint32;
uint32 const UInt32Minimum = 0;
uint32 const UInt32Maximum = 0xffffffff;

typedef unsigned short uint16;
uint16 const UInt16Minimum = 0;
uint16 const UInt16Maximum = 0xffff;

typedef unsigned char uint8;
uint8 const UInt8Minimum = 0;
uint8 const UInt8Maximum = 0xff;

//
// Signed integers
//
#if COMPILER_MSVC
typedef _int64 int64;
int64 const Int64Minimum = (-9223372036854775807i64 - 1i64);
int64 const Int64Maximum = 9223372036854775807i64;

#elif PLATFORM_PS2

#if COMPILER_GCC
typedef signed long int64 __attribute__((mode(DI)));
int64 const Int64Minimum = (-9223372036854775807LL - 1LL);
int64 const Int64Maximum = 9223372036854775807LL;
#else
// ...there is some doubt...
typedef signed long int64;
int64 const Int64Minimum = (-9223372036854775807L - 1L);
int64 const Int64Maximum = 9223372036854775807L;
#endif

#elif PLATFORM_PSP

#if COMPILER_GCC || COMPILER_SNC
typedef signed long int64 __attribute__((mode(DI)));
int64 const Int64Minimum = (-9223372036854775807LL - 1LL);
int64 const Int64Maximum = 9223372036854775807LL;
#else
// ...there is some doubt...
typedef signed long int64;
int64 const Int64Minimum = (-9223372036854775807L - 1L);
int64 const Int64Maximum = 9223372036854775807L;
#endif

#elif COMPILER_GCC || COMPILER_METROWERKS || COMPILER_SNC
typedef long long int int64;
int64 const Int64Minimum = (-9223372036854775807LL - 1LL);
int64 const Int64Maximum = 9223372036854775807LL;
#endif

typedef int int32;
int32 const Int32Minimum = (-2147483647 - 1);
int32 const Int32Maximum = 2147483647;

typedef short int16;
int16 const Int16Minimum = (-32768);
int16 const Int16Maximum = 32767;

typedef char int8;
int8 const Int8Minimum = (-128);
int8 const Int8Maximum = 127;

//
// Floating point values
//

typedef float  real32;
typedef uint16 real16;

// note that we don't support writing real64, so we only have an "x" type for it.

typedef double real64x;
typedef real32 real32x;

//
// Floating point constants
//
real32 const Real32Minimum = -3.402823466e+38F;
real32 const Real32Maximum = 3.402823466e+38F;
real32 const Real32Epsilon = 1.192092896e-07F;
real64x const Real64Minimum = -1.7976931348623158e+308;
real64x const Real64Maximum = 1.7976931348623158e+308;
real64x const Real64Epsilon = 2.2204460492503131e-016;


//
// "At least this many bits" definitions
// In theory you could change the size of int32x to be an int64 on some
// machines if they're faster. However, all the 64-bit architectures
// we support don't care (we don't do an Alpha version :-) and it
// just wastes memory.
//
// You cannot store these in files, because they are allowed to change
// between platforms.
//
typedef uint64 uint64x;
typedef uint32 uint32x;
typedef uint32 uint16x;
typedef uint32 uint8x;

typedef int64 int64x;
typedef int32 int32x;
typedef int32 int16x;
typedef int32 int8x;


// An integer big enough to store a pointer or a memory offset You
// can't store these in files either, hence the "x" on the end.  Note
// that we do a dance here to get the maximum warning level in VC.NET.
// __w64 will spew tons of useful warnings when /Wp64 is turned on.
#if (COMPILER_MSVC && _MSC_VER >= 1300 && _Wp64)

  // Compiles to the correct size on both platforms
  #if PROCESSOR_32BIT_POINTER
    typedef __w64 long intaddrx;
    typedef __w64 unsigned long uintaddrx;
  #else // PROCESSOR_64BIT_POINTER
    typedef __w64 __int64 intaddrx;
    typedef __w64 unsigned __int64 uintaddrx;
  #endif

#else // non-vc.net compiler or /Wp64 turned off

  #if PROCESSOR_32BIT_POINTER
    typedef int32 intaddrx;
    typedef uint32 uintaddrx;
  #else // PROCESSOR_64BIT_POINTER
    typedef int64x intaddrx;
    typedef uint64x uintaddrx;
  #endif

#endif // compiler


//
// Boolean values
//
typedef int32 bool32;

//
// Arrays
//
typedef real32 pair[2];

// A triple is generally used to represent a column point or vector
typedef real32 triple[3];

// A quad is generally used to represent a quaternion column vector
typedef real32 quad[4];

// A matrix_3x3 is generally used to represent a linear transform
typedef real32 matrix_3x3[3][3];

// The matrix 4/3 variants are generally used to represent homogeneous
// transforms of various types.
typedef real32 matrix_4x4[4][4];
typedef real32 matrix_3x4[3][4];

#ifndef NULL
#define NULL 0
#endif


END_GRANNY_NAMESPACE;

// We need this macro to be a compile time constant so we can use it in compile asserts.
// In gcc 4.0+, it apparently isn't recognized as such with the normal macro, so use the
// version in stddef.h
#if !(COMPILER_GCC && (__GNUC__ >= 4))
    #define OffsetFromType(structure, Member) ((intaddrx)&((structure *)0)->Member)
#else
    #include <stddef.h>
    #define OffsetFromType(structure, Member) (offsetof(structure, Member))
#endif

// SizeOf is provided as preferred macros to the standard sizeof and offsetof in ANSI C,
// since it's handy to have this return an int32x, rather than a size_t
#define SizeOf(something)               ((intaddrx)sizeof(something))
#define SizeOfMember(structure, member) SizeOf(((structure *)0)->member)
#define OffsetFromPtr(Ptr, Member)      ((intaddrx)((uint8 *)(&(Ptr)->Member) - (uint8 *)(Ptr)))
#define ArrayLength(Array)              (SizeOf(Array)/SizeOf((Array)[0]))

#include "header_postfix.h"
#define GRANNY_TYPES_H
#endif
