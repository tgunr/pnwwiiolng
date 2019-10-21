#pragma once

#include <Windows.h>
#include <tchar.h>

extern const TCHAR* registryInstallValue(const TCHAR *Key);

extern bool registryGetVal(const TCHAR *SubKey, const TCHAR *Key, LPBYTE Value);

extern bool registryGetSubKey(const TCHAR *SubKey, HKEY *Key);

extern bool registryGetKeyVal(HKEY hKey, const TCHAR *key, const TCHAR *valueName, const TCHAR *valueData);
extern bool registrySetKeyVal(HKEY hKey, const TCHAR *key, const TCHAR *valueName, const TCHAR *valueData);
extern bool registryDeleteKey(HKEY hKey, const TCHAR *key, const TCHAR *keyName);
extern bool registryDeleteKeyValue(HKEY hKey, const TCHAR *key, const TCHAR *valueName);

extern bool registryGetKeyData(HKEY hRootKey, const TCHAR *key, const TCHAR *valueName, LPBYTE data, DWORD cbData);
extern bool registrySetKeyData(HKEY hRootKey, const TCHAR *key, DWORD dwType, const TCHAR *valueName, LPBYTE data, DWORD cbData);