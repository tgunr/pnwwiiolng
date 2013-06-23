#pragma once

////////// SYSTEM INCLUDES ONLY -- NO PLAYNET SPECIFICS

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdexcept>
#include <wchar.h>
#include <float.h>
#include <ctype.h>

#include <thread>
#include <mutex>
#include <chrono>
#include <cstdint>

#if defined(HAVE_SYS_CDEFS_H)
# include <sys/cdefs.h>
#endif
#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif

#if platOS == OS_MACOS
#include <sys/types.h>
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

////////// PLAYNET SYSTYPES NEEDS TO GO HERE

#include "include/systypes.h"

////////// PLAYNET INCLUDES GO AFTER HERE

#include "include/defs.h"
#include "mem/mem.h"

#include "wwii/wwiiHash.h"
#include "chat/chatHandle.h"
#include "dbug/dbug.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiTypes.h"
#include "wwii/wwiiBranchEnums.h"
#if defined(AS_SERVER)
#include "wwii/wwiiConfig.h"
#include "server/serverServerInfo.h"
#endif
#include "teul/teulTime.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulPackers.h"
#include "teul/teulPackEnums.h"
#include "teul/teulKit.h"
#include "teul/teulBasic.h"
#include "dbug/dbugExtErrors.h"
#include "wwii/wwiiPermissions.h"
#include "inst/instTypes.h"
#include "wwii/wwiiFullObjID.h"
#include "teul/teulPacket.h"
#include "weap/weapWeapon.h"
#include "teul/teulEndpoint.h"
#include "chat/chatLStringIDs.h"

