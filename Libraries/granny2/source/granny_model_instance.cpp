// ========================================================================
// $File: //jeffr/granny/rt/granny_model_instance.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #32 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_model_instance.h"

#include "granny_control.h"
#include "granny_degree_of_freedom.h"
#include "granny_fixed_allocator.h"
#include "granny_local_pose.h"
#include "granny_math.h"
#include "granny_matrix_operations.h"
#include "granny_model.h"
#include "granny_parameter_checking.h"
#include "granny_skeleton.h"
#include "granny_world_pose.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#undef SubsystemCode
#define SubsystemCode AnimationLogMessage

USING_GRANNY_NAMESPACE;

CompileAssert(IS_ALIGNED_16(SizeOf(model_instance)));
static fixed_allocator Allocator = {SizeOf(model_instance)};

static model_instance &
GetGlobalModelInstanceSentinel(void)
{
    GRANNY_ENTER();

    static model_instance Sentinel;
    if(!Sentinel.Next)
    {
        Sentinel.Next = Sentinel.Prev = &Sentinel;
    }

    return(Sentinel);
}

model_instance *GRANNY
InstantiateModel(model const &Model)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Model.Skeleton, return 0);

    model_instance *ModelInstance = (model_instance *)AllocateFixed(Allocator);

    if(ModelInstance)
    {
        CheckCondition(IS_ALIGNED_16(ModelInstance),
                       FreeModelInstance(ModelInstance); return NULL);

        ModelInstance->Model = &Model;

        ModelInstance->CachedSkeleton       = Model.Skeleton;
        ModelInstance->CachedBones          = Model.Skeleton->Bones;
        ModelInstance->CachedBoneCount      = Model.Skeleton->BoneCount;
        ModelInstance->MirrorTransformCache = 0;

        ModelInstance->Reserved0 = 0;

        InitializeSentinel(ModelInstance->BindingSentinel);

        ModelInstance->Next = GetGlobalModelInstanceSentinel().Next;
        ModelInstance->Prev = &GetGlobalModelInstanceSentinel();
        ModelInstance->Next->Prev = ModelInstance;
        ModelInstance->Prev->Next = ModelInstance;
    }

    return ModelInstance;
}

void GRANNY
CreateMirrorTransformCache(model_instance& Instance)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Instance.Model, return);

    // Already created?
    if (Instance.MirrorTransformCache != 0)
        return;

    Instance.MirrorTransformCache = AllocateArray(Instance.Model->Skeleton->BoneCount, transform);
    if (Instance.MirrorTransformCache == 0)
    {
        Log1(ErrorLogMessage, AnimationLogMessage,
             "CreateMirrorTransformCache: Unable to allocate a mirror cache of %d bones",
             Instance.Model->Skeleton->BoneCount);
    }
}


void GRANNY
FreeModelInstance(model_instance *Instance)
{
    GRANNY_ENTER();

    if(Instance)
    {
        FreeModelRing(GetModelBindingSentinel(*Instance));

        Instance->Next->Prev = Instance->Prev;
        Instance->Prev->Next = Instance->Next;

        if (Instance->MirrorTransformCache)
        {
            Deallocate(Instance->MirrorTransformCache);
            Instance->MirrorTransformCache = 0;
        }
    }

    DeallocateFixed(Allocator, Instance);
}

model &GRANNY
GetSourceModel(model_instance const &Model)
{
    GRANNY_ENTER();

    Assert(Model.Model);
    return(*(model *)Model.Model);
}

skeleton *GRANNY
GetSourceSkeleton(model_instance const &Model)
{
    GRANNY_ENTER();

    Assert(Model.CachedSkeleton == Model.Model->Skeleton);
    return (skeleton*)Model.CachedSkeleton;
}

model_control_binding &GRANNY
GetModelBindingSentinel(model_instance const &ModelInstance)
{
    GRANNY_ENTER();

    return((model_control_binding &)ModelInstance.BindingSentinel);
}

void GRANNY
SetModelClock(model_instance const &ModelInstance, real32 NewClock)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    {for(model_control_binding *Iterator = Sentinel->ModelNext;
         Iterator != Sentinel;
         Iterator = Iterator->ModelNext)
    {
        Assert(Iterator->Control);
        SetControlClockInline(*Iterator->Control, NewClock);
    }}
}

void GRANNY
FreeCompletedModelControls(model_instance const &ModelInstance)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    {for(model_control_binding *Iterator = Sentinel->ModelNext;
         Iterator != Sentinel;)
    {
        control *Control = Iterator->Control;
        Iterator = Iterator->ModelNext;

        FreeControlIfComplete(Control);
    }}
}

void GRANNY
AccumulateModelAnimationsLODSparse(model_instance const &ModelInstance,
                                    int32x FirstBone, int32x BoneCount,
                                    local_pose &Result, real32 AllowedError,
                                    int32x const *SparseBoneArray)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    {for(model_control_binding *Iterator = Sentinel->ModelNext;
         Iterator != Sentinel;
         Iterator = Iterator->ModelNext)
    {
        if (ControlHasEffect(*Iterator->Control))
        {
            SampleModelControlBindingLODSparse(*Iterator, FirstBone, BoneCount, Result, AllowedError, SparseBoneArray);
        }
        else
        {
            // Bindings are sorted by their state, so as soon as
            // we hit one that is inactive, we stop.
            break;
        }
    }}
}

void GRANNY
SampleModelAnimationsLODSparse(model_instance const &ModelInstance,
                               int32x FirstBone, int32x BoneCount,
                               local_pose &Result, real32 AllowedError,
                               int32x const *SparseBoneArray)
{
    GRANNY_ENTER();

    BeginLocalPoseAccumulation(Result, FirstBone, BoneCount, SparseBoneArray);

    AccumulateModelAnimationsLODSparse(ModelInstance, FirstBone, BoneCount, Result, AllowedError, SparseBoneArray);

    skeleton const *Skeleton = GetSourceSkeleton(ModelInstance);
    Assert(Skeleton);

    EndLocalPoseAccumulationLOD(Result, FirstBone, BoneCount, *Skeleton, AllowedError, SparseBoneArray);
}

bool GRANNY
SampleSingleModelAnimationLODSparse(model_instance const &ModelInstance,
                                    control const &Control,
                                    int32x FirstBone, int32x BoneCount,
                                    local_pose &Result, real32 AllowedError,
                                    int32x const *SparseBoneArray)
{
    GRANNY_ENTER();

    bool FoundAnimation = false;

    BeginLocalPoseAccumulation(Result, FirstBone, BoneCount, SparseBoneArray);

    model_control_binding *Sentinel = &GetControlBindingSentinel(Control);
    {for(model_control_binding *Iterator = Sentinel->ControlNext;
         Iterator != Sentinel;
         Iterator = Iterator->ControlNext)
    {
        Assert(Iterator->Control == &Control);
        if(Iterator->ModelInstance == &ModelInstance)
        {
            if(ControlHasEffect(*Iterator->Control))
            {
                SampleModelControlBindingLODSparse(*Iterator, FirstBone, BoneCount, Result, AllowedError, SparseBoneArray);
            }

            FoundAnimation = true;
            break;
        }
    }}

    skeleton const *Skeleton = GetSourceSkeleton(ModelInstance);
    Assert(Skeleton);
    EndLocalPoseAccumulationLOD(Result, FirstBone, BoneCount, *Skeleton, AllowedError, SparseBoneArray);

    return(FoundAnimation);
}


void GRANNY
SampleModelAnimationsAcceleratedLOD(model_instance const &ModelInstance,
                                    int32x BoneCount,
                                    real32 const *Offset4x4,
                                    local_pose &Scratch,
                                    world_pose &Result,
                                    real32 AllowedError)
{
    GRANNY_ENTER();

    // Some people don't realise you MUST pass me a local_pose, even if I don't use it in the fast path.
    CheckPointerNotNull ( &Scratch, return );

    skeleton const *Skeleton = GetSourceSkeleton(ModelInstance);
    if (BoneCount > Skeleton->BoneCount)
    {
        BoneCount = Skeleton->BoneCount;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if (!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }
    else
    {
        if (!IS_ALIGNED_16(Offset4x4))
        {
            MatrixEquals4x4(OffsetBuffer, Offset4x4);
            Offset4x4 = OffsetBuffer;
        }
    }

    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    if((Sentinel->ModelNext != Sentinel) &&
       (ControlHasEffect(*Sentinel->ModelNext->Control)))
    {
        // How many bones to sample for this LOD?  Only compute this if
        //  we're going to use it...
        int32x LODBoneCount = GetBoneCountForLOD(Skeleton, AllowedError);

        if (((Sentinel->ModelNext->ModelNext != Sentinel) &&
             (ControlHasEffect(*Sentinel->ModelNext->ModelNext->Control))) ||
            ControlIsRetargeted(*Sentinel->ModelNext->Control) ||
            ControlIsMirrored(*Sentinel->ModelNext->Control))
        {
            SampleModelAnimationsLODSparse(ModelInstance,
                                           0, LODBoneCount,
                                           Scratch,
                                           AllowedError, NULL);
            BuildWorldPoseLOD(*Skeleton,
                              0, BoneCount,
                              0, LODBoneCount,
                              Scratch, Offset4x4, Result);
        }
        else
        {
            BuildBindingDirect(*Sentinel->ModelNext, BoneCount,
                               Offset4x4, Result, AllowedError);
        }
    }
    else
    {
        BuildRestWorldPose(*Skeleton, 0, BoneCount, Offset4x4, Result);
    }
}


void GRANNY
GetUnnormalizedRootMotionVectors(model_instance const &ModelInstance,
                     real32 SecondsElapsed,
                     real32 &TotalWeight,
                     real32 *ResultTranslation,
                     real32 *ResultRotation,
                     bool Inverse)
{
    GRANNY_ENTER();

    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    {for(model_control_binding *Iterator = Sentinel->ModelNext;
         Iterator != Sentinel;
         Iterator = Iterator->ModelNext)
    {
        if(ControlHasEffect(*Iterator->Control))
        {
            AccumulateModelControlBindingLoopTransform(
                *Iterator, SecondsElapsed, TotalWeight,
                ResultTranslation, ResultRotation, Inverse);
        }
        else
        {
            // Bindings are sorted by their state, so as soon as
            // we hit one that is inactive, we stop.
            break;
        }
    }}
}


void GRANNY
GetRootMotionVectors(model_instance const &ModelInstance,
                     real32 SecondsElapsed,
                     real32 *ResultTranslation,
                     real32 *ResultRotation,
                     bool Inverse)
{
    GRANNY_ENTER();

    real32 TotalWeight = 0.0f;

    VectorZero3(ResultTranslation);
    VectorZero3(ResultRotation);

    GetUnnormalizedRootMotionVectors ( ModelInstance, SecondsElapsed, TotalWeight, ResultTranslation, ResultRotation, Inverse );

    if(TotalWeight > TrackWeightEpsilon)
    {
        real32 const InverseTotalWeight = 1.0f / TotalWeight;
        VectorScale3(ResultTranslation, InverseTotalWeight);
        VectorScale3(ResultRotation, InverseTotalWeight);
    }
}

void GRANNY
ClipRootMotionVectors(real32 const *Translation3,
                      real32 const *Rotation3,
                      uint32 AllowableDOFs,
                      real32 *AllowedTranslation3,
                      real32 *AllowedRotation3,
                      real32 *DisallowedTranslation3,
                      real32 *DisallowedRotation3)
{
    GRANNY_ENTER();

    VectorEquals3(AllowedTranslation3, Translation3);
    VectorEquals3(DisallowedTranslation3, Translation3);
    VectorEquals3(AllowedRotation3, Rotation3);
    VectorEquals3(DisallowedRotation3, Rotation3);

    ClipPositionDOFs(AllowedTranslation3, AllowableDOFs);
    ClipAngularVelocityDOFs(AllowedRotation3, AllowableDOFs);

    ClipPositionDOFs(DisallowedTranslation3, ~AllowableDOFs);
    ClipAngularVelocityDOFs(DisallowedRotation3, ~AllowableDOFs);
}

void GRANNY
ApplyRootMotionVectorsToMatrix(real32 const *ModelMatrix4x4,
                               real32 const *Translation,
                               real32 const *Rotation,
                               real32 *DestMatrix4x4)
{
    GRANNY_ENTER();

    quad UpdateMatrix[4];
    UpdateMatrix[0][3] = UpdateMatrix[1][3] = UpdateMatrix[2][3] = 0.0f;
    UpdateMatrix[3][3] = 1.0f;

    UpdateMatrix[3][0] = Translation[0];
    UpdateMatrix[3][1] = Translation[1];
    UpdateMatrix[3][2] = Translation[2];

    real32 RotationLength = VectorLength3(Rotation);
    if(RotationLength > 0.0f)
    {
        triple ScaledRotation;
        VectorScale3(ScaledRotation, 1.0f / RotationLength, Rotation);

        triple RotationMatrix[3];
        AxisRotationColumns3x3(RotationMatrix, ScaledRotation,
                               RotationLength);
        UpdateMatrix[0][0] = RotationMatrix[0][0];
        UpdateMatrix[0][1] = RotationMatrix[1][0];
        UpdateMatrix[0][2] = RotationMatrix[2][0];
        UpdateMatrix[1][0] = RotationMatrix[0][1];
        UpdateMatrix[1][1] = RotationMatrix[1][1];
        UpdateMatrix[1][2] = RotationMatrix[2][1];
        UpdateMatrix[2][0] = RotationMatrix[0][2];
        UpdateMatrix[2][1] = RotationMatrix[1][2];
        UpdateMatrix[2][2] = RotationMatrix[2][2];
    }
    else
    {
        UpdateMatrix[0][0] = UpdateMatrix[1][1] = UpdateMatrix[2][2] = 1.0f;
        UpdateMatrix[0][1] = UpdateMatrix[0][2] =
            UpdateMatrix[1][0] = UpdateMatrix[1][2] =
            UpdateMatrix[2][0] = UpdateMatrix[2][1] = 0.0f;
    }

    real32 Temp[16];
    if(DestMatrix4x4 == ModelMatrix4x4)
    {
        DestMatrix4x4 = Temp;
    }

    ColumnMatrixMultiply4x3(DestMatrix4x4, (real32 *)UpdateMatrix, ModelMatrix4x4);

    if(DestMatrix4x4 == Temp)
    {
        MatrixEquals4x4((real32 *)ModelMatrix4x4, Temp);
    }
}

static void
AngularVelocityToQuaternion(real32 *Rotation4, real32 const *Rotation3)
{
    GRANNY_ENTER();

    real32 Angle = VectorLength3(Rotation3);
    triple Normalized;
    VectorScale3(Normalized, 1.0f / Angle, Rotation3);
    ConstructQuaternion4(Rotation4, Normalized, Angle);
}

void GRANNY
ApplyRootMotionVectorsToLocalPose(local_pose &Pose,
                                  real32 const *Translation3,
                                  real32 const *Rotation3)
{
    GRANNY_ENTER();

    transform *Root = GetLocalPoseTransform(Pose, 0);
    if(Root)
    {
        VectorAdd3(Root->Position, Translation3);
        quad Rotation4;
        AngularVelocityToQuaternion(Rotation4, Rotation3);
        QuaternionMultiply4(Root->Orientation,
                            Rotation4,
                            Root->Orientation);
    }
}

void GRANNY
UpdateModelMatrix(model_instance const &ModelInstance,
                  real32 SecondsElapsed,
                  real32 const *ModelMatrix4x4,
                  real32 *DestMatrix4x4,
                  bool Inverse)
{
    GRANNY_ENTER();

    triple ResultTranslation;
    triple ResultRotation;
    GetRootMotionVectors(ModelInstance, SecondsElapsed,
                         ResultTranslation, ResultRotation, Inverse);
    ApplyRootMotionVectorsToMatrix(ModelMatrix4x4,
                                   ResultTranslation, ResultRotation,
                                   DestMatrix4x4);
}

void **GRANNY
GetModelUserDataArray(model_instance const &ModelInstance)
{
    GRANNY_ENTER();

    return((void **)ModelInstance.UserData);
}

model_instance *GRANNY
GetGlobalModelInstancesBegin(void)
{
    GRANNY_ENTER();

    return(GetGlobalModelInstanceSentinel().Next);
}

model_instance *GRANNY
GetGlobalModelInstancesEnd(void)
{
    GRANNY_ENTER();

    return(&GetGlobalModelInstanceSentinel());
}

model_instance *GRANNY
GetGlobalNextModelInstance(model_instance *Instance)
{
    GRANNY_ENTER();

    return(Instance ? Instance->Next : 0);
}
