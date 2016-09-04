#pragma once

// This file is just a place to define all those
// Microsoft/ISO departure from those "evil",
// portable, POSIX standards like "open()".

// You need to make this the last thing you #include
#include <algorithm>
#include <direct.h>

#define OS_ALLOCA(...) _malloca(__VA_ARGS__)
#define OS_CLOSE(...) _close(__VA_ARGS__)
#define OS_OPEN(...) _open(__VA_ARGS__)
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
