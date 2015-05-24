// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_file_reader.cpp $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_assert.h"
#include "granny_file_reader.h"
#include "granny_memory.h"
#include "granny_parameter_checking.h"
#include "winxx_granny_windows.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode FileReadingLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;


struct winxx_file_reader
{
    file_reader Base;
    HANDLE WinXXFileHandle;
};

END_GRANNY_NAMESPACE;

static void
WinXXCloseFileReader(file_reader* ReaderInit)
{
    winxx_file_reader *Reader = (winxx_file_reader *)ReaderInit;

    if(Reader)
    {
        if(!CloseHandle(Reader->WinXXFileHandle))
        {
            WinXXLogErrorAsWarning(CloseHandle);
        }

        Deallocate(Reader);
    }
}

static int32x
WinXXReadAtMost(file_reader* ReaderInit,
                int32x FilePosition,
                int32x UInt8Count, void *Buffer)
{
    CheckPointerNotNull(ReaderInit, return 0);

    DWORD UInt8sRead = 0;
    winxx_file_reader &Reader = *((winxx_file_reader*)ReaderInit);

    if(WinXXSeek(Reader.WinXXFileHandle, FilePosition, FILE_BEGIN)
       == FilePosition)
    {
        if(!ReadFile(Reader.WinXXFileHandle, Buffer,
                     UInt8Count, &UInt8sRead, 0))
        {
            WinXXLogErrorAsWarning(ReadFile);
        }
    }

    return(UInt8sRead);
}

static bool
WinXXGetReaderSize(file_reader* ReaderInit, int32x* SizeVar)
{
    CheckPointerNotNull(ReaderInit, return false);
    CheckPointerNotNull(SizeVar, return false);

    winxx_file_reader& Reader = *((winxx_file_reader*)ReaderInit);

    DWORD FileSize = GetFileSize(Reader.WinXXFileHandle, NULL);
    if (FileSize != -1)
    {
        CheckConvertToInt32(*SizeVar, FileSize, return false);
        return true;
    }

    return false;
}

file_reader *GRANNY
CreatePlatformFileReader(char const *FileNameToOpen)
{
    winxx_file_reader *Reader = 0;

    HANDLE WinXXFileHandle =
        CreateFileA(FileNameToOpen,
                   GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, 0);
    if (WinXXFileHandle != INVALID_HANDLE_VALUE)
    {
        Reader = Allocate(winxx_file_reader);
        if(Reader)
        {
            InitializeFileReader(WinXXCloseFileReader,
                                 WinXXReadAtMost,
                                 WinXXGetReaderSize,
                                 Reader->Base);
            Reader->WinXXFileHandle = WinXXFileHandle;
        }
        else
        {
            CloseHandle(WinXXFileHandle);
        }
    }
    else
    {
        // TODO: We decided to stop logging this, since files
        // often cannot be opened and it is not considered an
        // error condition.
//        WinXXLogErrorAsWarning(CreateFile);
    }

    return (file_reader *)Reader;
}

open_file_reader_callback *GRANNY OpenFileReaderCallback = CreatePlatformFileReader;
