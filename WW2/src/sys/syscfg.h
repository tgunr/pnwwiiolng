#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard, because there are multiple syscfgs.

#define platBuild TEUL_CLIENT

// Windows defines
#define OPENGL3D	1
#define WIN32_LEAN_AND_MEAN
#define FRONT_END

#if !defined(USE_OSG)
	#define _GDI32_
#endif

#define NO_DEBUG_WRITE
#define USING_EXT_NAMESPACE using namespace stdext
#pragma warning (disable: 4800) // shut the #$!@ up

#include "include/platforms.h"

#include <windows.h>
#undef PostMessage

// Project defines - Non-Specific
typedef LOGICAL (*SYS_DDINITFUNC)(void);
typedef LOGICAL (*SYS_D3DINITFUNC)(void);
typedef LOGICAL (*SYS_DDRENDERFUNC)(void);
typedef LOGICAL (*SYS_D3DRENDERFUNC)(void);
typedef void (*SYS_DDKILLFUNC)(void);
typedef void (*SYS_D3DKILLFUNC)(void);
typedef void (*SYS_MOUSEFUNC)(UINT message, WPARAM wparam, LPARAM lparam);
typedef void (*SYS_TIMERFUNC)(WPARAM wparam, LPARAM lparam);
typedef void (*SYS_KEYBOARDFUNC)(UINT message, WPARAM wparam, LPARAM lparam);

#define MAXENUMDRIVERS		32
#define MAXMIPLEVELS		12
#define MINSOUNDS			32
#define MINDYNALITES		16

#define SYS_TITLE				"WWIIOL"
#define SYS_COMPANY_NAME		"Playnet"
#define SYS_REGISTRY_NAME		_T(SYS_COMPANY_NAME) _T("\\") _T(SYS_TITLE)

#define PREPATH ".\\Data\\"
#define TERRAIN_PREPATH "Terrain\\"
#define GR2DATA_PREPATH "gr2Data\\"
#define USERDATA_PREPATH "\\Battleground Europe\\"
#define USERDATA_PREPATH_BETA "\\Battleground Europe TEST\\"

#define HIGHSPEED_LIGHTING

// Use the 3d API lighting mechanism (may be hardware)
#define USE_3DAPI_LIGHTING

// Project defines - DEBUG
#ifdef _DEBUG
	#ifndef NO_LOGGING
		#define EVENT_LOG
		#define EVENT_LOG_TIMES
//		#define EVENT_LOG_EXTREME
	#endif
	// #define MEMORY_DEBUG_EXTREME
#elif defined(BETA_RELEASE)
	#define EVENT_LOG
#endif	// _DEBUG

#define NO_DEBUG_WRITE

#include "log/log.h"

#define USING_EXT_NAMESPACE using namespace stdext
#pragma warning (disable: 4800) // shut the #$!@ up

#ifndef snprintf
# define snprintf _snprintf
#endif
#ifndef usleep
# define usleep(n) Sleep((n))
#endif

#if _MSC_VER >= 1500
	// Visual Studio 9
#else
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif // _MSC_VER >= 1500

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

// we undefine these so we can redefine them below
// we need to redefine them because the game was
// never coded for unicode, but we need to turn on
// unicode for chinese IMEs; redefining these
// saves a ton of work
#undef lstrcpy
#undef lstrcat
#undef lstrcmp
#undef lstrlen

#define lstrcpy strcpy
#define lstrcat strcat
#define lstrcmp strcmp
#define lstrlen strlen

// these defines replace the above
// defines so can can still conditionally
// compile the unicode flag in certain
// string handling situations (if needed)
#ifdef _UNICODE
#define ustrcpy lstrcpyW
#define ustrcat lstrcatW
#define ustrcmp lstrcmpW
#define ustrlen lstrlenW
#define usprintf swprintf
#else
#define ustrcpy lstrcpyA
#define ustrcat lstrcatA
#define ustrcmp lstrcmpA
#define ustrlen lstrlenA
#define usprintf sprintf
#endif

#include "include/precompile_includes.h"
#include <deque>

#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/Matrixd.h"
#include "math/Matrixf.h"
#include "spt/random.h"

#include "include/vars.h"
#include "ddp.h"

#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiCovert.h"
#include "weap/weapDefs.h"
#include "weap/weapSpec.h"
#include "inst/instDescriptor.h"

#include "phui/Javascript.h"

#include "pi/pi.h"

#endif
