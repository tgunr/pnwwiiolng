// ========================================================================
// $File: //jeffr/granny/preprocessor/samples/model_mesh_bounds.cpp $
// $DateTime: 2010/03/18 16:18:02 $
// $Change: 25775 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "../preprocessor.h"
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace std;

inline void UpdateMin(granny_real32* Base, granny_real32 const* New)
{
    Base[0] = min(Base[0], New[0]);
    Base[1] = min(Base[1], New[1]);
    Base[2] = min(Base[2], New[2]);
}

inline void UpdateMax(granny_real32* Base, granny_real32 const* New)
{
    Base[0] = max(Base[0], New[0]);
    Base[1] = max(Base[1], New[1]);
    Base[2] = max(Base[2], New[2]);
}


bool ModelMeshBounds(input_file&     InputFile,
                     key_value_pair* KeyValues,
                     granny_int32x   NumKeyValues,
                     granny_memory_arena* TempArena)
{
    char const* OutputFile = FindFirstValueForKey(KeyValues, NumKeyValues, "output");
    if (OutputFile == 0)
    {
        ErrOut("must specify an output file with \"-output <filename>\"\n");
        return false;
    }

    granny_file_info* Info = GrannyGetFileInfo(InputFile.GrannyFile);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    if (Info->ModelCount == 0)
    {
        ErrOut("this version of the command requires at least one model to work with\n");
        return false;
    }

    // We'll use this in the variant builder...
    granny_string_table* StringTable = GrannyNewStringTable();

    for (int ModelIdx = 0; ModelIdx < Info->ModelCount; ++ModelIdx)
    {
        granny_model* Model = Info->Models[ModelIdx];
        if (Model == 0)
            continue;

        if (Model->MeshBindingCount == 0)
        {
            WarnOut("skipping model: '%s'.  No meshes bound.\n", Model->Name);
            continue;
        }

        bool BoundsValid = false;
        granny_real32 MeshMin[3] = { 1e38f, 1e38f, 1e38f };
        granny_real32 MeshMax[3] = { -1e38f, -1e38f, -1e38f };

        {for (int MeshIdx = 0; MeshIdx < Model->MeshBindingCount; ++MeshIdx)
        {
            granny_mesh* Mesh = Model->MeshBindings[MeshIdx].Mesh;
            if (!Mesh)
                continue;

            int const VertCount = GrannyGetMeshVertexCount(Mesh);
            if (VertCount == 0)
                continue;

            BoundsValid = true;

            vector<granny_p3_vertex> Verts(VertCount);
            GrannyCopyMeshVertices(Mesh, GrannyP3VertexType, &Verts[0]);

            {for (int VertIdx = 0; VertIdx < VertCount; ++VertIdx)
            {
                UpdateMin(MeshMin, Verts[VertIdx].Position);
                UpdateMax(MeshMax, Verts[VertIdx].Position);
            }}
        }}


        // Ok, we now have the three arrays we want.  Create a variant that we'll attach
        // to the model
        if (BoundsValid)
        {
            granny_variant_builder* VariantBuilder = GrannyBeginVariant(StringTable);

            GrannyAddScalarArrayMember(VariantBuilder, "MeshAABBMin", 3, MeshMin);
            GrannyAddScalarArrayMember(VariantBuilder, "MeshAABBMax", 3, MeshMax);

            granny_int32x TypeSize = GrannyGetResultingVariantTypeSize(VariantBuilder);
            granny_int32x ObjSize  = GrannyGetResultingVariantObjectSize(VariantBuilder);

            GrannyEndVariantInPlace(VariantBuilder,
                                    PushArray(TempArena, TypeSize, granny_uint8),
                                    &Model->ExtendedData.Type,
                                    PushArray(TempArena, ObjSize, granny_uint8),
                                    &Model->ExtendedData.Object);
        }
        else
        {
            WarnOut("skipping model: '%s'.  No meshes with vertices found.\n", Model->Name);
        }
    }

    // Write out the file, we're done!
    granny_int32x FinalSectionCount = InputFile.GrannyFile->SectionCount;
    granny_file_builder *Builder = GrannyBeginFile(FinalSectionCount,
                                                   GrannyCurrentGRNStandardTag,
                                                   GrannyGRNFileMV_ThisPlatform,
                                                   GrannyGetTemporaryDirectory(),
                                                   "Prefix");
    granny_file_data_tree_writer *Writer =
        GrannyBeginFileDataTreeWriting(GrannyFileInfoType, Info,
                                       GrannyStandardDiscardableSection, 0);
    GrannyPreserveFileSectionFormats(Builder, InputFile.GrannyFile);
    GrannyPreserveObjectFileSections(Writer, InputFile.GrannyFile);

    GrannyWriteDataTreeToFileBuilder(Writer, Builder);
    GrannyEndFileDataTreeWriting(Writer);

    bool const success = GrannyEndFile(Builder, OutputFile);

    // Clean up the string table, now that the file has been written
    GrannyFreeStringTable(StringTable);

    return success;
}

static CommandRegistrar RegModelMeshBounds(eSingleFile_NoMultiRun,
                                           ModelMeshBounds,
                                           "ModelMeshBounds",
                                           "Attached bounds information for the input animation");
