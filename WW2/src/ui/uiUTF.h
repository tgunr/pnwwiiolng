#pragma once

extern const wchar_t* uiUTF8toUTF16(const char* text);
extern const wchar_t* uiUTF8toUTF16(const unsigned char* text);
extern const char* uiUTF16toUTF8(const wchar_t* text);

#ifdef _UNICODE
#define UTF8(x) uiUTF16toUTF8(x)
#define UTF16(x) uiUTF8toUTF16(x)
#else
#define UTF8(x) x
#define UTF16(x) x
#endif
