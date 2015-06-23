// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_file_operations.cpp $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_file_operations.h"
#include "winxx_granny_windows.h"
#include "granny_log.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

static void
DeleteFileProxy(char const *FileName)
{
    DeleteFileA(FileName);
}

inline DWORD
GetFileSizeProxy(HANDLE WinXXFileHandle, DWORD *HighSize)
{
    return(GetFileSize(WinXXFileHandle, HighSize));
}

#undef DeleteFile
#undef GetFileSize

static bool
WinXXFindFile(char const * const FileName, WIN32_FIND_DATAA &Data)
{
    HANDLE Handle = FindFirstFileA(FileName, &Data);
    if(Handle != INVALID_HANDLE_VALUE)
    {
        FindClose(Handle);
        return(true);
    }

    return(false);
}

char const *GRANNY
GetTemporaryDirectory(void)
{
    static char TempDirectoryBuffer[MAX_PATH] = { 0 };

    if (TempDirectoryBuffer[0] == '\0')
    {
        GetTempPathA(sizeof(TempDirectoryBuffer) - 1, TempDirectoryBuffer);
        {for(char *Replace = TempDirectoryBuffer;
             *Replace;
             ++Replace)
        {
            if(*Replace == '\\')
            {
                *Replace = '/';
            }
        }}
    }

    return (TempDirectoryBuffer);
}

void GRANNY
DeleteFile(char const *FileName)
{
    DeleteFileProxy(FileName);
}

int32x GRANNY
GetFileSize(char const *FileName)
{
    int32x FileSize = 0;

    WIN32_FIND_DATAA Data;
    if(WinXXFindFile(FileName, Data))
    {
        DWORD HighSize = Data.nFileSizeHigh;
        DWORD LowSize = Data.nFileSizeLow;
        if(HighSize == 0)
        {
            FileSize = LowSize;
        }
        else
        {
            Log1(ErrorLogMessage, FileReadingLogMessage,
                 "File %s is too long (> 4 gigs)", FileName);
        }
    }
    else
    {
        Log1(ErrorLogMessage, FileReadingLogMessage,
             "Unable to get file size for %s", FileName);
    }

    return(FileSize);
}
