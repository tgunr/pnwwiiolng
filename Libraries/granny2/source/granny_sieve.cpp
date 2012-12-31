// ========================================================================
// $File: //jeffr/granny/rt/granny_sieve.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #10 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_sieve.h"

#include "granny_aggr_alloc.h"
#include "granny_memory.h"
#include "granny_stack_allocator.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct sieve
{
    int32x GroupCount;
    stack_allocator *Groups;
};

END_GRANNY_NAMESPACE;

sieve *GRANNY
NewSieve(int32x GroupCount, int32x UnitSize)
{
    GRANNY_ENTER();

    sieve *Sieve;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrAllocPtr(Allocator, Sieve);
    AggrAllocOffsetArrayPtr(Allocator, Sieve, GroupCount, GroupCount, Groups);
    if(EndAggrAlloc(Allocator))
    {
        {for(int32x GroupIndex = 0;
             GroupIndex < GroupCount;
             ++GroupIndex)
        {
            StackInitialize(Sieve->Groups[GroupIndex], UnitSize, 1024);
        }}
    }

    return(Sieve);
}

int32x GRANNY
GetUsedGroupCount(sieve &Sieve)
{
    GRANNY_ENTER();

    int32x Count = 0;

    {for(int32x GroupIndex = 0;
         GroupIndex < Sieve.GroupCount;
         ++GroupIndex)
    {
        if(GetStackUnitCount(Sieve.Groups[GroupIndex]) > 0)
        {
            ++Count;
        }
    }}

    return(Count);
}

void GRANNY
ClearSieve(sieve &Sieve)
{
    GRANNY_ENTER();

    {for(int32x GroupIndex = 0;
         GroupIndex < Sieve.GroupCount;
         ++GroupIndex)
    {
        StackCleanUp(Sieve.Groups[GroupIndex]);
    }}
}

void GRANNY
FreeSieve(sieve *Sieve)
{
    GRANNY_ENTER();

    if(Sieve)
    {
        ClearSieve(*Sieve);
        Deallocate(Sieve);
    }
}

void *GRANNY
AddSieveUnit(sieve &Sieve, int32x GroupIndex)
{
    GRANNY_ENTER();

    Assert(GroupIndex < Sieve.GroupCount);

    int32x UnitIndex;
    if(NewStackUnit(Sieve.Groups[GroupIndex], &UnitIndex))
    {
        return(GetStackUnit(Sieve.Groups[GroupIndex], UnitIndex));
    }

    return(0);
}

int32x GRANNY
GetSieveGroupUnitCount(sieve &Sieve, int32x GroupIndex)
{
    GRANNY_ENTER();

    Assert(GroupIndex < Sieve.GroupCount);

    return(GetStackUnitCount(Sieve.Groups[GroupIndex]));
}

void GRANNY
SerializeSieveGroup(sieve &Sieve, int32x GroupIndex, void *Dest)
{
    GRANNY_ENTER();

    Assert(GroupIndex < Sieve.GroupCount);

    SerializeStack(Sieve.Groups[GroupIndex], Dest);
}
