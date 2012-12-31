// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_controlled_animation.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #13 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_spu_controlled_animation.h"

#include "granny_aggr_alloc.h"
#include "granny_control.h"
#include "granny_model.h"
#include "granny_model_instance.h"
#include "granny_parameter_checking.h"
#include "granny_retargeter.h"
#include "granny_spu_animation.h"
#include "granny_spu_animation_binding.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#undef SubsystemCode
#define SubsystemCode ControlLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;

struct spu_track_target
{
    spu_animation_binding* Binding;
    spu_animation_binding_id ID;

    model_instance *OnInstance;
    accumulation_mode AccumulationMode;

    track_mask *TrackMask;
    track_mask *ModelMask;
};

struct controlled_spu_animation_builder
{
    real32 CurrentClock;
    spu_animation const *Animation;

    int32x TrackCount;
    spu_track_target *Tracks;
};

END_GRANNY_NAMESPACE;

static bool
SPUAnimationInitializeBindingState(model_control_binding &Binding,
                                   void *InitData)
{
    GRANNY_ENTER();

    VAR_FROM_DERIVED(spu_controlled_animation, State, Binding);
    Assert(State);

    spu_track_target *TrackTarget = (spu_track_target *)InitData;
    Assert(TrackTarget);

    State->Binding = AcquireSPUAnimationBindingFromID(TrackTarget->ID);
    State->AccumulationMode = TrackTarget->AccumulationMode;
    State->TrackMask = TrackTarget->TrackMask;
    State->ModelMask = TrackTarget->ModelMask;

    Assert(State->Binding);
    Assert(State->Binding->TrackNameRemaps);

    Binding.BindingType = SPUControlledAnim;

    return(State->Binding != 0);
}

static void
SPUAnimationCleanupBindingState(model_control_binding &Binding)
{
    GRANNY_ENTER();

    VAR_FROM_DERIVED(spu_controlled_animation, State, Binding);
    Assert(State);

    ReleaseSPUAnimationBinding(State->Binding);
}

static bool
SPUAnimationControlIsRebased(model_control_binding &Binding)
{
    GRANNY_ENTER();

    VAR_FROM_DERIVED(spu_controlled_animation, State, Binding);
    Assert(State);

    return State && State->Binding && State->Binding->Retargeter != 0;
}


static model_control_callbacks ControlledAnimationCallbacks =
{
    NULL,                               // AnimationGetControlledAnimation,
    NULL,                               // AnimationGetControlledPose,
    SPUAnimationInitializeBindingState,
    NULL,                               // AnimationAccumulateBindingState,
    NULL,                               // AnimationBuildDirect,
    NULL,                               // AnimationAccumulateLoopTransform,
    SPUAnimationCleanupBindingState,    // AnimationCleanupBindingState,
    SPUAnimationControlIsRebased
};


control* GRANNY
PlayControlledSPUAnimation(real32 StartTime,
                           spu_animation const& Animation,
                           model_instance& Model)
{
    GRANNY_ENTER();

    control *Result = 0;

    int32x TrackGroupIndex;
    if (FindSPUTrackGroupForModel(Animation,
                                  GetSourceModel(Model).Name,
                                  TrackGroupIndex))
    {
        controlled_spu_animation_builder *Builder =
            BeginControlledSPUAnimation(StartTime, Animation);
        if(Builder)
        {
            SetSPUTrackGroupTarget(*Builder, TrackGroupIndex, Model);
            Result = EndControlledSPUAnimation(Builder);
        }
    }

    return Result;
}


controlled_spu_animation_builder *GRANNY
BeginControlledSPUAnimation(real32 StartTime, spu_animation const &Animation)
{
    GRANNY_ENTER();

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    controlled_spu_animation_builder* Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder,
                            Animation.TrackGroupCount,
                            TrackCount, Tracks);
    if(EndAggrAlloc(Allocator))
    {
        Builder->CurrentClock = StartTime;
        Builder->Animation = &Animation;

        {for(int32x TrackIndex = 0;
             TrackIndex < Builder->TrackCount;
             ++TrackIndex)
        {
            spu_track_target &Track = Builder->Tracks[TrackIndex];
            ZeroStructure(Track);

            // TODO: accumulation
        }}
    }

    return Builder;
}


control *GRANNY
EndControlledSPUAnimation(controlled_spu_animation_builder *Builder)
{
    GRANNY_ENTER();

    control* Control = 0;
    if(Builder)
    {
        // Create the control
        Control = CreateControl(Builder->CurrentClock,
                                Builder->Animation->Duration);
        if(Control)
        {
            // Create the bindings
            {for (int32x TrackIndex = 0;
                  TrackIndex < Builder->TrackCount;
                  ++TrackIndex)
            {
                spu_track_target &Track = Builder->Tracks[TrackIndex];
                if (Track.OnInstance)
                {
                     model_control_binding *Binding =
                         CreateModelControlBinding(ControlledAnimationCallbacks, *Control,
                                                   *Track.OnInstance,
                                                   ControlIsActive(*Control), &Track);
                     if (!Binding)
                         Log0(ErrorLogMessage, ControlLogMessage,
                              "Unable to bind track group");
                }
            }}
        }
        else
        {
            Log0(ErrorLogMessage, ControlLogMessage,
                 "Unable to create control");
        }

        Deallocate(Builder);
    }

    return Control;
}

void GRANNY
SetSPUTrackGroupTarget(controlled_spu_animation_builder& Builder,
                       int32x TrackGroupIndex,
                       model_instance& Model)
{
    GRANNY_ENTER();

    CheckInt32Index(TrackGroupIndex, Builder.TrackCount, return);
    spu_track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.OnInstance = &Model;

    MakeDefaultSPUAnimationBindingID(Track.ID, Builder.Animation, TrackGroupIndex, &GetSourceModel(Model));
//     Track.ID.Model = &GetSourceModel(Model);
//     Track.ID.Animation = Builder.Animation;
//     Track.ID.TrackGroup = Builder.Animation->TrackGroups[TrackGroupIndex];
//     Track.ID.TransformTrackCount = Track.ID.TrackGroup->TransformTrackCount;
//     Track.ID.TransformTracks     = Track.ID.TrackGroup->TransformTracks;
//     Track.ID.CurveByteCount      = Track.ID.TrackGroup->CurveByteCount;
//     Track.ID.CurveBytes          = Track.ID.TrackGroup->CurveBytes;
}

void GRANNY
SetSPUTrackGroupTrackMask(controlled_spu_animation_builder &Builder,
                          int32x TrackGroupIndex,
                          track_mask &TrackMask)
{
    GRANNY_ENTER();

    CheckInt32Index(TrackGroupIndex, Builder.TrackCount, return);
    spu_track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.TrackMask = &TrackMask;
}

void GRANNY
SetSPUTrackGroupModelMask(controlled_spu_animation_builder &Builder,
                          int32x TrackGroupIndex,
                          track_mask &ModelMask)
{
    GRANNY_ENTER();

    CheckInt32Index(TrackGroupIndex, Builder.TrackCount, return);
    spu_track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.ModelMask = &ModelMask;
}

void GRANNY
SetSPUTrackGroupBasisTransform(controlled_spu_animation_builder &Builder,
                               int32x TrackGroupIndex,
                               model &FromModel, model &ToModel)
{
    GRANNY_ENTER();

    CheckInt32Index(TrackGroupIndex, Builder.TrackCount, return);
    spu_track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.ID.FromBasis = &FromModel;
    Track.ID.ToBasis = &ToModel;
}


