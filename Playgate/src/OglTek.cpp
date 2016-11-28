// OglTek.c

#include "syscfg.h"

#define GLOBALS
#include "vars.h"
#undef GLOBALS

#include "ddp.h"
#include "ogl.h"
#include <Iphlpapi.h>
#include "include/infoMessageTypes.h"

#pragma comment(lib, "opengl32.lib")

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

HINSTANCE	hinst;

static bool
_createWindow(HINSTANCE hInstance)
{
	WNDCLASS	wndClass;
	TCHAR		classname[256];

	WantedWidth = 1;
	WantedHeight = 1;
	WantedBpp = 32;
	WantedWindow = 0;

	hinst = hInstance;

#pragma warning(push)
#pragma warning(disable: 4302)
	memset(&wndClass, 0, sizeof(WNDCLASS));
	wndClass.style          = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	wndClass.lpfnWndProc    = WindowProc;
	wndClass.hInstance      = hInstance;
	wndClass.hIcon          = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_WINLOGO));
	wndClass.hCursor        = LoadCursorA(hInstance, MAKEINTRESOURCEA(IDC_ARROW));
	wndClass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName   = nullptr;
	wsprintf(classname, _T("OpenGLClass_%dx%dx%d"), WantedWidth, WantedHeight, WantedBpp);
	wndClass.lpszClassName  = classname;
#pragma warning(pop)

	if (RegisterClass(&wndClass) == 0)
	{
		Write_Debug("Failed while trying to RegisterClass");
		return(0);
	}

	UINT32 style = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
	myhwnd = CreateWindowEx(0, wndClass.lpszClassName, _T("OglTek"),
							style, 0, 0, WantedWidth, WantedHeight,
							nullptr, nullptr, hinst, nullptr);
	if (!myhwnd)
	{
		Write_Debug("Failed to CreateWindowEx");
		return(0);
	}

	return TRUE;
}

LOGICAL GetOglInfo(HINSTANCE hInstance, PLAY_OGL *info, BOOL Messages)
{
UINT32		len;
SYSTEMTIME	SystemTime;

	len = sizeof(PLAY_COMPUTER_INFO);

	Write_Debug("size = %d",len);

	GetLocalTime(&SystemTime);
	Write_Debug("Date and time: %d/%d/%d %d:%02d:%02d",
				SystemTime.wMonth,
				SystemTime.wDay,
				SystemTime.wYear,
				SystemTime.wHour,
				SystemTime.wMinute,
				SystemTime.wSecond); 
	Write_Debug("");

	if (!_createWindow(hInstance))
		return -1;


	Write_Debug("");

	if (!oglBegin(myhwnd, &info->pixelFormat))
	{
		Write_Debug("Failed to oglBegin");
		return(0);
	}

	if (!oglInit(info))
	{
		Write_Debug("Failed to oglInit");
		return(0);
	}

	oglEnd();

	DestroyWindow( myhwnd );

	return(0);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	return(DefWindowProc(hwnd, umsg, wParam, lParam));
}
