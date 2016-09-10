#pragma once

#define REG_KEY_MAIN	L"Software\\" SYS_REGISTRY_NAME L"\\Config"
#define REG_MAIN_LOC  	L"MAIN_LOC"
#define REG_MAIN_SIZE  	L"MAIN_SIZE"

extern BOOL sysregGetHKeyText(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, std::wstring &keytext);
extern BOOL sysregGetHKeyDword(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, DWORD *keyword);
extern BOOL sysregSetHKeyText(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, const WCHAR *keytext);
extern BOOL sysregSetHKeyDword(HKEY HKey, const WCHAR *keyloc, const WCHAR *keyname, DWORD keydword);
