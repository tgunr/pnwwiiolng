#if !defined(GRANNY_DAG2_NWAY_BLEND_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_nway_blend.h $
// $DateTime: 2010/03/19 17:27:17 $
// $Change: 25804 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#ifndef GRANNY_TYPES_H
#include "granny_types.h"
#endif


BEGIN_GRANNY_NAMESPACE;

struct dag2;
struct dag2_node;
struct dag2_instance;
struct dag_pose_cache;
struct data_type_definition;
struct local_pose;
struct memory_arena;

struct dag2_nodedata_nwayblend
{
    int32 TypeMarker;
};
EXPCONST EXPGROUP(dag2_nodedata_blend) extern data_type_definition Dag2NodeDataNWayBlendType[];

dag2_node* NewNWayBlendNode(dag2& TheDag, memory_arena& Arena);

// Fast, runtime api
local_pose*
SampleDAG_NWayBlend(dag2_node& Node,
                        dag2_nodedata_nwayblend& NodeData,
                        dag2_instance& Instance,
                        int32x OnOutput,
                        real32 AtTime,
                        dag_pose_cache& PoseCache);

bool GetRootMotion_NWayBlend(dag2_node& Node,
                            dag2_nodedata_nwayblend& NodeData,
                            dag2_instance& Instance,
                            int32x OnOutput,
                            real32 SecondsElapsed,
                            real32 AtTime,
                            triple& ResultTranslation,
                            triple& ResultRotation);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DAG2_NWAY_BLEND_H
#endif /* GRANNY_DAG2_NWAY_BLEND_H */
