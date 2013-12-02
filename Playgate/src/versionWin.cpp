#include "stdafx.h"
#include "main.h"
#include "wwii/wwiiUTF.h"
#include "versionWin.h"

#ifdef _UNICODE
#define ustrcpy lstrcpyW
#define ustrcat lstrcatW
#define ustrcmp lstrcmpW
#define ustrlen lstrlenW
#define usprintf swprintf
#define _tcslen	wcslen
#else
#define ustrcpy lstrcpyA
#define ustrcat lstrcatA
#define ustrcmp lstrcmpA
#define ustrlen lstrlenA
#define usprintf sprintf
#define _tcslen	strlen
#endif

static bool GetFileVersion(TCHAR* const buffer, const size_t bufferSize, TCHAR* const filename);
static void chopOffRevision(TCHAR* fileversion);

void versionWinExec(const char* FileName)
{
	WinExec(FileName, SW_SHOW);
}

static UINT32 _GetInstanceVersion(const char* VersionKey, const char* SubKey, char* VersionStr)
{
	HKEY	RegKey;
	TCHAR	Location[300];
	char	NewKey[130];
	DWORD	Type;
	UINT32	Version = 0;

	sprintf_s(NewKey, 128, "%s\\%s", VersionKey, SubKey);

	if (! registryGetSubKey( NewKey, &RegKey) )
		return 0;

	DWORD Bytes = sizeof(Location) - 1;
	if (RegQueryValueExW(RegKey, L"Path", NULL, &Type, (LPBYTE)Location, &Bytes) !=
		ERROR_SUCCESS)
	{
		RegCloseKey(RegKey);
		return 0;
	}
	
	RegCloseKey(RegKey);

	wcscat(Location, L"\\WW2.exe");

	// Access Window's version information for the executable.
	TCHAR buffer[256+1];
	if (!GetFileVersion(buffer, sizeof(buffer), Location))
		return 0;

	char verStr[25 + 1];
	WWII::ConvertChars(buffer, verStr, sizeof(verStr));
	teulGetHost(verStr, &Version);

	return Version;
}

SINT32 versionWinGetVersion(SINT32 AppId, UINT32 MaxVer, UINT32 *CurrentVer, char* SavedRegistryKey)
{
	char    VersionKey[129];
	char    SubKey[128];
	char    SavedKey[128];
	char   *TmpPtr;
	HKEY     RegKey;
	DWORD    Bytes = 64;
	DWORD    Index = 0;
	FILETIME KeyTime;
	char    VersionStr[48];
	SINT32   NumberOfMatches = 0;

	sprintf_s(VersionKey, 128, "Software\\Playnet\\apps\\appid%u", AppId);

	*CurrentVer = 0;

	if (! registryGetSubKey(VersionKey,&RegKey))
		return 0;

	SavedKey[0] = '\0';

	//  Enumertate all the installed versions . . . .
	while (RegEnumKeyExA(RegKey, Index, SubKey, &Bytes, NULL, NULL, NULL,
						&KeyTime) == ERROR_SUCCESS)
	{
		//  make sure the key is of the type "instance#"
		if (!strncmp(SubKey, "instance", strlen("instance")))
		{
			UINT32 InstanceVersion = _GetInstanceVersion(VersionKey, SubKey, VersionStr);
			if (InstanceVersion == 0)
				continue;

			if (InstanceVersion <= MaxVer)
			{
				NumberOfMatches++;
				strcpy_s(SavedKey, VersionKey);
				*CurrentVer = InstanceVersion;
				if ( SavedRegistryKey != NULL )
					sprintf_s(SavedRegistryKey, 255, "%s\\%s", VersionKey, SubKey);
			}
		}

		Bytes = 64;
		Index++;
	}

	if (SavedKey[0] == '\0')
		return 0;

	TmpPtr = strchr(SavedKey, 'n');

	if (TmpPtr == NULL)
		return 0;

	TmpPtr++;

	if (TmpPtr == NULL || *TmpPtr == '\0')
		return 0;

//Z	*CurrentVer = atoi(TmpPtr);

	return NumberOfMatches;
}

bool GetFileVersion(TCHAR* const dest, const size_t destSize, TCHAR* const filename)
{
	// Initialize dest as empty string.
	dest[0] = 0;

	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;

	} *ptranslate;

	const DWORD VSize = GetFileVersionInfoSize(filename, NULL);
	if (VSize == 0)
	{
		DWORD n = GetLastError();
		// file not found
		//Write_Debug("No file version info available for %s", filename);
		return false;
	}

	void* pVersion = alloca(VSize);
	if (pVersion == NULL)
	{
		// Severe error
		return false;
	}

	if (!GetFileVersionInfo(filename, 0, VSize, pVersion))
	{
		//Write_Debug("Problem with GetFileVersionInfo");
		return false;
	}

	DWORD Len = 0;
	if (!VerQueryValue(pVersion, _T("\\VarFileInfo\\Translation"), (LPVOID*)&ptranslate, (PUINT)&Len))
	{
		//Write_Debug("Problem with VerQueryValue");
		return false;
	}

	TCHAR* ptrFVI;		// FileVersion info
	TCHAR keyName[128];
	usprintf(keyName, sizeof(keyName) - 1, _T("\\StringFileInfo\\%04x%04x\\FileVersion"), (LPVOID*)ptranslate->wLanguage, (PUINT)ptranslate->wCodePage);
	if (!VerQueryValue(pVersion, keyName, (LPVOID*)&ptrFVI, (PUINT)&Len))
	{
		//Write_Debug("Problem with VerQueryValue");
		return false;
	}

	// Copy everything but the revision number.
	static const TCHAR chColon = _T(':');
	static const TCHAR chNull = _T('\0');

	TCHAR* src = ptrFVI;
	TCHAR* dst = dest;
	while (*src && *src != chColon)
		*(dst++) = *(src++);
	*dst = 0;
		
	return true;
}

