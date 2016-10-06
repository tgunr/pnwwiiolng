#pragma once

// This file is just a place to define all those
// Microsoft/ISO departure from those "evil",
// portable, POSIX standards like "open()".

// You need to make this the last thing you #include
#include <algorithm>
#include "teulSockConfig.h"

#if platOS == OS_WIN32

#include <direct.h>

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

#endif
