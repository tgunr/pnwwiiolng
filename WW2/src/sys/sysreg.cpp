#include <syscfg.h> // Path is project specific.
//

#include <winreg.h>
#include "sys/sysreg.h"
#include "VersionNo.h"

#include "auth/authAppIDs.h"

BOOL sysregGetKeyText(const TCHAR* keyloc, const TCHAR* keyname, TCHAR* keytext)
{
TCHAR	keyvalue[256];
DWORD	keysize=256;
DWORD	keytype;
HKEY	hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, (const TCHAR*)keyloc, 0, KEY_READ, &hkey))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hkey, keyname, nullptr, &keytype, (BYTE*)keyvalue, &keysize))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	ustrcpy(keytext, keyvalue);

	return(TRUE);
}


BOOL sysregGetKeyDword(const TCHAR* keyloc, const TCHAR* keyname, DWORD* keydword)
{
DWORD	keyvalue;
DWORD	keysize=4;
DWORD	keytype;
HKEY	hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, (const TCHAR*)keyloc, 0, KEY_READ, &hkey))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hkey, keyname, nullptr, &keytype, (BYTE*)&keyvalue, &keysize))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	*keydword = keyvalue;

	return(TRUE);
}


BOOL sysregSetKeyText(const TCHAR* keyloc, const TCHAR* keyname, const TCHAR* keytext)
{
DWORD	disp;
HKEY	hkey;

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, (const TCHAR*)keyloc, 0, _T(""),
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hkey, &disp))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegSetValueEx(hkey, keyname, 0, REG_SZ, (const BYTE*)keytext, ustrlen(keytext)))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	return(TRUE);
}


BOOL sysregSetKeyDword(const TCHAR* keyloc, const TCHAR* keyname, const DWORD* keydword)
{
DWORD	disp;
HKEY	hkey;

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, (const TCHAR*)keyloc, 0, _T(""),
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hkey, &disp))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegSetValueEx(hkey, keyname, 0, REG_DWORD, (const BYTE*)keydword, 4))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	return(TRUE);
}


BOOL sysregGetHKeyDword(HKEY HKey, const TCHAR* keyloc, const TCHAR* keyname, DWORD* keydword)
{
DWORD	keyvalue;
DWORD	keysize=4;
DWORD	keytype;
HKEY	hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKey, (const TCHAR*)keyloc, 0, KEY_READ, &hkey))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hkey, keyname, nullptr, &keytype, (BYTE*)&keyvalue, &keysize))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	*keydword = keyvalue;

	return(TRUE);
}


BOOL sysregGetHKeyText(HKEY HKey, const TCHAR* keyloc, const TCHAR* keyname, TCHAR* keytext)
{
TCHAR	keyvalue[256];
DWORD	keysize=256;
DWORD	keytype;
HKEY	hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKey, (const TCHAR*)keyloc, 0, KEY_READ, &hkey))
	{
		return(FALSE);
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hkey, keyname, nullptr, &keytype, (BYTE*)keyvalue, &keysize))
	{
		RegCloseKey(hkey);

		return(FALSE);
	}

	RegCloseKey(hkey);

	ustrcpy(keytext, keyvalue);

	return(TRUE);
}

#include "versionNo.h"

const TCHAR* sysregGetInstallValue(const TCHAR* Key)
{
	int appid = PN_WW2_INSTALL_ID;

	static TCHAR value[256+1];
	value[0] = 0;

	TCHAR path[256+1];

	// create the path based on the appid
	wsprintf(path, _T("SOFTWARE\\Playnet\\apps\\appid%d\\instance1"), appid);

	if(sysregGetHKeyText(HKEY_LOCAL_MACHINE, path, Key, (TCHAR*)value) )
		return value;

	// return it
	return nullptr;
}
