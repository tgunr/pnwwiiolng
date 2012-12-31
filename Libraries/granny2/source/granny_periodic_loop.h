#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_periodic_loop.h $
// $DateTime: 2008/04/10 12:54:07 $
// $Change: 17377 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationGroup);

EXPTYPE struct periodic_loop
{
    real32 Radius;
    real32 dAngle;
    real32 dZ;

    triple BasisX;
    triple BasisY;
    triple Axis;
};
EXPCONST EXPGROUP(periodic_loop) extern data_type_definition PeriodicLoopType[];

void ZeroPeriodicLoop(periodic_loop &Loop);
void FitPeriodicLoop(real32 const *StartPosition3,
                     real32 const *StartOrientation4,
                     real32 const *EndPosition3,
                     real32 const *EndOrientation4,
                     real32 Seconds, periodic_loop &Loop);

void ComputePeriodicLoopVector(periodic_loop const &Loop,
                               real32 Seconds,
                               real32 *Result3);
void ComputePeriodicLoopLog(periodic_loop const &Loop,
                            real32 Seconds,
                            real32 *Result4);
void StepPeriodicLoop(periodic_loop const &Loop, real32 Seconds,
                      real32 *Position3,
                      real32 *Orientation4);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PERIODIC_LOOP_H
#endif
