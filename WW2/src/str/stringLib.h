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
# define _strlwr(n) strlwr(n)
# define _strupr(n) strupr(n)
# define _stricmp(x,y) strcasecmp(x,y)
# define _strnicmp(x,y,len) strncasecmp(x,y,len)
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

template <size_t _DestSize>
static inline void strsafecpy(char (&dest)[_DestSize], const char* src) noexcept
{
	strncpy(dest, src, _DestSize - 1);
	dest[_DestSize - 1] = 0;
}

template <size_t _DestSize>
static inline void strsafecpy(wchar_t (&dest)[_DestSize], const wchar_t* src) noexcept
{
	wcsncpy(dest, src, _DestSize - 1);
	dest[_DestSize - 1] = 0;
}

static inline void strsafecpy(char* dest, const char* src, size_t sizeWithNul) noexcept
{
	strncpy(dest, src, sizeWithNul - 1);
	dest[sizeWithNul - 1] = 0;
}

static inline void strsafecpy(wchar_t* dest, const wchar_t* src, size_t sizeWithNul) noexcept
{
	wcsncpy(dest, src, sizeWithNul - 1);
	dest[sizeWithNul - 1] = 0;
}

// Now let's have some actual C++ string utility going on here.
std::string string_replace(const std::string &haystack, const std::string &needle, const std::string &replacement);
std::vector<std::string> split_string(const std::string &str, const std::string &delimiter, size_t max_split = SIZE_MAX);
bool has_prefix(const std::string &str, const std::string &prefix);
std::string strtolower(const std::string &str);
std::string strtoupper(const std::string &str);
std::string strtrim(const std::string &str);
