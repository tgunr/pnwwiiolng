#include "stdafx.h"

#include <direct.h>
#include <string.h>
#include "pnRegion.h"
#ifdef ANTIHACK
#include "Antihack.h"
#endif

#define PN_MAX_STRING 500

struct REG_ENTRY
{
	const TCHAR* name;
	TCHAR* value;
};

static SINT32
_FillInParamValue(const APP_LAUNCH *AppLaunch, const TCHAR* path, const TCHAR* Param, TCHAR* Destination, size_t DestSize)
{
	const TCHAR* p = Param ;
	SINT32 Length ;

	if ( *p != '$' )
		return 0 ;

	for ( p = Param, Length = 0 ; *p != 0 ; p++, Length++ )
	{
		if ( *p == ' ' )
			break;
	}

	// Skip the $ for the comparisons
	p = Param + 1 ;

	if      (!_tcsnccmp(p, _T("ARENAID"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%u"), AppLaunch->arenaID);
	else if (!_tcsnccmp(p, _T("SESSIONID"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%u"), g_sessionID);
	else if (!_tcsnccmp(p, _T("APPID"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%d"), AppLaunch->appID);
	else if (!_tcsnccmp(p, _T("APPVER"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%d"), AppLaunch->appVersion);
	else if (!_tcsnccmp(p, _T("OTP"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%u"), AppLaunch->otp);
	else if (!_tcsnccmp(p, _T("SERVERIP"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%d"), AppLaunch->serverIP);
	else if (!_tcsnccmp(p, _T("PORT"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%u"), AppLaunch->serverPort);
	else if (!_tcsnccmp(p, _T("STAT"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%d"), AppLaunch->status);
	else if (!_tcsnccmp(p, _T("DATA"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%hs"), AppLaunch->data);
	else if (!_tcsnccmp(p, _T("QUAD"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%hs"), teulGetAddress(AppLaunch->serverIP));
	else if (!_tcsnccmp(p, _T("AUTH"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("auth%hs"), g_region.authSuffix().c_str()) ;
	else if (!_tcsnccmp(p, _T("REGION"), Length-1))
		_stprintf_s(Destination, DestSize - 1, _T("%hs"), g_region.authSuffix().c_str()) ;

	return Length;
}

static SINT32
_FillInParamValues(const APP_LAUNCH *AppLaunch, const TCHAR* path, const TCHAR* Params, TCHAR* FilledInParams, size_t FIPSize)
{
	UINT32 otp = AppLaunch->otp ;
	const TCHAR* ParamPtr = Params ;
	TCHAR* FilledPtr = FilledInParams ;
	TCHAR Value[PN_MAX_STRING+1];
	SINT32 ParamFormatLen;

	while (ParamPtr && *ParamPtr)
	{
		if (*ParamPtr == '$')
		{
			memset(Value, 0, sizeof(Value));

			ParamFormatLen = _FillInParamValue(AppLaunch, path, ParamPtr, Value, sizeof(Value) / sizeof(TCHAR));

			_tcscat_s(FilledInParams, FIPSize, Value);

			ParamPtr += ParamFormatLen;
			FilledPtr = FilledInParams + _tcslen(FilledInParams) ;
		}
		else
		{
			*(FilledPtr++) = *ParamPtr;
			ParamPtr++;
		}
		*FilledPtr = 0 ;
	}

	return 1;
}

static SINT32 _GetRegistryValues(SINT32 ArenaID, const char* RegistryKey,
								 char *Location, TCHAR* Params, TCHAR* Exec, TCHAR* path)
{
HKEY   RegKey;
DWORD  Bytes = PN_MAX_STRING;
DWORD  Type;
LOGICAL retval;
REG_ENTRY entries[] = {{_T("Params"),Params},{_T("Path"),path},{_T("Exec"),Exec},{NULL, NULL}};
REG_ENTRY entry;
int i;

	retval = FALSE;

	if ( registryGetSubKey(RegistryKey,&RegKey))
	{
		retval = TRUE;

		for (i = 0, entry = entries[i] ; entry.name != NULL; ++i, entry = entries[i])
		{
			if (RegQueryValueEx(RegKey, entry.name, NULL, &Type, (LPBYTE)entry.value, &Bytes) != ERROR_SUCCESS)
			{
				retval = FALSE;
				break;
			}
		}

		RegCloseKey(RegKey);
	}

	if (! retval)
	{
		playgate.SetStatus(GetLoc(REGISTRY_FAILED), AppError);
		playgate.PostCommand(webID, (UINT32)ERR_GAME_REINSTALL);
	}

	return retval;
}
 
bool
launchApplication(const TCHAR* exeFile, const TCHAR* parameters, std::wstring path)
{
	SHELLEXECUTEINFO sei;
	static TCHAR message[PN_MAX_STRING+1];

	while ( path.size() > 1 && path[path.size() - 1] == '\\' )
	{
		path.erase(path.size() - 1) ;
	}

	if ( _tchdir(path.c_str()) != 0 )
	{
		_stprintf_s(message, _T("\"%s\""), path.c_str()) ;
		_tchdir(message) ;
	}

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.hwnd = NULL;
	sei.lpFile = exeFile;		// binary .exe we are launching
	sei.lpParameters = parameters;	// command line arguments
	sei.lpDirectory = path.c_str();		// initial working directory
	sei.nShow = 1;
	sei.lpVerb = _T("open") ;

	if (!ShellExecuteEx(&sei))
	{
		playgate.SetStatus(GetLoc(EXECUTE_FAILED, exeFile), AppError);
		return false ;
	}

	return true ;
}

#ifdef ANTIHACK
	void detectDCHack( TCHAR* FilledInParams, UINT32 seed )
	{
		Antihack a;

		std::wstring s = a.hide(a.check(),seed);
		_tcscat( FilledInParams, L" -o " );
		_tcscat( FilledInParams, s.c_str() );
	}
#endif

	bool launchAppWin(const APP_LAUNCH *AppLaunch, const char* RegistryKey)
{
	char Location[PN_MAX_STRING+1];
	TCHAR Params[PN_MAX_STRING+1];
	TCHAR Exec[PN_MAX_STRING+1];
	TCHAR FilledInParams[PN_MAX_STRING+1];
	TCHAR path[PN_MAX_STRING+1];
	TCHAR CommandLine[PN_MAX_STRING+1];

	if (!_GetRegistryValues(AppLaunch->appID, RegistryKey, Location, Params, Exec, path))
	{
		playgate.SetStatus(GetLoc(REGISTRY_BAD), AppError);
		return false ;
	}

	memset(FilledInParams, 0, sizeof(FilledInParams));

	if (Params[0])
		_FillInParamValues(AppLaunch, path, Params, FilledInParams, sizeof(FilledInParams) / sizeof(TCHAR));

#ifdef ANTIHACK
	detectDCHack( FilledInParams, AppLaunch->otp );
#endif
	_stprintf_s(CommandLine, _T("\"%s\\%s\""), path, Exec);

	return launchApplication(CommandLine, FilledInParams, path);
}

bool launchAppWinGetHost(char* HostIP, char *HostPort)
{
	static const char* key = "Software\\Playnet\\apps";
	HKEY	RegKey;
	if (! registryGetSubKey(key, &RegKey))
	{
		return false ;
	}

	DWORD  Bytes = 64;
	DWORD  Type;
	if (RegQueryValueExA(RegKey, "hostIP", NULL, &Type, (LPBYTE)HostIP, &Bytes) !=
		ERROR_SUCCESS)
	{
		RegCloseKey(RegKey);
		return false ;
	}

	Bytes = 64;

	if (RegQueryValueExA(RegKey, "hostPort", NULL, &Type, (UCHAR *)HostPort, 
						&Bytes) != ERROR_SUCCESS)
	{
		RegCloseKey(RegKey);
		return false ;
	}

	RegCloseKey(RegKey);

	return true ;
}
