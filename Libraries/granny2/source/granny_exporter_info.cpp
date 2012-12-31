// ========================================================================
// $File: //jeffr/granny/rt/granny_exporter_info.cpp $
// $DateTime: 2007/12/03 22:49:11 $
// $Change: 16696 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_exporter_info.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ExporterInfoType[] =
{
    {StringMember, "ExporterName"},

    {Int32Member, "ExporterMajorRevision"},
    {Int32Member, "ExporterMinorRevision"},
    {Int32Member, "ExporterCustomization"},
    {Int32Member, "ExporterBuildNumber"},

    {VariantReferenceMember, "ExtendedData" },

    {EndMember},
};

END_GRANNY_NAMESPACE;
