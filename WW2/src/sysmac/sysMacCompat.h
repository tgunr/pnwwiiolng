#pragma once

#include <sys/types.h>
#include <wchar.h>

#include "include/systypes.h"

using DWORD = UINT32;

#define _isnan std::isnan
#define _finite std::isfinite

// TODO: Why are there so many different names for this?
#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp

// TODO: REVIEW ALL USES OF THESE VERY UNSAFE FUNCTIONS
#define lstrcpy strcpy
#define lstrcat strcat
#define lstrlen strlen
#define lstrcpyn strncpy
#define lstrcmp strcmp
#define wsprintf swprintf

#define _MAX_PATH PATH_MAX
#define MAX_PATH _MAX_PATH

#ifdef __cplusplus
#include <algorithm>
#else
#define max(a,b)  ({ __typeof__((a)) __a = (a); __typeof__((b)) __b = (b); __a > __b ? __a : __b; })
#define min(a,b)  ({ __typeof__((a)) __a = (a); __typeof__((b)) __b = (b); __a < __b ? __a : __b; })
#endif

static inline int wstrncasecmp(const wchar_t* ws1, const wchar_t* ws2, int n) { return wcsncasecmp(ws1, ws2, n); }
static inline int wstrcasecmp(const wchar_t* ws1, const wchar_t* ws2) { return wcscasecmp(ws1, ws2); }

#ifdef __cplusplus
extern "C"
{
#endif

extern void MacVideoInitializationFailed(const char* problem);
extern void PostModalErrorDialog(const char *title, const char *message);

extern void chAppDir();
extern const char *macPreferencesPath();
extern const char *macDocumentsPath();

extern const char *GetCommandLineA();

extern UINT64 GetMicroSeconds();
extern UINT64 GetMillisecondTick();
extern UINT32 timeGetTime();

extern char GetOSMajorVersion();
extern char GetOSMinorVersion();
extern char GetOSRevision();
extern char GetOSBuild();

#ifdef __cplusplus
}
#endif
