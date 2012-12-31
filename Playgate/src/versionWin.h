#pragma once
// $Id: versionWin.h 102 2009-01-09 20:42:45Z osmith@CRS $

extern void   versionWinExec(const char* FileName);
extern void   versionWinExit(void);
extern SINT32 versionWinGetVersion(SINT32 AppId, UINT32 MaxVer, 
                            UINT32 *CurrentVer,
                            char* SavedRegistryKey);
