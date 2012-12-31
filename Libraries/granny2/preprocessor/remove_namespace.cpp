// ========================================================================
// $File: //jeffr/granny/preprocessor/remove_namespace.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #9 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <vector>
#include <algorithm>

using namespace std;

static char const*
ModifyName(char const* InputName,
           bool RemovingAll,
           char const* TargetNamespace)
{
    if (RemovingAll)
    {
        char const* LastColon = strrchr(InputName, ':');

        if (!LastColon)
            return InputName;

        // This isn't super awesome, but if the name /ends/ in a colon, I don't want to create an empty string...
        if (LastColon+1 != '\0')
        {
            InputName = LastColon + 1;
        }
        else
        {
            ErrOut("\"%s\" ends in a colon, leaving it alone\n", InputName);
        }
    }
    else if (TargetNamespace)
    {
        granny_int32x const TargetLen = (granny_int32x)strlen(TargetNamespace);

        if (_strnicmp(TargetNamespace, InputName, TargetLen) == 0)
        {
            InputName += TargetLen;
        }
    }

    return InputName;
}


struct VectorTrackMatch
{
    granny_vector_track* Track;
    granny_skeleton*     Skeleton;
    granny_int32x        BoneIndex;
};

static void
CreateTrackMatches(granny_file_info*         Info,
                   vector<VectorTrackMatch>& TrackMatches)
{
    // Ugly.  There's no way to do this other than brute force, really.

    {for (int AnimIdx = 0; AnimIdx < Info->AnimationCount; ++AnimIdx)
    {
        granny_animation* Animation = Info->Animations[AnimIdx];
        if (!Animation)
            continue;

        {for (int TrackGroupIdx = 0; TrackGroupIdx < Animation->TrackGroupCount; ++TrackGroupIdx)
        {
            granny_track_group* TrackGroup = Animation->TrackGroups[TrackGroupIdx];
            if (!TrackGroup)
                continue;

            {for (int VecIdx = 0; VecIdx < TrackGroup->VectorTrackCount; ++VecIdx)
            {
                granny_vector_track& Track = TrackGroup->VectorTracks[VecIdx];

                bool FoundTrack = false;
                {for (int ModelIdx = 0; ModelIdx < Info->ModelCount && !FoundTrack; ++ModelIdx)
                {
                    granny_model* Model = Info->Models[ModelIdx];
                    if (!Model)
                        continue;

                    {for (int BoneIdx = 0; BoneIdx < Model->Skeleton->BoneCount; ++BoneIdx)
                    {
                        granny_int32x Key =
                            GrannyVectorTrackKeyForBone(Model->Skeleton, BoneIdx, Track.Name);

                        if (Key == Track.TrackKey)
                        {
                            VectorTrackMatch vtm = { &Track, Model->Skeleton, BoneIdx };
                            TrackMatches.push_back(vtm);
                            FoundTrack = true;
                            break;
                        }
                    }}
                }}
            }}
        }}
    }}
}

static void
RefreshTrackKeys(granny_file_info*         Info,
                 vector<VectorTrackMatch>& TrackMatches)
{
    {for (size_t Idx = 0; Idx < TrackMatches.size(); ++Idx)
    {
        VectorTrackMatch& Match = TrackMatches[Idx];

        Match.Track->TrackKey =
            GrannyVectorTrackKeyForBone(Match.Skeleton, Match.BoneIndex, Match.Track->Name);
    }}
}


static bool
RemoveNamespaceDriver(granny_file_info* Info,
                      bool RemovingAll,
                      char const* TargetNamespace)
{
    vector<VectorTrackMatch> TrackMatches;
    CreateTrackMatches(Info, TrackMatches);

#define DO_MOD(Name) Name = ModifyName(Name, RemovingAll, TargetNamespace)
    {for(granny_int32x ModelIdx = 0; ModelIdx < Info->ModelCount; ++ModelIdx)
    {
        granny_model* Model = Info->Models[ModelIdx];
        if (!Model)
            continue;

        DO_MOD(Model->Name);

        granny_skeleton* Skeleton = Model->Skeleton;
        if (!Skeleton)
            continue;

        DO_MOD(Skeleton->Name);

        {for(granny_int32x BoneIdx = 0; BoneIdx < Skeleton->BoneCount; ++BoneIdx)
        {
            DO_MOD(Skeleton->Bones[BoneIdx].Name);
        }}
    }}

    {for(granny_int32x AnimIdx = 0; AnimIdx < Info->AnimationCount; ++AnimIdx)
    {
        granny_animation* Animation = Info->Animations[AnimIdx];
        if (!Animation)
            continue;

        {for(granny_int32x TGIdx = 0; TGIdx < Animation->TrackGroupCount; ++TGIdx)
        {
            granny_track_group* TrackGroup = Animation->TrackGroups[TGIdx];
            if (!TrackGroup)
                continue;

            DO_MOD(TrackGroup->Name);

            {for(granny_int32x TTIdx = 0; TTIdx < TrackGroup->TransformTrackCount; ++TTIdx)
            {
                DO_MOD(TrackGroup->TransformTracks[TTIdx].Name);
            }}
        }}
    }}

    {for(granny_int32x MeshIdx = 0; MeshIdx < Info->MeshCount; ++MeshIdx)
    {
        granny_mesh* Mesh = Info->Meshes[MeshIdx];
        if (!Mesh)
            continue;

        DO_MOD(Mesh->Name);

        {for(granny_int32x BBIdx = 0; BBIdx < Mesh->BoneBindingCount; ++BBIdx)
        {
            DO_MOD(Mesh->BoneBindings[BBIdx].BoneName);
        }}
    }}
#undef MOD

    RefreshTrackKeys(Info, TrackMatches);

    return true;
}

bool RemoveAllNamespaces(input_file& InputFile,
                         key_value_pair* KeyValues,
                         granny_int32x   NumKeyValues,
                         granny_memory_arena* Arena)

{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    RemoveNamespaceDriver(Info, true, 0);

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}

bool RemoveNamespace(input_file&     InputFile,
                     key_value_pair* KeyValues,
                     granny_int32x   NumKeyValues,
                     granny_memory_arena* Arena)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    char const* TargetName = FindFirstValueForKey(KeyValues, NumKeyValues, "namespace");
    if (TargetName == 0)
    {
        ErrOut("must specify a NameSpace to remove with \"-namespace <value>\"\n");
        return false;
    }

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    RemoveNamespaceDriver(Info, false, TargetName);

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}


static CommandRegistrar RegRemoveNamespace(eSingleFile_NoMultiRun,
                                           RemoveNamespace,
                                           "RemoveNamespace",
                                           "Removes a Maya namespace prefix string from animation/model/skeleton entries");

static CommandRegistrar RegRemoveAllNamespaces(eSingleFile_NoMultiRun,
                                               RemoveAllNamespaces,
                                               "RemoveAllNamespaces",
                                               "Removes all Maya namespaces from animation/model/skeleton entries");

