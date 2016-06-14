#include "loc.h"
#include "registry.h"

#define LOC(x,y) (LOC_BASE + (x * LOC_RANGE) + (y))
#define MAX_LOC_VALUE 4095

unsigned int language;

const TCHAR* GetLoc(UINT id, ...)
{
	const int instances = 8;
	static int instance = 0;

	static TCHAR lookup[instances][MAX_LOC_VALUE+1];
	static TCHAR format[MAX_LOC_VALUE+1];
	
	instance = instance & (instances - 1);

	HMODULE hm = GetModuleHandle(nullptr);

	// lookup the format string
	int read = LoadString(hm, LOC(language,id), format, MAX_LOC_VALUE);

	// fallback to english if none was found
	if(!read && language != LANGUAGE_ENGLISH)
		read = LoadString(hm, LOC(LANGUAGE_ENGLISH,id), format, MAX_LOC_VALUE);
	
	if(read)
	{
		va_list marker;
		va_start(marker, id);

#ifdef UNICODE
		vswprintf(lookup[instance], MAX_LOC_VALUE, format, marker); 
#else
		vsprintf(lookup[instance], format, marker); 
#endif

		va_end(marker);	
	}
	else
	{
		lstrcpy(lookup[instance], L"?");
	}
	
	return lookup[instance++];
}

void LocInit()
{
	language = LANGUAGE_ENGLISH;

	TCHAR name[LOCALE_NAME_MAX_LENGTH+1];

	if(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, name, LOCALE_NAME_MAX_LENGTH) > 0)
	{
		if(lstrcmp(name, _T("Italian")) == 0)
			language = LANGUAGE_ITALIAN;
		else if(lstrcmp(name, _T("French")) == 0)
			language = LANGUAGE_FRENCH;
		else if(lstrcmp(name, _T("German")) == 0)
			language = LANGUAGE_GERMAN;
		else if(lstrcmp(name, _T("Spanish")) == 0)
			language = LANGUAGE_SPANISH;
		else if(lstrcmp(name, _T("Chinese")) == 0)
			language = LANGUAGE_CHINESE;
	}
	
#ifndef IGNORE_REGISTRY
	char buffer[MAX_PARAM+1];
	buffer[0] = 0;
	if(!registryGetVal(g_registryKey, "Region", (LPBYTE)buffer) || buffer[0] == 0)
		return;

	if ( _stricmp(buffer, "china") == 0 || _stricmp(buffer, "cn") == 0 || _stricmp(buffer, "zh") == 0 )
		language = LANGUAGE_CHINESE;
#endif
}
