// ========================================================================
// $File: //jeffr/granny/preprocessor/samples/combine_rigid_meshes.cpp $
// $DateTime: 2009/05/28 17:04:46 $
// $Change: 21863 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "../preprocessor.h"
#include <assert.h>
#include <vector>
#include <set>

using namespace std;


// Designates a mesh/material grouping pair to combine
typedef pair<granny_mesh*, int> Submesh;

struct SICmp : public binary_function<char const*, char const*, bool>
{
    bool operator()(char const* One, char const* Two) const
    {
        return _stricmp(One, Two) < 0;
    }
};

typedef set<char const*, SICmp> BoneSet;

struct CombineSpec
{
    granny_material* Material;

    vector<Submesh> Submeshes;

    // This is lazy, but it's an easy way to allow multiple meshes that share a bone to
    // use the same mesh grouping without blowing the bone limit.
    BoneSet UniqueBones;
};


// This function only handles meshes with 32-bit indices.  It outputs pwnt3132 verts.
granny_mesh*
MeshFromSpec(CombineSpec& Spec,
             granny_memory_arena* Arena)
{
    granny_mesh* NewMesh = PushObject(Arena, granny_mesh);
    memset(NewMesh, 0, sizeof(granny_mesh));
    NewMesh->Name = PushString(Arena, "Combined");

    NewMesh->PrimaryVertexData = PushObject(Arena, granny_vertex_data);
    memset(NewMesh->PrimaryVertexData, 0, sizeof(granny_vertex_data));

    NewMesh->PrimaryTopology = PushObject(Arena, granny_tri_topology);
    memset(NewMesh->PrimaryTopology, 0, sizeof(granny_tri_topology));

    NewMesh->MaterialBindingCount = 1;
    NewMesh->MaterialBindings = PushArray(Arena, 1, granny_material_binding);
    NewMesh->MaterialBindings[0].Material = Spec.Material;

    vector<granny_bone_binding>    BoneBindings;
    vector<granny_pwnt3132_vertex> NewVertices;
    vector<granny_int32>           NewIndices;

    {for(int SubIdx = 0; SubIdx < (int)Spec.Submeshes.size(); ++SubIdx)
    {
        Submesh& sub = Spec.Submeshes[SubIdx];

        granny_mesh* OrigMesh = sub.first;
        granny_tri_material_group& Group = OrigMesh->PrimaryTopology->Groups[sub.second];

        int const VertSize = GrannyGetTotalObjectSize(OrigMesh->PrimaryVertexData->VertexType);

        int BindingIdx = -1;
        {for(int Idx = 0; Idx < (int)BoneBindings.size(); ++Idx)
        {
            if (_stricmp(BoneBindings[Idx].BoneName, OrigMesh->BoneBindings[0].BoneName) == 0)
            {
                BindingIdx = Idx;
                break;
            }
        }}
        if (BindingIdx == -1)
        {
            BindingIdx = (int)BoneBindings.size();
            BoneBindings.push_back(OrigMesh->BoneBindings[0]);
            memset(&BoneBindings.back().OBBMin, 0, sizeof(BoneBindings.back().OBBMin));
            memset(&BoneBindings.back().OBBMax, 0, sizeof(BoneBindings.back().OBBMax));
        }


        vector<granny_int32> Indices32(GrannyGetMeshIndexCount(OrigMesh));
        GrannyCopyMeshIndices(OrigMesh, 4, &Indices32[0]);

        vector<int> NewVertLocs(OrigMesh->PrimaryVertexData->VertexCount, -1);
        {for(int Idx = Group.TriFirst * 3; Idx < (Group.TriFirst + Group.TriCount) * 3; ++Idx)
        {
            granny_int32 OldVertLoc = Indices32[Idx];
            granny_int32 NewVertLoc = NewVertLocs[OldVertLoc];
            if (NewVertLoc == -1)
            {
                // Need to copy the vert into the new buffer
                NewVertLocs[OldVertLoc] = (int)NewVertices.size();
                NewVertLoc = NewVertLocs[OldVertLoc];
                NewVertices.resize(NewVertices.size() + 1);

                granny_pwnt3132_vertex* NewVert = &(NewVertices.back());
                granny_uint8*           OldVert = OrigMesh->PrimaryVertexData->Vertices + (OldVertLoc * VertSize);

                GrannyConvertSingleObject(OrigMesh->PrimaryVertexData->VertexType,
                                          OldVert,
                                          GrannyPWNT3132VertexType,
                                          NewVert);

                NewVert->BoneIndex = BindingIdx;
            }

            NewIndices.push_back(NewVertLoc);
        }}
    }}

    NewMesh->PrimaryVertexData->VertexType = GrannyPWNT3132VertexType;
    NewMesh->PrimaryVertexData->Vertices = (granny_uint8*)PushArray(Arena, (int)NewVertices.size(), granny_pwnt3132_vertex);
    memcpy(NewMesh->PrimaryVertexData->Vertices, &NewVertices[0], NewVertices.size() * sizeof(granny_pwnt3132_vertex));
    NewMesh->PrimaryVertexData->VertexCount = (int)NewVertices.size();

    NewMesh->PrimaryTopology->Indices = PushArray(Arena, (int)NewIndices.size(), granny_int32);
    memcpy(NewMesh->PrimaryTopology->Indices, &NewIndices[0], NewIndices.size() * 4);
    NewMesh->PrimaryTopology->IndexCount = (int)NewIndices.size();

    NewMesh->PrimaryTopology->GroupCount = 1;
    NewMesh->PrimaryTopology->Groups = PushArray(Arena, 1, granny_tri_material_group);
    NewMesh->PrimaryTopology->Groups[0].MaterialIndex = 0;
    NewMesh->PrimaryTopology->Groups[0].TriFirst = 0;
    NewMesh->PrimaryTopology->Groups[0].TriCount = (int)(NewIndices.size() / 3);

    NewMesh->BoneBindings = PushArray(Arena, (int)BoneBindings.size(), granny_bone_binding);
    memcpy(NewMesh->BoneBindings, &BoneBindings[0], BoneBindings.size() * sizeof(granny_bone_binding));
    NewMesh->BoneBindingCount = (int)BoneBindings.size();

    return NewMesh;
}



bool CombineRigidMeshes(input_file&     InputFile,
                        key_value_pair* KeyValues,
                        granny_int32x   NumKeyValues,
                        granny_memory_arena* Arena)
{
    char const* OutputFile = FindFirstValueForKey(KeyValues, NumKeyValues, "output");
    if (OutputFile == 0)
    {
        ErrOut("must specify an output file with \"-output <filename>\"\n");
        return false;
    }

    char const* BoneLimitStr = FindFirstValueForKey(KeyValues, NumKeyValues, "bonelimit");
    int BoneLimit = -1;
    if (BoneLimitStr != 0)
    {
        BoneLimit = atoi(BoneLimitStr);
        if (BoneLimit <= 0)
        {
            ErrOut("Invalid bone limit number (must be > 0)\n");
        }
    }

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("must specify an output file with \"-output <filename>\"\n");
        return false;
    }

    vector<granny_mesh*> NewMeshes;
    vector<CombineSpec> Specs;
    {for (int MeshIdx = 0; MeshIdx < Info->MeshCount; ++MeshIdx)
    {
        granny_mesh* Mesh = Info->Meshes[MeshIdx];
        if (Mesh == 0)
            continue;

        if (!GrannyMeshIsRigid(Mesh))
        {
            // Already skinned
            NewMeshes.push_back(Mesh);
            continue;
        }

        // This /is/ a rigid mesh, but it's not one that we can throw into a skin, since
        // there's no bone to reference.
        if (Mesh->BoneBindingCount == 0)
            continue;

        assert(Mesh->PrimaryTopology->GroupCount == Mesh->MaterialBindingCount);
        {for(int GroupIdx = 0; GroupIdx < Mesh->MaterialBindingCount; ++GroupIdx)
        {
            granny_material_binding& Binding = Mesh->MaterialBindings[GroupIdx];

            bool Found = false;
            {for(int Idx = 0; Idx < (int)Specs.size(); ++Idx)
            {
                CombineSpec& Spec = Specs[Idx];
                if (Spec.Material != Binding.Material)
                    continue;

                char const* BoneName = Mesh->BoneBindings[0].BoneName;
                if (BoneLimit > 0)
                {
                    if (Spec.UniqueBones.find(BoneName) == Spec.UniqueBones.end() &&
                        int(Spec.UniqueBones.size()) >= BoneLimit)
                    {
                        continue;
                    }
                }

                Specs[Idx].Submeshes.push_back(Submesh(Mesh, GroupIdx));
                Specs[Idx].UniqueBones.insert(Mesh->BoneBindings[0].BoneName);

                Found = true;
                break;
            }}

            if (!Found)
            {
                // New spec
                CombineSpec spec;
                spec.Material = Binding.Material;
                spec.Submeshes.push_back(Submesh(Mesh, GroupIdx));
                spec.UniqueBones.insert(Mesh->BoneBindings[0].BoneName);
                Specs.push_back(spec);
            }
        }}
    }}

    {for(int Idx = 0; Idx < (int)Specs.size(); ++Idx)
    {
        granny_mesh* NewMesh = MeshFromSpec(Specs[Idx], Arena);
        NewMeshes.push_back(NewMesh);
    }}

    Info->MeshCount = (int)NewMeshes.size();
    Info->Meshes    = &NewMeshes[0];

    Info->Models[0]->MeshBindingCount = (int)NewMeshes.size();
    Info->Models[0]->MeshBindings = PushArray(Arena, Info->Models[0]->MeshBindingCount, granny_model_mesh_binding);
    for (int Binding = 0; Binding < Info->Models[0]->MeshBindingCount; ++Binding)
        Info->Models[0]->MeshBindings[Binding].Mesh = NewMeshes[Binding];


    // Replace model bindings...

    // ===================================================================================
    // ===================================================================================

    granny_int32x FinalSectionCount = File->SectionCount;
    if (FinalSectionCount < GrannyStandardSectionCount)
        FinalSectionCount = GrannyStandardSectionCount;

    granny_file_builder *Builder = GrannyBeginFile(FinalSectionCount,
                                                   GrannyCurrentGRNStandardTag,
                                                   GrannyGRNFileMV_ThisPlatform,
                                                   GrannyGetTemporaryDirectory(),
                                                   "Prefix");
    granny_file_data_tree_writer *Writer =
        GrannyBeginFileDataTreeWriting(GrannyFileInfoType, Info,
                                       GrannyStandardDiscardableSection, 0);
    GrannyPreserveObjectFileSections(Writer, File);

    GrannyWriteDataTreeToFileBuilder(Writer, Builder);
    GrannyEndFileDataTreeWriting(Writer);

    bool success = GrannyEndFile(Builder, OutputFile);
    if (!success)
        ErrOut("failed to write to %s\n", OutputFile);

    return success;
}


static CommandRegistrar RegCombineRigidMeshes(eSingleFile_NoMultiRun,
                                              CombineRigidMeshes, "CombineRigidMeshes",
                                              "Combines rigid meshes sharing a material into skinned meshes");
