#pragma once
//! @file platforms.h
//! @brief Platform-specific defines and includes.

#include "compiler_check.h"

//////////////////////////////////////////////////////////////////////////////
// Check baseline environment
#if defined(__BIG_ENDIAN__)
#error This is a big endian platform and is unsupported.
#endif

// Known operating systems
#define OS_WIN32    1
#define OS_MACOS    2
#define OS_LINUX    3
#define OS_MAX_PLAT 4

// Used by platBuild
#define BUILD_CLIENT 0
#define BUILD_SERVER 1

#if !defined(platOS) || platOS >= OS_MAX_PLAT || platOS < 1
# error "No platform defined by compiler"
#endif

#if !defined(platBuild) || platBuild > BUILD_SERVER
# error "No build defined by compiler"
#endif

//////////////////////////////////////////////////////////////////////////////
// Check actual compiler - Clang 3.6, GCC 4.8, or VS2015
#if !__check_Clang_version(3,6) && !__check_GCC_version(4,8) && !__check_MSVC_version(19)
# error "Clang 3.6 or newer, GCC 4.8 or newer, or VS2015 or newer is required."
#endif

#if __is_Clang()

#define PN_LIKELY(p)   __builtin_expect(!!(p), 1)
#define PN_UNLIKELY(p) __builtin_expect(!!(p), 0)
#define PRSizeT "zu"
#define PRSsizeT "zd"
#define FORMAT_FN(n,m) __attribute__((format(printf,n,m)))
#define FORMAT_ARG(n)
#define NORETURN_FN [[noreturn]]
#define _DISABLE_WARNING(pragma) _Pragma(#pragma)
#define DISABLE_WARNING(warning) \
    _Pragma("clang diagnostic push"); \
    _DISABLE_WARNING(clang diagnostic ignored warning);
#define POP_WARNINGS() \
    _Pragma("clang diagnostic pop");
#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH [[clang::fallthrough]]
#endif

#elif __is_GCC()

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

#define PN_LIKELY(p)   __builtin_expect(!!(p), 1)
#define PN_UNLIKELY(p) __builtin_expect(!!(p), 0)
#define PRSizeT "zu"
#define PRSsizeT "zd"
#define FORMAT_FN(n,m) __attribute__((format(printf,n,m)))
#define FORMAT_ARG(n)
#if __has_cpp_attribute(noreturn)
#define NORETURN_FN [[noreturn]]
#else
#define NORETURN_FN __attribute__ ((noreturn))
#endif
#define _DISABLE_WARNING(pragma) _Pragma(#pragma)
#define DISABLE_WARNING(warning) \
    _Pragma("GCC diagnostic push") \
    _DISABLE_WARNING(GCC diagnostic ignored warning);
#define POP_WARNINGS() \
    _Pragma("GCC diagnostic pop")
#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH
#endif

#else

// Microsoft recommends profile-guided optimization instead of static prediction
// hints. Well, such is life.
#define PN_LIKELY(p) (p)
#define PN_UNLIKELY(p) (p)
// MSVC also flouts the standard with size_t format specifiers.
#define PRSizeT "Iu"
#define PRSsizeT "Id"
// And finally, it ALSO doesn't implement compiler-checked format strings like everyone else!
#define FORMAT_FN(n,m)
#define FORMAT_ARG(n)   _Printf_format_string_params_(n)
#define NORETURN_FN [[noreturn]]
#define DISABLE_WARNING(warning)
#define POP_WARNINGS()
#define FALLTHROUGH __fallthrough

// Add some explicit warnings
#pragma warning( 1 : 4505 )	// unreferenced local functions
#pragma warning( 1 : 4101 )	// unreferenced local variables
#pragma warning( 1 : 4102 )	// unreferenced local label
#pragma warning( 1 : 4121 )	// packing caution

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#define _CRT_RAND_S

#endif

//////////////////////////////////////////////////////////////////////////////
#if defined(_WIN64) || defined(__x86_64__) || defined(__arm64__)
# define BUILD_64
// LP64 vs. ILP64 (MSVC vs. not)
# if defined(__LP64__)
#  define PR64BitT "lld"
#  define PR64BitUT "llu"
#  define PRPermMaskT "llx"
# else
#  define PR64BitT "ld"
#  define PR64BitUT "lu"
#  define PRPermMaskT "lx"
# endif
#else
# define BUILD_32
# define PR64BitT "lld"
# define PR64BitUT "llu"
# define PRPermMaskT "llx"
#endif

//////////////////////////////////////////////////////////////////////////////
// Overall inclusions
#include "include/precompile_includes.h"

//////////////////////////////////////////////////////////////////////////////
// Windows-specific things
#if platOS == OS_WIN32

#include <io.h>
#include <time.h>
#include <direct.h>

#ifndef stricmp
# define stricmp(x,y) _stricmp(x,y)
#endif
#ifndef strlwr
# define strlwr(x) _strlwr(x)
#endif
#ifndef strupr
# define strupr(x) _strupr(x)
#endif
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define swprintf swprintf_s
#define	vswprintf vswprintf_s

using ssize_t = intptr_t;

static inline struct tm *localtime_r(const time_t *t, struct tm *b) {
    localtime_s(b, t);
    return b;
}

static inline long random() {
    unsigned int value;
    rand_s(&value);
    return value; // Win32 is ILP64 means this returns enough entropy for a long
}


// TESTING
using fullpath_t = wchar_t;
#define PRpathT "ls"
#define PATHLit(s) L##s

#define OS_PATHSEP "\\"
#define OS_PATHSEP_W L"\\"

#define WRONG_PATHSEP "/"
#define WRONG_PATHSEP_W L"/"

static inline int OS_OPEN(int *fdp, const wchar_t *path, int oflag, int cmode) {
    return _wsopen_s(fdp, path, oflag, _SH_DENYRW, cmode);
}
static inline int OS_OPEN(int *fdp, const char *path, int oflag, int cmode) {
	return _sopen_s(fdp, path, oflag, _SH_DENYRW, cmode);
}

static inline void OS_CLOSE(int fd) { _close(fd); }
#define OS_WOPEN(...) _wopen(__VA_ARGS__)
#define OS_READ(...) _read(__VA_ARGS__)
#define OS_WRITE(...) _write(__VA_ARGS__)
#define OS_LSEEK(...) _lseek(__VA_ARGS__)
#define OS_MKDIR(_dir_, ...) _mkdir(_dir_)
#define OS_WMKDIR(_dir_, ...) _wmkdir(_dir_)
#define OS_CREAT(...) _creat(__VA_ARGS__)
static inline void OS_CLOSESOCKET(SOCKET &sock) { if (sock != INVALID_SOCKET) { closesocket(sock); } sock = INVALID_SOCKET; }

//////////////////////////////////////////////////////////////////////////////
// Equivalents for macOS/Linux
#elif platOS == OS_MACOS || platOS == OS_LINUX

using fullpath_t = char;
#define PATHLit(s) s
#define PRpathT "s"

#define OS_PATHSEP "/"
#define OS_PATHSEP_W L"/"

#define WRONG_PATHSEP "\\"
#define WRONG_PATHSEP_W L"\\"

static inline int OS_OPEN(int *fdp, const fullpath_t *path, int oflag, int cmode) {
    *fdp = open(path, oflag, cmode);
    return (*fdp == -1 ? 1 : 0);
}
static inline void OS_CLOSE(int fd) { close(fd); }
#define OS_WOPEN(...) wopen(__VA_ARGS__)
#define OS_READ(...) ::read(__VA_ARGS__)
#define OS_WRITE(...) ::write(__VA_ARGS__)
#define OS_LSEEK(...) ::lseek(__VA_ARGS__)
#define OS_MKDIR(...) mkdir(__VA_ARGS__)
#define OS_WMKDIR(...) wmkdir(__VA_ARGS__)
#define OS_CREAT(f,p) ::creat(f,p)
static inline void OS_CLOSESOCKET(SOCKET &sock) { if (sock != INVALID_SOCKET) { close(sock); } sock = INVALID_SOCKET; }

#else

#error Unknown platform

#endif

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#if platOS == OS_WIN32
#define PLAT_EWOULDBLOCK WSAEWOULDBLOCK
#define PLAT_EINTR WSAEINTR
#define PLAT_EAGAIN EAGAIN
#define PLAT_EINPROGRESS WSAEINPROGRESS
#define PLAT_ECONNREFUSED WSAECONNREFUSED
#else
#define PLAT_EWOULDBLOCK EWOULDBLOCK
#define PLAT_EINTR EINTR
#define PLAT_EAGAIN EAGAIN
#define PLAT_EINPROGRESS EINPROGRESS
#define PLAT_ECONNREFUSED ECONNREFUSED
#endif

