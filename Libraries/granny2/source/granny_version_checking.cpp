// ========================================================================
// $File: //jeffr/granny/rt/granny_version_checking.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_version_checking.h"
#include "granny_log.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

char const *GRANNY
GetVersionString(void)
{
    GRANNY_ENTER();

    return ProductVersion;
}

void GRANNY
GetVersion(int32x &MajorVersion,
           int32x &MinorVersion,
           int32x &BuildNumber,
           int32x &Customization)
{
    GRANNY_ENTER();

    MajorVersion  = ProductMajorVersion;
    MinorVersion  = ProductMinorVersion;
    BuildNumber   = ProductBuildNumber;
    Customization = ProductCustomization;
}

bool GRANNY
VersionsMatch_(int32x MajorVersion, int32x MinorVersion,
               int32x BuildNumber, int32x Customization)
{
    GRANNY_ENTER();

    if ((MajorVersion  == ProductMajorVersion) &&
        (MinorVersion  == ProductMinorVersion) &&
        (BuildNumber   == ProductBuildNumber)  &&
        (Customization == ProductCustomization))
    {
        return true;
    }
    else
    {
        Log8(WarningLogMessage, VersionLogMessage,
             "Version of DLL (%d.%d.%d.%d) does not match "
             "version of .h (%d.%d.%d.%d)",
             ProductMajorVersion, ProductMinorVersion,
             ProductBuildNumber,  ProductCustomization, 
             MajorVersion, MinorVersion,
             BuildNumber,  Customization);

        return false;
    }
}
