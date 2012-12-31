#ifndef PN_SYSMAC_SYSTHREAD_H
#define PN_SYSMAC_SYSTHREAD_H 1

typedef int (*systhread_start_routine)(void* userdata);
typedef void* systhread;
typedef void* systhread_mutex;

/************************************************************************************************
	systhread
*************************************************************************************************/

systhread systhreadCreate(systhread_start_routine, void* userdata);

systhread_mutex systhreadMutexCreate();
int systhreadMutexKill(systhread_mutex);

int systhreadMutexLock(systhread_mutex);
int systhreadMutexTryLock(systhread_mutex);
int systhreadMutexUnlock(systhread_mutex);

#endif // PN_SYSMAC_SYSTHREAD_H
