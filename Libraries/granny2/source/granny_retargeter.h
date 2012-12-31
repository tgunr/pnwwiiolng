#if !defined(GRANNY_RETARGETER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_retargeter.h $
// $DateTime: 2008/10/23 13:25:46 $
// $Change: 19193 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ModelGroup);

struct model;
struct local_pose;
struct skeleton;
struct transform;

struct retarget_identifier
{
    model *FromBasis;
    model *ToBasis;
};

EXPTYPE struct retargeter;
struct retargeter
{
    retarget_identifier ID;

    intaddrx   NumBones;
    intaddrx   ReferenceCount;

    int32*     SourceIndices;
    transform* RetargetingTransforms;

    retargeter* Right;
    retargeter* Left;
};

CompileAssert(IS_ALIGNED_16(SizeOf(retargeter)));

EXPAPI retargeter* AcquireRetargeter(model* From, model* To);
EXPAPI void ReleaseRetargeter(retargeter* Retargeter);

EXPAPI bool RetargetPose(local_pose const& FromPose,
                         local_pose& DestPose,
                         int32x DestBoneStart,
                         int32x DestBoneCount,
                         retargeter& Retargeter);

void RebasingTransform(transform& Dest,
                       transform const& Source,
                       transform const& Rebaser);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_RETARGETER_H
#endif
