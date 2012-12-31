#if !defined(GRANNY_TELEMETRY_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_telemetry.h $
// $DateTime: 2009/08/21 15:27:28 $
// $Change: 22857 $
// $Revision: #1 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_COMPILER_H)
#include "granny_compiler.h"
#endif

#if GRANNY_TELEMETRY

   #if COMPILER_MSVC || COMPILER_GCC || COMPILER_METROWERKS
      #define GRANNY_ENTER() (void)sizeof(__FUNCTION__)
   #else
      #define GRANNY_ENTER() (void)sizeof(0)
   #endif

#else

   #define GRANNY_ENTER() (void)sizeof(0)

#endif

#include "header_postfix.h"
#define GRANNY_TELEMETRY_H
#endif /* GRANNY_TELEMETRY_H */
