#pragma once
// $Id: launchApp.h 263 2009-07-06 22:50:28Z dchristopher@CRS $

#include "auth/authTypes.h"

extern APP_LAUNCH  AppLaunch;
extern char  ftpURL[];
extern const char* userName;

extern LOGICAL launchArenaRequest();

extern SINT32 launchAppVerify(APP_VERIFY *AppVerify);

extern SINT32 launchInstallCallbacks(void);

extern bool launchOpenConnection();

extern bool launchRequestGameList() ;
