// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_track_group.cpp $
// $DateTime: 2007/12/04 10:49:34 $
// $Change: 16700 $
// $Revision: #7 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_spu_track_group.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

#undef SubsystemCode
#define SubsystemCode TrackGroupLogMessage

BEGIN_GRANNY_NAMESPACE;

data_type_definition SPUTransformTrackType[] =
{
    {UInt32Member, "FromNameIndex"},
    {UInt32Member, "Flags"},

    {Real32Member,    "AnimLODValue"},
    {TransformMember, "Transform" },

    {UInt32Member,  "PosCurveOffset"},
    {UInt32Member,  "OriCurveOffset"},
    {UInt32Member,  "SSCurveOffset"},

    {UInt32Member, "Reserved"},

    {EndMember},
};

data_type_definition SPUTrackGroupType[] =
{
    { StringMember, "Name" },
    { Int32Member,  "Flags" },

    { ReferenceToArrayMember, "TrackNames",      StringType },
    { ReferenceToArrayMember, "TransformTracks", SPUTransformTrackType },
    { ReferenceToArrayMember, "CurveBytes",      UInt8Type },

    { EndMember },
};

END_GRANNY_NAMESPACE;
