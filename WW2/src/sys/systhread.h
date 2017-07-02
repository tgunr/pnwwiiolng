#pragma once

using systhread_start_routine = int (*)(void* userdata);
using systhread = void*;
using systhread_mutex = void*;

/************************************************************************************************
	systhread
*************************************************************************************************/

extern systhread systhreadCreate(systhread_start_routine, void* userdata);

extern systhread_mutex systhreadMutexCreate();
extern int systhreadMutexKill(systhread_mutex);

extern int systhreadMutexLock(systhread_mutex);
extern int systhreadMutexTryLock(systhread_mutex);
extern int systhreadMutexUnlock(systhread_mutex);

