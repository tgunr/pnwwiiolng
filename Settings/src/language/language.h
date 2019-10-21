#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

enum
{
    ENGLISH,
	ITALIAN,
	FRENCH,
    GERMAN,
    SPANISH,
	CHINESE,
	RUSSIAN,
	MAX_LANGUAGES
};

extern const char* langNames[MAX_LANGUAGES];

bool langInit(int language, const char *path);

const wchar_t *getLoc(int language, const char *key);
const wchar_t *utf8_to_utf16(const unsigned char *val);
const wchar_t *utf8_to_utf16(const char *val);
const unsigned char *utf16_to_utf8(const wchar_t *wval);
wchar_t *str_to_wstr(wchar_t *wstr, const char *str, int maxlen);


#endif