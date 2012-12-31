// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_selection.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_dag2_selection.h"

#include "granny_aggr_alloc.h"
#include "granny_animation_binding.h"
#include "granny_assert.h"
#include "granny_controlled_animation.h"
#include "granny_dag2.h"
#include "granny_dag2_instance.h"
#include "granny_dag2_utilities.h"
#include "granny_dag_pose_cache.h"
#include "granny_file_info.h"
#include "granny_math.h"
#include "granny_memory.h"
#include "granny_memory_arena.h"
#include "granny_model.h"
#include "granny_parameter_checking.h"
#include "granny_skeleton.h"

// Should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode BlendDagLogMessage
USING_GRANNY_NAMESPACE;

IMPL_GET_NODEDATA(Selection, selection);

data_type_definition GRANNY
Dag2NodeDataSelectionType[] =
{
    { Int32Member, "TypeMarker_NodeDataSelection" },
    { Int32Member, "DefaultEdge" },

    { EndMember },
};

struct dag2_instancedata_sel
{
    int32 CurrentEdge;
};


dag2_node* GRANNY
NewSelectionNode(dag2& TheDag, memory_arena& Arena)
{
    GRANNY_ENTER();

    AGGR_NODE(TheDag,
              NewNode, 2, 2, 1, 1,
              NodeState, dag2_nodedata_selection, Dag2NodeDataSelectionType,
              Arena);
    if (NewNode)
    {
        NewNode->Name = 0;
        NodeState->DefaultEdge = 0;

        INIT_EXTERNAL_INPUT_EDGE(NewNode, 0, "Pose in", Pose);
        INIT_EXTERNAL_INPUT_EDGE(NewNode, 1, "Pose in", Pose);
        INIT_EXTERNAL_OUTPUT_EDGE(NewNode, 0, "Output", Pose);

        Assert(GetNodeDataSelection(NewNode));
    }

    return NewNode;
}

dag2_node* GRANNY
MostInfluentialNode_Selection(dag2_node& Node,
                              dag2_nodedata_selection& NodeData,
                              dag2_instance& Instance,
                              int32x OnOutput,
                              real32 AtTime,
                              int32x* InfluentialOutput)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return 0);
    CheckPointerNotNull(Instance.SourceModel, return 0);
    CheckCondition(OnOutput == 0, return 0);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    Assert(Data);

    int32x const Edge = ClampInt32(0, Data->CurrentEdge, Node.ExternalInputEdgeCount);
    dag2_input_edge& Input = Node.InputEdges[Edge];

    if (IsValidNodeIndex(Input.InputNodeIndex))
    {
        return FindMostInfluentialNodeInternal(Instance,
                                               Input.InputNodeIndex,
                                               Input.ConnectedOutput,
                                               AtTime, InfluentialOutput);
    }

    return 0;
}

local_pose* GRANNY
SampleDAG_Selection(dag2_node& Node,
                   dag2_nodedata_selection& NodeData,
                   dag2_instance& Instance,
                   int32x OnOutput,
                   real32 AtTime,
                   dag_pose_cache& PoseCache)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return 0);
    CheckPointerNotNull(Instance.SourceModel, return 0);
    CheckCondition(OnOutput == 0, return 0);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    Assert(Data);

    int32x const Edge = ClampInt32(0, Data->CurrentEdge, Node.ExternalInputEdgeCount);
    dag2_input_edge& Input = Node.InputEdges[Edge];

    if (IsValidNodeIndex(Input.InputNodeIndex))
    {
        return SampleDAGNode(Instance,
                             Input.InputNodeIndex, Input.ConnectedOutput,
                             AtTime, PoseCache);
    }
    else
    {
        return 0;
    }
}

bool GRANNY
GetRootMotion_Selection(dag2_node& Node,
                       dag2_nodedata_selection& NodeData,
                       dag2_instance& Instance,
                       int32x OnOutput,
                       real32 SecondsElapsed,
                       real32 AtTime,
                       triple& ResultTranslation,
                       triple& ResultRotation)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return 0);
    CheckPointerNotNull(Instance.SourceModel, return 0);
    CheckCondition(OnOutput == 0, return 0);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    Assert(Data);

    //int32x const Edge = ClampInt32(0, Data->CurrentEdge, Node.ExternalInputEdgeCount);
    int32x const Edge = ClampInt32(0, NodeData.DefaultEdge, Node.ExternalInputEdgeCount);
    dag2_input_edge& Input = Node.InputEdges[Edge];

    if (IsValidNodeIndex(Input.InputNodeIndex))
    {
        return GetDagNodeRootMotionVectors(Instance,
                                           Input.InputNodeIndex, Input.ConnectedOutput,
                                           SecondsElapsed, AtTime,
                                           ResultTranslation, ResultRotation);
    }
    else
    {
        // Unconnected.
        ResultTranslation[0] =
            ResultTranslation[1] =
            ResultTranslation[2] = 0.0f;
        ResultRotation[0] =
            ResultRotation[1] =
            ResultRotation[2] = 0.0f;

        return true;
    }
}


int32 GRANNY
GetInstanceDataSize_Selection(dag2_node& Node,
                             dag2_nodedata_selection& Selection,
                             dag2& TheDag, model& FromModel, model& ToModel)
{
    GRANNY_ENTER();

    return SizeOf(dag2_instancedata_sel);
}


bool GRANNY
BindDAGInstanceData_Selection(dag2_node&              Node,
                             dag2_nodedata_selection& NodeData,
                             dag2_instance&          Instance)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    Data->CurrentEdge = NodeData.DefaultEdge;

    return true;
}


bool GRANNY
UnbindDAGInstanceData_Selection(dag2_node&              Node,
                               dag2_nodedata_selection& NodeData,
                               dag2_instance&          Instance)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);

    // dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);

    // Nothing to do here.

    return true;
}


bool GRANNY
BakeOutDAGInstanceData_Selection(dag2_node&              Node,
                                dag2_nodedata_selection& NodeData,
                                dag2_instance&          Instance,
                                memory_arena&           Arena)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    NodeData.DefaultEdge = Data->CurrentEdge;

    return true;
}

int32x GRANNY
GetCurrentInput_Selection(dag2_node& Node,
                          dag2_nodedata_selection& NodeData,
                          dag2_instance& Instance)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);
    return Data->CurrentEdge;
}


bool GRANNY
SetInt32_Selection(dag2_node& Node,
                   dag2_nodedata_selection& NodeData,
                   dag2_instance& Instance,
                   char const* ParamName,
                   int32 Value)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);

    if (StringsAreEqualLowercase(ParamName, "Current"))
    {
        Data->CurrentEdge = ClampInt32(0, Value, Node.ExternalInputEdgeCount);
        return true;
    }
    else
    {
        return false;
    }
}

bool GRANNY
GetInt32_Selection(dag2_node& Node,
                   dag2_nodedata_selection& NodeData,
                   dag2_instance& Instance,
                   char const* ParamName,
                   int32* Value)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.InstanceData, return false);
    CheckPointerNotNull(Value, return false);

    dag2_instancedata_sel* Data = GET_DAG2INST_DATA(dag2_instancedata_sel, Instance, Node);

    if (StringsAreEqualLowercase(ParamName, "Current"))
    {
        *Value = Data->CurrentEdge;
        return true;
    }

    return false;
}


