// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_file_writer.cpp $
// $DateTime: 2009/07/02 18:18:01 $
// $Change: 22232 $
// $Revision: #18 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "ansi_granny_std.h"
#include "granny_assert.h"
#include "granny_crc.h"
#include "granny_file_writer.h"
#include "granny_memory.h"
#include "granny_parameter_checking.h"

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode FileWritingLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;

struct ansi_file_writer
{
    file_writer Base;
    FILE *ANSIFileHandle;
};
CompileAssert(OffsetFromType(ansi_file_writer, Base) == 0);
CompileAssert(SizeOf(ansi_file_writer) == (SizeOf(file_writer) +
                                           SizeOf(FILE*)));

END_GRANNY_NAMESPACE;

void
AnsiDeleteFileWriter(file_writer *Writer)
{
    ansi_file_writer *AnsiWriter = (ansi_file_writer*)Writer;
    if(Writer)
    {
        Assert(!AnsiWriter->Base.CRCing);

        if(fclose(AnsiWriter->ANSIFileHandle))
        {
            ANSILogErrorAsWarning(fclose);
        }

        Deallocate(Writer);
    }
}

int32x
AnsiSeekWriter(file_writer* WriterInit, int32x OffsetInUInt8s,
               file_writer_seek_type SeekType)
{
    CheckPointerNotNull(WriterInit, return 0);
    ansi_file_writer &AnsiWriter = *((ansi_file_writer*)WriterInit);
    Assert(!AnsiWriter.Base.CRCing || (SeekType == SeekCurrent && OffsetInUInt8s == 0));

    switch (SeekType)
    {
        case SeekStart:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_SET));
        case SeekEnd:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_END));
        case SeekCurrent:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_CUR));

        default:
            InvalidCodePath("Invalid seek type");
            return 0;
    }
}

bool
AnsiWrite(file_writer* WriterInit, int32x UInt8Count, void const *WritePointer)
{
    CheckPointerNotNull(WriterInit, return false);
    ansi_file_writer &AnsiWriter = *((ansi_file_writer*)WriterInit);
    if(AnsiWriter.Base.CRCing)
    {
        // We're CRCing, so we need to update our current CRC value
        AddToCRC32(AnsiWriter.Base.CRC, UInt8Count, WritePointer);
    }

    if(fwrite(WritePointer, UInt8Count, 1, AnsiWriter.ANSIFileHandle) == 1)
    {
        return(true);
    }
    else
    {
        ANSILogErrorAsWarning(fwrite);
        return(false);
    }
}

void
AnsiBeginWriterCRC(file_writer* WriterInit)
{
    CheckPointerNotNull(WriterInit, return);
    ansi_file_writer &AnsiWriter = *((ansi_file_writer*)WriterInit);
    Assert(!AnsiWriter.Base.CRCing);

    AnsiWriter.Base.CRCing = true;
    BeginCRC32(AnsiWriter.Base.CRC);
}

uint32
AnsiEndWriterCRC(file_writer* WriterInit)
{
    CheckPointerNotNull(WriterInit, return 0);
    ansi_file_writer &AnsiWriter = *((ansi_file_writer*)WriterInit);
    Assert(AnsiWriter.Base.CRCing);

    AnsiWriter.Base.CRCing = false;
    EndCRC32(AnsiWriter.Base.CRC);

    return(AnsiWriter.Base.CRC);
}


file_writer *GRANNY
CreatePlatformFileWriter(char const *FileNameToOpen, bool EraseExisting)
{
    FILE *ANSIFileHandle = fopen(FileNameToOpen, EraseExisting ? "wb" : "r+b");
    if(ANSIFileHandle)
    {
        ansi_file_writer *NewWriter = Allocate(ansi_file_writer);
        if(NewWriter)
        {
            InitializeFileWriter(AnsiDeleteFileWriter,
                                 AnsiSeekWriter,
                                 AnsiWrite,
                                 AnsiBeginWriterCRC,
                                 AnsiEndWriterCRC,
                                 NewWriter->Base);
            NewWriter->ANSIFileHandle = ANSIFileHandle;

            return (file_writer*)NewWriter;
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

    return 0;
}

open_file_writer_callback *GRANNY OpenFileWriterCallback = CreatePlatformFileWriter;
