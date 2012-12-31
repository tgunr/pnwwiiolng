#include <windows.h>
#include <stdio.h>
#include "texfont.h"


HINSTANCE   hinst;
HWND        hwnd;
HFONT       font;
FONTLAYOUT  flayout;


LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MakeupFont(void);


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
WNDCLASS  wndclass;
MSG msg;

  hinst = hInstance;

  wndclass.style = CS_HREDRAW|CS_VREDRAW;
  wndclass.lpfnWndProc = WindowProc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = hinst;
  wndclass.hIcon = NULL;
  wndclass.hCursor = NULL;
  wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
  wndclass.lpszMenuName = NULL;
  wndclass.lpszClassName = "Texture Font";

  if (!RegisterClass(&wndclass))
  {
    return(0);
  }

  hwnd = CreateWindowEx(0, wndclass.lpszClassName, "Texture Font", WS_POPUP|WS_VISIBLE,
                        20, 20, TEXTWH, TEXTWH, NULL, NULL, hinst, 0);

  if (hwnd == NULL)
  {
    return(0);
  }

  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return(msg.wParam);
}

 
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
char  c;
int   i, w[NUMCHARS], x, y, aw;
PAINTSTRUCT ps;
HDC   hdc;
HFONT oldfont;
TEXTMETRIC  tm;
FILE  *stream;

  switch (uMsg)
  {
  case WM_CREATE:
    MakeupFont();
    break;
  case WM_CLOSE:
    PostQuitMessage(0);
    break;
  case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);
    oldfont = SelectObject(hdc, font);
    SetTextColor(hdc, RGB(255,255,255));
    SetBkColor(hdc, RGB(0,0,0));
    SetBkMode(hdc, TRANSPARENT);
    SetTextAlign(hdc, TA_BOTTOM|TA_LEFT);

    if (!GetCharWidth(hdc, STARTCHAR, STARTCHAR + NUMCHARS, w))
    {
      memset(w, 0, sizeof(int) * NUMCHARS);
    }

    aw = 0;
    GetTextMetrics(hdc, &tm);
    flayout.Height = tm.tmHeight;
	/*
    if (tm.tmHeight > CELLHEIGHT)
    {
      MessageBox(hwnd, "Font height too large", "TextFont", MB_OK);
    }
	*/
    for (i=0; i < NUMCHARS ; i++)
    {
		if (i < 95)
		{
			c = i + STARTCHAR;
		}
		else
		{
			c = i + STARTCHAR + 64; // This jumps over the low 8-bit ascii garbage.
		}
		x = i % (TEXTWH / CELLWIDTH) * CELLWIDTH;
		y = i / (TEXTWH / CELLWIDTH) * CELLHEIGHT + CELLHEIGHT;
		flayout.CharLayout[i].x = x;
		flayout.CharLayout[i].y = y;
		flayout.CharLayout[i].width = w[i];
		flayout.CharLayout[i].u1 = (float)x / TEXTWH;
		flayout.CharLayout[i].u2 = (float)(x + w[i]) / TEXTWH;
		flayout.CharLayout[i].v1 = (float)(TEXTWH - y) / TEXTWH;
		flayout.CharLayout[i].v2 = (float)(TEXTWH - (y - flayout.Height)) / TEXTWH;
		aw += w[i];
		TextOut(hdc, x, y, &c, 1);
		/*
		if (w[i] > CELLWIDTH)
		{
			MessageBox(hwnd, "Character width too large", "TextFont", MB_OK);
			break;
		}
		*/
    }

    flayout.AvgWidth = aw / NUMCHARS;

    stream = fopen("texfont.dat", "wb+");
    fwrite(&flayout, sizeof(FONTLAYOUT), 1, stream);
    fclose(stream);

    SelectObject(hdc, oldfont);
    EndPaint(hwnd, &ps);
    return(0);
  default:
    break;
  }

  return(DefWindowProc(hwnd, uMsg, wParam, lParam));
}

void MakeupFont(void)
{
  font = CreateFont(24, //int nHeight,    // 11/12, 16, 20, 24
                    0,  //int nWidth,
                    0,  //int nEscapement,
                    0,  //int nOrientation,
                    FW_MEDIUM,  //int fnWeight,
                    FALSE,  //DWORD fdwItalic,
                    FALSE,  //DWORD fdwUnderline,
                    FALSE,  //DWORD fdwStrikeOut,
                    ANSI_CHARSET, //DWORD fdwCharSet,
                    OUT_DEFAULT_PRECIS, //DWORD fdwOutputPrecision,
                    CLIP_DEFAULT_PRECIS,  //DWORD fdwClipPrecision,
                    DEFAULT_QUALITY /*NONANTIALIASED_QUALITY*/,  //DWORD fdwQuality,
                    VARIABLE_PITCH|FF_DONTCARE,  //DWORD fdwPitchAndFamily,
                    "Univers Condensed Bold"); //LPCTSTR lpszFace);

  if (font == NULL)
  {
    OutputDebugString("Could not create the font\n");
  }
}
