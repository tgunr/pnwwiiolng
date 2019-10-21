#pragma once

#include "pi/pi.h"
#include "include/vars.h"

#include "fio/fio.h"

enum ENUM_CATEGORIES
{
    IDE_LANG_UI,
	IDE_LANG_IAC,
	IDE_LANG_FUNCTION,
	IDE_LANG_HOST,
    IDE_LANG_MOBILE,
	IDE_LANG_CLIENT, // old loclMain crap
    NUMBER_OF_CATEGORIES
};

enum ENUM_LANGUAGES
{
    IDE_LANGUAGE_ENGLISH,
	IDE_LANGUAGE_ITALIAN,
    IDE_LANGUAGE_FRENCH,
    IDE_LANGUAGE_GERMAN,
    IDE_LANGUAGE_SPANISH,
	IDE_LANGUAGE_CHINESE,
	IDE_LANGUAGE_RUSSIAN,
	NUMBER_OF_LANGUAGES
};

struct LANGTABLE
{
    LANGTABLE* next;
    LANGTABLE* prev;

    UINT32 uID;

    UCHAR szStr[256];

};


extern LANGTABLE g_LangTable[NUMBER_OF_CATEGORIES];

// Any separate localization files.
extern SCHAR g_szFiles[NUMBER_OF_CATEGORIES][64];

// Language paths.
extern SCHAR g_szPaths[NUMBER_OF_LANGUAGES][64];

extern const SCHAR* g_szLanguages[NUMBER_OF_LANGUAGES];

