// ========================================================================
// $File: //jeffr/granny/rt/granny_thread_safety.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #9 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_thread_safety.h"

#include "granny_parameter_checking.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

#undef SubsystemCode
#define SubsystemCode ThreadSafetyLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

thread_id_callback *ThreadIDCallback = 0;

END_GRANNY_NAMESPACE;

static bool AllowGlobalStateChanges = true;
static uint32 ThreadMakingChanges   = 0xFFFFBEEF;  // randomish, don't use 0 or -1


void GRANNY
SetAllowGlobalStateChanges(bool Allowed)
{
    GRANNY_ENTER();

    if (Allowed)
    {
        if (AllowGlobalStateChanges)
        {
            CheckCondition(ThreadMakingChanges == GetThreadID(), return);
            return;
        }

        AllowGlobalStateChanges = true;
        ThreadMakingChanges = GetThreadID();
    }
    else
    {
        AllowGlobalStateChanges    = false;
        ThreadMakingChanges = 0xFFFFBEEF;
    }
}

bool GRANNY
GetAllowGlobalStateChanges()
{
    GRANNY_ENTER();

    return (AllowGlobalStateChanges);
}


bool GRANNY
ThreadAllowedToCallGranny()
{
    GRANNY_ENTER();

    if (ThreadIDCallback && AllowGlobalStateChanges)
    {
        return GetThreadID() == ThreadMakingChanges;
    }

    return true;
}


thread_id_callback *GRANNY
SetThreadIDCallback(thread_id_callback *Callback)
{
    GRANNY_ENTER();

    thread_id_callback *OldCallback = ThreadIDCallback;

    ThreadIDCallback = Callback;

    // If the allowglobalstatechanges is set, then we assume that the
    // correct thread id is the current thread
    if (ThreadIDCallback != 0 && AllowGlobalStateChanges)
    {
        ThreadMakingChanges = (*Callback)();
    }

    return OldCallback;
}
