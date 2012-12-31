#pragma once


extern UCHAR *getPassWord(void) ;
extern bool logonProcessOK(HWND hWnd) ;
extern void logonReadLoginData() ;
extern SINT32 logonResetLogon(void) ;
extern bool TOSDialog(void) ;
extern BOOL logonFinishDialog(HWND hWnd, int result) ;
extern void logonOnDrawItem(DRAWITEMSTRUCT *) ;
extern bool logonOnInitDialog(HWND hWnd) ;
extern void logonUserSelection(int) ;
extern UINT32 logonGetServerSelection() ;
extern void logonEnableInput(BOOL isEnabled) ;

extern UINT32 g_nextGameListRequest ;

extern bool RegionSelection() ;
