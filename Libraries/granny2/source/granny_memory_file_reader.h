#if !defined(GRANNY_MEMORY_FILE_READER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_file_reader.h $
// $DateTime: 2009/01/05 15:45:49 $
// $Change: 19963 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileReaderGroup);

struct file_reader;
EXPAPI GS_SAFE file_reader* CreateMemoryFileReader(int32x MemorySize, void const* Memory);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMORY_FILE_READER_H
#endif
