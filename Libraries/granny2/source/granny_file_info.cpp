// ========================================================================
// $File: //jeffr/granny/rt/granny_file_info.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #9 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_file_info.h"

#include "granny_animation.h"
#include "granny_art_tool_info.h"
#include "granny_data_type_conversion.h"
#include "granny_exporter_info.h"
#include "granny_file.h"
#include "granny_file_format.h"
#include "granny_log.h"
#include "granny_material.h"
#include "granny_mesh.h"
#include "granny_model.h"
#include "granny_texture.h"
#include "granny_track_group.h"
#include "granny_tri_topology.h"
#include "granny_vertex_data.h"

// This should always be the last header included
#include "granny_cpp_settings.h"


USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition FileInfoType[] =
{
    {ReferenceMember, "ArtToolInfo", ArtToolInfoType},
    {ReferenceMember, "ExporterInfo", ExporterInfoType},

    {StringMember, "FromFileName"},

    {ArrayOfReferencesMember, "Textures", TextureType},
    {ArrayOfReferencesMember, "Materials", MaterialType},

    {ArrayOfReferencesMember, "Skeletons", SkeletonType},
    {ArrayOfReferencesMember, "VertexDatas", VertexDataType},
    {ArrayOfReferencesMember, "TriTopologies", TriTopologyType},
    {ArrayOfReferencesMember, "Meshes", MeshType},
    {ArrayOfReferencesMember, "Models", ModelType},

    {ArrayOfReferencesMember, "TrackGroups", TrackGroupType},
    {ArrayOfReferencesMember, "Animations", AnimationType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

file_info *GRANNY
GetFileInfo(file &File)
{
    GRANNY_ENTER();

    variant Root;
    GetDataTreeFromFile(File, &Root);

    uint32 TypeTag = File.Header->TypeTag;
    if(TypeTag == CurrentGRNStandardTag)
    {
        return((file_info *)Root.Object);
    }
    else
    {
        if(!File.ConversionBuffer)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File has run-time type tag of 0x%x, which doesn't match this "
                 "version of Granny (0x%x).  Automatic conversion will "
                 "be attempted.", TypeTag, CurrentGRNStandardTag);

            File.ConversionBuffer =
                ConvertTree(Root.Type, Root.Object, FileInfoType);
        }

        return((file_info *)File.ConversionBuffer);
    }
}

