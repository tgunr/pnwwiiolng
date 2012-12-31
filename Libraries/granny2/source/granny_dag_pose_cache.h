#if !defined(GRANNY_DAG_POSE_CACHE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_dag_pose_cache.h $
// $DateTime: 2009/09/10 17:19:40 $
// $Change: 23190 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(DagGroup);

EXPTYPE struct dag_pose_cache;
struct local_pose;
struct model;

// Controlling the (global!) local_pose cache.
EXPAPI GS_MODIFY void FreeBlendDagLocalPoseCache();
EXPAPI dag_pose_cache* GetGlobalDagPoseCache();

// Local caches for worker threads
EXPAPI GS_SAFE dag_pose_cache *CreateDagPoseCache();

EXPAPI GS_PARAM void ClearDagPoseCache(dag_pose_cache &Cache);
EXPAPI GS_PARAM void FreeDagPoseCache(dag_pose_cache *Cache);
EXPAPI GS_PARAM void ReleaseCachePose(dag_pose_cache &Cache, local_pose* Pose);

EXPAPI GS_PARAM local_pose* GetNewBlendDagLocalPose(dag_pose_cache& PoseCache, uint32x NumBones);

local_pose* GetNewRestLocalPose(dag_pose_cache& PoseCache, model& ForModel);

bool PoseWillBeImmediatelyFreed(dag_pose_cache& PoseCache, local_pose* Pose);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DAG_POSE_CACHE_H
#endif
