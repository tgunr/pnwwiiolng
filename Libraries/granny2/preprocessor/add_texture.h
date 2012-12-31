#if !defined(ADD_TEXTURE_H)
// ========================================================================
// $File: //jeffr/granny/preprocessor/add_texture.h $
// $DateTime: 2009/03/02 17:01:40 $
// $Change: 20892 $
// $Revision: #3 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"

bool AddTexture(input_file*,
                granny_int32x,
                key_value_pair* KeyValues,
                granny_int32x NumKeyValues,
                granny_memory_arena* Arena);

#define ADD_TEXTURE_H
#endif /* ADD_TEXTURE_H */
