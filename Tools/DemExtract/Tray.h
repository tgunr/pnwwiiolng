// tray.h
//

#ifndef tray_HEADER

  #define tray_HEADER


  #define TRAY_CALLBACK   WM_USER + 1

  UINT trayMessage(HINSTANCE hinst, HWND hwnd, UINT message, char *tooltip);
  long trayHandler(WPARAM wParam, LPARAM lParam);

#endif
