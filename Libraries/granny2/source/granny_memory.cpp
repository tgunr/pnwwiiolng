// ========================================================================
// $File: //jeffr/granny/rt/granny_memory.cpp $
// $DateTime: 2009/10/29 14:36:42 $
// $Change: 23792 $
// $Revision: #36 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_memory.h"

#include "granny_memory_arena.h"
#include "granny_parameter_checking.h"

#if DIALOG_ON_MEMORY_EXHAUSTION
#if !PLATFORM_WINXX
#error "Only on windows, please."
#endif
#include "winxx_granny_windows.h"
#endif

// This should always be the last header included
#include "granny_cpp_settings.h"

#undef SubsystemCode
#define SubsystemCode MemoryLogMessage

USING_GRANNY_NAMESPACE;

/* ========================================================================
   Basic allocator
   ======================================================================== */
static allocation_header MemorySentinel =
{
    (uint32)allocation_header::AllocationHeaderMV, 0,
    0, 0,
    __FILE__, __LINE__,
    &MemorySentinel,
    &MemorySentinel,
};

static void *
GetMemoryFromHeader(allocation_header const *Header)
{
    GRANNY_ENTER();

    return (void*)(Header + 1);
}

allocation_header* GRANNY
GetHeaderFromMemory(void *Memory)
{
    GRANNY_ENTER();

    // Back up the pointer to the header we stored before it
    allocation_header *Header = (allocation_header *)Memory;
    --Header;

    return(Header);
}

void
SetBlockToBAADF00D(uintaddrx Size, void *Pointer)
{
    GRANNY_ENTER();

    SetUInt32(Size / 4, 0xBAADF00D, Pointer);
}

void *
BasicTrackedAllocate(char const *File, int32x Line,
                     uintaddrx Alignment,
                     uintaddrx Size)
{
    GRANNY_ENTER();

    uint8 *Memory = 0;

    // Allocate the requested space, plus space for our hidden header,
    // plus space for the alignment we might need to do.
    uintaddrx const MemorySize =
        (sizeof(allocation_header) + (Alignment - 1) + Size);

    void *ActualPointer = PlatformAllocate(MemorySize);
    if (ActualPointer)
    {
        Memory = (uint8 *)ActualPointer;
        Memory += sizeof(allocation_header);
        Memory = (uint8*)AlignN((uintaddrx)Memory, Alignment);

        allocation_header *Header = GetHeaderFromMemory(Memory);
        Assert(((uintaddrx)Memory % Alignment) == 0);
        Assert(PtrDiffSignOnly(Header, ActualPointer) >= 0);

        // Record the information about this allocation
        Header->MagicValue = (uint32)allocation_header::AllocationHeaderMV;
        Header->ActualPointer = ActualPointer;
        Header->SourceFileName = File;
        Header->SourceLineNumber = Line;
        Header->Size = Size;
        Header->ActualSize = MemorySize;

#if DEBUG_MEMORY
        // Initialize the entire block with bogus values
        SetBlockToBAADF00D(Header->Size, Memory);
#endif

#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif

        // Link us in with the rest of the memory
        Header->Next = MemorySentinel.Next;
        Header->Previous = &MemorySentinel;
        MemorySentinel.Next->Previous = Header;
        MemorySentinel.Next = Header;

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif
    }
    else
    {
#if DIALOG_ON_MEMORY_EXHAUSTION
        WinXXPopMessageBox(("Granny was unable to allocate memory from the operating system.\r\n"
                            "If this has occurred in the exporter, this typically indicates\r\n"
                            "that you're exporting a scene that is extremely large, or has\r\n"
                            "otherwise fragmented virtual memory to the point that few large\r\n"
                            "continuous regions exist in the address space.  Write to us at\r\n"
                            "granny3@radgametools.com for assistance."),
                           "Memory exhaustion", eOK);
        DWORD WinXXPopMessageBox(char const* Message, char const* Title);
#endif

        Log4(ErrorLogMessage, MemoryLogMessage,
             "Unable to allocate %d bytes (%d requested by %s:%d).",
             MemorySize, Size, File, Line);
#if DEBUG_MEMORY
        Assert ( false );
#endif
    }

    return(Memory);
}

static bool
AllocationHeaderIsValid(allocation_header *Header)
{
    GRANNY_ENTER();

    return (Header &&
            (Header->MagicValue == allocation_header::AllocationHeaderMV) &&
            (Header->Size != (~(uintaddrx)0)));
}

void
BasicTrackedDeallocate(char const *File, int32x Line, void *Memory)
{
    GRANNY_ENTER();

    if (Memory)
    {
        // Back up the pointer to the header we stored before it
        allocation_header *Header = GetHeaderFromMemory(Memory);

        // We might be able to grab the spinlock a little later, but
        // this is slightly safer
#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif

        // Make sure we actually allocated it in the first place
        if(AllocationHeaderIsValid(Header))
        {
#if DEBUG_MEMORY
            // Overwrite the entire block with bogus values
            SetBlockToBAADF00D(Header->Size, Memory);
#endif

            // Overwrite the size so we can catch quick double-frees
            Header->Size = ~(uintaddrx)0;

            // Unlink
            Assert(AllocationHeaderIsValid(Header->Next));
            Assert(AllocationHeaderIsValid(Header->Previous));
            Header->Next->Previous = Header->Previous;
            Header->Previous->Next = Header->Next;

            PlatformDeallocate(Header->ActualPointer);
        }
        else
        {
            Log3(ErrorLogMessage, MemoryLogMessage,
                 "%s:%d Attempted to free 0x%x, which was not allocated "
                 "by this allocator.", File, Line, Memory);
#if DEBUG_MEMORY
            // Annoyingly, the Granny viewer does this all the time.
            //Assert ( false );
#endif
        }

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif
    }
}

allocation_header *GRANNY
AllocationsBegin(void)
{
    GRANNY_ENTER();

    return(MemorySentinel.Next);
}

allocation_header *GRANNY
NextAllocation(allocation_header *Current)
{
    GRANNY_ENTER();

    // Make sure it's actually a header
    Assert(AllocationHeaderIsValid(Current));

    // Make sure people aren't trying to iterate past the end
    Assert(Current != &MemorySentinel);

    Current = Current->Next;

    // Double check to make sure the next allocation block is valid
    Assert(AllocationHeaderIsValid(Current));

    return(Current);
}

allocation_header *GRANNY
AllocationsEnd(void)
{
    GRANNY_ENTER();

    return(&MemorySentinel);
}

void GRANNY
GetAllocationInformation(allocation_header const *Header,
                         allocation_information &Information)
{
    GRANNY_ENTER();

    Assert(Header);
    Assert(Header->MagicValue == allocation_header::AllocationHeaderMV);

    // TODO: Report actual size for real
    Information.Memory = GetMemoryFromHeader(Header);
    Information.RequestedSize = Header->Size;
    Information.ActualSize = Header->Size + sizeof(allocation_header);
    Information.SourceFileName = Header->SourceFileName;
    Information.SourceLineNumber = Header->SourceLineNumber;
}

void *GRANNY
BeginAllocationCheck(void)
{
    GRANNY_ENTER();

    return(AllocateSize(0));
}

allocation_header *GRANNY
CheckedAllocationsEnd(void *CheckIdentifier)
{
    GRANNY_ENTER();

    allocation_header *Header = GetHeaderFromMemory(CheckIdentifier);
    Assert(AllocationHeaderIsValid(Header));

    return(Header);
}

bool GRANNY
EndAllocationCheck(void *CheckIdentifier)
{
    GRANNY_ENTER();

    bool Result = (GetHeaderFromMemory(CheckIdentifier) == MemorySentinel.Next);
    if(!Result)
    {
#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif
        {for(allocation_header *Current = AllocationsBegin();
             Current != CheckedAllocationsEnd(CheckIdentifier);
             Current = NextAllocation(Current))
        {
            allocation_information Information;
            GetAllocationInformation(Current, Information);
            Log4(WarningLogMessage, MemoryLogMessage,
                 "Unfreed block %s:%d (%d bytes at 0x%x)",
                 Information.SourceFileName,
                 Information.SourceLineNumber,
                 Information.RequestedSize,
                 Information.Memory);
        }}

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif

        Assert(!"Blockable allocation check failed - allocations written to log");
    }

    Deallocate(CheckIdentifier);

    return(Result);
}

void GRANNY
FreeBuilderResult(void *Result)
{
    GRANNY_ENTER();

    Deallocate(Result);
}

allocate_callback *GRANNY AllocateCallback     = BasicTrackedAllocate;
deallocate_callback *GRANNY DeallocateCallback = BasicTrackedDeallocate;

void GRANNY
GetAllocator(allocate_callback *&ReturnAllocateCallback,
             deallocate_callback *&ReturnDeallocateCallback)
{
    GRANNY_ENTER();

    ReturnAllocateCallback = AllocateCallback;
    ReturnDeallocateCallback = DeallocateCallback;
}

void GRANNY
SetAllocator(allocate_callback *AllocateCallbackInit,
             deallocate_callback *DeallocateCallbackInit)
{
    GRANNY_ENTER();

    AllocateCallback = AllocateCallbackInit;
    DeallocateCallback = DeallocateCallbackInit;
}

