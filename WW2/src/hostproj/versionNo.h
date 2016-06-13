#pragma once
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library

#ifndef WWII_RELEASE_TYPE
#undef FRES_SERVER				///config=fres
#undef DEBUG_BUILD				///config=debug
#undef DOROTHY					///config=dot
#endif

#define BRIDGES

#ifdef DOROTHY
# define DEV_BUILD
#endif

#ifdef DEV_BUILD
# define CLOSED_BETA
# if !defined(DEBUG)
#  define DEBUG
# endif
#endif

#if defined(DEV_BUILD)
# define SINGLE_CELL_HOST
#endif

#if defined(CLOSED_BETA) || defined(TEST_RELEASE) || defined(OPEN_BETA)
# ifndef TEST_RELEASE
#  define BETA_RELEASE
# else	// TEST_RELEASE
#  define PUBLIC_RELEASE
# endif // TEST_RELEASE
#elif ! (defined(DEBUG_BUILD) || defined(PUBLIC_RELEASE))
# error "No build type defined"
#endif

#ifdef BETA_RELEASE
static const bool isBETA_RELEASE = true;
#else
static const bool isBETA_RELEASE = false;
#endif

#ifdef PUBLIC_RELEASE
static const bool isPUBLIC_RELEASE = true;
#else
static const bool isPUBLIC_RELEASE = false;
#endif

#if defined(PN_DISABLE_AUTH)
static const bool useAuthServer = false;
#else
static const bool useAuthServer = true;
#endif

