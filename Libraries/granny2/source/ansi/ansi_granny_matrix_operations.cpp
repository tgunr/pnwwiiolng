// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_matrix_operations.cpp $
// $DateTime: 2008/10/30 10:12:58 $
// $Change: 19276 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_matrix_operations.h"

#include "granny_assert.h"
#include "granny_memory.h"
#include "granny_parameter_checking.h"

// Should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode Undefined_LogMessage
USING_GRANNY_NAMESPACE;


BEGIN_GRANNY_NAMESPACE;

OPTIMIZED_DISPATCH(ColumnMatrixMultiply4x3Impl)          = ColumnMatrixMultiply4x3Impl_Generic;
OPTIMIZED_DISPATCH(ColumnMatrixMultiply4x3TransposeImpl) = ColumnMatrixMultiply4x3TransposeImpl_Generic;
OPTIMIZED_DISPATCH(ColumnMatrixMultiply4x4Impl)          = ColumnMatrixMultiply4x4Impl_Generic;

END_GRANNY_NAMESPACE;
