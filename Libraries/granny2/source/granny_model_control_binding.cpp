// ========================================================================
// $File: //jeffr/granny/rt/granny_model_control_binding.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #15 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_model_control_binding.h"

#include "granny_control.h"
#include "granny_fixed_allocator.h"
#include "granny_model_instance.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

CompileAssert(IS_ALIGNED_16(SizeOf(model_control_binding)));
static fixed_allocator ModelControlBindingAllocator =
    {SizeOf(model_control_binding)};

void GRANNY
UnlinkModelControlBinding(model_control_binding &Binding)
{
    GRANNY_ENTER();

    Binding.ControlPrev->ControlNext = Binding.ControlNext;
    Binding.ControlNext->ControlPrev = Binding.ControlPrev;

    Binding.ModelPrev->ModelNext = Binding.ModelNext;
    Binding.ModelNext->ModelPrev = Binding.ModelPrev;
}

void GRANNY
LinkModelControlBinding(model_control_binding &Binding,
                        control &Control,
                        model_instance &ModelInstance,
                        bool Active)
{
    GRANNY_ENTER();

    model_control_binding *ControlSentinel =
        &GetControlBindingSentinel(Control);
    Binding.Control = &Control;
    Binding.ControlPrev = ControlSentinel;
    Binding.ControlNext = ControlSentinel->ControlNext;
    Binding.ControlPrev->ControlNext = &Binding;
    Binding.ControlNext->ControlPrev = &Binding;

    model_control_binding *ModelSentinel =
        &GetModelBindingSentinel(ModelInstance);
    Binding.ModelInstance = &ModelInstance;
    if(Active)
    {
        Binding.ModelPrev = ModelSentinel;
        Binding.ModelNext = ModelSentinel->ModelNext;
    }
    else
    {
        Binding.ModelPrev = ModelSentinel->ModelPrev;
        Binding.ModelNext = ModelSentinel;
    }
    Binding.ModelPrev->ModelNext = &Binding;
    Binding.ModelNext->ModelPrev = &Binding;
}

void GRANNY
RelinkModelSideOfControlBinding(model_control_binding &Binding,
                                bool Active)
{
    GRANNY_ENTER();

    Binding.ModelPrev->ModelNext = Binding.ModelNext;
    Binding.ModelNext->ModelPrev = Binding.ModelPrev;

    model_control_binding *ModelSentinel =
        &GetModelBindingSentinel(*Binding.ModelInstance);
    if(Active)
    {
        Binding.ModelPrev = ModelSentinel;
        Binding.ModelNext = ModelSentinel->ModelNext;
    }
    else
    {
        Binding.ModelPrev = ModelSentinel->ModelPrev;
        Binding.ModelNext = ModelSentinel;
    }
    Binding.ModelPrev->ModelNext = &Binding;
    Binding.ModelNext->ModelPrev = &Binding;
}

void GRANNY
InitializeSentinel(model_control_binding &Binding)
{
    GRANNY_ENTER();

    Binding.Control = 0;
    Binding.ControlPrev = &Binding;
    Binding.ControlNext = &Binding;

    Binding.ModelInstance = 0;
    Binding.ModelPrev = &Binding;
    Binding.ModelNext = &Binding;

    Binding.Callbacks = 0;
}

model_control_binding *GRANNY
CreateModelControlBinding(model_control_callbacks const &Callbacks,
                          control &Control, model_instance &ModelInstance,
                          bool Active, void *InitData)
{
    GRANNY_ENTER();

    model_control_binding *Binding = (model_control_binding *)
        AllocateFixed(ModelControlBindingAllocator);
    Assert(IS_ALIGNED_16(Binding));

    if(Binding)
    {
        Binding->Callbacks = &Callbacks;
        Binding->BindingType = 0;

        LinkModelControlBinding(*Binding, Control, ModelInstance, Active);
        if(Callbacks.InitializeBindingState)
        {
            Callbacks.InitializeBindingState(*Binding, InitData);
        }
    }

    return Binding;
}

void GRANNY
FreeModelControlBinding(model_control_binding *Binding)
{
    GRANNY_ENTER();

    if(Binding)
    {
        UnlinkModelControlBinding(*Binding);
        if(Binding->Callbacks->CleanupBindingState)
        {
            Binding->Callbacks->CleanupBindingState(*Binding);
        }

        FreeControlIfUnused(Binding->Control);
    }

    DeallocateFixed(ModelControlBindingAllocator, Binding);
}

void GRANNY
SampleModelControlBindingLODSparse(model_control_binding &Binding,
                                    int32x FirstBone, int32x BoneCount,
                                    local_pose &Result, real32 AllowedError,
                                    int32x const *SparseBoneArray)
{
    GRANNY_ENTER();

    Assert(Binding.Control);

    if(Binding.Callbacks->AccumulateBindingState)
    {
        Binding.Callbacks->AccumulateBindingState(
            Binding, FirstBone, BoneCount, Result, AllowedError, SparseBoneArray);
    }
}

void GRANNY
BuildBindingDirect(model_control_binding &Binding,
                   int32x BoneCount,
                   real32 const *Offset4x4,
                   world_pose &Result,
                   real32 AllowedError)
{
    GRANNY_ENTER();

    Assert(Binding.Control);

    if(Binding.Callbacks->BuildBindingDirect)
    {
        Binding.Callbacks->BuildBindingDirect(Binding, BoneCount,
                                              Offset4x4, Result, AllowedError);
    }
}

void GRANNY
AccumulateModelControlBindingLoopTransform(
    model_control_binding &Binding, real32 SecondsElapsed,
    real32 &TotalWeight, real32 *ResultTranslation, real32 *ResultRotation, bool Inverse)
{
    GRANNY_ENTER();

    Assert(Binding.Control);

    if(Binding.Callbacks->AccumulateLoopTransform)
    {
        Binding.Callbacks->AccumulateLoopTransform(
            Binding, SecondsElapsed, TotalWeight,
            ResultTranslation, ResultRotation, Inverse);
    }
}

void GRANNY
FreeControlRing(model_control_binding &Sentinel)
{
    GRANNY_ENTER();

    while(Sentinel.ControlNext != &Sentinel)
    {
        FreeModelControlBinding(Sentinel.ControlNext);
    }
}

void GRANNY
FreeModelRing(model_control_binding &Sentinel)
{
    GRANNY_ENTER();

    while(Sentinel.ModelNext != &Sentinel)
    {
        FreeModelControlBinding(Sentinel.ModelNext);
    }
}

model_control_binding &GRANNY
ModelControlsBegin(model_instance &Model)
{
    GRANNY_ENTER();

    return(*GetModelBindingSentinel(Model).ModelNext);
}

model_control_binding &GRANNY
ModelControlsNext(model_control_binding &Binding)
{
    GRANNY_ENTER();

    return(*Binding.ModelNext);
}

model_control_binding &GRANNY
ModelControlsEnd(model_instance &Model)
{
    GRANNY_ENTER();

    return(GetModelBindingSentinel(Model));
}

model_control_binding &GRANNY
ControlModelsBegin(control &Control)
{
    GRANNY_ENTER();

    return(*GetControlBindingSentinel(Control).ControlNext);
}

model_control_binding &GRANNY
ControlModelsNext(model_control_binding &Binding)
{
    GRANNY_ENTER();

    return(*Binding.ControlNext);
}

model_control_binding &GRANNY
ControlModelsEnd(control &Control)
{
    GRANNY_ENTER();

    return(GetControlBindingSentinel(Control));
}

model_instance *GRANNY
GetModelInstanceFromBinding(model_control_binding &Binding)
{
    GRANNY_ENTER();

    return(Binding.ModelInstance);
}

control *GRANNY
GetControlFromBinding(model_control_binding &Binding)
{
    GRANNY_ENTER();

    return(Binding.Control);
}
