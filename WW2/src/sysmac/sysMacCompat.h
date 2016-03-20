#ifndef PN_SYSMAC_SYSMACCOMPAT_H
#define PN_SYSMAC_SYSMACCOMPAT_H 1

#include <sys/types.h>
#include <wchar.h>

#include "include/sysTypes.h"

typedef UINT32 DWORD;

#define _isnan std::isnan
#define _finite std::isfinite
#define stricmp strcmpi
#define _stricmp strcmpi
#define timeGetTime GetTickCount
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

#define _T(__t_string__) L##__t_string__

void MacVideoInitializationFailed(const char* problem);
static inline int wstrncasecmp(const wchar_t* ws1, const wchar_t* ws2, int n) { return wcsncasecmp(ws1, ws2, n); }
static inline int wstrcasecmp(const wchar_t* ws1, const wchar_t* ws2) { return wcscasecmp(ws1, ws2); }

#ifdef __cplusplus
extern "C"
{
#endif


unsigned long GetTickCount(void);
void chAppDir();
const char* macPreferencesPath();
const char* macFileSeparator();
void SetCommandLine(int argc, char** argv);
const char* GetCommandLine();

void SetMouseScalingEnabled(int enabled);

SINT32 GetMicroSeconds(void);
DWORD GetMillisecondTick();


#ifdef __cplusplus
}
#endif

#endif // PN_SYSMAC_SYSMACCOMPAT_H
