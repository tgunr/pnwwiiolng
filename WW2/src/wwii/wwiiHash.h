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

#include <unordered_map>
#include <unordered_set>

struct OBJECT;

namespace std
{
		template<> struct hash<OBJECT*>  { size_t operator()(OBJECT const* o) const  { return *(size_t*)(&o); } };
		template<> struct hash<void*>  { size_t operator()(void const* o) const  { return *(size_t*)(&o); } };
}

// Crude hasher for const char* strings
template<size_t BaseValue, size_t PrimeFactor>
size_t ConstCharHasher(const char* text)
{
	size_t hash = BaseValue;
	const char* it;
	for (it = text; *it; ++it) {
		hash = (hash << 3) ^ *it ^ (hash >> 29);
	}
	hash ^= (it - text);
	return hash % PrimeFactor;
}
