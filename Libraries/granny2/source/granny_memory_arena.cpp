// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_arena.cpp $
// $DateTime: 2009/09/10 15:56:08 $
// $Change: 23184 $
// $Revision: #16 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_memory_arena.h"

#include "granny_aggr_alloc.h"
#include "granny_parameter_checking.h"
#include "granny_string.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode ArenaAllocatorLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct memory_arena
{
    uintaddrx     Alignment;
    memory_arena* Next;
};

END_GRANNY_NAMESPACE;

memory_arena *GRANNY
NewMemoryArena(void)
{
    GRANNY_ENTER();

    memory_arena *NewArena = Allocate(memory_arena);
    if(NewArena)
    {
        NewArena->Alignment = 4;
        NewArena->Next = 0;
    }

    return(NewArena);
}

void GRANNY
SetArenaAlignment(memory_arena& Arena, uintaddrx Align)
{
    GRANNY_ENTER();

    CheckGreater0(Align, return);
    CheckNotSignCoerced(Align, return);

    Arena.Alignment = Align;
}

uintaddrx GRANNY
GetArenaAlignment(memory_arena& Arena)
{
    GRANNY_ENTER();

    return Arena.Alignment;
}

void GRANNY
ClearArena(memory_arena *Arena)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Arena, return);

    FreeMemoryArena(Arena->Next);
    Arena->Next = 0;
}

void GRANNY
FreeMemoryArena(memory_arena *Arena)
{
    GRANNY_ENTER();

    memory_arena *ArenaIterator = Arena;
    while(ArenaIterator)
    {
        memory_arena *DeleteArena = ArenaIterator;
        ArenaIterator = ArenaIterator->Next;

        Deallocate(DeleteArena);
    }
}

void *GRANNY
MemoryArenaPush(memory_arena &Arena, uintaddrx Size)
{
    GRANNY_ENTER();

    CheckNotSignCoerced(Size, return 0);

    // Special case
    if (Size == 0)
        return 0;

    void *Result = 0;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    if (Arena.Alignment != 0)
        SetAggrAlignment(Allocator, Arena.Alignment);

    memory_arena *NewChunk;
    AggrAllocPtr(Allocator, NewChunk);
    AggrAllocSizePtr(Allocator, Size, Result);
    if(EndAggrAlloc(Allocator))
    {
        NewChunk->Next = Arena.Next;
        Arena.Next = NewChunk;
    }
    else
    {
        Assert(Result == NULL);
    }

    return(Result);
}

char *GRANNY
MemoryArenaPushString(memory_arena &Arena, char const *String)
{
    GRANNY_ENTER();

    CheckPointerNotNull(String, return 0);

    return (char*)MemoryArenaPushBinary(Arena, StringLength(String) + 1, String);
}

void *GRANNY
MemoryArenaPushBinary(memory_arena &Arena, uintaddrx Size, void const *Data)
{
    GRANNY_ENTER();

    CheckNotSignCoerced(Size, return 0);
    CheckPointerNotNull(Data, return 0);

    void* Memory = MemoryArenaPush(Arena, Size);
    if (Memory)
    {
        Copy(Size, Data, Memory);
    }

    return Memory;
}

char* GRANNY
MemoryArenaPushConcat(memory_arena &Arena, char const* First, char const* Second)
{
    GRANNY_ENTER();

    CheckPointerNotNull(First,  return 0);
    CheckPointerNotNull(Second, return 0);

    int32x const FirstLen   = StringLength(First);
    int32x const SecondLen  = StringLength(Second);
    int32x const BufferSize = (FirstLen + SecondLen + 1);

    char* Buffer = (char*)MemoryArenaPush(Arena, BufferSize);
    if (Buffer)
    {
        Copy(FirstLen, First, Buffer);
        Copy(SecondLen + 1, Second, Buffer + FirstLen);
        Assert(StringLength(Buffer) == BufferSize-1);
    }

    return Buffer;
}

