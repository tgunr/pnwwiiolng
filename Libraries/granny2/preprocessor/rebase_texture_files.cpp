// ========================================================================
// $File: //jeffr/granny/preprocessor/rebase_texture_files.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>


static char const*
RebasePath(char const* BasePath,
           char const* TexPath)
{
    size_t const BasePathLen = strlen(BasePath);
    if (_strnicmp(BasePath, TexPath, BasePathLen) == 0)
    {
        // Can rebase this one, but account for any path separator at the beginning, since
        // the basepath might not have included it...
        char const* RetPath = TexPath + BasePathLen;
        while (*RetPath == '\\' || *RetPath == '/')
            ++RetPath;

        return RetPath;
    }
    else
    {
        // Path doesn't match, return unchanged...
        return TexPath;
    }
}


bool RebaseTextureFiles(input_file&     InputFile,
                        key_value_pair* KeyValues,
                        granny_int32x   NumKeyValues,
                        granny_memory_arena*)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from %s\n", InputFile.Filename);
        return false;
    }

    char const* BasePath = FindFirstValueForKey(KeyValues, NumKeyValues, "basepath");
    if (BasePath == 0)
    {
        ErrOut("must specify an base path for rebasing with \"-basepath path\"\n");
        return false;
    }

    {for(granny_int32x TexIdx = 0; TexIdx < Info->TextureCount; ++TexIdx)
    {
        if (!Info->Textures[TexIdx])
            continue;
        Info->Textures[TexIdx]->FromFileName =
            RebasePath(BasePath, Info->Textures[TexIdx]->FromFileName);
    }}


    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}


static CommandRegistrar RegRebaseTextureFiles(eSingleFile_MultiRun,
                                              RebaseTextureFiles,
                                              "RebaseTextureFiles",
                                              "Given a -basepath argument, makes any absolute textures relative to that path, if possible");
