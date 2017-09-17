#pragma once

#include <Windows.h>
#include <tchar.h>

extern const TCHAR* registryInstallValue(const TCHAR *Key);

extern bool registryGetVal(const TCHAR *SubKey, const TCHAR *Key, LPBYTE Value);

extern bool registryGetSubKey(const TCHAR *SubKey, HKEY *Key);
