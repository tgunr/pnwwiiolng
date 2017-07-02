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

// Crude hasher for const char* strings
struct ConstCharHasher
{
	size_t operator()(const char* str) const noexcept
	{
		uint64_t hash = 0xc3a5c85c97cb3127ULL;
		auto* it = str;
		while (*it) {
			hash = (hash << 3) ^ static_cast<unsigned char>(*(it++)) ^ (hash >> 29);
		}
		hash += static_cast<uint64_t>(it - str) * 0x1b873593ULL;
		return (sizeof(size_t) == 4)
			? static_cast<size_t>(hash % 0xcc9e2d51ULL)
			: static_cast<size_t>(hash % 0x9ae16a3b2f90404fULL);
	}
};

