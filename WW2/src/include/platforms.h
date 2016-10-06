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
#define FORMAT_FN(n,m) __attribute__((format(printf,n,m)))
#define FORMAT_ARG(n)
#define NORETURN_FN [[noreturn]]

#elif __is_GCC()

#define PN_LIKELY(p)   __builtin_expect(!!(p), 1)
#define PN_UNLIKELY(p) __builtin_expect(!!(p), 0)
#define PRSizeT "zu"
#define FORMAT_FN(n,m) __attribute__((format(printf,n,m)))
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
#define FORMAT_FN(n,m)
#define FORMAT_ARG(n)   _Printf_format_string_params_(n)
#define NORETURN_FN [[noreturn]]

// Add some explicit warnings
#pragma warning( 1 : 4505 )	// unreferenced local functions
#pragma warning( 1 : 4101 )	// unreferenced local variables
#pragma warning( 1 : 4102 )	// unreferenced local label
#pragma warning( 1 : 4121 )	// packing caution

#endif

//////////////////////////////////////////////////////////////////////////////
// Overall inclusions
#include "include/precompile_includes.h"

//////////////////////////////////////////////////////////////////////////////
// Windows-specific things
#if platOS == OS_WIN32

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

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

static inline struct tm *localtime_r(const time_t *t, struct tm *b) {
    localtime_s(b, t);
    return b;
}

static inline long random() {
    unsigned int value;
    rand_s(&value);
    return value; // Win32 is ILP64 means this returns enough entropy for a long
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
static inline void OS_CLOSESOCKET(SOCKET sock) { closesocket(sock); }

//////////////////////////////////////////////////////////////////////////////
// Equivalents for macOS/Linux
#elif platOS == OS_MACOS || platOS == OS_LINUX

static inline int OS_OPEN(int *fdp, const char *path, int oflag, int cmode) {
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
static inline void OS_CLOSESOCKET(SOCKET sock) { close(sock); }

#else

#error Unknown platform

#endif

//////////////////////////////////////////////////////////////////////////////
// Some common definitions

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

#ifndef PATH_MAX
#define PATH_MAX 256
#endif
