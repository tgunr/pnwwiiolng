#if !defined(GRANNY_DAG2_INFO_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_dag2_info.h $
// $DateTime: 2008/10/23 13:25:46 $
// $Change: 19193 $
// $Revision: #4 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(DagGroup);

struct dag2;
struct file;

EXPTYPE struct dag2_info
{
    int32  DAGCount;
    dag2** DAGs;

    variant ExtendedData;
};
EXPCONST EXPGROUP(dag2) extern data_type_definition Dag2InfoType[];

EXPAPI GS_READ dag2_info *GetDag2Info(file &File);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DAG2_INFO_H
#endif
