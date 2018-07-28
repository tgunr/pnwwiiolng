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

	bool gotData = (RegQueryValueEx(RegKey, Key, NULL, &Type, Value, &Bytes) == ERROR_SUCCESS);

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
	
	// try to get the value and return it.
	if (registryGetVal(path, Key, (LPBYTE)value))
	{
		return value;
	}
	else
	{
		//  return NULL;  MJH
		return L"";
	}
}

bool
registryGetKeyVal(HKEY hKey, const TCHAR *key, const TCHAR *valueName, const TCHAR *valueData)
{
	HKEY  RegKey;
	LONG lResult = -10;
	bool returnValue = false; 
	DWORD Type = REG_NONE;
	DWORD Bytes = 256 + 1;

	lResult = RegOpenKeyEx(hKey, key, 0, KEY_READ, &RegKey);

	if (lResult == ERROR_SUCCESS)
	{
		//  All Good
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Open Key - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		return false;
	}
	else
	{
		return false;
	}

	lResult = RegQueryValueEx(RegKey, valueName, NULL, &Type, (LPBYTE)valueData, &Bytes);

	if (lResult == ERROR_SUCCESS)
	{
		//  MessageBox(nullptr, L"Query Value - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		returnValue = true;
	}
	else
	{
		returnValue = false;
	}

	RegCloseKey(RegKey);

	return returnValue;
}

bool
registrySetKeyVal(HKEY hKey, const TCHAR *key, const TCHAR *valueName, const TCHAR *valueData)
{
	HKEY  RegKey;
	LONG lResult = -10;
	bool returnValue = false;
	DWORD Type = REG_NONE;
	DWORD Bytes = 256 + 1;

	lResult = RegOpenKeyEx(hKey, key, 0, KEY_WRITE, &RegKey);

	if (lResult == ERROR_SUCCESS)
	{
		//  All Good
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Open Key - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		return false;
	}
	else
	{
		return false;
	}

	lResult = RegSetKeyValue(RegKey, NULL, valueName, REG_SZ, &valueData[0], Bytes);

	if (lResult == ERROR_SUCCESS)
	{
		returnValue = true;
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Set Value - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		returnValue = false;
	}
	else
	{
		returnValue = false;
	}
	
	RegCloseKey(RegKey);

	return returnValue;
}

bool
registryDeleteKey(HKEY hKey, const TCHAR *key, const TCHAR *keyName)
{
	HKEY RegKey;
	bool returnValue = false;
	LONG lResult = -10;

	lResult = RegOpenKeyEx(hKey, key, 0, KEY_ALL_ACCESS, &RegKey);

	if (lResult == ERROR_SUCCESS)
	{
		//  All Good
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Open Key - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		return false;
	}
	else
	{
		return false;
	}

	lResult = -10;
	lResult = RegDeleteKeyEx(RegKey, keyName, KEY_WOW64_32KEY, 0);
	//  lResult = RegDeleteKeyEx(RegKey, keyName, KEY_WOW64_64KEY, 0);  //  64bit regestry.

	if (lResult == ERROR_SUCCESS)
	{
		returnValue = true;
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Delete Key - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		returnValue = false;
	}
	else
	{
		returnValue = false;
	}

	RegCloseKey(RegKey);

	return returnValue;
}

bool
registryDeleteKeyValue(HKEY hKey, const TCHAR *key, const TCHAR *valueName)
{
	HKEY RegKey;
	bool returnValue = false;
	LONG lResult = -10;

	lResult = RegOpenKeyEx(hKey, key, 0, KEY_ALL_ACCESS, &RegKey);

	if (lResult == ERROR_SUCCESS)
	{
		//  All Good
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Open Key - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		return false;
	}
	else
	{
		return false;
	}

	lResult = -10;
	lResult = RegDeleteValue(RegKey, valueName);

	if (lResult == ERROR_SUCCESS)
	{
		returnValue = true;
	}
	else if (lResult == ERROR_ACCESS_DENIED)
	{
		//  MessageBox(nullptr, L"Delete Value - Access Denied.", L"Testing", MB_ICONSTOP | MB_OK);
		returnValue = false;
	}
	else
	{
		returnValue = false;
	}

	RegCloseKey(RegKey);

	return returnValue;
}

bool
registryGetKeyData(HKEY hRootKey, const TCHAR *key, const TCHAR *valueName, LPBYTE data, DWORD cbData)
{
	//  GetKeyData	(
	//				HKEY_LOCAL_MACHINE,
	//				“Software\\Microsoft\\Windows\\CurrentVersion\\Run”,
	//				“ApplicationName”,
	//				storeHere,
	//				strlen(storeHere)
	//	);

	HKEY hKey;

	if (RegOpenKeyEx(hRootKey, key, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return false;

	if (RegQueryValueEx(hKey, valueName, NULL, NULL, data, &cbData) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

bool
registrySetKeyData(HKEY hRootKey, const TCHAR *key, DWORD dwType, const TCHAR *valueName, LPBYTE data, DWORD cbData)
{
	//  SetKeyData	(
	//				HKEY_LOCAL_MACHINE,
	//				“Software\\Microsoft\\Windows\\CurrentVersion\\Run”,
	//				REG_SZ,
	//				“ApplicationName”,
	//				“C:\\ApplicationPath\\ApplicationName.exe”,
	//				strlen(“C:\\ApplicationPath\\ApplicationName.exe”)
	//	);

	HKEY hKey;

	if (RegCreateKey(hRootKey, key, &hKey) != ERROR_SUCCESS)
		return false;

	if (RegSetValueEx(hKey, valueName, 0, dwType, data, cbData) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}