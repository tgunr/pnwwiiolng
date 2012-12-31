// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_file_reader.cpp $
// $DateTime: 2009/09/09 16:27:35 $
// $Change: 23143 $
// $Revision: #9 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include "ansi_granny_std.h"
#include "granny_file_reader.h"
#include "granny_memory.h"
#include "granny_assert.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode ANSISubsystemLogMessage

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif

USING_GRANNY_NAMESPACE;


struct ansi_file_reader
{
    file_reader Base;
    FILE *ANSIFileHandle;
};


static void
ANSICloseFileReader(file_reader *ReaderInit)
{
    ansi_file_reader *Reader = (ansi_file_reader *)ReaderInit;

    if(Reader)
    {
        if(fclose(Reader->ANSIFileHandle))
        {
            ANSILogErrorAsWarning(fclose);
        }

        Deallocate(Reader);
    }
}

static int32x
ANSIReadAtMost(file_reader* ReaderInit,
               int32x       FilePosition,
               int32x       UInt8Count,
               void*        Buffer)
{
    intaddrx UInt8sRead = 0;

    ansi_file_reader& Reader = *((ansi_file_reader*)ReaderInit);

    if(ANSISeek(Reader.ANSIFileHandle, FilePosition, SEEK_SET) ==
       FilePosition)
    {
        UInt8sRead = fread(Buffer, 1, UInt8Count, Reader.ANSIFileHandle);
        if(UInt8sRead < UInt8Count)
        {
            if(ferror(Reader.ANSIFileHandle))
            {
                ANSILogErrorAsWarning(fread);
            }
        }
    }

    int32x RetVal;
    CheckConvertToInt32(RetVal, UInt8sRead, return -1); // serious wierdness if this fires...
    return RetVal;
}

static bool
ANSIGetReaderSize(file_reader* ReaderInit, int32x* ReaderSize)
{
    CheckPointerNotNull(ReaderSize, return false);

    ansi_file_reader& Reader = *((ansi_file_reader*)ReaderInit);

    long cur = ftell(Reader.ANSIFileHandle);
    fseek(Reader.ANSIFileHandle, 0, SEEK_END);
    *ReaderSize = ftell(Reader.ANSIFileHandle);
    fseek(Reader.ANSIFileHandle, cur, SEEK_SET);

    return true;
}


file_reader* GRANNY
CreatePlatformFileReader(char const* FileNameToOpen)
{
    ansi_file_reader *Reader = 0;

    FILE *ANSIFileHandle = fopen(FileNameToOpen, "rb");
    if (ANSIFileHandle)
    {
        Reader = Allocate(ansi_file_reader);
        if(Reader)
        {
            InitializeFileReader(ANSICloseFileReader,
                                 ANSIReadAtMost,
                                 ANSIGetReaderSize,
                                 Reader->Base);
            Reader->ANSIFileHandle = ANSIFileHandle;
        }
        else
        {
            fclose(ANSIFileHandle);
        }
    }
    else
    {
        ANSILogErrorAsWarning(fopen);
    }

    return((file_reader *)Reader);
}

open_file_reader_callback *GRANNY OpenFileReaderCallback = CreatePlatformFileReader;
