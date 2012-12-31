// ========================================================================
// $File: //jeffr/granny/rt/granny_binary_search.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_binary_search.h"

#include "granny_assert.h"
#include "granny_parameter_checking.h"

// Should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode Undefined_LogMessage
USING_GRANNY_NAMESPACE;


void GRANNY
InitializeSearch(binary_searcher& Searcher, int32x Count)
{
    GRANNY_ENTER();

    if (Count <= 0)
    {
        // Zero search
        Searcher.Base   = 0;
        Searcher.Window = 0;
    }
    else
    {
        Searcher.Base   = 0;
        Searcher.Window = Count;
    }
}

bool GRANNY
SearchRemaining(binary_searcher& Searcher)
{
    GRANNY_ENTER();

    return (Searcher.Window > 0);
}

int32x GRANNY
GetSearchProbe(binary_searcher& Searcher)
{
    GRANNY_ENTER();

    int32x Halfway = Searcher.Base + (Searcher.Window / 2);

    Assert(Halfway >= Searcher.Base &&
           Halfway < (Searcher.Base + Searcher.Window));

    return Halfway;
}

void GRANNY
UpdateLessThan(binary_searcher& Searcher, int32x Probe)
{
    GRANNY_ENTER();

    Assert(SearchRemaining(Searcher));
    Assert(Probe >= Searcher.Base &&
           Probe < Searcher.Base + Searcher.Window);

    Searcher.Window = Probe - Searcher.Base;
}

void GRANNY
UpdateGreaterThan(binary_searcher& Searcher, int32x Probe)
{
    GRANNY_ENTER();

    Assert(SearchRemaining(Searcher));
    Assert(Probe >= Searcher.Base &&
           Probe < Searcher.Base + Searcher.Window);

    Searcher.Window -= (Probe - Searcher.Base + 1);
    Searcher.Base    = Probe + 1;
}
