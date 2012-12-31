// barewnd.c
//

#define NONLS
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <winnt.h>
#include <winuser.h>
#include <commctrl.h>
#include "resource.h"
#include "barewnd.h"
#include "dbug.h"
#include "tray.h"
#include "extract.h"

// Defines

// Variables
HINSTANCE		hinst;
HWND			myhwnd, pbhwnd;
BOOL			okdisabled;
BOOL			threadabort;
int				threadsactive;
HANDLE			threadhandle;
DWORD			threadid;

// External globals
extern HICON	hIcon;

// Typedefs

// Static functions
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
int			vscroll;
MSG         msg;
RECT		rect;

	hinst = hInstance;

	// No duplicate instances; set the other instace to active status and exit
	{
	HWND	hother;

		hother = FindWindow("#32770", "DEM Theater Tool...");
		if (hother != NULL)
		{
			PostMessage(hother, WM_SIZE, SIZE_RESTORED, 0);
			return(0);
		}
	}

	threadsactive = 0;
	threadhandle = NULL;
	threadabort = FALSE;

	Open_Debug("DEM Theater Tool");

	myhwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_EXTRACT_PROPERTIES), NULL, WindowProc);
	if (!myhwnd)
	{
		return(FALSE);
	}

	SetClassLong(myhwnd, GCL_HICON, (long)hIcon);

	vscroll = GetSystemMetrics(SM_CYVSCROLL);
	InitCommonControls();
	GetClientRect(myhwnd, &rect);
	pbhwnd = CreateWindowEx(0, PROGRESS_CLASS, "Progress",
							WS_CHILD|WS_VISIBLE|PBS_SMOOTH,
							rect.left, rect.bottom - vscroll,
							rect.right, vscroll,
							myhwnd, NULL, hInstance, NULL);

	if (!pbhwnd)
	{
		return(0);
	}

	SendMessage(pbhwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(pbhwnd, PBM_SETSTEP, 1, 0);

	ShowWindow(myhwnd, nCmdShow);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	threadabort = TRUE;
	while (threadsactive)
	{
		Sleep(10);
	}

	if (threadhandle != NULL)
	{
		CloseHandle(threadhandle);
	}

	Close_Debug();

	return(msg.wParam);
}

// WindowProc
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
int		x, y;
RECT	wrect, srect;

	switch (uMsg)
	{
	case TRAY_CALLBACK:
	{
		return(trayHandler(wParam, lParam));
	}
	case WM_CREATE:
	case WM_INITDIALOG:
	{
		trayMessage(hinst, hwnd, NIM_ADD, "DEM Theater Tool");

		GetWindowRect(hwnd, &wrect);

		SystemParametersInfo(SPI_GETWORKAREA, 0, &srect, 0);
		x = ((srect.right - srect.left + 1) - (wrect.right - wrect.left + 1)) / 2;
		y = ((srect.bottom - srect.top + 1) - (wrect.bottom - wrect.top + 1)) / 2;
		MoveWindow(hwnd, x, y, wrect.right - wrect.left, wrect.bottom - wrect.top, FALSE);

		ListBox_AddString(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS), "N");
		ListBox_AddString(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS), "S");
		ListBox_AddString(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW), "E");
		ListBox_AddString(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW), "W");
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_DEGREES_SPIN), UDM_SETRANGE, 0, MAKELONG(90, 0));
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_DEGREES_SPIN), UDM_SETRANGE, 0, MAKELONG(180, 0));
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_MINUTES_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_MINUTES_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_SECONDS_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_SECONDS_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));

		ListBox_AddString(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS), "N");
		ListBox_AddString(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS), "S");
		ListBox_AddString(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW), "E");
		ListBox_AddString(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW), "W");
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_DEGREES_SPIN), UDM_SETRANGE, 0, MAKELONG(90, 0));
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_DEGREES_SPIN), UDM_SETRANGE, 0, MAKELONG(180, 0));
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_MINUTES_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_MINUTES_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_SECONDS_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));
		SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_SECONDS_SPIN), UDM_SETRANGE, 0, MAKELONG(59, 0));

		okdisabled = FALSE;
		SetTimer(hwnd, 627, 500, NULL);
		break;
	}
	case WM_DESTROY:
	{
		trayMessage(hinst, hwnd, NIM_DELETE, NULL);
		break;
	}
	case WM_SIZE:
	{
		switch (wParam)
		{
		case SIZE_MINIMIZED:
		{
			ShowWindow(hwnd, SW_HIDE);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_VISIBLE);
			break;
		}
		case SIZE_RESTORED:
		{
			ShowWindow(hwnd, SW_RESTORE);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_VISIBLE);
			SetForegroundWindow(hwnd);
			break;
		}
		}
		break;
	}
	case WM_PAINT:
	{
	HDC           hdc;
	PAINTSTRUCT   ps;
	RECT          rect;

		GetClientRect(hwnd, &rect);
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_CLOSE:
	{
		DestroyWindow(pbhwnd);
		KillTimer(hwnd, 627);
		DestroyWindow(myhwnd);
		myhwnd = NULL;
		PostQuitMessage(0);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
		LPDEMDETAIL		detail;

			detail = malloc(sizeof(DEMDETAIL));
			ListBox_GetText(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS), ListBox_GetTopIndex(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS)), &detail->ulns);
			ListBox_GetText(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW), ListBox_GetTopIndex(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW)), &detail->ulew);
			detail->ulnsdeg = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_DEGREES_SPIN), UDM_GETPOS, 0, 0);
			detail->ulnsmin = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_MINUTES_SPIN), UDM_GETPOS, 0, 0);
			detail->ulnssec = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_NS_SECONDS_SPIN), UDM_GETPOS, 0, 0);
			detail->ulewdeg = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_DEGREES_SPIN), UDM_GETPOS, 0, 0);
			detail->ulewmin = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_MINUTES_SPIN), UDM_GETPOS, 0, 0);
			detail->ulewsec = SendMessage(GetDlgItem(hwnd, IDC_UPPER_LEFT_EW_SECONDS_SPIN), UDM_GETPOS, 0, 0);

			ListBox_GetText(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS), ListBox_GetTopIndex(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS)), &detail->lrns);
			ListBox_GetText(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW), ListBox_GetTopIndex(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW)), &detail->lrew);
			detail->lrnsdeg = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_DEGREES_SPIN), UDM_GETPOS, 0, 0);
			detail->lrnsmin = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_MINUTES_SPIN), UDM_GETPOS, 0, 0);
			detail->lrnssec = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_NS_SECONDS_SPIN), UDM_GETPOS, 0, 0);
			detail->lrewdeg = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_DEGREES_SPIN), UDM_GETPOS, 0, 0);
			detail->lrewmin = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_MINUTES_SPIN), UDM_GETPOS, 0, 0);
			detail->lrewsec = SendMessage(GetDlgItem(hwnd, IDC_LOWER_RIGHT_EW_SECONDS_SPIN), UDM_GETPOS, 0, 0);

			threadhandle = CreateThread(NULL, 0, extractMap, detail, 0, &threadid);

			break;
		}
		case IDCANCEL:
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		}
		break;
	}
	case WM_TIMER:
	{
		if (threadsactive != 0)
		{
			if (!okdisabled)
			{
				Button_Enable(GetDlgItem(hwnd, IDOK), okdisabled);
				InvalidateRect(hwnd, NULL, FALSE);
				okdisabled = TRUE;
			}
		}
		else
		{
			if (threadhandle != NULL)
			{
				CloseHandle(threadhandle);
				threadhandle = NULL;
			}

			if (okdisabled)
			{
				Button_Enable(GetDlgItem(hwnd, IDOK), okdisabled);
				InvalidateRect(hwnd, NULL, FALSE);
				okdisabled = FALSE;
			}
		}
		break;
	}
	}

	return(0);//DefWindowProc(hwnd, uMsg, wParam, lParam));
}
