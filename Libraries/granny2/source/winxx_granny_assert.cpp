// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_assert.cpp $
// $DateTime: 2009/10/29 14:36:42 $
// $Change: 23792 $
// $Revision: #4 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// Covers full file
#if DEBUG

#include "winxx_granny_windows.h"
#include "granny_assert.h"
#include "granny_limits.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

static char AssertionBuffer[MaximumMessageBufferSize];
bool GRANNY
DisplayAssertion(char const * const Expression,
                 char const * const File,
                 int32x const LineNumber,
                 char const * const Function,
                 bool* IgnoredPtr,
                 void* /*UserData*/)
{
    bool& Ignored = *IgnoredPtr;
    if (Ignored)
        return false;

    wsprintfA(AssertionBuffer,
             "Expression: %s\r\n"
             "File: %s\r\n"
             "Line: %d\r\n"
             "Function: %s\r\n"
             "\r\n"
             "(Abort = debug, Retry = ignore once, Ignore = ignore forever)\r\n",
             Expression, File, LineNumber, Function);

    DWORD ReturnCode = WinXXPopMessageBox(AssertionBuffer, "Run-time Error", eAbortRetryIgnore);
    switch (ReturnCode)
    {
        default:
        case IDABORT:
        {
            // We break into the debugger
            return true;
        } break;

        case IDIGNORE:
        {
            // We don't ever assert here again
            Ignored = true;
            return false;
        } break;

        case IDRETRY:
        {
            // We let things go
            return false;
        } break;
    }
}

#endif // DEBUG
