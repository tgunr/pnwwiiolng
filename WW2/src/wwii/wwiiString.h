#pragma once
// Created: Thu May  4 13:32:56 CDT 2006
//
// "strlcpy - consistent, safe, string copy and concatenation"
//  see http://www.courtesan.com/todd/papers/strlcpy.html
//  or use web.archive.org if this page becomes unavailable.

extern size_t strlcpy(char* dst, const char* src, size_t size);
