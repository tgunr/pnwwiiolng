// ========================================================================
// $File: //jeffr/granny/rt/granny_model.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #8 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_model.h"

#include "granny_mesh.h"

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ModelMeshBindingType[] =
{
    {ReferenceMember, "Mesh", MeshType},
    {EndMember},
};

data_type_definition ModelType[] =
{
    {StringMember, "Name"},
    {ReferenceMember, "Skeleton", SkeletonType},

    {TransformMember, "InitialPlacement"},

    {ReferenceToArrayMember, "MeshBindings", ModelMeshBindingType},
    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

void GRANNY
GetModelInitialPlacement4x4(model const &Model,
                            real32 *Placement4x4)
{
    GRANNY_ENTER();

    BuildCompositeTransform4x4(Model.InitialPlacement, Placement4x4);
}
