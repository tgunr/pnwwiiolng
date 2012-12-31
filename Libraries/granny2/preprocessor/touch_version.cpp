// ========================================================================
// $File: //jeffr/granny/preprocessor/touch_version.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #8 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "touch_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool TouchVersion(input_file&      InputFile,
                   key_value_pair* KeyValues,
                   granny_int32x   NumKeyValues,
                   granny_memory_arena*)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, true);
}

static CommandRegistrar RegTouchVersion(eSingleFile_NoMultiRun,
                                        TouchVersion,
                                        "TouchVersion",
                                        "Updates the specified file to the current file format.");

