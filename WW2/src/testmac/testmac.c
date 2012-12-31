#ifdef TESTMAC

#include <stdlib.h>
#include <stdio.h>
#include <malloc/malloc.h>
#include "testmac/testmac.h"
#include "testmac/trackmac.h"
#include "testmac/uthash.h"
#include "testmac/utlist.h"

typedef struct _malloc_zone_X // actual structure (private)
{
  /* Only zone implementors should depend on the layout of this structure;
   Regular callers should use the access functions below */
  void*	reserved1;	/* RESERVED FOR CFAllocator DO NOT USE */
  void*	reserved2;	/* RESERVED FOR CFAllocator DO NOT USE */
  size_t 	(*size)(struct _malloc_zone_t* zone, const void* ptr); /* returns the size of a block or 0 if not in this zone; must be fast, especially for negative answers */
  void* 	(*malloc)(struct _malloc_zone_t* zone, size_t size);
  void* 	(*calloc)(struct _malloc_zone_t* zone, size_t num_items, size_t size); /* same as malloc, but block returned is set to zero */
  void* 	(*valloc)(struct _malloc_zone_t* zone, size_t size); /* same as malloc, but block returned is set to zero and is guaranteed to be page aligned */
  void 	(*free)(struct _malloc_zone_t* zone, void* ptr);
  void* 	(*realloc)(struct _malloc_zone_t* zone, void* ptr, size_t size);
  void 	(*destroy)(struct _malloc_zone_t* zone); /* zone is destroyed and all memory reclaimed */
  const char*	zone_name;

  /* Optional batch callbacks; these may be NULL */
  unsigned	(*batch_malloc)(struct _malloc_zone_t* zone, size_t size, void** results, unsigned num_requested); /* given a size, returns pointers capable of holding that size; returns the number of pointers allocated (maybe 0 or less than num_requested) */
  void	(*batch_free)(struct _malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed); /* frees all the pointers in to_be_freed; note that to_be_freed may be overwritten during the process */

  struct malloc_introspection_t*	introspect;
  unsigned	version;

  /* aligned memory allocation. The callback may be NULL. */
	void* (*memalign)(struct _malloc_zone_t* zone, size_t alignment, size_t size);

  /* free a pointer known to be in zone and known to have the given size. The callback may be NULL. */
  void (*free_definite_size)(struct _malloc_zone_t* zone, void* ptr, size_t size);
} malloc_zone_X;

static void*     (*prealloc)(struct _malloc_zone_t* zone, void* ptr, size_t size) = NULL;
static void*     (*pmalloc)(struct _malloc_zone_t* zone, size_t size) = NULL;
static void*     (*pcalloc)(struct _malloc_zone_t* zone, size_t num_items, size_t size) = NULL;
static void*     (*pvalloc)(struct _malloc_zone_t* zone, size_t size) = NULL;
static void     (*pfree)(struct _malloc_zone_t* zone, void* ptr) = NULL;
static unsigned	(*p_batch_malloc)(struct _malloc_zone_t* zone, size_t size, void** results, unsigned num_requested);
static void     (*p_batch_free)(struct _malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed);
static void*     (*p_memalign)(struct _malloc_zone_t* zone, size_t alignment, size_t size);

unsigned batch_malloc_hook(struct _malloc_zone_t* zone, size_t size, void** results, unsigned num_requested);
void batch_free_hook(struct _malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed);
void*  vallochook(struct _malloc_zone_t* zone, size_t size);
void*  mallochook(struct _malloc_zone_t* zone, size_t size);
void*  reallochook(struct _malloc_zone_t* zone, void* ptr, size_t size);
void* callochook(struct _malloc_zone_t* zone, size_t num_items, size_t size);
void freehook(struct _malloc_zone_t* zone, void* p);
void* memalignhook(struct _malloc_zone_t* zone, size_t alignment, size_t size);
void* altrealloc(void* p,size_t sz);
void* altcalloc(size_t num, size_t sz);

static void trackAllocation( void* p );
static void trackDeallocation( void* p );

static char inited = 0;
static malloc_zone_t* defaultZone = 0;

#define MAXBUFFERSIZE 10240
static char* buffer;

int TrackMemory = 1;

//-------------------------------------------------

static void trackAllocation( void* p )
{
  size_t size = malloc_size(p);
}

static void trackDeallocation( void* p )
{
}

void enterspawn()
{
}

void exitspawn()
{
}

//---------------------- memory interception ---------------------------

unsigned batch_malloc_hook(struct _malloc_zone_t* zone, size_t size, void** results, unsigned num_requested)
{
  unsigned u = p_batch_malloc( zone, size, results, num_requested );
  return u;
}

void batch_free_hook(struct _malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed)
{
  p_batch_free( zone, to_be_freed, num_to_be_freed );
}

void* memalignhook(struct _malloc_zone_t* zone, size_t alignment, size_t size)
{
  init_testmac();
  void* p = p_memalign(zone,alignment,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void*  vallochook(struct _malloc_zone_t* zone, size_t size)
{
  init_testmac();
  void* p = pvalloc(zone,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void*  mallochook(struct _malloc_zone_t* zone, size_t size)
{
  init_testmac();
  void* p = pmalloc(zone,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void*  mallochook2(struct _malloc_zone_t* zone, size_t size)
{
  init_testmac();
  void* p = pmalloc(zone,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void*  reallochook(struct _malloc_zone_t* zone, void* ptr, size_t size)
{
  void* p = prealloc(zone,ptr,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void* callochook(struct _malloc_zone_t* zone, size_t num_items, size_t size)
{
  init_testmac();
  void* p = pcalloc(zone,num_items,size);
  if ( TrackMemory ) trackAllocation(p);
  return p;
}

void deletehook(void* ptr)
{
  trackDeallocation(ptr);
  malloc_zone_free( defaultZone, ptr );
}

void freehook(struct _malloc_zone_t* zone, void* p)
{
  trackDeallocation(p);
  pfree(zone,p);
}

void* altrealloc(void* p,size_t sz)
{
  return prealloc(defaultZone,p,sz);
}

void* altmalloc(size_t sz)
{
  return pmalloc(defaultZone,sz);
}

void* altcalloc(size_t num, size_t sz)
{
  return pcalloc(defaultZone,num, sz);
}

void altfree(void* p)
{
  pfree(defaultZone,p);
}

void init_testmac()
{
  if ( ! inited )
  {
    inited = 1;
    defaultZone = malloc_default_zone();
    malloc_zone_X* p = ((malloc_zone_X*)defaultZone);

    pvalloc = defaultZone->valloc;
    prealloc = defaultZone->realloc;
    pmalloc = defaultZone->malloc;
    pcalloc = defaultZone->calloc;
    pfree = defaultZone->free;
    p_batch_malloc = defaultZone->batch_malloc;
    p_batch_free = defaultZone->batch_free;
    p_memalign = p->memalign;

    defaultZone->valloc = vallochook;
    defaultZone->realloc = reallochook;
    defaultZone->malloc = mallochook;
    defaultZone->calloc = callochook;
    defaultZone->free = freehook;
    defaultZone->batch_malloc = batch_malloc_hook;
    defaultZone->batch_free = batch_free_hook;

    p->free_definite_size = NULL;
    p->memalign = memalignhook;

    buffer = altmalloc( MAXBUFFERSIZE );

  }
}

void exit_testmac()
{
}

#endif
