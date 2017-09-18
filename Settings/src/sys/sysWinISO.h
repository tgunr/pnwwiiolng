#if defined(WIN32) || defined(WIN64)

#pragma once

// This file is just a place to define all those
// Microsoft/ISO departure from those "evil",
// portable, POSIX standards like "open()".

// You need to make this the last thing you #include

#if _MSC_VER >= 1500 && !defined(VS_2008)
#define VS_2008
#endif

#if defined(VS_2005) || defined(VS_2008)

#if !USE_OSG
// We have access functions and the Java Script uses close....see fiod.cpp for definitions
# define close(f) _close(f)
#endif

# define open(f,...) _open(f,__VA_ARGS__)
# define read(...) _read(__VA_ARGS__)
# define write(...) _write(__VA_ARGS__)
# define lseek(f, m, o) _lseek(f, m, o)
# define mkdir(f) _mkdir(f)
# define creat(f,p) _creat(f,p)
# define fileno(f) _fileno(f)
# ifndef stricmp
#  define stricmp(x,y) _stricmp(x,y)
# endif
# ifndef strlwr
#  define strlwr(x) _strlwr(x)
# endif
# ifndef strupr
#  define strupr(x) _strupr(x)
# endif

#endif // VS_2005

#define   swprintf   _snwprintf
#define	  vswprintf	 _vsnwprintf

#endif // WIN32 || WIN64
