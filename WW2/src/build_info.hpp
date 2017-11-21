//
//  build_info.hpp
//  World War II Online
//
//  Created by Gwynne Raskind on 4/29/2016
//  Copyright © 2016 Playnet, Inc.
//

#ifndef PN__BUILD_INFO_HPP__
#define PN__BUILD_INFO_HPP__

// In debug builds, we want all the information possible available. In release
// builds, we want only the absolute minimum information necessary to identify
// a given build. Because this is security-critical information, we check both
// DEBUG and NDEBUG to guard against info leakage in poorly configured release
// builds. Yes, this check is pretty paranoid. DO NOT REUSE THIS MACRO IN OTHER
// CODE. I can't be clearer on this point: The debug check macro in this header
// is for use by THIS HEADER and its corresponding source file ONLY. You MAY NOT
// pollute the global macro namespace even further by misusing this macro (by
// using it anywhere else whatsoever). The only reason it's even in the header
// at all is so extern names for variables that shouldn't be provided to release
// clients do not get declared in the first place. Any use of the macro found
// outside these two files will be deleted with extreme prejudice.
#if defined(DEBUG) && !defined(NDEBUG) // If DEBUG is defined *and* NDEBUG is not
    #define BUILD_INFO_SENSITIVE_INFO_AVAILABLE 12345 // Require a specific value, not just zero or nonzero.
#endif
// Don't even define the macro at all if not in a debug build. No #ifdef vs #if
// mistakes, please.

///////////////////////////////////////////////////////////////////////////////////
// BEGIN GLOBAL VERSION INFORMATION
// This is the central version information for both hosts and clients at the current
// time. It may be changed later, but this is presently the *only* place to change
// the "product" version. Everything else is derived from these values by the
// build_info.py script.
static constexpr unsigned char
    PN_build_version_major = 1,
    PN_build_version_minor = 35,
    PN_build_version_patch = 9,
    PN_build_version_build = 8;
// END GLOBAL VERSION INFORMATION
///////////////////////////////////////////////////////////////////////////////////

static constexpr unsigned int
	PN_build_version_value =  // Guaranteed to be at least 32 bits, but may be wider
	    (PN_build_version_major << 24 | PN_build_version_minor << 16 |
	     PN_build_version_patch <<  8 | PN_build_version_build <<  0);

extern const char
    * const PN_build_datetime_string,
    * const PN_build_platform_string,
    * const PN_build_type_string,
    * const PN_product_version_string,
    * const PN_code_git_commitish_string,
    * const PN_data_git_commitish_string
#if defined(BUILD_INFO_SENSITIVE_INFO_AVAILABLE) && BUILD_INFO_SENSITIVE_INFO_AVAILABLE == 12345
    ,
    * const PN_build_info_input_string,
    * const PN_build_info_output_string,
    * const PN_build_user_string,
    * const PN_build_host_string,
    * const PN_build_location_string,
    * const PN_code_git_commit_string,
    * const PN_code_git_repository_state_string,
    * const PN_data_git_commit_string,
    * const PN_data_git_repository_state_string
#endif
;

#endif
