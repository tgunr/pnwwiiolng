#if !defined(GRANNY_DAG2_NODE_REGISTRY_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_node_registry.h $
// $DateTime: 2008/08/26 17:10:51 $
// $Change: 18193 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct data_type_definition;

data_type_definition* GetDag2NodeDataTypeFromTypeMarker(char const*);
int32x GetDag2NodeDataTypeIndexFromTypeMarker(char const*);
bool RegisterCustomNodeType(data_type_definition*);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DAG2_NODE_REGISTRY_H
#endif
