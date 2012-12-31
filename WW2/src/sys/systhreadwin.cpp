#include <syscfg.h>

#include "systhread.h"

/************************************************************************************************
	systhreadCreate
*************************************************************************************************/

systhread systhreadCreate(systhread_start_routine func, void* userdata)
{
	DWORD threadid;
	return (systhread)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, userdata, 0, &threadid);
}

/************************************************************************************************
	systhreadMutexCreate
*************************************************************************************************/

systhread_mutex systhreadMutexCreate()
{
	return (systhread_mutex)CreateMutex(NULL, FALSE, NULL);
}

/************************************************************************************************
	systhreadMutexKill
*************************************************************************************************/

int systhreadMutexKill(systhread_mutex mutex)
{
	return CloseHandle((HANDLE)mutex);
}

/************************************************************************************************
	systhreadMutexLock
*************************************************************************************************/

int systhreadMutexLock(systhread_mutex mutex)
{
	return WaitForSingleObject((HANDLE)mutex, INFINITE);
}

/************************************************************************************************
	systhreadMutexTrylock
*************************************************************************************************/

int systhreadMutexTrylock(systhread_mutex mutex)
{
	return -1;
}

/************************************************************************************************
	systhreadMutexUnlock
*************************************************************************************************/

int systhreadMutexUnlock(systhread_mutex mutex)
{
	return ReleaseMutex((HANDLE)mutex);
}
