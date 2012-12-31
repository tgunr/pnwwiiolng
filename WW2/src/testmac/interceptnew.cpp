#include <syscfg.h> // Path is project specific.
#ifdef TESTMAC

#include <malloc/malloc.h>
#include <string>
#include "testmac/testmac.h"

void* operator new(std::size_t len) throw (std::bad_alloc)
{
  return mallochook2(malloc_default_zone(),len);
}

void* operator new[](std::size_t len) throw (std::bad_alloc)
{
  return mallochook2(malloc_default_zone(),len);
}

void operator delete (void* p)
{
  deletehook(p);
}

void operator delete[] (void* p)
{
  deletehook(p);
}

#endif
