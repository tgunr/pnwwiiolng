// ========================================================================
// $File: //jeffr/granny/rt/granny_file_writer.cpp $
// $DateTime: 2010/02/19 12:25:19 $
// $Change: 25257 $
// $Revision: #16 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_file_writer.h"

#include "granny_assert.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#define SubsystemCode FileWritingLogMessage

USING_GRANNY_NAMESPACE;

bool GRANNY InitializeFileWriter(delete_file_writer_callback*   DeleteFileWriterCallback,
                                 seek_file_writer_callback*     SeekWriterCallback,
                                 write_file_writer_callback*    WriteCallback,
                                 begincrc_file_writer_callback* BeginCRCCallback,
                                 endcrc_file_writer_callback*   EndCRCCallback,
                                 file_writer& Writer)
{
    GRANNY_ENTER();

    CheckPointerNotNull(DeleteFileWriterCallback, return false);
    CheckPointerNotNull(SeekWriterCallback,       return false);
    CheckPointerNotNull(WriteCallback,            return false);
    CheckPointerNotNull(BeginCRCCallback,         return false);
    CheckPointerNotNull(EndCRCCallback,           return false);

    Writer.DeleteFileWriterCallback = DeleteFileWriterCallback;
    Writer.SeekWriterCallback       = SeekWriterCallback;
    Writer.WriteCallback            = WriteCallback;
    Writer.BeginCRCCallback         = BeginCRCCallback;
    Writer.EndCRCCallback           = EndCRCCallback;

    Writer.CRCing = false;
    Writer.CRC    = 0;

    return true;
}

open_file_writer_callback *GRANNY
GetDefaultFileWriterOpenCallback()
{
    GRANNY_ENTER();

    return(OpenFileWriterCallback);
}

void GRANNY
SetDefaultFileWriterOpenCallback(open_file_writer_callback *OpenFileWriterCallbackInit)
{
    GRANNY_ENTER();

    OpenFileWriterCallback = OpenFileWriterCallbackInit;
}


int32x GRANNY
PredictWriterAlignment(int32x Position, int32x Alignment)
{
    GRANNY_ENTER();

    // Bad.
    CheckCondition((Alignment & (Alignment-1)) == 0, return Position);

    // Determine where we are, and where we'd like to be
    int32x const NewPosition = (Position + (Alignment-1)) & ~(Alignment-1);

    // Return the difference
    return(NewPosition - Position);
}

int32x GRANNY
AlignWriterTo(file_writer* Writer, uint32x Alignment)
{
    GRANNY_ENTER();

    return AlignWriterToWith(Writer, Alignment, 0);
}

int32x GRANNY
AlignWriterToWith(file_writer* Writer, uint32x Alignment, uint8 PadByte)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Writer, return 0);
    CheckCondition((Alignment & (Alignment - 1)) == 0, return 0);

    // Determine how far we'll have to go to get to where we want to be
    uint32 Position = GetWriterPosition(Writer);
    uint32 Aligned  = (Position + (Alignment-1)) & ~(Alignment-1);
    Assert(Aligned >= Position);
    Assert((Aligned - Position) < Alignment);

    int32x const Correction = Aligned - Position;

    while (Position != Aligned)
    {
        Write(Writer, 1, &PadByte);
        ++Position;
    }

    return Correction;
}

int32x GRANNY
SeekWriterFromStartStub(file_writer* Writer,
                        int32x OffsetInUInt8s)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Writer, return 0);
    return (*Writer->SeekWriterCallback)(Writer, OffsetInUInt8s, SeekStart);
}

int32x GRANNY
SeekWriterFromEndStub(file_writer* Writer,
                      int32x OffsetInUInt8s)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Writer, return 0);
    return (*Writer->SeekWriterCallback)(Writer, OffsetInUInt8s, SeekEnd);
}

int32x GRANNY
SeekWriterFromCurrentPositionStub(file_writer* Writer,
                                  int32x OffsetInUInt8s)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Writer, return 0);
    return (*Writer->SeekWriterCallback)(Writer, OffsetInUInt8s, SeekCurrent);
}
