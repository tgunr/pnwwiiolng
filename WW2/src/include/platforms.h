#pragma once
//! @file platforms.h
//! @brief Platform-specific defines and includes.

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#ifndef platOS
# error "No platform defined by compiler"
#endif

// Known operating systems
#define	OS_WIN32	1
#define	OS_MACOS	2
#define	OS_LINUX	3
#define OS_MAX_PLAT	4

// Which is TEUL_CLIENT and which is TEUL_SERVER
// Used by platBuild
#define TEUL_CLIENT		0
#define	TEUL_SERVER		1

#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1700)
# define OVERRIDE_FN override
#else
# define OVERRIDE_FN
#endif

#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1701)
# define CONSTEXPR_FN constexpr
#else
# define CONSTEXPR_FN
#endif

// Determine which version of GCC, if any, is being used,
// and for GCC/MSVC, determine what C++0X support is
// available.
#ifdef __GNUC__
# define GCC_VERSION ( ((__GNUC__) * 100) + (__GNUC_MINOR__) )
# if GCC_VERSION < 400
#	error("GCC version 4.0 or higher is required to compile this code.")
# endif
# ifdef __LP64__
#  define PRSizeT "zu"
# else
#  define PRSizeT "u"
# endif
# define PROffT "lld"
# define NORETURN_FN __attribute__ ((noreturn))
#else	// Windows or non-GCC
# define GCC_VERSION 0
# define __attribute__(...)
# define PRSizeT "lu"
# define PROffT "ld"
# if defined(_MSC_VER)
#  pragma warning( 1 : 4505 )	// unreferenced local functions
#  pragma warning( 1 : 4101 )	// unreferenced local variables
#  pragma warning( 1 : 4102 )	// unreferenced local label
#  pragma warning( 1 : 4121 )	// packing caution
//#  pragma warning( 1 : 4702 )
#  define NORETURN_FN __declspec(noreturn)
# endif
#endif

#if (defined(GCC_VERSION) && GCC_VERSION >= 460) || (defined(_MSC_VER) && _MSC_VER > 1600)
# define ENUMCLASS enum class
#else
# define ENUMCLASS enum
#endif

#if defined(__BIG_ENDIAN__)
#error This is a big endian platform and is unsupported.
#endif

#if defined(WIN32) || defined(WIN64)
# ifndef NOMINMAX
#  define NOMINMAX 1	//@ for windows.
# endif
# define localtime_r(x, y) localtime(x)
# define PATH_SEPARATOR '\\'
# define PATH_SEPSTR "\\"
#else
# define PATH_SEPARATOR '/'
# define PATH_SEPSTR "/"
#endif

# define SUB_PATH(Pfx, Sfx) Pfx PATH_SEPSTR Sfx

#undef DBA_ENABLE_SQLITE

#if defined(WIN32) || defined(WIN64)

#if _MSC_VER < 1800	// VS2013 has this in stdlib.h
static inline unsigned long long strtoull(const char* const src_, const char* const, unsigned int);
static inline signed long long strtoll(const char* const src_, const char* const, unsigned int);
static inline float strtof(const char* const src_, const char* const);
#endif

#include <io.h>
#endif

#if platOS == OS_LINUX
# define MAX_PATH PATH_MAX
#endif

#ifndef MAXPATHLEN
# define MAXPATHLEN MAX_PATH
#endif

#include "include/precompile_includes.h"

#include "sys/sysWinISO.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

// Cross-platform path builder.
template<size_t bufSize>
size_t pathprintf(char (& buf)[bufSize], const char* lhs, const char* rhs)
{
	int printed = snprintf(buf, bufSize, "%s" PATH_SEPSTR "%s", lhs, rhs);
	if ( printed != -1 && static_cast<size_t>(printed) < bufSize )
		return static_cast<size_t>(printed);
	buf[bufSize - 1] = 0;
	return (bufSize - 1);
}

// snprintf wrapper
template<size_t bufSize>
size_t strprintf(char (& buf)[bufSize], const char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	int printed = vsnprintf(buf, bufSize, fmt, list);
	va_end(list);
	if ( printed != -1 && static_cast<size_t>(printed) < bufSize )
		return static_cast<size_t>(printed);
	buf[bufSize - 1] = 0;
	return (bufSize - 1);
}
