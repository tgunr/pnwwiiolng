// ========================================================================
// $File: //jeffr/granny/rt/granny_file_reader.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #8 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_file_reader.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode FileReadingLogMessage

USING_GRANNY_NAMESPACE;

bool GRANNY
InitializeFileReader(close_file_reader_callback *CloseFileReaderCallback,
                     read_at_most_callback *ReadAtMostCallback,
                     get_reader_size_callback* GetReaderSizeCallback,
                     file_reader &Reader)
{
    GRANNY_ENTER();

    CheckPointerNotNull(CloseFileReaderCallback,   return false);
    CheckPointerNotNull(ReadAtMostCallback,    return false);
    CheckPointerNotNull(GetReaderSizeCallback, return false);

    Reader.CloseFileReaderCallback = CloseFileReaderCallback;
    Reader.ReadAtMostCallback = ReadAtMostCallback;
    Reader.GetReaderSizeCallback = GetReaderSizeCallback;

    return true;
}

open_file_reader_callback *GRANNY
GetDefaultFileReaderOpenCallback(void)
{
    GRANNY_ENTER();

    return(OpenFileReaderCallback);
}

void GRANNY
SetDefaultFileReaderOpenCallback(
    open_file_reader_callback *OpenFileReaderCallbackInit)
{
    GRANNY_ENTER();

    OpenFileReaderCallback = OpenFileReaderCallbackInit;
}
