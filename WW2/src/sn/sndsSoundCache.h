#pragma once

#include "sn/sndsSoundControl.h"

typedef struct SOUNDCACHE* LPSOUNDCACHE;

#define MINSOUNDCACHE	1024

extern LOGICAL sndsInitCache(void);
extern void sndsKillCache(void);
extern LOGICAL sndsEmptyCache(void);

#ifdef SOUND_DEBUG
	#define sndsPreloadSoundCache(a)   sndsPreloadSoundCacheDebug(a, _FL)
	extern LOGICAL sndsPreloadSoundCacheDebug(const SCHAR* pszName, const SCHAR* file, SINT32 line);
#else
	extern LOGICAL sndsPreloadSoundCache(const SCHAR* pszName, float maxattenuation = 0.0f);
#endif

#ifdef SOUND_DEBUG
	#define sndsLoadSoundCache(a)   sndsLoadSoundCacheDebug(a, _FL)
	extern LOGICAL sndsLoadSoundCacheDebug(snSound* newSound, const SCHAR* file, SINT32 line);
#else
	extern LOGICAL sndsLoadSoundCache(snSound* newSound);
#endif

extern LOGICAL sndsReleaseSoundInCache(snSound* oldSound);

