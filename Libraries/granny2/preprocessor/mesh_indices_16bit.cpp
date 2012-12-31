// ========================================================================
// $File: //jeffr/granny/preprocessor/mesh_indices_16bit.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #14 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>


bool MeshIndices16Bit(input_file&     InputFile,
                      key_value_pair* KeyValues,
                      granny_int32x   NumKeyValues,
                      granny_memory_arena* TempArena)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file_info* Info = GrannyGetFileInfo(InputFile.GrannyFile);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    // Optimize all meshes for the pre/post-transform cache
    {for(granny_int32x i = 0; i < Info->MeshCount; i++)
    {
        granny_mesh* Mesh = Info->Meshes[i];
        if (!Mesh)
            continue;

        if (GrannyGetMeshVertexCount(Mesh) <= (1 << 16))
        {
            // Replace the 32-bit indices with 16-bit indices
            granny_int32x  IndexCount   = GrannyGetMeshIndexCount(Mesh);
            granny_uint16* NewIndices16 =
                PushArray(TempArena, IndexCount, granny_uint16);

            GrannyCopyMeshIndices(Mesh, 2, NewIndices16);
            Mesh->PrimaryTopology->IndexCount = 0;
            Mesh->PrimaryTopology->Indices = 0;
            Mesh->PrimaryTopology->Index16Count = IndexCount;
            Mesh->PrimaryTopology->Indices16 = NewIndices16;
        }
        else
        {
            // error here potentially?
        }
    }}

    // Write out the resulting file...
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, InputFile.GrannyFile, true, true, true);
}


static CommandRegistrar RegMeshIndices16Bit(eSingleFile_NoMultiRun,
                                            MeshIndices16Bit,
                                            "MeshIndices16Bit",
                                            "Converts Meshes to 16-bit indices where possible (i.e, vert counts of less than 65536)");

