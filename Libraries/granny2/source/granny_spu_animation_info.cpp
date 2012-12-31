// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation_info.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_spu_animation_info.h"

#include "granny_data_type_conversion.h"
#include "granny_file.h"
#include "granny_file_format.h"
#include "granny_log.h"
#include "granny_spu_animation.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition SPUAnimationInfoType[] =
{
    {ArrayOfReferencesMember, "SPUAnimations", SPUAnimationType},
    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

spu_animation_info* GRANNY
GetSPUAnimationInfo(file &File)
{
    GRANNY_ENTER();

    variant Root;
    GetDataTreeFromFile(File, &Root);

    uint32 TypeTag = File.Header->TypeTag;
    if(TypeTag == CurrentGRNStandardTag)
    {
        return((spu_animation_info *)Root.Object);
    }
    else
    {
        if (!File.ConversionBuffer)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File has run-time type tag of 0x%x, which doesn't match this "
                 "version of Granny (0x%x).  Automatic conversion will "
                 "be attempted.", TypeTag, CurrentGRNStandardTag);

            File.ConversionBuffer =
                ConvertTree(Root.Type, Root.Object, SPUAnimationInfoType);
        }

        return((spu_animation_info *)File.ConversionBuffer);
    }
}

