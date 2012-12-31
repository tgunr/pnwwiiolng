#if !defined(GRANNY_OPTIMIZED_DISPATCH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_optimized_dispatch.h $
// $DateTime: 2008/10/29 17:33:08 $
// $Change: 19269 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include "header_postfix.h"
#define GRANNY_OPTIMIZED_DISPATCH_H
#endif

#define OPTIMIZED_DISPATCH_TYPE(function) function##_type
#define OPTIMIZED_DISPATCH(function) function##_type *function

#define DECL_GENERIC_DISPATCH(function) void function##_Generic
#define GENERIC_DISPATCH(function) void GRANNY function##_Generic
