// ========================================================================
// $File: //jeffr/granny/preprocessor/utilities.cpp $
// $DateTime: 2008/12/11 15:42:13 $
// $Change: 19754 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "utilities.h"
#include <granny.h>
#include <math.h>

static granny_real32
SquaredDistanceBetweenN(granny_int32x ElementCount,
                        granny_real32 const* Vector0,
                        granny_real32 const* Vector1)
{
    granny_real32 Result = 0.0f;
    while(ElementCount--)
    {
        granny_real32 ElemDist = (*Vector0++ - *Vector1++);
        Result += ElemDist * ElemDist;
    }

    return Result;
}

float
GetPosError(granny_transform& One, granny_transform& Two)
{
    return sqrt(SquaredDistanceBetweenN(3, One.Position, Two.Position));
}

float
GetOriError(granny_transform& One, granny_transform& Two)
{
    float InnerProd = 0.0f;
    for (int i = 0; i < 4; i++)
        InnerProd += One.Orientation[i] * Two.Orientation[i];

    // We want the neighborhooded distance
    InnerProd = fabsf(InnerProd);

    return ((1.0f - InnerProd) * (1.0f - InnerProd));
}

float
GetSSError(granny_transform& One, granny_transform& Two)
{
    return sqrt(SquaredDistanceBetweenN(9, (granny_real32*)One.ScaleShear, (granny_real32*)Two.ScaleShear));
}


