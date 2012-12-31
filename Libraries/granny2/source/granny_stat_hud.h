#if !defined(GRANNY_STAT_HUD_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_stat_hud.h $
// $DateTime: 2008/11/25 15:29:40 $
// $Change: 19567 $
// $Revision: #14 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_ANIMATION_BINDING_H)
#include "granny_animation_binding.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(StatisticsGroup);

// TODO: I don't like having this here, but depending on
// the input order to CRAPI, this can be defined in granny.h
// /after/ the stat_hud structure, which is obviously wrong.
// CRAPI should be doing a tree walk to get the dependencies
// right.
EXPTYPE_EPHEMERAL struct animation_binding_cache_status
{
    int32x TotalBindingsCreated;
    int32x TotalBindingsDestroyed;

    int32x DirectAcquireCount;
    int32x IndirectAcquireCount;
    int32x ReleaseCount;

    int32x CurrentTotalBindingCount;
    int32x CurrentUsedBindingCount;

    int32x CacheHits;
    int32x CacheMisses;

    int32x ExplicitFlushCount;
    int32x ExplicitFlushFrees;
    int32x OverflowFrees;
};


EXPTYPE_EPHEMERAL struct stat_hud_alloc_point
{
    char *SourceFilename;
    int32x SourceLineNumber;

    intaddrx AllocationCount;
    intaddrx BytesRequested;
    intaddrx BytesAllocated;
};

EXPTYPE_EPHEMERAL struct stat_hud_model_instances
{
    int32x TotalInstanceCount;
    int32x TotalInstancedBoneCount;
    int32x TotalUsedModelCount;

    int32x MaxInstanceCount;
    int32x MaxBoneCount;

    int32x MaxInstanceControlCount;
};

EXPTYPE_EPHEMERAL struct stat_hud_model_controls
{
    int32x TotalControlCount;
    int32x ActiveControlCount;
    int32x ActiveAndWeightedControlCount;
    int32x ActiveAndWeightedButEasedOutControlCount;

    int32x CompletableControlCount;
    int32x CompletedControlCount;
    int32x UnusedControlCount;

    real32 MinClockTime;
    real32 MaxClockTime;

    real32 MinCompletionTime;
    real32 MaxCompletionTime;
};

EXPTYPE_EPHEMERAL struct stat_hud_footprint
{
    intaddrx TotalAllocationCount;
    intaddrx TotalBytesRequested;
    intaddrx TotalBytesAllocated;
};

EXPTYPE_EPHEMERAL struct stat_hud_animation_types
{
    int32x TotalTrackCount;

    // [Position][Rotation][Scale/shear][Rebased]
    int32x TrackTypes[3][3][3][2];
};

EXPTYPE_EPHEMERAL struct stat_hud
{
    // TODO: Vertex deformation stats
    // TODO: File stats time stats

    // The number of frames this snapshot represents
    int32x FrameCount;

    // Model instances
    stat_hud_model_instances ModelInstances;

    // Animations
    stat_hud_model_controls Controls;
    animation_binding_cache_status AnimBindingCacheStatus;
    stat_hud_animation_types AnimTypes;

    // Memory footprint
    stat_hud_footprint Footprint;
    int32x AllocPointCount;
    stat_hud_alloc_point *AllocPoints;
};

EXPAPI GS_MODIFY stat_hud *CaptureCurrentStats(int32x FramesSinceLastCapture);
EXPAPI GS_PARAM   void FreeStats(stat_hud *StatHUD);

EXPAPI GS_MODIFY char **DumpStatHUD(stat_hud &StatHUD);
EXPAPI GS_PARAM   void FreeStatHUDDump(char **StatHUDDump);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STAT_HUD_H
#endif
