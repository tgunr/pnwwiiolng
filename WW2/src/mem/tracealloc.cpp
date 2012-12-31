#include <syscfg.h> // Path is project specific.

//#if defined(_DEBUG) && defined(WIN32) && defined(DETECT_LEAKS)
#if defined(WIN32) && defined(DETECT_LEAKS)

#include "MemLeakFindDll.h"

// Take over global new and delete
void* operator new(size_t s)
{
    return TraceAlloc(s);
}

void* operator new[](size_t s)
{
    return TraceAlloc(s);
}

void operator delete(void* pMem)
{
    TraceDealloc(pMem);
}

void operator delete[] (void* pMem)
{
    TraceDealloc(pMem);
}

// And then some crap for taking over MFC allocations.
void* __cdecl operator new(size_t s, LPCSTR lpszFileName, int nLine)
{
    return TraceAlloc(s);
}

void* __cdecl operator new[](size_t s, LPCSTR lpszFileName, int nLine)
{
    return TraceAlloc(s);
}

void __cdecl operator delete(void* pMem, LPCSTR /* lpszFileName */, int /* nLine */)
{
    TraceDealloc(pMem);
}

void __cdecl operator delete[](void* pMem, LPCSTR /* lpszFileName */, int /* nLine */)
{
    TraceDealloc(pMem);
}

#endif
