#pragma once
//! @file platforms.h
//! @brief Platform-specific defines and includes.

#include "compiler_check.h"

// Check actual compiler - Clang 3.6, GCC 4.8, or VS2015
#if !__check_Clang_version(3,6) && !__check_GCC_version(4,8) && !__check_MSVC_version(19)
# error "Clang 3.6 or newer, GCC 4.8 or newer, or VS2015 or newer is required."
#endif

#if __is_Clang()

#define PN_LIKELY(p)   __builtin_expect(!!(p), 1)
#define PN_UNLIKELY(p) __builtin_expect(!!(p), 0)
#define PRSizeT "zu"
#define FORMAT_FUN(n,m) [[gnu::format(printf,n,m)]]
#define FORMAT_ARG(n)
#define NORETURN_FN [[noreturn]]

#elif __is_GCC()

#define PN_LIKELY(p)   __builtin_expect(!!(p), 1)
#define PN_UNLIKELY(p) __builtin_expect(!!(p), 0)
#define PRSizeT "zu"
#define FORMAT_FUN(n,m) __attribute__((format(printf,n,m)))
#define FORMAT_ARG(n)
#define NORETURN_FN __attribute__ ((noreturn))

#else

// Microsoft recommends profile-guided optimization instead of static prediction
// hints. Well, such is life.
#define PN_LIKELY(p) (p)
#define PN_UNLIKELY(p) (p)
// MSVC also flouts the standard with size_t format specifiers.
#define PRSizeT "Iu"
// And finally, it ALSO doesn't implement compiler-checked format strings like everyone else!
#define FORMAT_FUN(n,m)
#define FORMAT_ARG(n)   _Printf_format_string_params_(n)
#define NORETURN_FN [[noreturn]]

// Add some explicit warnings
#pragma warning( 1 : 4505 )	// unreferenced local functions
#pragma warning( 1 : 4101 )	// unreferenced local variables
#pragma warning( 1 : 4102 )	// unreferenced local label
#pragma warning( 1 : 4121 )	// packing caution

#endif

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#ifndef platOS
# error "No platform defined by compiler"
#endif

#if defined(__BIG_ENDIAN__)
#error This is a big endian platform and is unsupported.
#endif

// Known operating systems
#define	OS_WIN32	1
#define	OS_MACOS	2
#define	OS_LINUX	3
#define OS_MAX_PLAT	4

// Used by platBuild
#define TEUL_CLIENT		0
#define	TEUL_SERVER		1

#if defined(WIN32) || defined(WIN64)

#ifndef NOMINMAX
# define NOMINMAX 1	//@ for windows.
#endif

#include <io.h>
#include <time.h>
static inline struct tm *localtime_r(const time_t *t, struct tm *b) { localtime_s(b, t); return b; }
#endif

#include "include/precompile_includes.h"
#include "sys/sysWinISO.h"

// snprintf wrapper
template<size_t bufSize, typename... Args>
UINT32 strprintf(char(&buf)[bufSize], const char* fmt, Args&&... args)
{
    return static_cast<UINT32>(snprintf(buf, bufSize, fmt, std::forward<Args>(args)...));
}

// Cross-platform path builder.
template<size_t bufSize>
UINT32 pathprintf(char (&buf)[bufSize], const char* lhs, const char* rhs)
{
	return strprintf<bufSize>(buf, "%s/%s", lhs, rhs);
}

#if defined(AS_SERVER)
#ifndef PATH_MAX
#define PATH_MAX 256
#endif
#endif