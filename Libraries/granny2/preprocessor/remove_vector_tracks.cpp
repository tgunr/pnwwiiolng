// ========================================================================
// $File: //jeffr/granny/preprocessor/remove_vector_tracks.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #13 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>

bool RemoveVectorTracks(input_file&     InputFile,
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

    // Remove all vector tracks from each track group
    {for(granny_int32x TGIdx = 0; TGIdx < Info->TrackGroupCount; ++TGIdx)
    {
        granny_track_group* TrackGroup = Info->TrackGroups[TGIdx];
        TrackGroup->VectorTrackCount = 0;
        TrackGroup->VectorTracks = 0;
    }}

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}

static CommandRegistrar RegRemoveVectorTracks(eSingleFile_NoMultiRun,
                                              RemoveVectorTracks,
                                              "RemoveVectorTracks",
                                              "Removes any vector tracks contained in the file");

