#pragma once
// Date: Mon Feb 02 14:36:43 2001
//
// Copyright (c) 2001 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: John "Kango" Lundy

// TODO: Replace memory allocators ACROSS THE BOARD with the much smarter stuff
// that C++11 allows. - Gadget
#if platOS == OS_WIN32
#define WEAK_MALLOC_ATTRIBUTE
#define STRONG_MALLOC_ATTRIBUTE
#else
#define WEAK_MALLOC_ATTRIBUTE __attribute__((always_inline, nothrow, warn_unused_result))
#define STRONG_MALLOC_ATTRIBUTE WEAK_MALLOC_ATTRIBUTE __attribute__((malloc))
#endif

#include <cstdlib>

static inline void memInit() {}
static inline void memKill() {}

static inline STRONG_MALLOC_ATTRIBUTE void *memAlloc(size_t s) { return std::calloc(1, s); }
static inline STRONG_MALLOC_ATTRIBUTE void *memCalloc(size_t n, size_t s) { return std::calloc(n, s); }
static inline WEAK_MALLOC_ATTRIBUTE   void *memRealloc(void *p, size_t s) { return std::realloc(p, s); }
static inline void memFree(void *p) { std::free(p); }
