#include <pthread.h>
#include <stdlib.h>

#include "systhread.h"

/************************************************************************************************
	systhread
*************************************************************************************************/

typedef void* (*pthread_start_routine)(void*);

/************************************************************************************************
	systhreadCreate
*************************************************************************************************/

systhread systhreadCreate(systhread_start_routine func, void* userdata)
{
	pthread_t thread = NULL;

	if(pthread_create(&thread, NULL, (pthread_start_routine)func, userdata) != 0)
		return NULL;

	return thread;
}


/************************************************************************************************
	systhreadMutexCreate
*************************************************************************************************/

systhread_mutex systhreadMutexCreate()
{
	pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

	if(mutex && pthread_mutex_init(mutex, NULL) != 0)
	{
		free(mutex);
		return NULL;
	}

	return mutex;
}

/************************************************************************************************
	systhreadMutexKill
*************************************************************************************************/

int systhreadMutexKill(systhread_mutex id)
{
	pthread_mutex_t* mutex = (pthread_mutex_t*)id;
	if(mutex)
	{
		 pthread_mutex_destroy(mutex);
		 free(mutex);
	}
	return 0; 
}

/************************************************************************************************
	systhreadMutexLock
*************************************************************************************************/

int systhreadMutexLock(systhread_mutex mutex)
{
	return pthread_mutex_lock((pthread_mutex_t*)mutex);
}

/************************************************************************************************
	systhreadMutexTryLock
*************************************************************************************************/

int systhreadMutexTryLock(systhread_mutex mutex)
{
	return pthread_mutex_trylock((pthread_mutex_t*)mutex);
}

/************************************************************************************************
	systhreadMutexUnlock
*************************************************************************************************/

int systhreadMutexUnlock(systhread_mutex mutex) 
{
	return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}
