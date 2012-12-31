#ifdef TESTMAC

#include <malloc/malloc.h>

#ifdef __cplusplus
#define EXTERN__C extern "C"
#else
#define EXTERN__C extern
#endif

EXTERN__C void init_testmac();

EXTERN__C void exit_textmac();

EXTERN__C void*  mallochook2(struct _malloc_zone_t* zone, size_t size);
EXTERN__C void deletehook(void* ptr);

EXTERN__C void* altmalloc(size_t sz);
EXTERN__C void altfree(void* p);

extern int TrackMemory;

#undef EXTERN__C

#endif
