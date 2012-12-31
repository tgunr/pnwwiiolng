// ========================================================================
// $File: //jeffr/granny/preprocessor/add_texture.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #11 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "add_texture.h"
#include "utilities.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.h"

#include <assert.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

static granny_texture_builder*
GetTextureBuilder(key_value_pair* KeyValues,
                  granny_int32x NumKeyValues,
                  int Width, int Height)
{
    char const* Format = FindFirstValueForKey(KeyValues, NumKeyValues, "format");
    if (!Format)
        Format = "Bink";

    int CompLevel = 8;
    {
        char const* Comp = FindFirstValueForKey(KeyValues, NumKeyValues, "compression");
        if (Comp)
            CompLevel = atoi(Comp);
    }

    if (_stricmp(Format, "Bink") == 0)
        return GrannyBeginBinkTexture(Width, Height, CompLevel, GrannyBinkUseBink1);
    else if (_stricmp(Format, "BinkAlpha") == 0)
        return GrannyBeginBinkTexture(Width, Height, CompLevel, GrannyBinkUseBink1|GrannyBinkEncodeAlpha);
    else if (_stricmp(Format, "S3TC") == 0)
        return GrannyBeginBestMatchS3TCTexture(Width, Height);
    else if (_stricmp(Format, "RawRGB") == 0)
        return GrannyBeginRawTexture(Width, Height, GrannyRGB888PixelFormat, 1);
    else if (_stricmp(Format, "RawRGBA") == 0)
        return GrannyBeginRawTexture(Width, Height, GrannyRGBA8888PixelFormat, 1);

    return 0;
}


bool
AddTexture(input_file*,
           granny_int32x,
           key_value_pair* KeyValues,
           granny_int32x NumKeyValues,
           granny_memory_arena* Arena)
{
    RequireKey("database", "must specify a database with \"-database <filename>\"\n");
    char const* DatabaseName = FindFirstValueForKey(KeyValues, NumKeyValues, "database");

    granny_file_info Dummy;
    memset(&Dummy, 0, sizeof(Dummy));

    granny_file*      File = GrannyReadEntireFile(DatabaseName);
    granny_file_info* Info = 0;
    {
        if (File) Info = GrannyGetFileInfo(File);
        else      Info = &Dummy;
    }

    typedef map<char const*, granny_texture*, CIStrcmp> texture_map_t;
    texture_map_t TexMap;
    for (int Idx = 0; Idx < Info->TextureCount; ++Idx)
    {
        TexMap[Info->Textures[Idx]->FromFileName] = Info->Textures[Idx];
    }

    {for(int Idx = 0; Idx < NumKeyValues; ++Idx)
    {
        char const* Filename = KeyValues[Idx].Value;
        int x, y, comp;
        stbi_uc* ImageBytes = stbi_load(Filename, &x, &y, &comp, 4);
        if (!ImageBytes)
            continue;

        granny_texture_builder* Builder = GetTextureBuilder(KeyValues, NumKeyValues, x, y);
        if (!Builder)
        {
            stbi_image_free(ImageBytes);
            continue;
        }

        GrannyEncodeImage(Builder, x, y, x * 4, 1, ImageBytes);
        granny_texture *ResultingTexture =
            GrannyEndTextureInPlace(Builder,
                                    PushArray(Arena, GrannyGetResultingTextureSize(Builder),
                                              granny_uint8));
        ResultingTexture->FromFileName = Filename;
        TexMap[Filename] = ResultingTexture;
    }}

    if (TexMap.empty())
    {
        ErrOut("no textures found in db or in added list\n");
        return false;
    }

    int NumNewTextures = (int)TexMap.size();
    granny_texture** NewTextures = PushArray(Arena, NumNewTextures, granny_texture*);
    {
        int Idx = 0;
        for (texture_map_t::iterator Itr = TexMap.begin();
             Itr != TexMap.end(); ++Itr)
        {
            NewTextures[Idx++] = Itr->second;
        }
        assert(Idx == NumNewTextures);
    }

    Info->TextureCount = NumNewTextures;
    Info->Textures = NewTextures;

    // Write out the resulting file.  Note that we have to fake the options, since
    // WriteInfoPreserve wants -database, not -output...
    key_value_pair Pair = { "output", (char*)DatabaseName };
    return WriteInfoPreserve(&Pair, 1, Info, File, true, true, true);
}

// Register the commands
static CommandRegistrar RegAddTexture(AddTexture, "AddTexture",
                                      "Adds a texture to the file specified with '-database'");

