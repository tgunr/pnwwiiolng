// ========================================================================
// $File: //jeffr/granny/preprocessor/threshold_vector_track.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #10 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include "utilities.h"

#include <algorithm>
#include <assert.h>
#include <set>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

struct TTTimeCmp : public binary_function<granny_text_track_entry const&, granny_text_track_entry const&, bool>
{
    bool operator()(granny_text_track_entry const& One, granny_text_track_entry const& Two) const
    {
        return One.TimeStamp < Two.TimeStamp;
    }
};


void InsertEvent(vector<granny_text_track_entry>& Entries,
                 char const* String, granny_real32 Time)
{
    granny_text_track_entry Entry = { Time, String };
    Entries.push_back(Entry);
}

void ThresholdVecTrack(granny_vector_track& Track,
                       granny_real32 Duration,
                       granny_real32 TimeStep,
                       granny_real32 Threshold,
                       vector<granny_text_track_entry>& Entries,
                       granny_memory_arena* Arena)
{
    // First, let's get some things out of the way.  If the Track is constant or identity,
    // or not dimension 1, we can't deal with it
    if (GrannyCurveIsConstantOrIdentity(&Track.ValueCurve) || Track.Dimension != 1)
        return;

    char const* HiString = 0;
    char const* LoString = 0;
    {
        const int BufferSize = 512;
        char TempBuffer[BufferSize];

        sprintf(TempBuffer, "%s_hi", Track.Name);
        HiString = PushString(Arena, TempBuffer);

        sprintf(TempBuffer, "%s_lo", Track.Name);
        LoString = PushString(Arena, TempBuffer);
    }

    // Handle the first sample specially, we need to establish the first value
    // (Do we want to encode a special entry for the initial value?)
    granny_real32 LastValue;
    GrannyEvaluateCurveAtT(Track.Dimension, false, false, &Track.ValueCurve, false,
                           Duration, Duration,
                           &LastValue, NULL);

    granny_real32 CurrT = 0.0f;
    while (CurrT <= Duration)
    {
        granny_real32 NewValue;
        GrannyEvaluateCurveAtT(Track.Dimension, false, false, &Track.ValueCurve, false,
                               Duration, CurrT,
                               &NewValue, NULL);

        bool LastAbove = LastValue >= Threshold;
        bool NewAbove  = NewValue >= Threshold;

        // Is this an edge?
        if (LastAbove != NewAbove)
        {
            // Hi or Lo?
            if (NewAbove)
                InsertEvent(Entries, HiString, CurrT);
            else
                InsertEvent(Entries, LoString, CurrT);
        }

        LastValue = NewValue;
        CurrT += TimeStep;
    }
}

bool ThresholdVectorTrack(input_file& InputFile,
                          key_value_pair* KeyValues,
                          granny_int32x NumKeyValues,
                          granny_memory_arena* Arena)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file_info* Info = GrannyGetFileInfo(InputFile.GrannyFile);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input model file\n");
        return false;
    }

    if (Info->AnimationCount == 0)
    {
        ErrOut("File contains no animations\n");
        return false;
    }

    float Threshold = 0.5f;
    {
        char const* ThresholdValue = FindFirstValueForKey(KeyValues, NumKeyValues, "threshold");
        if (ThresholdValue)
            Threshold = (float)atof(ThresholdValue);
    }

    char const* TrackName = FindFirstValueForKey(KeyValues, NumKeyValues, "trackname");
    if (TrackName == 0)
    {
        ErrOut("must provide a name for the new track with -trackname\n");
        return false;
    }

    set<char const*, CIStrcmp> Tracknames;
    {
        {for(int Key = 0; Key < NumKeyValues; ++Key)
        {
            if (_stricmp(KeyValues[Key].Key, "track") == 0)
                Tracknames.insert(KeyValues[Key].Value);
        }}
    }

    if (Tracknames.empty())
    {
        ErrOut("no vectortracks to threshold\n");
        return false;
    }

    {for (int AnimIdx = 0; AnimIdx < Info->AnimationCount; ++AnimIdx)
    {
        granny_animation* Anim = Info->Animations[AnimIdx];
        assert(Anim);

        {for(int TGIdx = 0; TGIdx < Anim->TrackGroupCount; ++TGIdx)
        {
            granny_track_group* TrackGroup = Anim->TrackGroups[TGIdx];
            assert(TrackGroup);

            if (TrackGroup->VectorTrackCount == 0)
                continue;

            int NumNewVecTracks = 0;
            granny_vector_track* NewVecTracks =
                PushArray(Arena, TrackGroup->VectorTrackCount, granny_vector_track);

            int NumNewTextTracks = TrackGroup->TextTrackCount + 1;
            granny_text_track* NewTextTracks =
                PushArray(Arena, NumNewTextTracks, granny_text_track);
            memcpy(NewTextTracks, TrackGroup->TextTracks,
                   sizeof(granny_text_track) * TrackGroup->TextTrackCount);

            vector<granny_text_track_entry> TextEntries;
            {for(int VTIdx = 0; VTIdx < TrackGroup->VectorTrackCount; ++VTIdx)
            {
                if (Tracknames.find(TrackGroup->VectorTracks[VTIdx].Name) == Tracknames.end())
                {
                    NewVecTracks[NumNewVecTracks++] = TrackGroup->VectorTracks[VTIdx];
                    assert(NumNewVecTracks <= TrackGroup->VectorTrackCount);
                }
                else
                {
                    // This needs to be thresholded and entered into the texttrack.  Don't
                    // put it back into the vector tracks...
                    ThresholdVecTrack(TrackGroup->VectorTracks[VTIdx],
                                      Anim->Duration, Anim->TimeStep, Threshold,
                                      TextEntries, Arena);
                }
            }}

            // Sort the entries by time
            sort(TextEntries.begin(), TextEntries.end(), TTTimeCmp());

            // At the end, create the new vector track...
            granny_text_track& NewTrack = NewTextTracks[TrackGroup->TextTrackCount];
            NewTrack.Name = TrackName;
            NewTrack.EntryCount = (granny_int32)TextEntries.size();
            NewTrack.Entries = (granny_text_track_entry*)
                GrannyMemoryArenaPushBinary(Arena, NewTrack.EntryCount * sizeof(granny_text_track_entry),
                                            &TextEntries[0]);

            TrackGroup->VectorTrackCount = NumNewVecTracks;
            TrackGroup->VectorTracks = NewVecTracks;

            TrackGroup->TextTrackCount = NumNewTextTracks;
            TrackGroup->TextTracks = NewTextTracks;
        }}
    }}

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, InputFile.GrannyFile, true, true, true);
}

static CommandRegistrar RegThresholdVectorTrack(eSingleFile_NoMultiRun,
                                                ThresholdVectorTrack,
                                                "ThresholdVectorTrack",
                                                "Make text_tracks from vector_tracks");

