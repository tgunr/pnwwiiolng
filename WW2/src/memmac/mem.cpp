#include <syscfg.h> // Path is project specific.
//

#include <stdlib.h>
#include <string.h>

#include "pi/pi.h"
#include "mem/mem.h"

static long numberallocs;
static long numberalloced;
static long maxallocs;
static long maxalloced;
static long totalallocs;
static long totalalloced;

#ifdef TRACKER_OF_MEMORY

static MemTracker* memTracker = NULL;
static bool dump = 0;
static size_t tcounter = 0;

bool recording = true;

void* tracker_alloc(size_t sz,const char* file, int line)
{
  if ( memTracker == NULL ) memTracker = new MemTracker();
  return memTracker->trackedAlloc(sz,file,line);
}

void tracker_dealloc(void* ptr,const char* file, int line)
{
  if ( memTracker == NULL ) memTracker = new MemTracker();
  return memTracker->trackedFree(ptr,file,line);
}

MemTracker::MemTracker()
{
}

MemTracker::~MemTracker()
{
  int left = trackerSet.size();
  printf("left: %d\n",left);
}

typedef struct TInfo
{
  size_t count;
  size_t sum;
} TInfo;

void MemTracker::dumpData()
{
  printf("Count=%ld\n",trackerSet.size());
  std::map<void*,TrackerData>::iterator it = trackerSet.begin();
  size_t sum = 0;

  std::map<std::string,TInfo> f;

  while (it!=trackerSet.end())
  {
    sum += it->second.size;
    char s[80];
    sprintf(s,"%s %d",it->second.filename, it->second.line);
    std::string st(s);

    std::map<std::string,TInfo>::iterator t = f.find(st);
    if ( t != f.end() )
    {
      t->second.count++;
      t->second.sum+=it->second.size;
    }
    else
    {
      TInfo ti;
      ti.count = 1;
      ti.sum = it->second.size;
      f.insert(std::pair<std::string,TInfo>(st,ti));
    }


    //printf("%s = %ld\n",s, it->second.size);
    it++;
  }
  printf("Sum=%ld\n",sum);

  for (std::map<std::string,TInfo>::iterator i = f.begin(); i!=f.end(); i++)
  {
    printf("%s %ld (%ld) \n",i->first.c_str(), i->second.count, i->second.sum);
  }
}

void* MemTracker::trackedAlloc(size_t sz,const char* file, int line)
{
  tcounter++;

  if ( (tcounter % 1000) == 0 )
  {
    dump = true;
  }

  void* ptr = calloc(1,sz);
  //if ( recording )
  {
    std::map<void*,TrackerData>::iterator it = trackerSet.find(ptr);

    if ( it != trackerSet.end() )
    {
      size_t sze = trackerSet.size();
      TrackerData& tt = (*it).second;

      printf("non-tracked-free object: %s %d\n",tt.filename,tt.line);
    }

    TrackerData t;
    t.size = sz;
    strncpy( t.filename, file, 63 );
    t.line = line;

    trackerSet.insert(std::pair<void*,TrackerData>(ptr,t));
  }

  //printf("%ld %s %d\n",sz,file,line);

  if ( dump )
  {
    dumpData();
    dump = 0;
  }

  return ptr;
}

void MemTracker::trackedFree(void* ptr,const char* file, int line)
{
  //if ( recording )
  {
    std::map<void*,TrackerData>::iterator it = trackerSet.find(ptr);

    if( it == trackerSet.end() )
    {
      size_t sz = trackerSet.size();
      printf("non-tracked-allocted object being freed: %s %d\n",file,line);
    }

    trackerSet.erase(ptr);
  }

  free(ptr);
}

#else

void memInit(void)
{
	numberallocs = numberalloced = maxallocs = maxalloced = 0;
	totalallocs = totalalloced = 0;
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

#endif

void* memAllocRelease(SINT32 Size)
{
	void*	ptr = NULL;

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

void memFreeRelease(void* ptr)
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
