// ========================================================================
// $File: //jeffr/granny/preprocessor/transform_file.cpp $
// $DateTime: 2010/03/24 14:46:42 $
// $Change: 25872 $
// $Revision: #15 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AxisMapping
{
    const char*   AxisString;
    granny_triple Axis;
};
static const AxisMapping AxisMap[] = {
    { "X", { 1, 0, 0 } },
    { "Y", { 0, 1, 0 } },
    { "Z", { 0, 0, 1 } },
    { "NegX", { -1, 0, 0 } },
    { "NegY", { 0, -1, 0 } },
    { "NegZ", { 0, 0, -1 } },

    { NULL, { 0, 0, 0 } }
};

void FindAxisArg(const char* Arg, granny_triple* Axis)
{
    if (Arg == 0)
        return;

    {for(int i = 0; AxisMap[i].AxisString; ++i)
    {
        if (_stricmp(AxisMap[i].AxisString, Arg) == 0)
        {
            memcpy(Axis, &AxisMap[i].Axis, sizeof(granny_triple));
            return;
        }
    }}

}


bool TransformFile(input_file&     InputFile,
                   key_value_pair* KeyValues,
                   granny_int32x   NumKeyValues,
                   granny_memory_arena*)
{
    RequireKey("output", "must specify an output file with \"-output <filename>\"\n");

    granny_file* File = InputFile.GrannyFile;
    granny_file_info* Info = GrannyGetFileInfo(File);
    if (Info == 0)
    {
        ErrOut("unable to obtain a granny_file_info from the input file\n");
        return false;
    }

    // Parse out the coordinate system
    granny_real32 UnitsPerMeter = 1.0f;
    granny_triple RightVector = { 1, 0, 0 };
    granny_triple UpVector    = { 0, 1, 0 };
    granny_triple BackVector  = { 0, 0, 1 };
    granny_triple Origin      = { 0, 0, 0 };
    {
        char const* UPMArg = FindFirstValueForKey(KeyValues, NumKeyValues, "unitsPerMeter");
        if (UPMArg != 0)
        {
            UnitsPerMeter = granny_real32(atof(UPMArg));
            if (UnitsPerMeter <= 0.0f)
            {
                ErrOut("units per meter must be > 0 (%f)\n", UnitsPerMeter);
                return false;
            }
        }

        FindAxisArg(FindFirstValueForKey(KeyValues, NumKeyValues, "right"), &RightVector);
        FindAxisArg(FindFirstValueForKey(KeyValues, NumKeyValues, "up"),    &UpVector);
        FindAxisArg(FindFirstValueForKey(KeyValues, NumKeyValues, "back"),  &BackVector);
    }

    // Compute the basis transformation
    granny_real32 Affine3[3];
    granny_real32 Linear3x3[9];
    granny_real32 InverseLinear3x3[9];
    if (GrannyComputeBasisConversion(Info,
                                     UnitsPerMeter,
                                     Origin,
                                     RightVector,
                                     UpVector,
                                     BackVector,
                                     Affine3, Linear3x3, InverseLinear3x3) == false)
    {
        ErrOut("GrannyComputeBasisConversion failed.  Probably bad coordinate system specified\n");
        return false;
    }

    // Just transform the file...
    GrannyTransformFile(Info,
                        Affine3, Linear3x3, InverseLinear3x3,
                        1e-5f, 1e-5f,
                        GrannyRenormalizeNormals | GrannyReorderTriangleIndices);

    // Since we've transformed into the final coordinate system, we
    // don't need the ArtToolInfo anymore.  Go ahead and remove that
    // from the output
    Info->ArtToolInfo = 0;


    // Write out the modified Info
    return WriteInfoPreserve(KeyValues, NumKeyValues, Info, File, true, true, false);
}

static CommandRegistrar RegTransformFile(eSingleFile_NoMultiRun,
                                         TransformFile,
                                         "TransformFile",
                                         "Converts to a consistent coordinate system.");

