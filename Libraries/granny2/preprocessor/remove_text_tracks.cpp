// ========================================================================
// $File: //jeffr/granny/preprocessor/remove_text_tracks.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>


bool RemoveTextTracks(input_file&     InputFile,
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

    // Remove all text tracks from each track group
    {for(granny_int32x TGIdx = 0; TGIdx < Info->TrackGroupCount; ++TGIdx)
    {
        granny_track_group* TrackGroup = Info->TrackGroups[TGIdx];
        TrackGroup->TextTrackCount = 0;
        TrackGroup->TextTracks = 0;
    }}

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}

static CommandRegistrar RegRemoveTextTracks(eSingleFile_NoMultiRun,
                                            RemoveTextTracks,
                                            "RemoveTextTracks",
                                            "Removes any text tracks contained in the file");
