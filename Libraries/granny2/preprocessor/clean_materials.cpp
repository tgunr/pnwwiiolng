// ========================================================================
// $File: //jeffr/granny/preprocessor/clean_materials.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #18 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>
#include <set>

using namespace std;

typedef set<granny_material*> MaterialSet;
typedef set<granny_texture*>  TextureSet;


void CleanUnreferencedMaterials(granny_file_info* FileInfo,
                                granny_memory_arena* TempArena)
{
    MaterialSet MeshReferencedMaterials;

    // Loop through all the meshes, checking references
    {for(granny_int32x Idx = 0; Idx < FileInfo->MeshCount; ++Idx)
    {
        granny_mesh* Mesh = FileInfo->Meshes[Idx];

        // and all the material bindings on the mesh
        {for(granny_int32x Binding = 0; Binding < Mesh->MaterialBindingCount; ++Binding)
        {
            if (Mesh->MaterialBindings[Binding].Material != NULL)
                MeshReferencedMaterials.insert(Mesh->MaterialBindings[Binding].Material);
        }}
    }}

    // This is really a suck-tastic way to walk the material tree, but it's easy to verify
    // that it works.
    bool inserted;
    MaterialSet FinalMaterials;
    TextureSet FinalTextures;
    do
    {
        inserted = false;

		MaterialSet TempMaterials;
        {for(MaterialSet::iterator itr = MeshReferencedMaterials.begin();
             itr != MeshReferencedMaterials.end();
             ++itr)
        {
            granny_material* MeshMaterial = *itr;
            if (FinalMaterials.insert(MeshMaterial).second)
            {
                inserted = true;
                if (MeshMaterial->Texture != NULL)
                    FinalTextures.insert(MeshMaterial->Texture);

                {for(granny_int32x Map = 0; Map < MeshMaterial->MapCount; ++Map)
                {
                    granny_material* SubMaterial = MeshMaterial->Maps[Map].Material;
					TempMaterials.insert(SubMaterial);
                    //FinalMaterials.insert(MeshMaterial->Maps[Map].Material);
                    //if (SubMaterial->Texture != NULL)
                    //    FinalTextures.insert(SubMaterial->Texture);
                }}
            }
        }}

        MeshReferencedMaterials.insert(TempMaterials.begin(), TempMaterials.end());
    } while (inserted);

    // Ok, create the final material array...
    FileInfo->MaterialCount = (granny_int32)FinalMaterials.size();
    if (FileInfo->MaterialCount != 0)
    {
        FileInfo->Materials =
            PushArray(TempArena, FileInfo->MaterialCount, granny_material*);

        granny_int32x Current = 0;
        {for(MaterialSet::iterator itr = FinalMaterials.begin();
             itr != FinalMaterials.end();
             ++itr)
        {
            FileInfo->Materials[Current++] = *itr;
        }}
    }
    else
    {
        FileInfo->Materials = 0;
    }

    // ...and the final texture array
    FileInfo->TextureCount = (granny_int32)FinalTextures.size();
    if (FileInfo->TextureCount != 0)
    {
        FileInfo->Textures =
            PushArray(TempArena, FileInfo->TextureCount, granny_texture*);

        granny_int32x Current = 0;
        {for(TextureSet::iterator itr = FinalTextures.begin();
             itr != FinalTextures.end();
             ++itr)
        {
            FileInfo->Textures[Current++] = *itr;
        }}
    }
    else
    {
        FileInfo->Textures = 0;
    }
}

bool CleanMaterials(input_file&     InputFile,
                    key_value_pair* KeyValues,
                    granny_int32x   NumKeyValues,
                    granny_memory_arena* TempArena)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* FileInfo = GrannyGetFileInfo(File);
    if (FileInfo == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input model file\n");
        return false;
    }

    // Remove unused materials and textures
    CleanUnreferencedMaterials(FileInfo, TempArena);

    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, FileInfo, File, true, true, false);
}


static CommandRegistrar RegCleanMaterials(eSingleFile_NoMultiRun,
                                          CleanMaterials,
                                          "CleanMaterials",
                                          "Removes unreferenced materials from the file");

