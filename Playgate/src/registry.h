#pragma once

extern bool registryGetVal(const char *SubKey, const char *Key, LPBYTE Value);

extern bool registryGetSubKey(const char *SubKey, HKEY *Key);

//  extern bool registryGetPaths(std::wstring& exePath, std::wstring& dataPath);  //  MJH - Feburary 27, 2018.
extern bool registryGetPaths(std::wstring& exePath, std::wstring& dataPath, std::wstring& configPath);
