// ========================================================================
// $File: //jeffr/granny/rt/granny_control.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #36 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_control.h"

#include "granny_animation_binding.h"
#include "granny_controlled_animation.h"
#include "granny_controlled_pose.h"
#include "granny_fixed_allocator.h"
#include "granny_floats.h"
#include "granny_math.h"
#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#undef SubsystemCode
#define SubsystemCode ControlLogMessage

USING_GRANNY_NAMESPACE;


// Purely a debugging function to make sure these two are in valid ranges.
static void
CheckLocalClockAndLoopIndex ( control &Control )
{
    GRANNY_ENTER();

#if DEBUG
#if !COMPILER_GCC
    // Due to small precision errors when doing modulo arithmetic,
    // these values can sometimes be epsilon out. You may need to
    // tweak epsilon according to your app (or processor!)
    real32 const Epsilon = 1e-6f;
    if ( Control.LoopCount == 0 )
    {
        // Infinite looping.
        Assert ( Control.LocalClock >= 0.0f - Epsilon );
        Assert ( Control.LocalClock < Control.LocalDuration + Epsilon );
    }
    else
    {
        // Fixed number of loops.
        Assert ( ( Control.LoopIndex >= 0 ) && ( Control.LoopIndex < Control.LoopCount ) );
        Assert ( ( Control.LoopIndex == 0 ) || ( Control.LocalClock >= 0.0f - Epsilon ) );
        Assert ( ( Control.LoopIndex == ( Control.LoopCount - 1 ) ) || ( Control.LocalClock < Control.LocalDuration + Epsilon ) );
    }
#endif
#endif
}

inline void
ModulusLocalClock(control &Control)
{
    GRANNY_ENTER();

#if 0       // I never like doing this - the precision issues scare me.
    Control.LoopIndex += TruncateReal32ToInt32(Control.LocalClock /
                                               Control.LocalDuration);
    Control.LocalClock = (real32)Modulus(Control.LocalClock,
                                         Control.LocalDuration);
    if(Control.LocalClock < 0.0f)
    {
        Control.LocalClock += Control.LocalDuration;
    }
#else       // Whereas this is robust - index and clock will never go "out of sync"
    int32x NewLoopIndex = Control.LoopIndex + TruncateReal32ToInt32(Control.LocalClock / Control.LocalDuration);
    if ( Control.LocalClock < 0.0f )
    {
        // -0.1 gets truncated to 0.0f for example.
        NewLoopIndex--;
    }
    // Now clamp to the valid ranges of LoopIndex
    if ( Control.LoopCount != 0 )
    {
        if ( NewLoopIndex < 0 )
        {
            // Underflow.
            NewLoopIndex = 0;
        }
        else if ( NewLoopIndex >= Control.LoopCount )
        {
            // Overflow.
            NewLoopIndex = Control.LoopCount - 1;
        }
    }

    // And apply to the actual values.
    int32x LoopIndexDelta = NewLoopIndex - Control.LoopIndex;
    if ( LoopIndexDelta != 0 )
    {
        Control.LoopIndex += LoopIndexDelta;
        Control.LocalClock -= (real32)LoopIndexDelta * Control.LocalDuration;
    }
#endif

    CheckLocalClockAndLoopIndex ( Control );
}



// Organize the control functions into the bits that we need for the SPU, and those we
// don't.  This avoids some linking hassles caused by the fact that GCC is a stupid piece
// of poo with respect to unused code elimination.

#if !PROCESSOR_CELL_SPU

// Make sure that we align everything to 16byte blocks for the SPU optimizations
#define ControlAlignedSize ((SizeOf(control) + 0xf) & ~0xf)
static fixed_allocator Allocator = { ControlAlignedSize };


inline void
PackEaseCurve(uint32x &Result, real32 Af, real32 Bf, real32 Cf, real32 Df)
{
    GRANNY_ENTER();

    int32x Ai = RoundReal32ToInt32(Af * 255.0f);
    int32x Bi = RoundReal32ToInt32(Bf * 255.0f);
    int32x Ci = RoundReal32ToInt32(Cf * 255.0f);
    int32x Di = RoundReal32ToInt32(Df * 255.0f);

    Assert((Ai >= 0) && (Ai <= 255));
    Assert((Bi >= 0) && (Bi <= 255));
    Assert((Ci >= 0) && (Ci <= 255));
    Assert((Di >= 0) && (Di <= 255));

    Result = (Ai << 0) | (Bi << 8) | (Ci << 16) | (Di << 24);
}

static control &
GetGlobalControlSentinel(void)
{
    GRANNY_ENTER();

    static control Sentinel;
    if(!Sentinel.Next)
    {
        Sentinel.Next = Sentinel.Prev = &Sentinel;
    }

    return(Sentinel);
}

static void
SortBindings(control &Control)
{
    GRANNY_ENTER();

    bool Active = ControlHasEffect(Control);

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);
    {for(model_control_binding *Iterator = Sentinel->ControlNext;
         Iterator != Sentinel;
         Iterator = Iterator->ControlNext)
    {
        Assert(Iterator->ModelInstance);
        RelinkModelSideOfControlBinding(*Iterator, Active);
    }}
}

model_control_binding &GRANNY
GetControlBindingSentinel(control const &Control)
{
    GRANNY_ENTER();

    return((model_control_binding &)Control.BindingSentinel);
}

control *GRANNY
CreateControl(real32 CurrentClock, real32 LocalDuration)
{
    GRANNY_ENTER();

    control *Control = (control *)AllocateFixed(Allocator);
    if(Control)
    {
        InitializeSentinel(Control->BindingSentinel);

        Control->Flags = control::ActiveFlag;

        Control->dTLocalClockPending = 0.0f;
        Control->CurrentClock = CurrentClock;
        Control->LocalClock = 0.0f;

        Control->Speed = 1.0f;

        Control->LocalDuration = LocalDuration;
        Control->LoopIndex = 0;
        Control->LoopCount = 1;

        Control->KillClock = 0.0f;

        Control->CurrentWeight = 1.0f;

        Control->Next = GetGlobalControlSentinel().Next;
        Control->Prev = &GetGlobalControlSentinel();
        Control->Next->Prev = Control;
        Control->Prev->Next = Control;

        // SUBTLETY: These have to be initialized because otherwise
        // you can FPU-exception in the clock recentering code, which
        // doesn't know if they've been initialized or not
        Control->EaseInStartClock =
            Control->EaseInEndClock =
            Control->EaseOutStartClock =
            Control->EaseOutEndClock = 0.0f;

        // Set some sensible defaults.
        Control->EaseInValues = (0 << 0) | (0 << 8) | (0xff << 16) | ((uint32)0xff << 24);
        Control->EaseOutValues = (0xff << 0) | (0xff << 8) | (0 << 16) | (0 << 24);
    }

    return(Control);
}

void GRANNY
FreeControl(control *Control)
{
    GRANNY_ENTER();

    if(Control)
    {
        // NOTE: I set these all to false, that way if some other
        // deletion would have triggerred during out FreeControlRing()
        // call, it won't trigger, so we'll avoid the double delete.
        SetFlags(*Control,
                 control::ActiveFlag |
                 control::KillOnceCompleteFlag |
                 control::KillOnceUnusedFlag,
                 false);

        Control->Next->Prev = Control->Prev;
        Control->Prev->Next = Control->Next;

        FreeControlRing(GetControlBindingSentinel(*Control));
        DeallocateFixed(Allocator, Control);
    }
}

void GRANNY
FreeControlOnceUnused(control &Control)
{
    GRANNY_ENTER();

    SetFlags(Control, control::KillOnceUnusedFlag, true);
    FreeControlIfUnused(&Control);
}

void GRANNY
CompleteControlAt(control &Control, real32 AtSeconds)
{
    GRANNY_ENTER();

    SetFlags(Control, control::KillOnceCompleteFlag, true);
    Control.KillClock = AtSeconds;
}

real32 GRANNY
GetControlCompletionClock(control const &Control)
{
    GRANNY_ENTER();

    return(Control.KillClock);
}

bool GRANNY
GetControlCompletionCheckFlag(control const &Control)
{
    GRANNY_ENTER();

    return(GetFlags(Control, control::KillOnceCompleteFlag));
}

void GRANNY
SetControlCompletionCheckFlag(control &Control, bool CheckComplete)
{
    GRANNY_ENTER();

    SetFlags(Control, control::KillOnceCompleteFlag, CheckComplete);
}

real32 GRANNY
GetControlClock(control const &Control)
{
    GRANNY_ENTER();

    return(Control.CurrentClock);
}

void GRANNY
SetControlClock(control &Control, real32 NewClock)
{
    GRANNY_ENTER();

    SetControlClockInline(Control, NewClock);
}

void GRANNY
SetControlClockOnly(control &Control, real32 NewClock)
{
    GRANNY_ENTER();

    Control.CurrentClock = NewClock;
}

bool GRANNY
ControlIsComplete(control const &Control)
{
    GRANNY_ENTER();

    return(GetFlags(Control, control::KillOnceCompleteFlag) &&
           (Control.CurrentClock >= Control.KillClock));
}

bool GRANNY
FreeControlIfComplete(control *Control)
{
    GRANNY_ENTER();

    bool Freed = false;

    if(Control)
    {
        if(ControlIsComplete(*Control))
        {
            FreeControl(Control);
            Freed = true;
        }
    }

    return(Freed);
}

bool GRANNY
ControlIsUnused(control const &Control)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);
    return(Sentinel == Sentinel->ControlNext);
}

bool GRANNY
ControlIsRetargeted(control const& Control)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);

    if (Sentinel == Sentinel->ControlNext)
        return false;

    if (Sentinel != Sentinel->ControlNext->ControlNext)
        return false;

    model_control_binding* Binding = Sentinel->ControlNext;
    if (Binding->Callbacks->ControlIsRebased)
    {
        return (*Binding->Callbacks->ControlIsRebased)(*Binding);
    }
    else
    {
        return false;
    }
}

bool GRANNY
ControlIsMirrored(control const& Control)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);

    if (Sentinel == Sentinel->ControlNext)
        return false;

    if (Sentinel != Sentinel->ControlNext->ControlNext)
        return false;

    model_control_binding* Binding = Sentinel->ControlNext;
    if (Binding->Callbacks->ControlIsMirrored)
    {
        return (*Binding->Callbacks->ControlIsMirrored)(*Binding);
    }
    else
    {
        return false;
    }
}

bool GRANNY
FreeControlIfUnused(control *Control)
{
    GRANNY_ENTER();

    bool Freed = false;

    if(Control)
    {
        if(GetFlags(*Control, control::KillOnceUnusedFlag) &&
           ControlIsUnused(*Control))
        {
            FreeControl(Control);
            Freed = true;
        }
    }

    return(Freed);
}

real32 GRANNY
GetControlWeight(control const &Control)
{
    GRANNY_ENTER();

    return(Control.CurrentWeight);
}


track_mask const* GRANNY
GetControlTrackGroupModelMask(control const& Control,
                              model_instance const* ModelInstance)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);
    {for(model_control_binding *Iterator = Sentinel->ControlNext;
         Iterator != Sentinel;
         Iterator = Iterator->ControlNext)
    {
        if (Iterator->ModelInstance == ModelInstance)
        {
            // This is the binding to examine.
            controlled_animation *ControlledAnim = Iterator->Callbacks->GetControlledAnimation ( *Iterator );
            controlled_pose *ControlledPose = Iterator->Callbacks->GetControlledPose ( *Iterator );
            if (ControlledAnim != NULL)
            {
                return ControlledAnim->ModelMask;
            }
            else if (ControlledPose != NULL)
            {
                return ControlledPose->ModelMask;
            }
            else
            {
                // Probably bad, but no action at present.
            }
        }
    }}

    return NULL;
}

track_mask const* GRANNY
GetControlTrackGroupTrackMask(control const&   Control,
                              animation const* Animation,
                              int32x TrackGroupIndex)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Animation, return NULL);
    CheckInt32Index(TrackGroupIndex, Animation->TrackGroupCount, return NULL);

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);
    {for(model_control_binding *Iterator = Sentinel->ControlNext;
         Iterator != Sentinel;
         Iterator = Iterator->ControlNext)
    {
        // This is the binding to examine.  Note that poses obviously don't have
        // trackgroups, so always return null
        controlled_animation *ControlledAnim =
            Iterator->Callbacks->GetControlledAnimation ( *Iterator );
        if (ControlledAnim != NULL)
        {
            animation_binding *Binding =
                GetControlledAnimationBinding(*ControlledAnim);
            if (Binding &&
                Binding->ID.Animation == Animation &&
                Binding->ID.SourceTrackGroupIndex == TrackGroupIndex)
            {
                return ControlledAnim->TrackMask;
            }
        }
    }}

    return NULL;
}


void GRANNY
SetControlWeight(control &Control, real32 Weight)
{
    GRANNY_ENTER();

    real32 OldWeight = Control.CurrentWeight;
    Control.CurrentWeight = Weight;

    if(((OldWeight == 0.0f) && (Control.CurrentWeight != 0.0)) ||
       ((OldWeight != 0.0f) && (Control.CurrentWeight == 0.0f)))
    {
        SortBindings(Control);
    }
}

int32x GRANNY
GetControlLoopCount(control const &Control)
{
    GRANNY_ENTER();

    return(Control.LoopCount);
}

void GRANNY
SetControlLoopCount(control &Control, int32x LoopCount)
{
    GRANNY_ENTER();

    Control.LoopCount = LoopCount;
}

int32x GRANNY
GetControlLoopIndex(control &Control)
{
    GRANNY_ENTER();

    // This has to be done here, to ensure that the loop index
    // has been updated
    GetControlRawLocalClock(Control);

    return(Control.LoopIndex);
}

void GRANNY
SetControlLoopIndex(control &Control, int32x LoopIndex)
{
    GRANNY_ENTER();

    Control.LoopIndex = LoopIndex;
}

real32 GRANNY
GetControlSpeed(control const &Control)
{
    GRANNY_ENTER();

    return(Control.Speed);
}

void GRANNY
SetControlSpeed(control &Control, real32 Speed)
{
    GRANNY_ENTER();

    Control.Speed = Speed;
}

real32 GRANNY
GetControlDuration(control const &Control)
{
    GRANNY_ENTER();

    real32 Duration = GetReal32VeryLarge();

    if (Control.LoopCount == 0)
    {
        // Loops forever.
        Duration = GetReal32VeryLarge();
    }
    else if ( Control.Speed > 0.0f )
    {
        Duration = (Control.LocalDuration / Control.Speed) *
            (real32)Control.LoopCount;
    }
    else if ( Control.Speed < 0.0f )
    {
        Duration = (Control.LocalDuration / -Control.Speed) *
            (real32)Control.LoopCount;
    }
    else
    {
        // Speed is 0.0
        Duration = GetReal32VeryLarge();
    }

    return(Duration);
}

real32 GRANNY
GetControlDurationLeft(control &Control)
{
    GRANNY_ENTER();

    real32 Duration = GetReal32VeryLarge();

    // NOTE: This has to be done here, to ensure that the loop index
    // and such are set properly
    real32 LocalClock = GetControlRawLocalClock(Control);

    if(Control.LoopCount == 0)
    {
        // Loops forever.
        Duration = GetReal32VeryLarge();
    }
    else if(Control.Speed > 0.0f )
    {
        // Going forwards.
        if(Control.LoopIndex >= Control.LoopCount)
        {
            Duration = 0.0f;
        }
        else
        {
            Duration =
                // The remainder of this loop:
                ((Control.LocalDuration - LocalClock) / Control.Speed) +
                // The rest of the loops:
                ((Control.LocalDuration / Control.Speed) *
                (real32)(Control.LoopCount - Control.LoopIndex - 1));
        }
    }
    else if(Control.Speed < 0.0f )
    {
        // Going backwards.
        if(Control.LoopIndex < 0)
        {
            Duration = 0.0f;
        }
        else
        {
            Duration =
                // The remainder of this loop:
                (LocalClock / -Control.Speed) +
                // The rest of the loops:
                ((Control.LocalDuration / -Control.Speed) *
                (real32)(Control.LoopIndex));
        }
    }
    else
    {
        // Control speed is zero, so the animation is going to last forever.
        Duration = GetReal32VeryLarge();
    }

    return(Duration);
}

real32 GRANNY
GetControlCycleDuration(control const &Control)
{
    GRANNY_ENTER();

    if (Control.Speed != 0)
        return (Control.LocalDuration / AbsoluteValue(Control.Speed));
    else
        return GetReal32VeryLarge();
}


void GRANNY
SetControlActive(control &Control, bool Active)
{
    GRANNY_ENTER();

    if(GetFlags(Control, control::ActiveFlag) != Active)
    {
        SetFlags(Control, control::ActiveFlag, Active);
        SortBindings(Control);
    }
}

void GRANNY
SetControlEaseIn(control &Control, bool EaseIn)
{
    GRANNY_ENTER();

    SetFlags(Control, control::EaseInFlag, EaseIn);
}

void GRANNY
SetControlEaseInCurve(control &Control,
                      real32 StartSeconds, real32 EndSeconds,
                      real32 StartValue, real32 StartTangent,
                      real32 EndTangent, real32 EndValue)
{
    GRANNY_ENTER();

    Control.EaseInStartClock = StartSeconds;
    Control.EaseInEndClock = EndSeconds;

    PackEaseCurve(Control.EaseInValues,
                  StartValue,
                  StartTangent,
                  EndTangent,
                  EndValue);
}

void GRANNY
SetControlEaseOut(control &Control, bool EaseOut)
{
    GRANNY_ENTER();

    SetFlags(Control, control::EaseOutFlag, EaseOut);
}

void GRANNY
SetControlEaseOutCurve(control &Control,
                       real32 StartSeconds, real32 EndSeconds,
                       real32 StartValue, real32 StartTangent,
                       real32 EndTangent, real32 EndValue)
{
    GRANNY_ENTER();

    Control.EaseOutStartClock = StartSeconds;
    Control.EaseOutEndClock = EndSeconds;

    PackEaseCurve(Control.EaseOutValues,
                  StartValue,
                  StartTangent,
                  EndTangent,
                  EndValue);
}

void GRANNY
SetControlRawLocalClock(control &Control, real32 LocalClock)
{
    GRANNY_ENTER();

    Control.dTLocalClockPending = 0.0f;
    Control.LocalClock = LocalClock;
}

real32 GRANNY
EaseControlIn(control &Control, real32 Duration, bool FromCurrent)
{
    GRANNY_ENTER();

    real32 const CurrentClock = GetControlClock(Control);
    real32 Multiplier = 0.0f;
    if(FromCurrent)
    {
        Multiplier = GetControlEaseCurveMultiplier(Control);
    }

    SetControlEaseIn(Control, true);
    SetControlEaseOut(Control, false);

    SetControlEaseInCurve(Control, CurrentClock, CurrentClock + Duration,
                          Multiplier, Multiplier, 1.0f, 1.0f);

    return(CurrentClock + Duration);
}

// Note: this is not consistent with the API for EaseIn.  Should really have a
// "FromCurrent", but not in the current API version, introducing that change would be
// somewhat disruptive.
real32 GRANNY
EaseControlOut(control &Control, real32 Duration)
{
    GRANNY_ENTER();

    real32 const CurrentClock = GetControlClock(Control);
    real32 const Multiplier = GetControlEaseCurveMultiplier(Control);

    SetControlEaseIn(Control, false);
    SetControlEaseOut(Control, true);

    SetControlEaseOutCurve(Control, CurrentClock, CurrentClock + Duration,
                           Multiplier, Multiplier, 0.0f, 0.0f);

    return(CurrentClock + Duration);
}

void **GRANNY
GetControlUserDataArray(control const &Control)
{
    GRANNY_ENTER();

    return((void **)Control.UserData);
}

control *GRANNY
GetGlobalControlsBegin(void)
{
    GRANNY_ENTER();

    return(GetGlobalControlSentinel().Next);
}

control *GRANNY
GetGlobalControlsEnd(void)
{
    GRANNY_ENTER();

    return(&GetGlobalControlSentinel());
}

control *GRANNY
GetGlobalNextControl(control *Control)
{
    GRANNY_ENTER();

    return(Control ? Control->Next : 0);
}

void GRANNY
RecenterControlClocks(control *Control, real32 dCurrentClock)
{
    GRANNY_ENTER();

    Control->CurrentClock += dCurrentClock;

    Control->KillClock += dCurrentClock;
    Control->EaseInStartClock += dCurrentClock;
    Control->EaseInEndClock += dCurrentClock;
    Control->EaseOutStartClock += dCurrentClock;
    Control->EaseOutEndClock += dCurrentClock;
}

void GRANNY
RecenterAllControlClocks(real32 dCurrentClock)
{
    GRANNY_ENTER();

    {for(control *Control = GetGlobalControlSentinel().Next;
         Control != &GetGlobalControlSentinel();
         Control = Control->Next)
    {
        RecenterControlClocks ( Control, dCurrentClock );
    }}
}

void GRANNY
RecenterAllModelInstanceControlClocks(model_instance *ModelInstance, real32 dCurrentClock)
{
    GRANNY_ENTER();

    // WARNING - this isn't 100% safe because a control can be controlling
    // multiple model_instances. However, very few people use this functionality,
    // so in most cases it is safe.
    model_control_binding *BindingEnd = &(ModelControlsEnd(*ModelInstance));
    for(model_control_binding *Binding = &(ModelControlsBegin(*ModelInstance));
        Binding != BindingEnd;
        Binding = &(ModelControlsNext(*Binding)))
    {
        RecenterControlClocks ( GetControlFromBinding(*Binding), dCurrentClock );
    }
}


void GRANNY
SetControlForceClampedLooping(control &Control, bool Clamp)
{
    GRANNY_ENTER();

    SetFlags(Control, control::ForceClampedLoopsFlag, Clamp);
}


void GRANNY
SetControlTargetState(control &Control, real32 CurrentGlobalTime, real32 TargetGlobalTime, real32 LocalTime, int32x LoopIndex)
{
    GRANNY_ENTER();

    Control.dTLocalClockPending = 0.0f;
    Control.CurrentClock = CurrentGlobalTime;

    real32 DeltaGlobalTime = TargetGlobalTime - CurrentGlobalTime;

    Control.LoopIndex = LoopIndex;
    Control.LocalClock = LocalTime - DeltaGlobalTime * Control.Speed;

    // And move the local clock back into the right ranges by adjusting the loop index appropriately.
    ModulusLocalClock ( Control );
}

#endif

// =============================================================================
// Functions after this point are used on the SPU
// =============================================================================

inline void
UnpackEaseCurve(uint32 Value, real32 *Result)
{
    GRANNY_ENTER();

    Result[0] = (real32)((Value >>  0) & 0xFF) * (1.0f / 255.0f);
    Result[1] = (real32)((Value >>  8) & 0xFF) * (1.0f / 255.0f);
    Result[2] = (real32)((Value >> 16) & 0xFF) * (1.0f / 255.0f);
    Result[3] = (real32)((Value >> 24) & 0xFF) * (1.0f / 255.0f);
}

static real32
ComputeEaseCurve(real32 StartClock, real32 CurrentClock, real32 EndClock,
                 real32 *Bezier)
{
    GRANNY_ENTER();

    real32 Range = EndClock - StartClock;
    real32 RelativeClock = CurrentClock - StartClock;

    real32 t;
    if(Range != 0.0f)
    {
        t = (RelativeClock / Range);
        if(t < 0.0f)
        {
            t = 0.0f;
        }
        else if(t > 1.0f)
        {
            t = 1.0f;
        }
    }
    else
    {
        Assert(StartClock == EndClock);
        t = (CurrentClock < StartClock) ? 0.0f : 1.0f;
    }

    real32 const u = (1.0f - t);

    // This is just a simple bezier curve.
    return(1*u*u*u*Bezier[0] +
           3*u*u*t*Bezier[1] +
           3*u*t*t*Bezier[2] +
           1*t*t*t*Bezier[3]);
}

real32 GRANNY
GetControlRawLocalClock(control &Control)
{
    GRANNY_ENTER();

    if(Control.dTLocalClockPending != 0.0f)
    {
        Control.LocalClock += Control.dTLocalClockPending * Control.Speed;

        if(Control.LocalClock < 0)
        {
            if((Control.LoopCount == 0) ||
               (Control.LoopIndex > 0))
            {
                ModulusLocalClock(Control);
            }
        }
        else if(Control.LocalClock >= Control.LocalDuration)
        {
            if((Control.LoopCount == 0) ||
               (Control.LoopIndex < (Control.LoopCount - 1)))
            {
                ModulusLocalClock(Control);
            }
        }

        Control.dTLocalClockPending = 0.0f;
    }

    // TODO: do we need to always call ModulusLocalClock
    // - what if they set the loop index to be something strange, or the
    // local time out of bounds or something?
    CheckLocalClockAndLoopIndex ( Control );

    return(Control.LocalClock);
}

real32 GRANNY
GetControlEaseCurveMultiplier(control const &Control)
{
    GRANNY_ENTER();

    real32 EaseInValues[4];
    real32 EaseOutValues[4];
    UnpackEaseCurve(Control.EaseInValues, EaseInValues);
    UnpackEaseCurve(Control.EaseOutValues, EaseOutValues);

    real32 Weight = 1.0f;


    if((Control.EaseInStartClock < Control.EaseOutStartClock) ||
       !(GetFlags(Control, control::EaseOutFlag) &&
         GetFlags(Control, control::EaseInFlag)))
    {
        // This is the ease-in, ease-out scenario
        if(GetFlags(Control, control::EaseOutFlag))
        {
            if(Control.CurrentClock >= Control.EaseOutStartClock)
            {
                Weight = ComputeEaseCurve(Control.EaseOutStartClock,
                                          Control.CurrentClock,
                                          Control.EaseOutEndClock,
                                          EaseOutValues);
            }
        }

        if(GetFlags(Control, control::EaseInFlag))
        {
            if(Control.CurrentClock <= Control.EaseInEndClock)
            {
                Weight = ComputeEaseCurve(Control.EaseInStartClock,
                                          Control.CurrentClock,
                                          Control.EaseInEndClock,
                                          EaseInValues);
            }
        }
    }
    else
    {
        // This is the ease-out, ease-back-in scenario
        if(Control.CurrentClock <= Control.EaseOutEndClock)
        {
            Weight = ComputeEaseCurve(Control.EaseOutStartClock,
                                      Control.CurrentClock,
                                      Control.EaseOutEndClock,
                                      EaseOutValues);
        }
        else if(Control.CurrentClock <= Control.EaseInStartClock)
        {
            real32 t = ((Control.CurrentClock - Control.EaseOutEndClock) /
                        (Control.EaseInStartClock - Control.EaseOutEndClock));
            Weight = ((1.0f - t)*EaseOutValues[3] +
                      t*EaseInValues[0]);
        }
        else if(Control.CurrentClock <= Control.EaseInEndClock)
        {
            Weight = ComputeEaseCurve(Control.EaseInStartClock,
                                      Control.CurrentClock,
                                      Control.EaseInEndClock,
                                      EaseInValues);
        }
    }

    return(Weight);
}

void GRANNY
GetControlLoopState(control &Control,
                    bool &UnderflowLoop, bool &OverflowLoop)
{
    GRANNY_ENTER();

    // This has to be done here, to ensure that the loop index
    // has been updated
    GetControlRawLocalClock(Control);

    GetControlLoopStateInline(Control, UnderflowLoop, OverflowLoop);
}

bool GRANNY
ControlHasEffect(control &Control)
{
    GRANNY_ENTER();

    return(GetFlags(Control, control::ActiveFlag) &&
           (Control.CurrentWeight != 0.0f));
}

bool GRANNY
ControlIsActive(control const &Control)
{
    GRANNY_ENTER();

    return(GetFlags(Control, control::ActiveFlag));
}

real32 GRANNY
GetControlLocalDuration(control const &Control)
{
    GRANNY_ENTER();

    return(GetControlLocalDurationInline(Control));
}

real32 GRANNY
GetControlClampedLocalClock(control &Control)
{
    GRANNY_ENTER();

    return(GetControlClampedLocalClockInline(Control));
}

real32 GRANNY
GetControlEffectiveWeight(control const &Control)
{
    GRANNY_ENTER();

    if(ControlIsActive(Control))
    {
        return(GetControlEaseCurveMultiplier(Control) *
               Control.CurrentWeight);
    }
    else
    {
        return(0);
    }
}

