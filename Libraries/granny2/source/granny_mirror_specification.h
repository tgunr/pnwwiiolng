#if !defined(GRANNY_MIRROR_SPECIFICATION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_mirror_specification.h $
// $DateTime: 2009/02/17 14:04:49 $
// $Change: 20696 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#ifndef GRANNY_TYPES_H
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ControlledAnimationGroup);

struct skeleton;
struct transform;

// Return true for success, even if the name is simply copied from base to mirrored
EXPAPI typedef bool mirror_name_callback(char const* BaseName,
                                         char*       MirroredNameBuffer,
                                         int32x      BufferSize,
                                         void*       UserData);

EXPTYPE_EPHEMERAL enum mirror_axis
{
    MirrorXAxis   = 0,
    MirrorYAxis   = 1,
    MirrorZAxis   = 2,

    // Special for the post flip axis, not valid for ComputedMirroredPose, or the
    // FlipAround argument of BuildMirroringBuffer
    MirrorAllAxes = 3
};

EXPTYPE_EPHEMERAL struct mirror_specification
{
    mirror_axis MirrorAxis;

    int32   RemapCount;
    uint16* Remaps;

    uint8*  PostFlips;  // This variable is uint8(mirror_axis)
};

EXPAPI GS_SAFE mirror_specification* NewMirrorSpecification(int32x BoneCount, mirror_axis MirrorAround);
EXPAPI GS_PARAM void FreeMirrorSpecification(mirror_specification* Specification);

EXPAPI GS_PARAM bool BuildMirroringIndices(mirror_specification* Specification,
                                           skeleton* Skeleton,
                                           mirror_name_callback* NameCallback,
                                           void *UserData);

void MirrorPoseTransforms(mirror_specification* Specification,
                          int32x TransformStride, transform* Transforms,
                          int32x SourceParentStride, int32 const *SourceParents,
                          int32x TransformCount);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MIRROR_SPECIFICATION_H
#endif /* GRANNY_MIRROR_SPECIFICATION_H */
