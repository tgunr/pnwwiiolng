#if !defined(GRANNY_FLOATS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_floats.h $
// $DateTime: 2009/03/02 10:52:26 $
// $Change: 20879 $
// $Revision: #13 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

// Getting exceptional floating point values
union float_conversion_32
{
    uint32 Integer;
    real32 Float;
};

union float_conversion_64
{
    uint64  Integer;
    real64x Float;
};

union float_conversion_64_from_32
{
    uint32  Integer[2];
    real64x Float;
};


#define RET_REAL32_FROM_INT32(x)                \
    float_conversion_32 cu;                     \
    cu.Integer = (x);                           \
    return cu.Float

#define RET_REAL64_FROM_INT64(x)                \
    float_conversion_64 cu;                     \
    cu.Integer = (x);                           \
    return cu.Float

#if PROCESSOR_LITTLE_ENDIAN
#define RET_REAL64_FROM_INTS(x0, x1)            \
    float_conversion_64_from_32 cu;             \
    cu.Integer[0] = (x1);                       \
    cu.Integer[1] = (x0);                       \
    return cu.Float
#else
#define RET_REAL64_FROM_INTS(x0, x1)            \
    float_conversion_64_from_32 cu;             \
    cu.Integer[0] = (x0);                       \
    cu.Integer[1] = (x1);                       \
    return cu.Float
#endif

inline real32  GetReal32FromInt32(uint32 Integer) { RET_REAL32_FROM_INT32(Integer); }
inline real64x GetReal64FromInt64(uint64 Integer) { RET_REAL64_FROM_INT64(Integer); }

inline uint32 GetInt32FromReal32(real32 Float)
{
    float_conversion_32 cu;
    cu.Float = Float;
    return cu.Integer;
}

inline uint64 GetInt64FromReal64(real64x Float)
{
    float_conversion_64 cu;
    cu.Float = Float;
    return cu.Integer;
}


// Exceptional values
inline real32 GetReal32Infinity()       { RET_REAL32_FROM_INT32(0x7f800000); }
inline real32 GetReal32QuietNaN()       { RET_REAL32_FROM_INT32(0x7fc00000); }
inline real32 GetReal32SignalingNaN()   { RET_REAL32_FROM_INT32(0xff800000); }
inline real32 GetReal32Max()            { RET_REAL32_FROM_INT32(0x7f7fffff); }
inline real32 GetReal32VeryLarge()      { RET_REAL32_FROM_INT32(0x7f0fffff); }

inline real64x GetReal64Infinity()     { RET_REAL64_FROM_INTS(0x7ff00000, 0x00000000); }
inline real64x GetReal64QuietNaN()     { RET_REAL64_FROM_INTS(0xfff80000, 0x00000000); }
inline real64x GetReal64SignalingNaN() { RET_REAL64_FROM_INTS(0xfff00000, 0x00000000); }
inline real64x GetReal64Max()          { RET_REAL64_FROM_INTS(0x7fEfffff, 0xffffffff); }
inline real64x GetReal64VeryLarge()    { RET_REAL64_FROM_INTS(0x7f0fffff, 0xffffffff); }

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FLOATS_H
#endif
