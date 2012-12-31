// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_spu_animation.h"

#include "granny_spu_track_group.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

data_type_definition GRANNY SPUAnimationType[] =
{
    {StringMember, "Name"},

    {Real32Member, "Duration"},
    {Real32Member, "TimeStep"},
    {Real32Member, "Oversampling"},

    {ArrayOfReferencesMember, "TrackGroups", SPUTrackGroupType},

    {EndMember},
};

bool GRANNY
FindSPUTrackGroupForModel(spu_animation const& Animation,
                          char const* ModelName,
                          int32x& TrackGroupIndex)
{
    GRANNY_ENTER();

    {for(TrackGroupIndex = 0;
         TrackGroupIndex < Animation.TrackGroupCount;
         ++TrackGroupIndex)
    {
        if (StringsAreEqualLowercaseOrCallback(
                Animation.TrackGroups[TrackGroupIndex]->Name,
                ModelName))
        {
            return true;
        }
    }}

    return false;
}
