// ========================================================================
// $File: //jeffr/granny/rt/granny_art_tool_info.cpp $
// $DateTime: 2008/10/09 15:44:24 $
// $Change: 18938 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_art_tool_info.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ArtToolInfoType[] =
{
    { StringMember, "FromArtToolName"      },
    { Int32Member,  "ArtToolMajorRevision" },
    { Int32Member,  "ArtToolMinorRevision" },
    { Int32Member,  "ArtToolPointerSize"   },

    { Real32Member, "UnitsPerMeter"     },
    { Real32Member, "Origin",      0, 3 },
    { Real32Member, "RightVector", 0, 3 },
    { Real32Member, "UpVector",    0, 3 },
    { Real32Member, "BackVector",  0, 3 },

    { VariantReferenceMember, "ExtendedData" },

    { EndMember },
};

END_GRANNY_NAMESPACE;
