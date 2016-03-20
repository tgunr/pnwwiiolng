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
#define MAKELONG(a,b) (((b) << 16) | (a & 0xffff))
#define HIWORD HiWord
#define LOWORD LoWord

#ifndef _MSL_USING_MW_C_HEADERS
#undef _MAX_PATH
#define _MAX_PATH 260
#define cosf cos
#define sinf sin
#endif

#define MAX_PATH _MAX_PATH

#ifdef __cplusplus
#include <algorithm>
#else
#define max(a,b)  ({ __typeof__((a)) __a = (a); __typeof__((b)) __b = (b); __a > __b ? __a : __b; })
#define min(a,b)  ({ __typeof__((a)) __a = (a); __typeof__((b)) __b = (b); __a < __b ? __a : __b; })
#endif
#endif

#define _T(__t_string__) L##__t_string__

void MacVideoInitializationFailed(const char* problem);

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _MSL_USING_MW_C_HEADERS
int isascii(int c);
void* memccpy(void* dst, const void* src, int c, size_t len);
#endif

char* strupr(char* string);
char* strlwr(char* string);
#define strcmpi strcasecmp
#define strnicmp strncasecmp
unsigned long GetTickCount(void);
void chAppDir();
const char* macPreferencesPath();
const char* macFileSeparator();
void SetCommandLine(int argc, char** argv);
const char* GetCommandLine();

int wstrncasecmp(const wchar_t* ws1, const wchar_t* ws2, int n);
int wstrcasecmp(const wchar_t* ws1, const wchar_t* ws2);
void SetMouseScalingEnabled(int enabled);

SINT32 GetMicroSeconds(void);
DWORD GetMillisecondTick();

char* getPrePath();

#ifdef __cplusplus
}
#endif

#endif // PN_SYSMAC_SYSMACCOMPAT_H
