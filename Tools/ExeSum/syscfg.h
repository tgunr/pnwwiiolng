#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard, because there are multiple syscfgs.

#define platBuild TEUL_CLIENT

// Windows defines
#define WIN32_LEAN_AND_MEAN
#define FRONT_END
#define NO_DEBUG_WRITE

#include "include/platforms.h"

#include <windows.h>
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

#include "include/precompile_includes.h"

#endif
