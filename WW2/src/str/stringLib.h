#pragma once
// Created: Fri Mar 31 16:24:00 2000
//
// The following file is a trimmed down version of snip_str.h from
// http://chesworth.com/pv/resources/snippets.htm#section1group5
//
// All functions used were declared to be public domain and therefore free
// for my bogarting
//
// Originating Programmer: Shane "Elmo" Wickson

#if platOS == OS_LINUX
#define __USE_GNU 1
#else
#define USE_LOCAL_STRNLEN
#endif

#if platOS == OS_WIN32 && !defined(strcasecmp)
# define strcasecmp _stricmp
#endif
#if platOS == OS_WIN32 && !defined(strncasecmp)
# define strncasecmp _strnicmp
#endif
#if platOS == OS_WIN32 && !defined(wstrcasecmp)
# define wstrcasecmp _wcsicmp
# define wstrncasecmp _wcsnicmp
#endif

#ifndef WIN32
# define _strlwr(n) strlwr((n))
# define _strupr(n) strupr((n))
# define _memccpy(dest,source,byte,maxlen) memccpy(dest,source,byte,maxlen)
# ifndef _stricmp
#  define _stricmp(x,y) stricmp(x,y)
# endif
# define _strnicmp(x,y,len) strnicmp(x,y,len)
#else
# define strlwr(n) _strlwr(n)
# define strupr(n) _strupr(n)
# define stricmp(s1,s2) _stricmp(s1,s2)
#endif

#if !defined(WIN32) && !defined(WIN64)
extern char* strlwr(char* string);
extern char* strupr(char* string);
#else
extern char* stptok(const char* s, char* tok, size_t toklen, char* brk);
#endif
extern char* rmlead(char*);
extern char* rmtrail(char* str);
extern char* strescape(char* d, const char* s, char c);

#if defined(_MSC_VER) && _MSC_VER >= 1400
#elif platOS == OS_MACOS
extern size_t strnlen(const char* string, size_t maxlen);
#endif

static inline int strlencpy(char* dest, const char* source, int maxlen)
{
char* end;
int len;

	end = (char*)_memccpy(dest,source,'\0',maxlen);

	if (end)
	{
		len = (int)(end-dest);
	}
	else
	{
		*(dest+maxlen) = '\0';
		len = maxlen;
	}

	return (len-1);
}
