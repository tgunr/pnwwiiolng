// ========================================================================
// $File: //jeffr/granny/preprocessor/set_compression.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #14 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>
#include <assert.h>

bool CompressionDriver(input_file&     InputFile,
                       key_value_pair* KeyValues,
                       granny_int32x   NumKeyValues,
                       granny_compression_type CompressionType)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_variant variant;
    GrannyGetDataTreeFromFile(InputFile.GrannyFile, &variant);

    // Write out the converted Info
    granny_int32x FinalSectionCount = InputFile.GrannyFile->SectionCount;
    granny_file_builder *Builder = GrannyBeginFile(FinalSectionCount,
                                                   InputFile.GrannyFile->Header->TypeTag,
                                                   InputFile.GrannyFile->SourceMagicValue,
                                                   GrannyGetTemporaryDirectory(),
                                                   "Prefix");
    {for (int SectionIdx = 0; SectionIdx < FinalSectionCount; ++SectionIdx)
    {
        granny_grn_section *SectionArray =
            GrannyGetGRNSectionArray(InputFile.GrannyFile->Header);

        GrannySetFileSectionFormat(Builder,
                                   SectionIdx,
                                   CompressionType,
                                   SectionArray[SectionIdx].InternalAlignment);
    }}

    granny_file_data_tree_writer *Writer =
        GrannyBeginFileDataTreeWriting(variant.Type, variant.Object,
                                       0, 0);
    GrannyPreserveObjectFileSections(Writer, InputFile.GrannyFile);
    GrannyWriteDataTreeToFileBuilder(Writer, Builder);
    GrannyEndFileDataTreeWriting(Writer);

    char const* OutputFile = FindFirstValueForKey(KeyValues, NumKeyValues, "output");
    assert(OutputFile);

	bool success = GrannyEndFile(Builder, OutputFile);
    if (!success)
    {
        ErrOut("failed to write to %s\n", OutputFile);
    }

    return success;
}

bool Decompress(input_file&     InputFile,
                key_value_pair* KeyValues,
                granny_int32x   NumKeyValues,
                granny_memory_arena*)
{
    return CompressionDriver(InputFile, KeyValues, NumKeyValues, GrannyNoCompression);
}


bool OodleCompress(input_file&     InputFile,
                   key_value_pair* KeyValues,
                   granny_int32x   NumKeyValues,
                   granny_memory_arena*)
{
    return CompressionDriver(InputFile, KeyValues, NumKeyValues, GrannyOodle1Compression);
}


static CommandRegistrar RegCompress(eSingleFile_NoMultiRun, OodleCompress, "Compress",
                                    "Compresses the input file using Oodle1");
static CommandRegistrar RegDecompress(eSingleFile_NoMultiRun, Decompress, "Decompress",
                                      "Removes any compression on the input file");

