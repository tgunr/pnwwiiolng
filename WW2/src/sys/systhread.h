#pragma once

typedef int (*systhread_start_routine)(void* userdata);
typedef void* systhread;
typedef void* systhread_mutex;

/************************************************************************************************
	systhread
*************************************************************************************************/

extern systhread systhreadCreate(systhread_start_routine, void* userdata);

extern systhread_mutex systhreadMutexCreate();
extern int systhreadMutexKill(systhread_mutex);

extern int systhreadMutexLock(systhread_mutex);
extern int systhreadMutexTryLock(systhread_mutex);
extern int systhreadMutexUnlock(systhread_mutex);

