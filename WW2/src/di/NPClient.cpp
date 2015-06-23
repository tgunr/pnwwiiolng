// *******************************************************************************
// *
// * Module Name:
// *   NPClientWraps.cpp
// *
// * Software Engineer:
// *   Doyle Nickless - GoFlight Inc., for Eye Control Technology.
// *
// * Abstract:
// *   This module implements the wrapper code for interfacing to the NaturalPoint
// *   Game Client API.  Developers of client apps can include this module into
// *   their projects to simplify communication with the NaturalPoint software.
// *
// *   This is necessary since the NPClient DLL is run-time linked rather than
// *   load-time linked, avoiding the need to link a static library into the
// *   client program (only this module is needed, and can be supplied in source
// *   form.)
// *
// * Environment:
// *   User mode
// *
// *******************************************************************************
//

#include "atlstr.h"

#include <string.h>

#include "di/NPClient.h"


/////////////
// Defines ///////////////////////////////////////////////////////////////////////
/////////////
//

/////////////////
// Global Data ///////////////////////////////////////////////////////////////////
/////////////////
//
PF_NP_REGISTERWINDOWHANDLE       gpfNP_RegisterWindowHandle = nullptr;
PF_NP_UNREGISTERWINDOWHANDLE     gpfNP_UnregisterWindowHandle = nullptr;
PF_NP_REGISTERPROGRAMPROFILEID   gpfNP_RegisterProgramProfileID = nullptr;
PF_NP_QUERYVERSION               gpfNP_QueryVersion = nullptr;
PF_NP_REQUESTDATA                gpfNP_RequestData = nullptr;
PF_NP_GETSIGNATURE               gpfNP_GetSignature = nullptr;
PF_NP_GETDATA                    gpfNP_GetData = nullptr;
PF_NP_STARTCURSOR                gpfNP_StartCursor = nullptr;
PF_NP_STOPCURSOR                 gpfNP_StopCursor = nullptr;
PF_NP_STARTDATATRANSMISSION      gpfNP_StartDataTransmission = nullptr;
PF_NP_STOPDATATRANSMISSION       gpfNP_StopDataTransmission = nullptr;

HMODULE ghNPClientDLL = (HMODULE)nullptr;

static CString GetDllLocation();

////////////////////////////////////////////////////
// NaturalPoint Game Client API function wrappers /////////////////////////////
////////////////////////////////////////////////////
//
NPRESULT  NP_RegisterWindowHandle( HWND hWnd  )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_RegisterWindowHandle )
		result = (*gpfNP_RegisterWindowHandle)( hWnd );

	return result;
} // NP_RegisterWindowHandle()


NPRESULT  NP_UnregisterWindowHandle()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_UnregisterWindowHandle )
		result = (*gpfNP_UnregisterWindowHandle)();

	return result;
} // NP_UnregisterWindowHandle()


NPRESULT  NP_RegisterProgramProfileID( unsigned short wPPID )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_RegisterProgramProfileID )
		result = (*gpfNP_RegisterProgramProfileID)( wPPID );

	return result;
} // NP_RegisterProgramProfileID()


NPRESULT  NP_QueryVersion( unsigned short* pwVersion )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_QueryVersion )
		result = (*gpfNP_QueryVersion)( pwVersion );

	return result;
} // NP_QueryVersion()


NPRESULT  NP_RequestData( unsigned short wDataReq )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_RequestData )
		result = (*gpfNP_RequestData)( wDataReq );

	return result;
} // NP_RequestData()

NPRESULT  NP_GetSignature( SIGNATUREDATA* pSignature )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_GetSignature )
		result = (*gpfNP_GetSignature)( pSignature );

	return result;
} // NP_GetSignature()


NPRESULT  NP_GetData( TRACKIRDATA* pTID )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_GetData )
		result = (*gpfNP_GetData)( pTID );

	return result;
} // NP_GetData()


NPRESULT  NP_StartCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_StartCursor )
		result = (*gpfNP_StartCursor)();

	return result;
} // NP_StartCursor()


NPRESULT  NP_StopCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_StopCursor )
		result = (*gpfNP_StopCursor)();

	return result;
} // NP_StopCursor()


NPRESULT  NP_StartDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_StartDataTransmission )
		result = (*gpfNP_StartDataTransmission)();

	return result;
} // NP_StartDataTransmission()


NPRESULT  NP_StopDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( nullptr != gpfNP_StopDataTransmission )
		result = (*gpfNP_StopDataTransmission)();

	return result;
} // NP_StopDataTransmission()


//////////////////////////////////////////////////////////////////////
// NPClientInit() -- Loads the DLL and retrieves pointers to all exports
//
NPRESULT NPClient_Init()
{
	NPRESULT result = NP_OK;

	CString csNPClientDLLFullPath;

	 CString csDLLPath = GetDllLocation();

	if( 0 != csDLLPath.GetLength() )
		csNPClientDLLFullPath = csDLLPath + _T("\\");
	csNPClientDLLFullPath += _T("NPClient.dll");

	ghNPClientDLL = ::LoadLibrary( csNPClientDLLFullPath );
	if( nullptr != ghNPClientDLL )
		{
			// verify the dll signature
			gpfNP_GetSignature             = (PF_NP_GETSIGNATURE)::GetProcAddress( ghNPClientDLL, "NP_GetSignature" );

			SIGNATUREDATA pSignature;
			SIGNATUREDATA verifySignature;
			// init the signatures
			strcpy(verifySignature.DllSignature, "precise head tracking\n put your head into the game\n now go look around\n\n Copyright EyeControl Technologies");
			strcpy(verifySignature.AppSignature, "hardware camera\n software processing data\n track user movement\n\n Copyright EyeControl Technologies");
			// query the dll and compare the results
			NPRESULT vresult = NP_GetSignature( &pSignature );
			if( vresult == NP_OK )
			{
				if ((strcmp(verifySignature.DllSignature,pSignature.DllSignature)==0)
					&& (strcmp(verifySignature.AppSignature,pSignature.AppSignature)==0))
				{
					result = NP_OK;

					// Get addresses of all exported functions
					gpfNP_RegisterWindowHandle     = (PF_NP_REGISTERWINDOWHANDLE)::GetProcAddress( ghNPClientDLL, "NP_RegisterWindowHandle" );
					gpfNP_UnregisterWindowHandle   = (PF_NP_UNREGISTERWINDOWHANDLE)::GetProcAddress( ghNPClientDLL, "NP_UnregisterWindowHandle" );
					gpfNP_RegisterProgramProfileID = (PF_NP_REGISTERPROGRAMPROFILEID)::GetProcAddress( ghNPClientDLL, "NP_RegisterProgramProfileID" );
					gpfNP_QueryVersion             = (PF_NP_QUERYVERSION)::GetProcAddress( ghNPClientDLL, "NP_QueryVersion" );
					gpfNP_RequestData              = (PF_NP_REQUESTDATA)::GetProcAddress( ghNPClientDLL, "NP_RequestData" );
					gpfNP_GetData                  = (PF_NP_GETDATA)::GetProcAddress( ghNPClientDLL, "NP_GetData" );
					gpfNP_StartCursor              = (PF_NP_STARTCURSOR)::GetProcAddress( ghNPClientDLL, "NP_StartCursor" );
					gpfNP_StopCursor               = (PF_NP_STOPCURSOR)::GetProcAddress( ghNPClientDLL, "NP_StopCursor" );
					gpfNP_StartDataTransmission    = (PF_NP_STARTDATATRANSMISSION)::GetProcAddress( ghNPClientDLL, "NP_StartDataTransmission" );
					gpfNP_StopDataTransmission     = (PF_NP_STOPDATATRANSMISSION)::GetProcAddress( ghNPClientDLL, "NP_StopDataTransmission" );
				}
				else
				{
					result = NP_ERR_DLL_NOT_FOUND;
				}
			}
			else
			{
				result = NP_ERR_DLL_NOT_FOUND;
			}
		}
		else
		{
			result = NP_ERR_DLL_NOT_FOUND;
		}
	return result;

} // NPClient_Init()

//////////////////////////////////////////////////////////////////////


static CString GetDllLocation()
{
	//find path to NPClient.dll
	HKEY pKey = nullptr;
	//open the registry key
	if (::RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("Software\\NaturalPoint\\NATURALPOINT\\NPClient Location"),
		0,
		KEY_READ,
		&pKey) != ERROR_SUCCESS)
	{
					//error condition
			return "Error";
	}

	//get the value from the key
	unsigned char* szValue;
	DWORD dwSize;
	//first discover the size of the value
	if (RegQueryValueEx(pKey,
		_T("Path"),
		nullptr,
		nullptr,
		nullptr,
		&dwSize) == ERROR_SUCCESS)
	{
		//allocate memory for the buffer for the value
		szValue = (unsigned char*)malloc(dwSize);
		if (szValue == nullptr)//error
			return "Error";
		//now get the value
        if (RegQueryValueEx(pKey,
			_T("Path"),
			nullptr,
			nullptr,
			szValue,
			&dwSize) == ERROR_SUCCESS)
		{
			//everything worked
			::RegCloseKey(pKey);
			TCHAR* tval = (TCHAR*)szValue;
			CString LValue(tval);
			return LValue;
		}
		else//error
		{
			return "Error";
		}

	}
	::RegCloseKey(pKey);
	return "Error";
}

