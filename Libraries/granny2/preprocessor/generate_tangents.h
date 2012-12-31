#if !defined(GENERATE_TANGENTS_H)
// ========================================================================
// $File: //jeffr/granny/preprocessor/generate_tangents.h $
// $DateTime: 2009/04/21 13:36:27 $
// $Change: 21418 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"

bool GenerateTangents(input_file&,
                      key_value_pair* KeyValues,
                      granny_int32x NumKeyValues,
                      granny_memory_arena* Arena);

#define GENERATE_TANGENTS_H
#endif
