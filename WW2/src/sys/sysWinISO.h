#pragma once

// This file is just a place to define all those
// Microsoft/ISO departure from those "evil",
// portable, POSIX standards like "open()".

// You need to make this the last thing you #include
#include <algorithm>

#if platOS == OS_WIN32

#include <direct.h>

#define OS_ALLOCA(...) _malloca(__VA_ARGS__)
#define OS_CLOSE(...) _close(__VA_ARGS__)
static inline int OS_OPEN(int *fdp, const char *path, int oflag, int cmode) { return _sopen_s(fdp, path, oflag, _SH_DENYRW, cmode); }
#define OS_WOPEN(...) _wopen(__VA_ARGS__)
#define OS_DUP(...) _dup(__VA_ARGS__)
#define OS_FDOPEN(...) _fdopen(__VA_ARGS__)
#define OS_READ(...) _read(__VA_ARGS__)
#define OS_WRITE(...) _write(__VA_ARGS__)
#define OS_LSEEK(...) _lseek(__VA_ARGS__)
#define OS_MKDIR(_dir_, ...) _mkdir(_dir_)
#define OS_WMKDIR(_dir_, ...) _wmkdir(_dir_)  // wide version
#define OS_CHDIR(...) _chdir(__VA_ARGS__)
#define OS_CREAT(...) _creat(__VA_ARGS__)
#define OS_WCREAT(...) _wcreat(__VA_ARGS__)  // wide version
#define OS_FILENO(...) _fileno(__VA_ARGS__)
#define OS_ACCESS(...) _access(__VA_ARGS__)
#define OS_WACCESS(...) _waccess(__VA_ARGS__)
#define OS_CLOSESOCKET(...) closesocket(__VA_ARGS__)

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

#else

// Provide the passthrough defines for non-Windows platforms
#define OS_ALLOCA(...) ::alloca(__VA_ARGS__)
static inline int OS_OPEN(int *fdp, const char *path, int oflag, int cmode) { *fdp = open(path, oflag, cmode); return (*fdp == -1 ? 1 : 0); }
#define OS_CLOSE(...) ::close(__VA_ARGS__)
#define OS_WOPEN(...) wopen(__VA_ARGS__)
#define OS_DUP(...) ::dup(__VA_ARGS__)
#define OS_FDOPEN(...) fdopen(__VA_ARGS__)
#define OS_READ(...) ::read(__VA_ARGS__)
#define OS_WRITE(...) ::write(__VA_ARGS__)
#define OS_LSEEK(...) ::lseek(__VA_ARGS__)
#define OS_MKDIR(...) mkdir(__VA_ARGS__)
#define OS_WMKDIR(...) wmkdir(__VA_ARGS__)  // wide version
#define OS_CHDIR(f) chdir(f)
#define OS_CREAT(f,p) ::creat(f,p)
#define OS_WCREAT(f,p) wcreat(f,p)  // wide version
#define OS_FILENO(f) fileno(f)
#define OS_ACCESS(...) ::access(__VA_ARGS__)
#define OS_WACCESS(...) waccess(__VA_ARGS__)
#define OS_CLOSESOCKET(...) close(__VA_ARGS__)

#endif
