#if !defined(WINXX_GRANNY_DLL_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/winxx/winxx_granny_dll.h $
// $DateTime: 2009/10/01 14:40:04 $
// $Change: 23425 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WINXX_GRANNY_WINDOWS_H)
#include "winxx_granny_windows.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

bool DLLIsNotInWindowsPath(HANDLE DllHandle);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define WINXX_GRANNY_DLL_H
#endif /* WINXX_GRANNY_DLL_H */
