// OglTek.c

#include "syscfg.h"
#include "ddp.h"
#include "ogl.h"
#include <Iphlpapi.h>

#define GLOBALS
#include "vars.h"
#undef GLOBALS

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

HANDLE	hinst;
char	classname[256];
char	compname[_MAX_PATH];
char	username[_MAX_PATH];
IP_ADAPTER_INFO		AdapterInfo[5], *pAdapter;


int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
UINT32		style, len;
WNDCLASS	wndClass;
SYSTEMTIME	SystemTime;

	dbugOpen("OglTek");

	GetLocalTime(&SystemTime);
	Write_Debug("Date and time: %d/%d/%d %d:%02d:%02d",
				SystemTime.wMonth,
				SystemTime.wDay,
				SystemTime.wYear,
				SystemTime.wHour,
				SystemTime.wMinute,
				SystemTime.wSecond); 
	Write_Debug("");

	WantedWidth = 800;
	WantedHeight = 600;
	WantedBpp = 32;
	WantedWindow = 0;

	hinst = hInstance;

	memset(&wndClass, 0, sizeof(WNDCLASS));
	wndClass.style          = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	wndClass.lpfnWndProc    = WindowProc;
	wndClass.hInstance      = hinst;
	wndClass.hIcon          = LoadIcon(hinst, MAKEINTRESOURCE(IDI_WINLOGO));
	wndClass.hCursor        = LoadCursor(hinst, MAKEINTRESOURCE(IDC_ARROW));
	wndClass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName   = NULL;
	wsprintf(classname, "OpenGLClass_%dx%dx%d", WantedWidth, WantedHeight, WantedBpp);
	wndClass.lpszClassName  = classname;

	if (RegisterClass(&wndClass) == 0)
	{
		Write_Debug("Failed while trying to RegisterClass");
		return(0);
	}

	style |= WS_POPUP;
	style |= WS_VISIBLE;
	style |= WS_CAPTION;
	style |= WS_SYSMENU;
	style |= WS_MINIMIZEBOX|WS_MAXIMIZEBOX;
	style |= WS_THICKFRAME;
	myhwnd = CreateWindowEx(0, wndClass.lpszClassName, "OglTek",
							style, 0, 0, WantedWidth, WantedHeight,
							NULL, NULL, hinst, NULL);
	if (!myhwnd)
	{
		Write_Debug("Failed to CreateWindowEx");
		return(0);
	}

	len = MAX_COMPUTERNAME_LENGTH + 1;
	if (!GetComputerName(compname, &len))
	{
		strcpy(compname, "localhost");
	}

	Write_Debug("Computer: %s", compname);

	len = _MAX_PATH;
	if (!GetUserName(username, &len))
	{
		strcpy(username, "UNKNOWN");
	}

	Write_Debug("User: %s", username);

	len = sizeof(AdapterInfo);
	if (ERROR_SUCCESS == GetAdaptersInfo((PIP_ADAPTER_INFO)&AdapterInfo, &len))
	{
		pAdapter = &AdapterInfo[0];
		do
		{
			Write_Debug("Adapter Name: %s", pAdapter->Description);
			Write_Debug("Adapter Address: %02x-%02x-%02x-%02x-%02x-%02x",
				pAdapter->Address[0],
				pAdapter->Address[1],
				pAdapter->Address[2],
				pAdapter->Address[3],
				pAdapter->Address[4],
				pAdapter->Address[5]);
			pAdapter = pAdapter->Next;
		}
		while (pAdapter != NULL);
	}

	Write_Debug("");

	if (!oglBegin(myhwnd))
	{
		Write_Debug("Failed to oglBegin");
		return(0);
	}

	if (!oglInit())
	{
		Write_Debug("Failed to oglInit");
		return(0);
	}

	oglEnd();

	dbugClose();

	return(0);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	return(DefWindowProc(hwnd, umsg, wParam, lParam));
}
