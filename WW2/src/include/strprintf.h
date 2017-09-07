#pragma once
// Copyright (C) PlayNet, Inc 2016
// Originating Programmer: Oliver "kfsone" Smith

// snprintf wrapper
template<size_t bufSize, typename... Args>
UINT32 strprintf(char(&buf)[bufSize], const char* fmt, Args&&... args)
{
    return static_cast<UINT32>(snprintf(buf, bufSize, fmt, std::forward<Args>(args)...));
}
template<size_t bufSize> UINT32 strprintf(char(&buf)[bufSize], const char* text) { return strprintf(buf, "%s", text); }

// swprintf wrapper
template<size_t bufSize, typename... Args>
UINT32 wstrprintf(wchar_t(&buf)[bufSize], const wchar_t* fmt, Args&&... args) {
	return static_cast<UINT32>(swprintf(buf, bufSize, fmt, std::forward<Args>(args)...));
}
template<size_t bufSize> UINT32 wstrprintf(wchar_t(&buf)[bufSize], const wchar_t* text) { return wstrprintf(buf, L"%s", text); }


// Cross-platform path builder.
#if platOS != OS_WIN32
	#define PATH_SEPARATOR "/"
	#define PATH_SEPARATOR_WIDE L"/"
#else
	#define PATH_SEPARATOR "\\"
	#define PATH_SEPARATOR_WIDE L"\\"
#endif

inline UINT32 pathprintf(char* buf, size_t bufsize, const char* lhs, const char* rhs)
{ return snprintf(buf, bufsize, "%s" PATH_SEPARATOR "%s", lhs, rhs); }
template<size_t bufSize>
UINT32 pathprintf(char(&buf)[bufSize], const char* lhs, const char* rhs)
{ return strprintf<bufSize>(buf, "%s" PATH_SEPARATOR "%s", lhs, rhs); }

inline UINT32 pathprintf(wchar_t* buf, size_t bufsize, const wchar_t* lhs, const wchar_t* rhs)
{ return swprintf(buf, bufsize, L"%ls" PATH_SEPARATOR_WIDE L"%ls", lhs, rhs); }
template<size_t bufSize>
UINT32 pathprintf(wchar_t(&buf)[bufSize], const wchar_t* lhs, const wchar_t* rhs)
{ return wstrprintf<bufSize>(buf, L"%ls" PATH_SEPARATOR_WIDE L"%ls", lhs, rhs); }