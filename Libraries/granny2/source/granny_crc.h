#if !defined(GRANNY_CRC_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_crc.h $
// $DateTime: 2009/09/10 15:35:57 $
// $Change: 23183 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

void BeginCRC32(uint32 &CRC);
void AddToCRC32(uint32 &CRC, uint64x Count, void const *UInt8s);
void EndCRC32(uint32 &CRC);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CRC_H
#endif
