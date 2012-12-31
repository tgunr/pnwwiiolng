#ifndef LOC_H
#define LOC_H 1

#include "../res/locIDs.h"

//	------------------------------	Public Definitions
// global current language
extern unsigned int language;


enum LANGUAGES
{
    LANGUAGE_ENGLISH,
    LANGUAGE_ITALIAN,
    LANGUAGE_FRENCH,
    LANGUAGE_GERMAN,
    LANGUAGE_SPANISH,
	LANGUAGE_CHINESE,
    NUM_LANGUAGES
};

void LocInit();
const TCHAR* GetLoc(UINT id, ...);

#endif // LOC_H
