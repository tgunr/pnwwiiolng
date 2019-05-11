//
//  build_info.generic.cpp
//  World War II Online
//
//  Created by Gwynne Raskind on 10/06/2018
//  Copyright © 2018 Playnet, Inc.
//

// **************************************************
// This is a version of build_info.cpp.in whose values are hardcoded defaults
// which are guaranteed to be wrong. This is here to enable faster iteration
// when doing lots of repeated builds, where having this file always being out
// of date as far as Visual Studio and Xcode are concerned wastes a lot of time.
// **************************************************

// Because otherwise MSVC cries like a whiny baby about PCH.
// This does not need to be here for any actual reason whatsoever.
#include "syscfg.h"

#include "build_info.hpp"
#include "include/platforms.h"

#if !defined(DEBUG) || defined(NDEBUG) || platBuild == BUILD_CLIENT
#error "This file should never be built into release builds"
#endif

// __attribute__((used)) prevents the compiler from dead-stripping these values
// under Clang/GCC. With MSVC we have to use the much more annoying #pragma.
#if __is_MSVC()
#define __used_variable__(x) x //__pragma(comment(linker, "/include:" #x)) x
#else
#define __used_variable__(x) __attribute__((used)) x
#endif

const char
    * const __used_variable__(PN_build_datetime_string) = __DATE__ __TIME__,
    * const __used_variable__(PN_build_platform_string) = "Dev",
    * const __used_variable__(PN_build_type_string) = "Dev",
    * const __used_variable__(PN_product_version_string) = "0.999.999.999",
    * const __used_variable__(PN_code_git_commitish_string) = "000000000",
    * const __used_variable__(PN_data_git_commitish_string) = "000000000"
#if BUILD_INFO_SENSITIVE_INFO_AVAILABLE == 12345
    ,
    * const __used_variable__(PN_build_info_input_string) = "build_info.generic.cpp",
    * const __used_variable__(PN_build_info_output_string) = "build_info.cpp",
    * const __used_variable__(PN_build_user_string) = "nobody",
    * const __used_variable__(PN_build_host_string) = "localhost",
    * const __used_variable__(PN_build_location_string) = "pwd",
    * const __used_variable__(PN_code_git_commit_string) = "0000000000000000000000000000000000000000",
    * const __used_variable__(PN_code_git_repository_state_string) = "albatross",
    * const __used_variable__(PN_data_git_commit_string) = "0000000000000000000000000000000000000000",
    * const __used_variable__(PN_data_git_repository_state_string) = "the larch"
#endif
;
