#if !defined(GRANNY_MEMORY_FILE_WRITER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_file_writer.h $
// $DateTime: 2009/06/23 13:30:11 $
// $Change: 22123 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileWriterGroup);

struct file_writer;
EXPAPI GS_SAFE file_writer *CreateMemoryFileWriter(int32x BlockSize);

// Allows you to grab the buffer from the writer once you're done.
// Dispose of the buffer with FreeMemoryWriterBuffer
EXPAPI GS_PARAM bool StealMemoryWriterBuffer(file_writer* Writer,
                                             uint8**      BufferPtr,
                                             int32x*      BufferSize);

EXPAPI GS_PARAM void FreeMemoryWriterBuffer(uint8* Buffer);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMORY_FILE_WRITER_H
#endif
