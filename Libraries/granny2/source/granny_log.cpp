// ========================================================================
// $File: //jeffr/granny/rt/granny_log.cpp $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #21 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_log.h"

#include "granny_file_operations.h"
#include "granny_file_writer.h"
#include "granny_limits.h"
#include "granny_memory.h"
#include "granny_string.h"
#include "granny_string_formatting.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

log_callback LogCallback;

END_GRANNY_NAMESPACE;


#if GRANNY_NO_LOGGING

bool GRANNY
SetLogFileName(char const *FileName, bool Clear)
{
    GRANNY_ENTER();

    return false;
}

void GRANNY
GetLogCallback(log_callback* Result)
{
    GRANNY_ENTER();

    Result->Function = 0;
    Result->UserData = 0;
}

void GRANNY
SetLogCallback(log_callback const &LogCallbackInit)
{
    GRANNY_ENTER();

}

bool GRANNY
Logging(void)
{
    GRANNY_ENTER();

    return false;
}

void GRANNY
FilterMessage(log_message_origin Origin, bool Enabled)
{
    GRANNY_ENTER();

}

void GRANNY
FilterAllMessages(bool Enabled)
{
    GRANNY_ENTER();

}

char const* GRANNY
GetLogMessageTypeString(log_message_type Type)
{
    GRANNY_ENTER();

    return "UnknownMessageType";
}

char const* GRANNY
GetLogMessageOriginString(log_message_origin Origin)
{
    GRANNY_ENTER();

    return "UnknownMessageOrigin";
}


#else //#if GRANNY_NO_LOGGING



static bool LogIsDisabled[OnePastLastMessageOrigin];
static char LogFileName[MaximumLogFileNameSize] = { 0 };
static char TempMessageBuffer[MaximumMessageBufferSize];

bool GRANNY
SetLogFileName(char const *FileName, bool Clear)
{
    GRANNY_ENTER();

    bool Result = true;

    if (FileName)
    {
        StringEquals(LogFileName, SizeOf(LogFileName), FileName);
    }
    else
    {
        LogFileName[0] = '\0';
    }

    if (*LogFileName)
    {
        file_writer *Writer = NewFileWriter(FileName, Clear);
        if (Writer)
        {
            DeleteFileWriter(Writer);
        }
        else
        {
            // Unable to open the file.  Don't try later...
            LogFileName[0] = '\0';
            Result = false;
        }
    }

    return Result;
}


void GRANNY
GetLogCallback(log_callback* Result)
{
    GRANNY_ENTER();

    Assert(Result);

    *Result = LogCallback;
}

void GRANNY
SetLogCallback(log_callback const &LogCallbackInit)
{
    GRANNY_ENTER();

    LogCallback = LogCallbackInit;
}

bool GRANNY
Logging(void)
{
    GRANNY_ENTER();

    return (*LogFileName || LogCallback.Function);
}

static void
WriteMessage(log_message_type Type,
             log_message_origin Origin,
             char const* File, int32x Line,
             int32x LengthToEndOfFile,
             int32x Length,
             char const *Message)
{
    GRANNY_ENTER();

    // Dump out to file if necessary...
    if (*LogFileName)
    {
        static bool AvoidInfiniteRecursion = true;
        if (AvoidInfiniteRecursion)
        {
            AvoidInfiniteRecursion = false;
            AppendStringToFile(LogFileName, Message);
            AvoidInfiniteRecursion = true;
        }
    }

    if(LogCallback.Function)
    {
        LogCallback.Function(Type, Origin, File, Line,
                             Message + LengthToEndOfFile,
                             LogCallback.UserData);
    }
}

void GRANNY
_Log(log_message_type Type, log_message_origin Origin,
     char const* File, int32x Line,
     char const *FormatString, ...)
{
    GRANNY_ENTER();

    Assert(Type < OnePastLastMessageType);
    Assert(Origin < OnePastLastMessageOrigin);

    if(Type != IgnoredLogMessage)
    {
        // This message is a regular log message, so we write it
        // directly to the log file.
        if(!LogIsDisabled[Origin] && Logging())
        {
            c_argument_list Args;
            OpenArgumentList(Args, FormatString);

            int32x const LengthToFileMarker =
                ConvertToStringVar(SizeOf(TempMessageBuffer),
                                   TempMessageBuffer,
                                   "%s(%d): ", File, Line);
            int32x const LengthOfMessage =
                ConvertToStringList(SizeOf(TempMessageBuffer) - LengthToFileMarker,
                                    TempMessageBuffer + LengthToFileMarker,
                                    FormatString, Args);
            CloseArgumentList(Args);

            WriteMessage(Type, Origin, File, Line,
                         LengthToFileMarker,
                         LengthToFileMarker + LengthOfMessage,
                         TempMessageBuffer);
        }
    }
}

void GRANNY
FilterMessage(log_message_origin Origin, bool Enabled)
{
    GRANNY_ENTER();

    Assert(Origin < OnePastLastMessageOrigin);
    LogIsDisabled[Origin] = !Enabled;
}

void GRANNY
FilterAllMessages(bool Enabled)
{
    GRANNY_ENTER();

    {for(int32x OriginIndex = 0;
         OriginIndex < OnePastLastMessageOrigin;
         ++OriginIndex)
    {
        LogIsDisabled[OriginIndex] = !Enabled;
    }}
}

#define ENUM_STRING_CASE(x) case x ## LogMessage: return #x

char const* GRANNY
GetLogMessageTypeString(log_message_type Type)
{
    GRANNY_ENTER();

    switch (Type)
    {
        ENUM_STRING_CASE(Ignored);
        ENUM_STRING_CASE(Note);
        ENUM_STRING_CASE(Warning);
        ENUM_STRING_CASE(Error);

        default:
            return "UnknownMessageType";
    }
}

char const* GRANNY
GetLogMessageOriginString(log_message_origin Origin)
{
    GRANNY_ENTER();

    switch (Origin)
    {
        ENUM_STRING_CASE(NotImplemented);
        ENUM_STRING_CASE(Application);
        ENUM_STRING_CASE(Win32Subsystem);
        ENUM_STRING_CASE(Win64Subsystem);
        ENUM_STRING_CASE(WinXXSubsystem);
        ENUM_STRING_CASE(MacOSSubsystem);
        ENUM_STRING_CASE(ANSISubsystem);
        ENUM_STRING_CASE(GamecubeSubsystem);
        ENUM_STRING_CASE(PS2Subsystem);
        ENUM_STRING_CASE(PSPSubsystem);
        ENUM_STRING_CASE(PS3Subsystem);
        ENUM_STRING_CASE(XboxSubsystem);
        ENUM_STRING_CASE(XenonSubsystem);
        ENUM_STRING_CASE(MAXSubsystem);
        ENUM_STRING_CASE(MayaSubsystem);
        ENUM_STRING_CASE(XSISubsystem);
        ENUM_STRING_CASE(ModoSubsystem);
        ENUM_STRING_CASE(LightwaveSubsystem);
        ENUM_STRING_CASE(FileWriting);
        ENUM_STRING_CASE(FileReading);
        ENUM_STRING_CASE(Exporter);
        ENUM_STRING_CASE(Compressor);
        ENUM_STRING_CASE(String);
        ENUM_STRING_CASE(StringTable);
        ENUM_STRING_CASE(VertexLayout);
        ENUM_STRING_CASE(Mesh);
        ENUM_STRING_CASE(Property);
        ENUM_STRING_CASE(Skeleton);
        ENUM_STRING_CASE(Animation);
        ENUM_STRING_CASE(SetupGraph);
        ENUM_STRING_CASE(Texture);
        ENUM_STRING_CASE(BSpline);
        ENUM_STRING_CASE(Hash);
        ENUM_STRING_CASE(Linker);
        ENUM_STRING_CASE(Instantiator);
        ENUM_STRING_CASE(DataType);
        ENUM_STRING_CASE(NameMap);
        ENUM_STRING_CASE(Material);
        ENUM_STRING_CASE(Model);
        ENUM_STRING_CASE(StackAllocator);
        ENUM_STRING_CASE(FixedAllocator);
        ENUM_STRING_CASE(Scene);
        ENUM_STRING_CASE(TrackMask);
        ENUM_STRING_CASE(LocalPose);
        ENUM_STRING_CASE(WorldPose);
        ENUM_STRING_CASE(NameLibrary);
        ENUM_STRING_CASE(Control);
        ENUM_STRING_CASE(MeshBinding);
        ENUM_STRING_CASE(Math);
        ENUM_STRING_CASE(Version);
        ENUM_STRING_CASE(Memory);
        ENUM_STRING_CASE(Deformer);
        ENUM_STRING_CASE(Voxel);
        ENUM_STRING_CASE(Bitmap);
        ENUM_STRING_CASE(IK);
        ENUM_STRING_CASE(Curve);
        ENUM_STRING_CASE(TrackGroup);
        ENUM_STRING_CASE(ThreadSafety);
        ENUM_STRING_CASE(Quantize);
        ENUM_STRING_CASE(BlendDag);
        ENUM_STRING_CASE(Assertion);
        ENUM_STRING_CASE(ArenaAllocator);
        ENUM_STRING_CASE(FileOperation);
        ENUM_STRING_CASE(Camera);

        default:
            return "UnknownMessageOrigin";
    }

}

#endif //#else //#if GRANNY_NO_LOGGING


