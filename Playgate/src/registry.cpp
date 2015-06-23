#include "stdafx.h"
#include "wwii/wwiiUTF.h"

static bool
_tryKey(HKEY hKey, const char* SubKey, const char* Key, HKEY& RegKey, LPBYTE Value) 
{
	if (RegOpenKeyExA(hKey, SubKey, 0, KEY_READ, &RegKey) != ERROR_SUCCESS)
		return false;
	
	DWORD Type = 0;
	DWORD Bytes = 256;

	bool gotData = (RegQueryValueExA(RegKey, Key, nullptr, &Type, Value, &Bytes) == ERROR_SUCCESS);

	RegCloseKey(RegKey);

	return gotData;
}

bool
registryGetVal(const char *SubKey, const char *Key, LPBYTE Value)
{
	HKEY  RegKey;

	if ( _tryKey(HKEY_LOCAL_MACHINE, SubKey, Key, RegKey, Value) == true )
		return true;

	return false;
}

bool
registryGetSubKey(const char *SubKey, HKEY *Key)
{
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, Key) == ERROR_SUCCESS)
		return true;

	return false;
}

static bool
_readWideKey(const wchar_t* SubKey, const wchar_t* Key, wchar_t* Value) 
{
	HKEY RegKey;

	if ( RegOpenKeyExW(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, &RegKey) != ERROR_SUCCESS )
		return false;
	
	DWORD Type = 0;
	DWORD Bytes = MAX_PATH;

	bool gotData = ( RegQueryValueExW(RegKey, Key, nullptr, &Type, (LPBYTE)Value, &Bytes) == ERROR_SUCCESS );

	RegCloseKey(RegKey);

	return gotData;
}

static inline void _copyPathToString(const char* name, std::wstring& to, wchar_t* from)
{
	to += from;
	if ( to.empty() == true || to.size() == 0 )
		to = L".\\";
	else if ( *to.rbegin() != L'\\' )
		to += L'\\';

	char buffer[MAX_PATH + 3];
	buffer[0] = 0;
	WWII::ConvertChars(to.c_str(), buffer, sizeof(buffer));
	dbugLog(_FLI, "%s = %s", name, buffer);
}

bool
registryGetPaths(std::wstring& exePath, std::wstring& dataPath)
{
	exePath = dataPath = L"";

	wchar_t wBuffer[MAX_PATH+1];
	wBuffer[0] = 0;
	const wchar_t* PATH_KEY = L"Path";
	if ( !_readWideKey(g_registryKeyW, PATH_KEY, wBuffer) || wBuffer[0] == 0 )
	{
		dbugLog(_FLA, "Unable to access %s\\%ls", g_registryKey, PATH_KEY);
		return false;
	}

	_copyPathToString("Path", exePath, wBuffer);

	// Ask Windows where the users documents folder is; oh, and create it if it doesn't exist
	// because we'll be needing it.
	wBuffer[0] = 0;
	if ( SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, wBuffer) != S_OK )
	{
		dbugLog(_FLA, "SHGetFolderPath(CSIDL_PERSONAL) failed");
		return false;
	}

	_copyPathToString("CSIDL_PERSONAL", dataPath, wBuffer);

	// Now query HKLM\Software\Playnet\apps\appidN\instanceN\Data for the name of the game folder.
	wBuffer[0] = 0;
	const wchar_t* DATA_KEY = L"version";
	if ( !_readWideKey(g_registryKeyW, DATA_KEY, wBuffer) || wBuffer[0] == 0 )
	{
		dbugLog(_FLA, "Unable to access %s\\%s", g_registryKey, DATA_KEY);
		return false;
	}

	_copyPathToString("DataPath", dataPath, wBuffer);

	// make sure it exists
	_wmkdir(dataPath.c_str());

	return true;
}

