#if !defined(UTILITIES_H)
// ========================================================================
// $File: //jeffr/granny/preprocessor/utilities.h $
// $DateTime: 2008/12/11 15:42:13 $
// $Change: 19754 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include <granny.h>
#include <functional>
#include <string.h>

struct CIStrcmp : public std::binary_function<char const*, char const*, bool>
{
    bool operator()(char const* One, char const* Two) const
    {
        return _stricmp(One, Two) < 0;
    }
};


float GetPosError(granny_transform& One, granny_transform& Two);
float GetOriError(granny_transform& One, granny_transform& Two);
float GetSSError(granny_transform& One, granny_transform& Two);


#define UTILITIES_H
#endif
