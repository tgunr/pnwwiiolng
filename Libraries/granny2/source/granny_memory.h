#if !defined(GRANNY_MEMORY_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_memory.h $
// $DateTime: 2010/03/03 23:00:54 $
// $Change: 25515 $
// $Revision: #44 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_OPS_H)
#include "granny_memory_ops.h"
#endif


#if PLATFORM_HAS_MEMORY_INTRINSICS
#if !defined(GRANNY_INTRINSICS_H)
#include "granny_intrinsics.h"
#endif
#endif

// By default, we turn on debug memory if DEBUG is on, but you can control
// it separately by defining DEBUG_MEMORY up front
#if !defined(DEBUG_MEMORY)
#define DEBUG_MEMORY DEBUG
#endif

BEGIN_GRANNY_NAMESPACE;

struct memory_arena;

// TODO: It'd be nice if I added tags to the memory allocator such that
// any Granny allocation is marked as "transient" or "permanent", or
// some sort of arena system, so people who override our allocators
// can make smart decisions about that sort of thing.

// TODO: At some point, it might be nice to extend the allocator
// functionality to include automated initialization and such, for
// inserting counts into structures and initializing blocks to constant
// 8-bit or 32-bit value (since these are the operations that seem
// to be used in many places).

// The basic allocate operates via the Allocate() and Deallocate()
// functions, which manage memory and also track allocations and
// deallocations.  These are _SLOW BY DESIGN_, in an effort to
// discourage people from calling them very often.  If you have lots
// of allocations to do, consider using the aggregation macros
// described later.
#define Allocate(Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, sizeof(Type))
#define AllocateArray(Count, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, sizeof(Type)*(Count))
#define AllocateSize(Size) (*AllocateCallback)(__FILE__, __LINE__, DefaultAllocationAlignment, Size)

#define AllocateAligned(Alignment, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, Alignment, sizeof(Type))
#define AllocateArrayAligned(Alignment, Count, Type) (Type *)(*AllocateCallback)(__FILE__, __LINE__, Alignment, sizeof(Type)*(Count))
#define AllocateSizeAligned(Alignment, Size) (*AllocateCallback)(__FILE__, __LINE__, Alignment, Size)

#define Deallocate(Memory) (*DeallocateCallback)(__FILE__, __LINE__, Memory)
#define DeallocateSafe(Memory) { (*DeallocateCallback)(__FILE__, __LINE__, Memory); (Memory) = 0; }

// All allocations in Granny are tracked, period, whether you're running
// in release or debug or whatever.  The idea is that everybody should
// be very conscientious about memory usage, so every running program should
// be able to provide a profile of the memory.  The following functions
// allow you to iterate over the allocated memory blocks, and find out
// who allocated them (and how much they allocated).
EXPGROUP(MemoryIterationGroup)
EXPTYPE struct allocation_header;
struct allocation_header
{
    enum {AllocationHeaderMV = 0xCA5ECA5E};
    uint32 MagicValue;
    uintaddrx Size;

    void *ActualPointer;
    uintaddrx ActualSize;

    char const *SourceFileName;
    int32x SourceLineNumber;

    allocation_header *Next;
    allocation_header *Previous;
};


// Iteration (STL style, only instead of ++ you use NextAllocation)
EXPAPI GS_MODIFY allocation_header *AllocationsBegin(void);
EXPAPI GS_MODIFY allocation_header *NextAllocation(allocation_header *Current);
EXPAPI GS_MODIFY allocation_header *AllocationsEnd(void);

// Accessors
EXPTYPE_EPHEMERAL struct allocation_information
{
    void *Memory;
    uintaddrx RequestedSize;
    uintaddrx ActualSize;
    char const *SourceFileName;
    int32x SourceLineNumber;
};
EXPAPI GS_READ void GetAllocationInformation(allocation_header const *Header,
                                             allocation_information &Information);

allocation_header* GetHeaderFromMemory(void* Memory);


// Blockable checks
EXPAPI GS_MODIFY void* BeginAllocationCheck(void);
EXPAPI GS_MODIFY allocation_header* CheckedAllocationsEnd(void *CheckIdentifier);
EXPAPI GS_MODIFY bool EndAllocationCheck(void *CheckIdentifier);


// For overriding the standard allocators, you can use the following
// types and functions.
// TODO: Define these directly as __cdecl?
EXPGROUP(MemoryCallbackGroup)
EXPAPI typedef void *allocate_callback(char const *File, int32x Line,
                                       uintaddrx Alignment, uintaddrx Size);
EXPAPI typedef void deallocate_callback(char const *File, int32x Line,
                                        void *Memory);
extern allocate_callback *AllocateCallback;
extern deallocate_callback *DeallocateCallback;

EXPAPI GS_READ void GetAllocator(allocate_callback *&AllocateCallback,
                                 deallocate_callback *&DeallocateCallback);
EXPAPI GS_MODIFY void SetAllocator(allocate_callback *AllocateCallback,
                                   deallocate_callback *DeallocateCallback);

// For freeing objects allocated by the builders
EXPAPI GS_PARAM void FreeBuilderResult(void *Result);


EXPAPI GS_MODIFY void AcquireMemorySpinlock();
EXPAPI GS_MODIFY void ReleaseMemorySpinlock();


// For computing offset sizes, these macros are helpful
#define MAXOFFSET(Store, Offset, Size) \
    if(Store < (Offset + Size)) {Store = Offset + Size;}

// Platform-specific operations
void *PlatformAllocate(uintaddrx Size);
void PlatformDeallocate(void *Memory);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMORY_H
#endif
