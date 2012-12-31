// tray.c
//

#define NONLS
#include <windows.h>
#include "dbug.h"
#include "tray.h"
#include "resource.h"

// Statics
static HINSTANCE  myhinst;
static HWND       myhwnd;

// Globals
HICON      hIcon;


UINT trayMessage(HINSTANCE hinst, HWND hwnd, UINT message, char *tooltip)
{
NOTIFYICONDATA tnd;

	memset (&tnd, 0, sizeof (NOTIFYICONDATA));
	tnd.cbSize = sizeof (NOTIFYICONDATA);
	tnd.hWnd = hwnd;
	tnd.uID = (UINT)IDI_TRAYICON;
	switch (message)
	{
	case NIM_ADD:
	{
		myhwnd = hwnd;
		myhinst = hinst;
		if (hIcon == NULL)
		{
			hIcon = LoadImage(hinst, MAKEINTRESOURCE(IDI_TRAYICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
		}

		tnd.uFlags = NIF_ICON | NIF_TIP;
		tnd.uCallbackMessage = (UINT)TRAY_CALLBACK;
		tnd.hIcon = hIcon;
		if (tooltip != NULL)
		{
			tnd.uFlags |= NIF_MESSAGE;
			lstrcpyn(tnd.szTip, tooltip, sizeof(tnd.szTip));
		}

		break;
	}
	case NIM_MODIFY:
	{
		tnd.uFlags = NIF_TIP;
		break;
	}
	case NIM_DELETE:
	{
		if (hIcon != NULL)
		{
			DestroyIcon(hIcon);
			hIcon = NULL;
		}

		tnd.uFlags = 0;
		break;
	}
	}

	return(Shell_NotifyIcon(message, &tnd));
} 


long trayHandler(WPARAM wParam, LPARAM lParam)
{
UINT uID;
UINT uMouseMsg;

	uID = (UINT)wParam;
	if (uID != (UINT)IDI_TRAYICON)
	{
		return(0);
	}

	uMouseMsg = (UINT)lParam;

	switch (uMouseMsg)
	{
	case WM_LBUTTONDBLCLK:
	{
		PostMessage(myhwnd, WM_SIZE, SIZE_RESTORED, 0);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		break;
	}
	case WM_RBUTTONDOWN:
	{
		break;
	}
	}

	return(0);
} 
