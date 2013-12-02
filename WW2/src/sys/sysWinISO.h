#pragma once

#if defined(WIN32) || defined(WIN64)

// This file is just a place to define all those
// Microsoft/ISO departure from those "evil",
// portable, POSIX standards like "open()".

// You need to make this the last thing you #include

#if _MSC_VER >= 1500 && !defined(VS_2008)
#define VS_2008
#endif

#if defined(VS_2005) || defined(VS_2008)

#include <direct.h>

# define OS_ALLOCA(...) _malloca(__VA_ARGS__)
# define OS_CLOSE(...) _close(__VA_ARGS__)
# define OS_OPEN(...) _open(__VA_ARGS__)
# define OS_WOPEN(...) _wopen(__VA_ARGS__)
# define OS_DUP(...) _dup(__VA_ARGS__)
# define OS_FDOPEN(...) _fdopen(__VA_ARGS__)
# define OS_READ(...) _read(__VA_ARGS__)
# define OS_WRITE(...) _write(__VA_ARGS__)
# define OS_LSEEK(...) _lseek(__VA_ARGS__)
# define OS_MKDIR(_dir_, ...) _mkdir(_dir_)
# define OS_WMKDIR(_dir_, ...) _wmkdir(_dir_)  // wide version
# define OS_CHDIR(...) _chdir(__VA_ARGS__)
# define OS_CREAT(...) _creat(__VA_ARGS__)
# define OS_WCREAT(...) _wcreat(__VA_ARGS__)  // wide version
# define OS_FILENO(...) _fileno(__VA_ARGS__)
# define OS_ACCESS(...) _access(__VA_ARGS__)
# define OS_WACCESS(...) _waccess(__VA_ARGS__)
# ifndef stricmp
#  define stricmp(x,y) _stricmp(x,y)
# endif
# ifndef strlwr
#  define strlwr(x) _strlwr(x)
# endif
# ifndef strupr
#  define strupr(x) _strupr(x)
# endif
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
# define snprintf _snprintf

#endif // VS_2005

#define   swprintf   _snwprintf
#define	  vswprintf	 _vsnwprintf

#if _MSC_VER < 1800 // visual studio 2013 defines this in stdlib.h
static inline unsigned long long strtoull(const char* const src_, const char* const, unsigned int)
{
	unsigned long long into = 0;
	(void)sscanf(src_, "%I64u", &into);
	return into;
}
static inline signed long long strtoll(const char* const src_, const char* const, unsigned int)
{
	signed long long into = 0;
	(void)sscanf(src_, "%I64d", &into);
	return into;
}
static inline float strtof(const char* const src_, const char* const)
{
	float into = 0;
	(void)sscanf(src_, "%f", &into);
	return into;
}
#endif

#else

# define OS_ALLOCA(...) ::alloca(__VA_ARGS__)
# define OS_OPEN(...) ::open(__VA_ARGS__)
# define OS_CLOSE(...) ::close(__VA_ARGS__)
# define OS_WOPEN(...) wopen(__VA_ARGS__)
# define OS_DUP(...) ::dup(__VA_ARGS__)
# define OS_FDOPEN(...) fdopen(__VA_ARGS__)
# define OS_READ(...) ::read(__VA_ARGS__)
# define OS_WRITE(...) ::write(__VA_ARGS__)
# define OS_LSEEK(...) ::lseek(__VA_ARGS__)
# define OS_MKDIR(...) mkdir(__VA_ARGS__)
# define OS_WMKDIR(...) wmkdir(__VA_ARGS__)  // wide version
# define OS_CHDIR(f) chdir(f)
# define OS_CREAT(f,p) ::creat(f,p)
# define OS_WCREAT(f,p) wcreat(f,p)  // wide version
# define OS_FILENO(f) fileno(f)
# define OS_ACCESS(...) ::access(__VA_ARGS__)
# define OS_WACCESS(...) waccess(__VA_ARGS__)

#endif // WIN32 || WIN64
