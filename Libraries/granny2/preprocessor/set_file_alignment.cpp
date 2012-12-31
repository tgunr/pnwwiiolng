// ========================================================================
// $File: //jeffr/granny/preprocessor/set_file_alignment.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #3 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
ValidAlignment(char const* Alignment, int* AlignVal)
{
    assert(AlignVal);

    if (Alignment == 0)
        return false;

    int& Val = *AlignVal;

    Val = atoi(Alignment);
    if (Val <= 3)
        return false;

    // !pow2?
    if (Val & (Val - 1))
        return false;

    return true;
}

// Special version of this function to preserve platform and compression settings while
// setting alignment.
static bool
WriteInfoPreserveWithAlign(char const* OutputFile,
                           granny_file_info* Info,
                           granny_file* OriginalFile,
                           int AlignVal)
{
    assert(OriginalFile);
    assert(AlignVal >= 4 && (AlignVal & (AlignVal-1)) == 0);

    granny_int32x FinalSectionCount = OriginalFile->SectionCount;
    if (FinalSectionCount < GrannyStandardSectionCount)
        FinalSectionCount = GrannyStandardSectionCount;

    granny_grn_file_magic_value const* PlatformMV = OriginalFile->SourceMagicValue;

    granny_file_builder *Builder =
        GrannyBeginFile(FinalSectionCount, GrannyCurrentGRNStandardTag,
                        PlatformMV, GrannyGetTemporaryDirectory(), "Prefix");

    granny_file_data_tree_writer *Writer =
        GrannyBeginFileDataTreeWriting(GrannyFileInfoType, Info,
                                       GrannyStandardDiscardableSection, 0);

    GrannyPreserveObjectFileSections(Writer, OriginalFile);
    GrannyPreserveFileSectionFormats(Builder, OriginalFile);
    GrannySetFileDiskAlignment(Builder, AlignVal);

    GrannyWriteDataTreeToFileBuilder(Writer, Builder);
    GrannyEndFileDataTreeWriting(Writer);

    bool success = GrannyEndFile(Builder, OutputFile);
    if (!success)
    {
        ErrOut("Failed to write granny_file_info to %s\n", OutputFile);
    }

    return success;
}


bool SetFileAlignment(input_file&     InputFile,
                      key_value_pair* KeyValues,
                      granny_int32x   NumKeyValues,
                      granny_memory_arena*)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    int AlignVal = -1;
    char const* Alignment = FindFirstValueForKey(KeyValues, NumKeyValues, "align");
    if (!ValidAlignment(Alignment, &AlignVal))
    {
        ErrOut("must specify an alignment with \"-align <value>\".  Value must be pow2 >= 4\n");
        return false;
    }

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    char const* OutputFile = FindFirstValueForKey(KeyValues, NumKeyValues, "output");
    assert(OutputFile);

    return WriteInfoPreserveWithAlign(OutputFile, Info, File, AlignVal);
}

static CommandRegistrar RegSetFileAlignment(eSingleFile_NoMultiRun,
                                            SetFileAlignment,
                                            "SetFileAlignment",
                                            "Sets the file's on disk alignment to the value specified with \"-align\".");

