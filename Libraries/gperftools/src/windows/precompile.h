#ifndef GPERFTOOLS_PRECOMPILE_H
#define GPERFTOOLS_PRECOMPILE_H 1

#define NOMINMAX       // so std::max, below, compiles correctly

#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <algorithm>
#include <string>
#include <new>

#include "common.h"
#include "getenv_safe.h"                // for TCMallocGetenvSafe
#include "internal_logging.h"
#include "mini_disassembler.h"

#if defined(PLATFORM_WINDOWS)
#include <process.h>          // for getpid() (actually, _getpid())
#include <shlwapi.h>          // for SHGetValueA()
#include <tlhelp32.h>         // for Module32First()
#endif

#include "base/sysinfo.h"
#include "base/commandlineflags.h"
#include "base/dynamic_annotations.h"   // for RunningOnValgrind
#include "base/logging.h"

#endif // GPERFTOOLS_PRECOMPILE_H
