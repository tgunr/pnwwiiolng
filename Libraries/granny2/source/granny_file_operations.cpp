// ========================================================================
// $File: //jeffr/granny/rt/granny_file_operations.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #11 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_file_operations.h"

#include "granny_file_reader.h"
#include "granny_file_writer.h"
#include "granny_memory.h"
#include "granny_string.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode FileOperationLogMessage
USING_GRANNY_NAMESPACE;

#define DefaultCopyBufferSize (1 << 16)

bool GRANNY
AppendStringToFile(char const* Filename, char const* String)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Filename, return false);
    CheckPointerNotNull(String, return false);

    file_writer *Writer = NewFileWriter(Filename, false);
    if (Writer == 0)
        return false;

    SeekWriterFromEnd(Writer, 0);
    Write(Writer, StringLength(String), String);
    Write(Writer, 1, "\n");
    DeleteFileWriter(Writer);

    return true;
}



bool GRANNY
ConcatenateFile(file_writer* Writer, char const *FileName,
                uint32x* BytesCopied)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Writer, return false);
    CheckPointerNotNull(FileName, return false);

    uint32x DummyBytesCopied;
    if (BytesCopied == 0)
        BytesCopied = &DummyBytesCopied;
    *BytesCopied = 0;

    bool Result = false;
    file_reader *Reader = OpenFileReader(FileName);
    if (Reader)
    {
        Result = ConcatenateFileReader(Writer, Reader, BytesCopied);
        CloseFileReader(Reader);
    }

    return Result;
}


bool GRANNY
ConcatenateFileReader(file_writer* Writer,
                      file_reader* Reader,
                      uint32x* BytesCopied)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Reader, return false);
    CheckPointerNotNull(Writer, return false);

    uint32x DummyBytesCopied;
    if (BytesCopied == 0)
        BytesCopied = &DummyBytesCopied;
    *BytesCopied = 0;

    void* CopyBuffer = AllocateSize(DefaultCopyBufferSize);
    if (CopyBuffer == 0)
        return false;

    bool Result = true;
    while (Result)
    {
        int32x const ReadSize = ReadAtMost(Reader, *BytesCopied,
                                           DefaultCopyBufferSize, CopyBuffer);
        if (ReadSize == 0)
            break;

        Result = Result && Write(Writer, ReadSize, CopyBuffer);
        *BytesCopied += ReadSize;
    }

    Deallocate(CopyBuffer);
    return Result;
}
