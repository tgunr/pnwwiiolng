#if !defined(MAKE_STRING_DATABASE_H)
// ========================================================================
// $File: //jeffr/granny/preprocessor/make_string_database.h $
// $DateTime: 2009/03/03 13:56:57 $
// $Change: 20905 $
// $Revision: #3 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include "preprocessor.h"

bool ExtractStrings(input_file*     InputFiles,
                    granny_int32x   NumInputFiles,
                    key_value_pair* KeyValues,
                    granny_int32x   NumKeyValues,
                    granny_memory_arena* TempArena);

bool RemapStrings(input_file& InputFile,
                  key_value_pair* KeyValues,
                  granny_int32x   NumKeyValues,
                  granny_memory_arena* TempArena);

#define MAKE_STRING_DATABASE_H
#endif /* MAKE_STRING_DATABASE_H */
