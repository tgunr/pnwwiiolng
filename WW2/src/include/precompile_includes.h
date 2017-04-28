#pragma once

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

////////// SYSTEM INCLUDES ONLY -- NO PLAYNET SPECIFICS

#include <algorithm>
#include <atomic>
#include <array>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <numeric>
#include <thread>
#include <mutex>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

////////// PLAYNET SYSTYPES NEEDS TO GO HERE

#include "include/systypes.h"

////////// PLAYNET INCLUDES GO AFTER HERE

#include "include/strprintf.h"
#include "include/defs.h"
#include "dbug/dbug.h"
#include "dbug/dbugExtErrors.h"

#include "wwii/wwiiHash.h"
#include "chat/chatHandle.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiTypes.h"
#include "wwii/wwiiBranchEnums.h"
#include "teul/teulTime.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulKit.h"
#include "teul/teulBasic.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiFullObjID.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiPermissions.h"
#include "inst/instTypes.h"
#include "teul/teulPacket.h"
#include "weap/weapWeapon.h"
#include "teul/teulEndpoint.h"
#include "chat/chatLStringIDs.h"
#include "str/stringLib.h"

////////// Core utility functions

template <typename T, std::size_t N>
constexpr std::size_t ArraySize(T(&)[N]) { return N; }