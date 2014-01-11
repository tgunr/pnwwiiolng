#pragma once

////////// SYSTEM INCLUDES ONLY -- NO PLAYNET SPECIFICS

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdint>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <list>
#include <algorithm>

#include <thread>
#include <mutex>

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
#include "dbug/dbug.h"
#include "dbug/dbugExtErrors.h"

#include "wwii/wwiiHash.h"
#include "chat/chatHandle.h"
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
#include "wwii/wwiiPermissions.h"
#include "inst/instTypes.h"
#include "wwii/wwiiFullObjID.h"
#include "teul/teulPacket.h"
#include "weap/weapWeapon.h"
#include "teul/teulEndpoint.h"
#include "chat/chatLStringIDs.h"
