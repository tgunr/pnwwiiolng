// ========================================================================
// $File: //jeffr/granny/rt/x86/x86_granny_cpu_queries.cpp $
// $DateTime: 2009/08/26 11:49:00 $
// $Change: 22891 $
// $Revision: #4 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(X86_GRANNY_CPU_QUERIES_H)
#include "x86_granny_cpu_queries.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

bool GRANNY
SSEIsAvailable(void)
{
    static bool Result = false;

    static bool Test = true;
    if(Test)
    {
        Test = false;

        __asm
        {
            pushfd
            pop eax                 ;EAX = original eflags word

            mov ebx,eax             ;save it in EBX

            xor eax,1 << 21         ;toggle bit 21
            push eax
            popfd

            pushfd                  ;examine eflags again
            pop eax

            push ebx                ;restore original eflags value
            popfd

            cmp eax,ebx             ;bit changed?
            je __no_SSE             ;no CPUID, ergo no SSE

            mov eax,1               ;else OK to perform CPUID
            cpuid
            test edx,0x2000000      ;test bit 25
            jz __no_SSE

            mov [Result],1

          __no_SSE:
        }
    }

    return(Result);
}
