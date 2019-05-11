#pragma once
// Created: Fri Mar 31 16:24:00 2000
//
// This file no longer bears any meaningful resemblance to the original Elmo
// cribbed it from.
//
// Originating Programmer: Shane "Elmo" Wickson

#if platOS == OS_WIN32

// Provide the POSIX names used by the rest of the code for various string functions
static inline int strcasecmp(const char *a, const char *b) { return _stricmp(a, b); }
static inline int strncasecmp(const char *a, const char *b, size_t n) { return _strnicmp(a, b, n); }
static inline int wcsncasecmp(const wchar_t *a, const wchar_t *b, size_t n) { return _wcsnicmp(a, b, n); }

#define strlwr(n) _strlwr(n)
#define strupr(n) _strupr(n)
#define vsnprintf _vsnprintf

extern char* stptok(const char* s, char* tok, size_t toklen, char* brk);

extern int vasprintf(char **result, const char *fmt, va_list args);

static inline size_t strlcpy(char *dest, const char *src, size_t len) {
    strcpy_s(dest, len, src);
    return strlen(dest);
}

static inline size_t strlcat(char *dest, const char *src, size_t len) {
    strcat_s(dest, len, src);
    return strlen(dest);
}

#else

extern char* strlwr(char* string);
extern char* strupr(char* string);

# define _strlwr(n) strlwr(n)
# define _strupr(n) strupr(n)

#endif

extern char* rmlead(char*);
extern char* rmtrail(char* str);
extern char* strescape(char* d, const char* s, char c);

template <size_t _DestSize>
static inline void strsafecpy(char (&dest)[_DestSize], const char* src) noexcept
{
    strlcpy(dest, src, _DestSize);
}

template <size_t _DestSize>
static inline void strsafecat(char (&dest)[_DestSize], const char* src) noexcept
{
    strlcat(dest, src, _DestSize);
}

template <size_t _DestSize>
static inline void strsafecpy(wchar_t (&dest)[_DestSize], const wchar_t* src) noexcept
{
	wcsncpy(dest, src, _DestSize - 1);
	dest[_DestSize - 1] = 0;
}

template <size_t _DestSize>
static inline void strsafecat(wchar_t (&dest)[_DestSize], const wchar_t* src) noexcept
{
    wcsncat(dest, src, _DestSize - 1);
    dest[_DestSize - 1] = 0;
}

static inline void strsafecpy(char* dest, const char* src, size_t sizeWithNul) noexcept
{
    strlcpy(dest, src, sizeWithNul);
}

static inline void strsafecat(char* dest, const char* src, size_t sizeWithNul) noexcept
{
    strlcat(dest, src, sizeWithNul);
}

static inline void strsafecpy(wchar_t* dest, const wchar_t* src, size_t sizeWithNul) noexcept
{
	wcsncpy(dest, src, sizeWithNul - 1);
	dest[sizeWithNul - 1] = 0;
}

static inline void strsafecat(wchar_t* dest, const wchar_t* src, size_t sizeWithNul) noexcept
{
    wcsncat(dest, src, sizeWithNul - 1);
    dest[sizeWithNul - 1] = 0;
}

// Now let's have some actual C++ string utility going on here.
std::string string_replace(const std::string &haystack, const std::string &needle, const std::string &replacement);
std::vector<std::string> split_string(const std::string &str, const std::string &delimiter, size_t max_split = SIZE_MAX);
bool has_prefix(const std::string &str, const std::string &prefix);
std::string strtolower(const std::string &str);
std::string strtoupper(const std::string &str);
std::string strtrim(const std::string &str);

// The Windows implementation of these is in sysmain.cpp
// The macOS implementation is in sysMacCompat.cpp
extern std::string Base64Encode(void const *buffer, size_t bufferLength);
extern bool Base64Decode(const std::string &input, void *outputBuffer, size_t *ioBufferLength);
