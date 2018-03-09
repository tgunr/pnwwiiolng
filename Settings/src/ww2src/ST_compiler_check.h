//
//  compiler_check.h
//  WW2OL
//
//  Created by Gwynne Raskind on 3/19/2016.
//
//


//  ============================
//
//  Settings.exe - ST_compiler_check.h
//
//  ============================


#ifndef compiler_check_h
#define compiler_check_h

// Compiler feature and version checks.
#ifndef __has_feature
#define __has_feature(...) 0
#endif

#ifndef __has_extension
#define __has_extension(...) 0
#endif

#ifndef __has_feature_or_ext
#define __has_feature_or_ext(...) __has_feature(__VA_ARGS__) || __has_extension(__VA_ARGS__)
#endif

#ifdef __clang_major__
#define __is_Clang() 1
#else
#define __is_Clang() 0
#endif

#ifndef __check_Clang_version
#define __check_Clang_version(major, minor) (__is_Clang() && (__clang_major__ > major || (__clang_major__ == major && __clang_minor__ >= minor)))
#endif

#if defined(__GNUC__) && !__is_Clang()
#define __is_GCC() 1
#else
#define __is_GCC() 0
#endif

#ifndef __check_GCC_version
#define __check_GCC_version(major, minor) ((__is_Clang() || __is_GCC()) && (__GNUC__ > major || (__GNUC__ == major && __GNUC_MINOR__ >= minor)))
#endif

#if defined(_MSC_VER) && !__is_Clang() && !__is_GCC()
#define __is_MSVC() 1
#else
#define __is_MSVC() 0
#endif

#ifndef __check_MSVC_version
#define __check_MSVC_version(major) (__is_MSVC() && (_MSC_VER >= major ## 00))
#endif

#endif /* compiler_check_h */
