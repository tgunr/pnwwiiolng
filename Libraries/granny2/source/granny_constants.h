#if !defined(GRANNY_CONSTANTS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_constants.h $
// $DateTime: 2009/08/19 14:49:04 $
// $Change: 22810 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MathGroup);

// Constants
ALIGN16(real32) const Pi32 = 3.14159265358979323846f;

ALIGN16(real32) const GlobalZeroVector[4] = {0, 0, 0, 0};
ALIGN16(real32) const GlobalXAxis[4] = {1, 0, 0, 0};
ALIGN16(real32) const GlobalYAxis[4] = {0, 1, 0, 0};
ALIGN16(real32) const GlobalZAxis[4] = {0, 0, 1, 0};
ALIGN16(real32) const GlobalWAxis[4] = {0, 0, 0, 1};
ALIGN16(real32) const GlobalNegativeXAxis[4] = {-1, 0, 0, 0};
ALIGN16(real32) const GlobalNegativeYAxis[4] = {0, -1, 0, 0};
ALIGN16(real32) const GlobalNegativeZAxis[4] = {0, 0, -1, 0};
ALIGN16(real32) const GlobalNegativeWAxis[4] = {0, 0, 0, -1};
ALIGN16(real32) const GlobalIdentity3x3[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
ALIGN16(real32) const GlobalIdentity4x4[4][4] = {{1, 0, 0, 0},
                                                 {0, 1, 0, 0},
                                                 {0, 0, 1, 0},
                                                 {0, 0, 0, 1}};
ALIGN16(real32) const GlobalIdentity6[6] = {1, 0, 0, 1, 0, 1};
ALIGN16(real32) const GlobalZero16[16] = {0};

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CONSTANTS_H
#endif
