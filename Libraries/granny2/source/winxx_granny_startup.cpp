// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_startup.cpp $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include "winxx_granny_windows.h"
#include "granny_startup.h"
#include "winxx_granny_dll.h"

/* @cdep pre
   $requires($clipfilename($file)/winxx_granny_msvc_stubs.cpp)
 */

// This should always be the last header included
#include "granny_cpp_settings.h"


USING_GRANNY_NAMESPACE;

#if PLATFORM_WIN32

extern "C"
{
    BOOL WINAPI
    _DllMainCRTStartup(HANDLE ThisDLL, DWORD Operation, LPVOID)
    {
        bool Result = true;

        if(Operation == DLL_PROCESS_ATTACH)
        {
            // Whenever a process attaches to us, we want to make sure we
            // don't hear from them _every_ time they create a new thread
            // (since we don't care).  DisableThreadLibraryCalls() tells
            // Windows never to bother us with DLL_THREAD_ATTACH and
            // DLL_THREAD_DETACH calls.
            DisableThreadLibraryCalls((HINSTANCE)ThisDLL);

            if(DLLIsNotInWindowsPath(ThisDLL))
            {
                // Now we let the platform non-specific part of Granny do any
                // work it needs to do on startup.
                Result = Startup();
            }
            else
            {
                Result = false;
            }
        }
        else if(Operation == DLL_PROCESS_DETACH)
        {
            // Now we let the platform non-specific part of Granny do any
            // work it needs to do on shutdown.
            Shutdown();
        }

        return(Result);
    }
}

#endif // PLATFORM_WIN32

