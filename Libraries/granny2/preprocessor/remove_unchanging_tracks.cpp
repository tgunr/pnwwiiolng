// ========================================================================
// $File: //jeffr/granny/preprocessor/remove_unchanging_tracks.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #11 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include "utilities.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

void CleanTrackGroup(granny_track_group* TrackGroup,
                     granny_model* Model,
                     granny_memory_arena* Arena)
{
    assert(TrackGroup);
    assert(Model);
    assert(Model->Skeleton);

    vector<granny_transform_track> RemainingTracks;
    vector<granny_real32> FinalLODs;
    {for(granny_int32x Idx = 0; Idx < TrackGroup->TransformTrackCount; ++Idx)
    {
        granny_transform_track& Track = TrackGroup->TransformTracks[Idx];
        bool strip = false;
        if (GrannyCurveIsConstantOrIdentity(&Track.OrientationCurve) &&
            GrannyCurveIsConstantOrIdentity(&Track.PositionCurve) &&
            GrannyCurveIsConstantOrIdentity(&Track.ScaleShearCurve))
        {
            // Find the corresponding bone on the model...
            granny_int32x BoneIdx;
            if (GrannyFindBoneByNameLowercase(Model->Skeleton, Track.Name, &BoneIdx))
            {
                granny_bone& Bone = Model->Skeleton->Bones[BoneIdx];

                granny_transform TrackTransform;
                GrannyGetTrackInitialTransform(&Track, &TrackTransform);

                // This is a bit lame, it only uses the exporter defaults.  We need a way
                // to explicitly control the tolerance here..
                float PosError = GetPosError(Bone.LocalTransform, TrackTransform);
                float OriError = GetOriError(Bone.LocalTransform, TrackTransform);
                float SSError  = GetSSError(Bone.LocalTransform, TrackTransform);

                if (PosError < 0.01f &&
                    OriError < GrannyOrientationToleranceFromDegrees(0.1f) &&
                    SSError  < 0.001f)
                {
                    strip = true;
                }
            }
        }

        if (!strip)
        {
            RemainingTracks.push_back(Track);
            if (TrackGroup->TransformLODErrorCount != 0)
                FinalLODs.push_back(TrackGroup->TransformLODErrors[Idx]);
        }
    }}

    if (RemainingTracks.size() != TrackGroup->TransformTrackCount)
    {
        TrackGroup->TransformTrackCount = (granny_int32)RemainingTracks.size();
        TrackGroup->TransformTracks =
            PushArray(Arena, TrackGroup->TransformTrackCount, granny_transform_track);
        memcpy(TrackGroup->TransformTracks, &RemainingTracks[0],
               sizeof(granny_transform_track) * RemainingTracks.size());

        if (FinalLODs.size())
        {
            assert(FinalLODs.size() == RemainingTracks.size());

            TrackGroup->TransformLODErrorCount = (granny_int32)FinalLODs.size();
            TrackGroup->TransformLODErrors =
                PushArray(Arena, TrackGroup->TransformLODErrorCount, granny_real32);
            memcpy(TrackGroup->TransformLODErrors, &FinalLODs[0],
                   sizeof(granny_real32) * TrackGroup->TransformLODErrorCount);
        }
    }
}


bool RemoveUnchangingTracks(input_file* InputFiles, granny_int32x NumInputFiles,
                            key_value_pair* KeyValues, granny_int32x NumKeyValues,
                            granny_memory_arena* Arena)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    if (NumInputFiles != 2)
    {
        ErrOut("only valid for 2 input files\n");
        return false;
    }

    granny_file_info* ModelInfo = GrannyGetFileInfo(InputFiles[0].GrannyFile);
    if (ModelInfo == 0)
    {
        ErrOut("unable to obtain a granny_file_info from: %s\n",
                InputFiles[0].Filename);
        return false;
    }

    granny_file* AnimFile = InputFiles[1].GrannyFile;
    granny_file_info* AnimInfo = GrannyGetFileInfo(AnimFile);
    if (AnimInfo == 0)
    {
        ErrOut("unable to obtain a granny_file_info from: %s\n",
                InputFiles[1].Filename);
        return false;
    }


    {for(granny_int32x AnimIdx = 0; AnimIdx < AnimInfo->AnimationCount; ++AnimIdx)
    {
        granny_animation* Animation = AnimInfo->Animations[AnimIdx];
        if (!Animation)
            continue;

        {for(granny_int32x TGIdx = 0; TGIdx < Animation->TrackGroupCount; ++TGIdx)
        {
            granny_track_group* TrackGroup = Animation->TrackGroups[TGIdx];
            if (!TrackGroup)
                continue;

            // Find the model for this track group
            bool Cleaned = false;
            {for(granny_int32x ModelIdx = 0; !Cleaned && ModelIdx < ModelInfo->ModelCount; ++ModelIdx)
            {
                granny_model* Model = ModelInfo->Models[ModelIdx];
                if (!Model)
                    continue;

                if (_stricmp(Model->Name, TrackGroup->Name) == 0)
                {
                    CleanTrackGroup(TrackGroup, Model, Arena);
                    Cleaned = true;
                }
            }}

            if (!Cleaned)
            {
                ErrOut("failed to find a model for track_group: %s\n",
                        TrackGroup->Name);
            }
        }}
    }}


    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, AnimInfo, AnimFile, true, true, false);
}


static CommandRegistrar RegCombineElements(RemoveUnchangingTracks, "RemoveUnchangingTracks",
                                           "Removes constant tracks from an animation that are the same as the base pose of the model");
