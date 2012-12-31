// mem.c
//

#include <stdlib.h>
#include <string.h>

#include "syscfg.h"
#include "platform.h"
#include "pi.h"
#include "mem.h"

#ifdef __cplusplus
extern "C"
{
#endif

static long numberallocs;
static long numberalloced;
static long maxallocs;
static long maxalloced;
static long totalallocs;
static long totalalloced;


LOGICAL memInit(void)
{
	numberallocs = numberalloced = maxallocs = maxalloced = 0;
	totalallocs = totalalloced = 0;
	
	return TRUE;
}

void memKill(void)
{
	Write_Debug("Total memory blocks allocated: %d", totalallocs);
	Write_Debug("Total memory allocated: %d", totalalloced);
	Write_Debug("Peak memory blocks allocated: %d", maxallocs);
	Write_Debug("Peak memory allocated: %d", maxalloced);
	Write_Debug("Total memory blocks unfreed: %d", numberallocs);
	Write_Debug("Total memory unfreed: %d", numberalloced);
}

void *memAllocRelease(SINT32 Size)
{
	void	*ptr = NULL;
	
	if(Size)
	{
		ptr = calloc(Size, 1);
		if (ptr == NULL)
		{
			piExitProgram(__FILE__,__LINE__,"Out of Memory!");				
			return(NULL);
		}

#ifdef EVENT_LOG
#if TARGET_API_MAC_OS8
		Size = __msize(ptr);
#endif

		totalalloced += Size;
		numberalloced += Size;
		
		if (maxalloced < numberalloced)
		{
			maxalloced = numberalloced;
		}

		totalallocs += 1;
		numberallocs += 1;

		if (maxallocs < numberallocs)
		{
			maxallocs = numberallocs;
		}
#endif // EVENT_LOG
	}
	
	return(ptr);
}

void memFreeRelease(void *ptr)
{
	if (ptr == NULL)
	{
		Write_Debug("Memory: Tried to free a NULL memory ptr");
		return;
	}

#ifdef EVENT_LOG
#if TARGET_API_MAC_OS8
	numberalloced -= __msize(ptr);
#endif
	numberallocs -= 1;
#endif // EVENT_LOG

	free(ptr);
}

#ifdef __cplusplus
}
#endif
