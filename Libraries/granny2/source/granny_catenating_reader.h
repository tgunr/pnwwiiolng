#if !defined(GRANNY_CATENATING_READER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_catenating_reader.h $
// $DateTime: 2009/01/06 19:02:10 $
// $Change: 19985 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct file_reader;
file_reader* CreateCatenatingReader(int32x        NumReaders,
                                    file_reader** Readers,
                                    int32x        Alignment);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CATENATING_READER_H
#endif
