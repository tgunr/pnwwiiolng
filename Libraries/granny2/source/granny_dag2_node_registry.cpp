// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_node_registry.cpp $
// $DateTime: 2010/03/19 17:27:17 $
// $Change: 25804 $
// $Revision: #10 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_dag2_node_registry.h"

#include "granny_assert.h"
#include "granny_dag2_additive_blend.h"
#include "granny_dag2_blend.h"
#include "granny_dag2_callback.h"
#include "granny_dag2_locked_blend.h"
#include "granny_dag2_nway_blend.h"
#include "granny_dag2_poseanimsource.h"
#include "granny_dag2_scalarsource.h"
#include "granny_dag2_scaleoffset.h"
#include "granny_dag2_selection.h"
#include "granny_dag2_sequence.h"
#include "granny_dag2_subgraph.h"
#include "granny_dag2_timeshift.h"
#include "granny_dag2_trackmask.h"
#include "granny_data_type_definition.h"
#include "granny_parameter_checking.h"

// Should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode BlendDagLogMessage
USING_GRANNY_NAMESPACE;

static const int32x MaxRegisteredNodes = 32;
static int32x       NumRegisteredNodes = 13;

// Don't forget to bump CurrentDAGStandardTag!
data_type_definition* NodeRegistry[MaxRegisteredNodes] = {
    Dag2NodeDataSubgraphType,
    Dag2NodeDataPoseAnimSourceType,
    Dag2NodeDataBlendType,
    Dag2NodeDataScalarSourceType,
    Dag2NodeDataScaleOffsetType,
    Dag2NodeDataSequenceType,
    Dag2NodeDataTrackmaskType,
    Dag2NodeDataSelectionType,
    Dag2NodeDataTimeShiftType,
    Dag2NodeDataCallbackType,
    Dag2NodeDataAdditiveBlendType,
    Dag2NodeDataLockedBlendType,
    Dag2NodeDataNWayBlendType,
};


int32x GRANNY
GetDag2NodeDataTypeIndexFromTypeMarker(char const* Marker)
{
    GRANNY_ENTER();

    Assert(Marker);

    {for(int32x Idx = 0; Idx < NumRegisteredNodes; ++Idx)
    {
        data_type_definition* Type = NodeRegistry[Idx];
        Assert(Type);

        if (StringsAreEqualLowercaseOrCallback(Type[0].Name, Marker))
            return Idx;
    }}

    InvalidCodePath("Should never reach this point, no node type found for marker\n");
    return -1;
}

data_type_definition* GRANNY
GetDag2NodeDataTypeFromTypeMarker(char const* Marker)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Marker, return 0);

    int32x Idx = GetDag2NodeDataTypeIndexFromTypeMarker(Marker);
    CheckInt32Index(Idx, NumRegisteredNodes, return 0);

    return NodeRegistry[Idx];
}

bool GRANNY
RegisterCustomNodeType(data_type_definition* Type)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Type, return false);

    {for(int32x Idx = 0; Idx < NumRegisteredNodes; ++Idx)
    {
        data_type_definition* TestType = NodeRegistry[Idx];
        if (StringsAreEqualLowercaseOrCallback(Type[0].Name, TestType[0].Name))
        {
            Assert(DataTypesAreEqual(Type, TestType));
            return false;
        }
    }}

    NodeRegistry[NumRegisteredNodes++] = Type;
    return true;
}

