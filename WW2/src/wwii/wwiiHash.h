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

# if platOS == OS_LINUX
#  if defined(__llvm__)
#   include <tr1/unordered_map>
#   include <tr1/unordered_set>
    #define UNORDERED_MAP std::tr1::unordered_map
    #define UNORDERED_SET std::tr1::unordered_set
#  elif GCC_VERSION < 404
#   error("Linux builds require Gnu C++ Compiler version 4.4 or higher (because we're not stupid).")
#  else
#   include <unordered_map>
#   include <unordered_set>
    #define UNORDERED_MAP std::unordered_map
    #define UNORDERED_SET std::unordered_set
#  endif
# else // Mac.
	// Mac is still using an old GCC and doesn't support C++0x,
	// and the Mac looks set to use clang in the future which
	// has even worse C++0x support than GCC 4.4 (lacks
	// tr1 changes, constexpr, pod changes, union changes,
	// inheriting constructors, class member changes,
	// preprocessor mods, lambdas and uniform initializers;
	// i.e. all the good stuff that wasn't already in C99).
	#include <ext/hash_map>
	#include <ext/hash_set>
	#include <ext/hash_fun.h>

	#define UNORDERED_MAP stdext::hash_map
	#define UNORDERED_SET stdext::hash_set

	// In GCC 3.3.4 the bastards not only relocated the
	// hash files, they also frickin' re-namespaced 'em
	namespace stdext = ::__gnu_cxx;

	struct OBJECT;

	// Define any common hash fns here
	namespace __gnu_cxx
	{
		template<> struct hash<UINT64> { UINT64 operator()(UINT64 __x) const { return __x; } };
		template<> struct hash<std::string>  { size_t operator()(std::string const& __s) const  { return __stl_hash_string(__s.c_str()); } };
		template<> struct hash<OBJECT*>  { size_t operator()(OBJECT const* o) const  { return *(size_t*)(&o); } };
		template<> struct hash<void*>  { size_t operator()(void const* o) const  { return *(size_t*)(&o); } };

	};
# endif
#elif defined(WIN32) || defined(WIN64)
	#include <hash_map>
	#include <hash_set>
	#if _MSC_VER >= 1600
		#include <unordered_map>
		#include <unordered_set>
		#define UNORDERED_MAP std::unordered_map
		#define UNORDERED_SET std::unordered_set
	#else
		#define UNORDERED_MAP std::map
		#define UNORDERED_SET std::set
	#endif
#else
#error "This platform has no stlhash.h support defined"
#endif

