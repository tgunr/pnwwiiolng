#pragma once


//  ============================
//
//  Settings.exe - ST_precompile_includes.h
//
//  ============================


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
//  #include "include/defs.h"
#include "dbug/dbug.h"
#include "dbug/dbugExtErrors.h"

//  #include "wwii/wwiiHash.h"
#include "chat/chatHandle.h"
//  #include "wwii/wwiiOwnershipEnums.h"
//  #include "wwii/wwiiTypes.h"
//  #include "wwii/wwiiBranchEnums.h"
#include "teul/teulTime.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulKit.h"
#include "teul/teulBasic.h"
#include "teul/teulPackers.h"
//  #include "wwii/wwiiFullObjID.h"
//  #include "weap/weapSpec.h"
//  #include "wwii/wwiiPermissions.h"
//  #include "inst/instTypes.h"
#include "teul/teulPacket.h"
//  #include "weap/weapWeapon.h"
#include "teul/teulEndpoint.h"
//  #include "chat/chatLStringIDs.h"
//  #include "str/stringLib.h"

////////// Core utility functions

// Use to silence warnings about unused parameters.
template<typename T> constexpr void silence_unused_parameter_warning(T&& t) { void(std::forward<T>(t)); }
template<typename T, typename... Args>
constexpr void silence_unused_parameter_warning(T&& t, Args&&... args) {
	(void)std::forward<T>(t), silence_unused_parameter_warning(std::forward<Args>(args)...);
}

// To avoid complaints about impossible control paths not returning a value.
NORETURN_FN inline void noreturn_fn(void) { abort(); }

//==============

template <typename T, std::size_t N>
constexpr std::size_t array_size(T(&)[N]) noexcept { return N; }

template <typename... Args>
struct num_args {
    static constexpr const size_t value = sizeof...(Args);
};

template <bool...> struct bool_pack;
template <bool... v>
using all_true = std::is_same<bool_pack<true, v...>, bool_pack<v..., true>>;

//==============
// For using RAII to guarantee a function is called at scope exit in a readable way.

struct WrapScope {
	WrapScope(const std::function<void(void)>& beg, const std::function<void(void)>& e)
	: end(e) { beg(); }
	~WrapScope() { end() ; }
	operator bool (void) const { return false; }

	std::function<void(void)> end;
};

struct AtScope {
	AtScope(const std::function<void(void)>& e) : end(e) {}
	~AtScope() { end(); }
	operator bool (void) const { return false; }

	std::function<void(void)> end;
};

#define WITH(...) if ( const auto& _with_=__VA_ARGS__ ) { noreturn_fn(); } else
