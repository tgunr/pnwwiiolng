#pragma once

/************************************************************************************************
	parchive
*************************************************************************************************/

#if defined(USE_OSG) && (defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__))
    #  if 0
    #    define PA_EXPORT   __declspec(dllexport)
    #  else
    #    define PA_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define PA_EXPORT
#endif

struct tagpaFILE;
typedef struct tagpaFILE paFILE;

PA_EXPORT paFILE* paAlloc();
PA_EXPORT void paFree(paFILE*);

PA_EXPORT int  paOpen(paFILE*, const char* archivename, const char* mode);
PA_EXPORT void paClose(paFILE*, int archiveIdx);

PA_EXPORT int paFetchMember (paFILE*, const char* membername, unsigned char** data, int* len);
PA_EXPORT int paAppendmember(paFILE*, const char* membername, const unsigned char* data, int len);
PA_EXPORT void paFreeMember(unsigned char* data);

