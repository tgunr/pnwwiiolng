#include "stdafx.h"
#include "wchar.h"
//#include "wwii/wwiiUTF.h"

static bool
_tryKey(HKEY hKey, const TCHAR* SubKey, const TCHAR* Key, HKEY& RegKey, LPBYTE Value) 
{
	if (RegOpenKeyEx(hKey, SubKey, 0, KEY_READ, &RegKey) != ERROR_SUCCESS)
		return false ;
	
	DWORD Type = 0;
	DWORD Bytes = 256;

	bool gotData = (RegQueryValueEx(RegKey, Key, NULL, &Type, Value, &Bytes) == ERROR_SUCCESS) ;

	RegCloseKey(RegKey) ;

	return gotData;
}

bool
registryGetVal(const TCHAR *SubKey, const TCHAR *Key, LPBYTE Value)
{
	HKEY  RegKey;

	if ( _tryKey(HKEY_LOCAL_MACHINE, SubKey, Key, RegKey, Value) == true )
		return true ;

	return false ;
}

bool
registryGetSubKey(const TCHAR *SubKey, HKEY *Key)
{
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, Key) == ERROR_SUCCESS)
		return true;

	return false;
}

const TCHAR* registryInstallValue(const TCHAR *Key)
{
    FILE* fp = fopen("manifest", "r");

	int appid = 1;// APPID;

    if(fp)
    {
        TCHAR buff[80];

#ifdef _UNICODE
		fgetws(buff, 79, fp);
#else
        fgets(buff, 79, fp);
#endif

        if(wcsstr(buff, _T(".255")))
        {
			appid = 1;
        }
		
        fclose(fp);
    }

	static TCHAR value[256+1] ;
	value[0] = 0 ;

	TCHAR path[256+1];
	
	// create the path based on the appid
	wsprintf(path, _T("SOFTWARE\\Playnet\\apps\\appid%d\\instance1"), appid);
	
	// try to get the value
	if(registryGetVal(path, Key, (LPBYTE)value))
		return value;
	
	// return it
	return NULL;
}