#if !defined(GRANNY_ASSERT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_assert.h $
// $DateTime: 2009/07/28 16:54:38 $
// $Change: 22552 $
// $Revision: #12 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif


// Handy place to do some checks.

// _DEBUG is defined if the compiler is working in debug mode,
// i.e. with no optimisations. But even in an optimised build,
// we might want asserts and suchlike.
#if defined(_DEBUG) && (_DEBUG==0)
#error Either define _DEBUG, or dont define it. Never set it to 0.
#endif

// DEBUG is the one that says "I want asserts". You might still
// want asserts and checks in an optimised build for example.
#if defined(DEBUG) && (DEBUG!=1)
#error Either define DEBUG to 1, or dont define it. Never set it to 0.
#endif

#if PLATFORM_WINXX
   #define IGNORE_STATIC static
#else
   #define IGNORE_STATIC
#endif


BEGIN_GRANNY_NAMESPACE;

typedef bool assertion_display_function(char const * const Expression,
                                        char const * const File,
                                        int32x const LineNumber,
                                        char const * const Function,
                                        bool* IgnoreAssertion,
                                        void* UserData);
struct assertion_callback
{
    assertion_display_function *Function;
    void *UserData;
};

bool DisplayAssertion(char const * const Expression,
                      char const * const File,
                      int32x const LineNumber,
                      char const * const Function,
                      bool* IgnoreAssertion,
                      void* UserData);

#if DEBUG

#undef Assert

#if COMPILER_MSVC
// MSVC
#define Assert(Expression)                                                                          \
    {                                                                                               \
        USING_GRANNY_NAMESPACE;                                                                     \
        IGNORE_STATIC bool IgnoreAssert = false;                                                    \
        if (!IgnoreAssert && !(Expression))                                                         \
        {                                                                                           \
            assertion_callback Callback;                                                            \
            GetAssertionCallback(&Callback);                                                        \
            if (Callback.Function)                                                                  \
            {                                                                                       \
                bool ShouldBreak = Callback.Function(#Expression, __FILE__, __LINE__,               \
                                                     "Unknown", &IgnoreAssert, Callback.UserData);  \
                if (ShouldBreak) __debugbreak();                                                    \
            }                                                                                       \
        }                                                                                           \
    } typedef int gr__AssertionsRequireSemiColons
#else
// Non-MSVC compiler
#define Assert(Expression)                                                      \
    {                                                                           \
        USING_GRANNY_NAMESPACE;                                                 \
        IGNORE_STATIC bool IgnoreAssert = false;                                \
        if (!IgnoreAssert && !(Expression))                                     \
        {                                                                       \
            assertion_callback Callback;                                        \
            GetAssertionCallback(&Callback);                                    \
            if (Callback.Function)                                              \
            {                                                                   \
                Callback.Function(#Expression, __FILE__, __LINE__,              \
                                  "Unknown", &IgnoreAssert, Callback.UserData); \
            }                                                                   \
        }                                                                       \
    } typedef int gr__AssertionsRequireSemiColons

#endif

#else
   #undef Assert
   #define Assert(Expression) ((void)sizeof(Expression))

#endif // DEBUG


// Compile assertions are always turned on
#define CompileAssert(Expression) typedef int CompileAssert_Type__[(Expression) ? 1 : -1]

#define InvalidCodePath(Text) Assert(!Text)
#define NotImplemented InvalidCodePath("NotImplemented")

void GetAssertionCallback(assertion_callback* Result);
void SetAssertionCallback(assertion_callback const &CallbackInit);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_ASSERT_H
#endif
