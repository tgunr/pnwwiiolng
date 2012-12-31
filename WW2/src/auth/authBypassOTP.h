#pragma once
// Created: Tue Nov  4 20:09:19 CST 2008
//
// Defines the One Time Password that provides auth bypass.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#if defined(HAPPYOO)
# define AUTH_BYPASS_OTP 3735928559UL
#else
# define AUTH_BYPASS_OTP 2342426UL
#endif

