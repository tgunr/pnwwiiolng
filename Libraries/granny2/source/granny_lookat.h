#if !defined(GRANNY_LOOKAT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_lookat.h $
// $DateTime: 2010/02/25 15:00:51 $
// $Change: 25418 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(IKGroup);

struct local_pose;
struct skeleton;
struct world_pose;

EXPAPI GS_PARAM bool IKAimAt(int32x const    HeadBoneIndex,
                             int32x const    LinkCount,
                             int32x const    InactiveLinkCount,
                             real32 const*   OSOffset3,
                             real32 const*   OSAimVec3,
                             real32 const*   OSLevelVec3,
                             real32 const*   WSGroundNormal,
                             real32 const*   TargetPosition,
                             skeleton const& Skeleton,
                             local_pose&     LocalPose,
                             real32*         Offset4x4,
                             world_pose&     WorldPose);


EXPAPI GS_PARAM bool IKOrientTowards(int32x          BoneIndex,
                                     real32 const*   OrientAxis,
                                     real32 const*   TargetPosition,
                                     skeleton const& Skeleton,
                                     local_pose&     LocalPose,
                                     real32*         Offset4x4,
                                     world_pose&     WorldPose);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_LOOKAT_H
#endif /* GRANNY_LOOKAT_H */
