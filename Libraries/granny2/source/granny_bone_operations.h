#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bone_operations.h $
// $DateTime: 2008/10/29 17:33:08 $
// $Change: 19269 $
// $Revision: #10 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_OPTIMIZED_DISPATCH_H)
#include "granny_optimized_dispatch.h"
#endif

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct transform;

typedef void OPTIMIZED_DISPATCH_TYPE(BuildIdentityWorldPoseOnly)(real32 const *ParentMatrix,
                                                                 real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildIdentityWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionWorldPoseOnly)(real32 const *Position,
                                                                 real32 const *ParentMatrix,
                                                                 real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionOrientationWorldPoseOnly)(real32 const *Position,
                                                                            real32 const *Orientation,
                                                                            real32 const *ParentMatrix,
                                                                            real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildFullWorldPoseOnly)(transform const &Transform,
                                                             real32 const *ParentMatrix,
                                                             real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildFullWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildSingleCompositeFromWorldPose)(real32 const *InverseWorld4x4,
                                                                        real32 const *WorldMatrix,
                                                                        real32 *ResultComposite );
extern OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPose);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildSingleCompositeFromWorldPoseTranspose)(real32 const *InverseWorld4x4,
                                                                                 real32 const *WorldMatrix,
                                                                                 real32 *ResultComposite);
extern OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPoseTranspose);


DECL_GENERIC_DISPATCH(BuildIdentityWorldPoseOnly)(real32 const *ParentMatrix,
                                                  real32 *ResultWorldMatrix);
DECL_GENERIC_DISPATCH(BuildPositionWorldPoseOnly)(real32 const *Position,
                                                  real32 const *ParentMatrix,
                                                  real32 *ResultWorldMatrix);
DECL_GENERIC_DISPATCH(BuildPositionOrientationWorldPoseOnly)(real32 const *Position,
                                                             real32 const *Orientation,
                                                             real32 const *ParentMatrix,
                                                             real32 * ResultWorldMatrix);
DECL_GENERIC_DISPATCH(BuildFullWorldPoseOnly)(transform const &Transform,
                                              real32 const *ParentMatrix,
                                              real32 * ResultWorldMatrix);

DECL_GENERIC_DISPATCH(BuildSingleCompositeFromWorldPose)(real32 const *InverseWorld4x4,
                                                         real32 const *WorldMatrix,
                                                         real32 *ResultComposite);
DECL_GENERIC_DISPATCH(BuildSingleCompositeFromWorldPoseTranspose)(real32 const *InverseWorld4x4,
                                                                  real32 const *WorldMatrix,
                                                                  real32 *ResultComposite3x4);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BONE_OPERATIONS_H
#endif
