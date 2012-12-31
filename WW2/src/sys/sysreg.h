#pragma once

#include "tchar.h"

#define REG_KEY_MAIN				_T("Software\\") SYS_REGISTRY_NAME _T("\\Config")
#define REG_MAIN_LOC  				_T("MAIN_LOC")
#define REG_MAIN_SIZE  				_T("MAIN_SIZE")

extern BOOL sysregGetKeyText(const TCHAR* keyloc, const TCHAR* keyname, TCHAR* keytext);
extern BOOL sysregGetKeyDword(const TCHAR* keyloc, const TCHAR* keyname, DWORD* keydword);
extern BOOL sysregSetKeyText(const TCHAR* keyloc, const TCHAR* keyname, const TCHAR* keytext);
extern BOOL sysregSetKeyDword(const TCHAR* keyloc, const TCHAR* keyname, const DWORD* keydword);
extern BOOL sysregGetHKeyDword(HKEY HKey, const TCHAR* keyloc, const TCHAR* keyname, DWORD* keydword);
extern BOOL sysregGetHKeyText(HKEY HKey, const TCHAR* keyloc, const TCHAR* keyname, TCHAR* keytext);

extern const TCHAR* sysregGetInstallValue(const TCHAR* Key);

