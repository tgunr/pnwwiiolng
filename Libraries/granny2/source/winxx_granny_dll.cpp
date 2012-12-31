// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_dll.cpp $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "winxx_granny_dll.h"
#include "granny_assert.h"
#include "granny_limits.h"
#include "granny_memory.h"
#include "granny_string.h"
#include "granny_string_formatting.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

bool GRANNY
DLLIsNotInWindowsPath(HANDLE DllHandle)
{
    bool Result = true;

    char DLLPath[MaximumSystemFileNameSize];
    GetModuleFileName((HINSTANCE)DllHandle, DLLPath, SizeOf(DLLPath));

    char *DLLFileName = DLLPath;
    {for(char *Scan = DLLFileName;
         *Scan;
         ++Scan)
    {
        if((*Scan == '\\') || (*Scan == '/'))
        {
            DLLFileName = Scan + 1;
        }
    }}

    char WindowsDirectory[MaximumSystemFileNameSize];
    GetWindowsDirectory(WindowsDirectory, SizeOf(WindowsDirectory));

    char SystemDirectory[MaximumSystemFileNameSize];
    GetSystemDirectory(SystemDirectory, SizeOf(SystemDirectory));

    if(StringBeginsWithLowercase(DLLPath, WindowsDirectory) ||
       StringBeginsWithLowercase(DLLPath, SystemDirectory))
    {
        static char ErrorMessage[MaximumMessageBufferSize];
        ConvertToStringVar(sizeof(ErrorMessage), ErrorMessage,
                           "%s has been incorrectly installed as "
                           "%s.  To avoid versioning problems, "
                           "%s must always be installed in an "
                           "application-specific directory, and never "
                           "in a system directory.",
                           DLLFileName, DLLPath, DLLFileName);

        MessageBox(0, ErrorMessage, "Fatal Granny Installation Error",
                   MB_OK | MB_ICONERROR);

        Result = false;
    }

    return(Result);
}
