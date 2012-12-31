// ========================================================================
// $File: //jeffr/granny/rt/granny_assert.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #2 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#define SubsystemCode AssertionLogMessage

USING_GRANNY_NAMESPACE;


#if DEBUG

BEGIN_GRANNY_NAMESPACE;

assertion_callback AssertionCallback = { DisplayAssertion, 0 };

END_GRANNY_NAMESPACE;

void GRANNY
GetAssertionCallback(assertion_callback* Result)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Result, return);
    Result->Function = AssertionCallback.Function;
    Result->UserData = AssertionCallback.UserData;
}

void GRANNY
SetAssertionCallback(assertion_callback const &CallbackInit)
{
    GRANNY_ENTER();

    AssertionCallback.Function = CallbackInit.Function;
    AssertionCallback.UserData = CallbackInit.UserData;
}

#else

BEGIN_GRANNY_NAMESPACE;

assertion_callback AssertionCallback = { 0, 0 };

END_GRANNY_NAMESPACE;

void GRANNY
GetAssertionCallback(assertion_callback* Result)
{
    GRANNY_ENTER();

    CheckPointerNotNull(Result, return);
    Result->Function = 0;
    Result->UserData = 0;
}

void GRANNY
SetAssertionCallback(assertion_callback const &CallbackInit)
{
    GRANNY_ENTER();

    // Nothing, asserts disabled.
}

#endif
