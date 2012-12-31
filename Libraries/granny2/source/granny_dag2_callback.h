#if !defined(GRANNY_DAG2_CALLBACK_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_callback.h $
// $DateTime: 2009/05/06 14:42:03 $
// $Change: 21599 $
// $Revision: #3 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(DagGroup);

struct dag2;
struct dag2_instance;
struct dag2_node;
struct dag_pose_cache;
struct data_type_definition;
struct local_pose;
struct memory_arena;
struct model;

EXPTYPE_EPHEMERAL struct dag2_nodedata_callback
{
    int32 TypeMarker;
};
EXPCONST EXPGROUP(dag2_nodedata_callback) extern data_type_definition Dag2NodeDataCallbackType[];


EXPAPI typedef local_pose* sample_node_pose_callback(dag2_node* Node,
                                                     dag2_instance* Instance,
                                                     int32x OnOutput,
                                                     real32 AtTime,
                                                     dag_pose_cache* PoseCache,
                                                     void* UserData);
EXPAPI typedef real32 sample_node_scalar_callback(dag2_node* Node,
                                                  dag2_instance* Instance,
                                                  int32x OnOutput,
                                                  real32 AtTime,
                                                  void* UserData);
EXPAPI typedef bool get_root_motion_callback(dag2_node* Node,
                                             dag2_instance* Instance,
                                             int32x OnOutput,
                                             real32 SecondsElapsed,
                                             real32 AtTime,
                                             real32* ResultTranslation,
                                             real32* ResultRotation,
                                             void*   UserData);



EXPTYPE_EPHEMERAL EXPGROUP(dag2_nodedata_callback) struct dag2_callbacks
{
    sample_node_pose_callback*   SamplePoseCallback;
    sample_node_scalar_callback* SampleScalarCallback;
    get_root_motion_callback*    GetRootMotionCallback;
};


dag2_node* NewCallback(dag2& TheDag, memory_arena& Arena);

EXPAPI bool InstallNodeCallback(dag2_instance&  Instance,
                                dag2_node&      Node,
                                dag2_callbacks* Callbacks,
                                void*           UserData);
local_pose*
SampleDAG_Callback(dag2_node& Node,
                   dag2_nodedata_callback& NodeData,
                   dag2_instance& Instance,
                   int32x OnOutput,
                   real32 AtTime,
                   dag_pose_cache& PoseCache);
real32
SampleDAGScalar_Callback(dag2_node& Node,
                         dag2_nodedata_callback& NodeData,
                         dag2_instance& Instance,
                         int32x OnOutput,
                         real32 AtTime);
dag2_node*
MostInfluentialNode_Callback(dag2_node& Node,
                             dag2_nodedata_callback& NodeData,
                             dag2_instance& Instance,
                             int32x OnOutput,
                             real32 AtTime,
                             int32x* InfluentialOutput);
bool
GetRootMotion_Callback(dag2_node& Node,
                       dag2_nodedata_callback& NodeData,
                       dag2_instance& Instance,
                       int32x OnOutput,
                       real32 SecondsElapsed,
                       real32 AtTime,
                       triple& ResultTranslation,
                       triple& ResultRotation);

int32 GetInstanceDataSize_Callback(dag2_node& Node, dag2_nodedata_callback&,
                                   dag2& TheDag, model& FromModel, model& ToModel);
bool BindDAGInstanceData_Callback(dag2_node&              Node,
                                  dag2_nodedata_callback& NodeData,
                                  dag2_instance&          Instance);
bool UnbindDAGInstanceData_Callback(dag2_node&              Node,
                                    dag2_nodedata_callback& NodeData,
                                    dag2_instance&          Instance);
bool BakeOutDAGInstanceData_Callback(dag2_node&              Node,
                                     dag2_nodedata_callback& NodeData,
                                     dag2_instance&          Instance,
                                     memory_arena&           Arena);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DAG2_CALLBACK_H
#endif /* GRANNY_DAG2_CALLBACK_H */
