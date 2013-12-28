#pragma once
// Created: Thu Apr 14 12:11:43 CDT 2005
//
// Put wrappers for STL hashes here so they can work
// between platforms.
// Also define your hash operators here.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#ifdef __GNUC__

# if !defined(GCC_VERSION)
#  define GCC_VERSION ( ((__GNUC__) * 100) + (__GNUC_MINOR__) )
# endif

# if defined(__llvm__)
#  define CLANG_VERSION ( (__clang_major__) * 1000 + (__clang_minor__ * 100)  + (__clang_patchlevel__) )
# endif

#endif

#include <hash_map>
#include <hash_set>
#include <unordered_map>
#include <unordered_set>

struct OBJECT;

namespace std
{
		template<> struct hash<OBJECT*>  { size_t operator()(OBJECT const* o) const  { return *(size_t*)(&o); } };
		template<> struct hash<void*>  { size_t operator()(void const* o) const  { return *(size_t*)(&o); } };
}

