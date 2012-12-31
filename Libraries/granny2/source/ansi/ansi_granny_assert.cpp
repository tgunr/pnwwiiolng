// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_assert.cpp $
// $DateTime: 2009/07/28 16:54:38 $
// $Change: 22552 $
// $Revision: #7 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if DEBUG

#include "ansi_granny_std.h"
#include "granny_assert.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

bool GRANNY
DisplayAssertion(char const * const Expression,
                 char const * const File,
                 int32x const LineNumber,
                 char const * const Function,
                 bool* IgnoreAssertion,
                 void* UserData)
{
    assert(!"Granny assertion");
    return false;
}

#endif // DEBUG
