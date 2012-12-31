// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_bone_operations.cpp $
// $DateTime: 2008/10/29 17:33:08 $
// $Change: 19269 $
// $Revision: #8 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_bone_operations.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

BEGIN_GRANNY_NAMESPACE;

OPTIMIZED_DISPATCH(BuildIdentityWorldPoseOnly) = BuildIdentityWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildPositionWorldPoseOnly) = BuildPositionWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseOnly) = BuildPositionOrientationWorldPoseOnly_Generic;
OPTIMIZED_DISPATCH(BuildFullWorldPoseOnly) = BuildFullWorldPoseOnly_Generic;

OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPose) = BuildSingleCompositeFromWorldPose_Generic;
OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPoseTranspose) = BuildSingleCompositeFromWorldPoseTranspose_Generic;

END_GRANNY_NAMESPACE;
