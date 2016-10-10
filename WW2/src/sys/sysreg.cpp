#include <syscfg.h>
#include <winreg.h>
#include "sys/sysreg.h"
#include "unique_opaque_ptr.hpp"

BOOL sysregGetHKeyDword(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, DWORD *keydword)
{
	unique_HKEY hkey;
	DWORD keylen = sizeof(DWORD);
	
	return RegOpenKeyEx(HKey, keyloc, 0, KEY_READ, &hkey) == ERROR_SUCCESS &&
		   RegQueryValueEx(hkey, keyname, nullptr, nullptr, reinterpret_cast<BYTE *>(&keydword), &keylen) == ERROR_SUCCESS;
}

BOOL sysregGetHKeyText(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, std::wstring &keytext)
{
	unique_HKEY hkey;
	DWORD keylen = 0;

	if (RegOpenKeyEx(HKey, keyloc, 0, KEY_READ, &hkey) != ERROR_SUCCESS ||
		RegQueryValueEx(hkey, keyname, nullptr, nullptr, nullptr, &keylen) != ERROR_SUCCESS)
	{
		return false;
	}
	keytext.resize(keylen + 1, L'\0');
	if (RegQueryValueEx(hkey, keyname, nullptr, nullptr, reinterpret_cast<BYTE *>(&keytext[0]), &keylen) != ERROR_SUCCESS) {
		return false;
	}
	keytext.resize(keylen);
	return true;
}

BOOL sysregSetHKeyDword(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, DWORD keydword)
{
	unique_HKEY hkey;

	return RegCreateKeyEx(HKey, keyloc, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkey, nullptr) == ERROR_SUCCESS &&
		   RegSetValueEx(hkey, keyname, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&keydword), sizeof(keydword)) == ERROR_SUCCESS;
}

BOOL sysregSetHKeyText(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, const WCHAR *keytext)
{
	unique_HKEY hkey;

	return RegCreateKeyEx(HKey, keyloc, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkey, nullptr) == ERROR_SUCCESS &&
		   RegSetValueEx(hkey, keyname, 0, REG_SZ, reinterpret_cast<const BYTE *>(keytext), wcslen(keytext) * sizeof(WCHAR)) == ERROR_SUCCESS;
}
