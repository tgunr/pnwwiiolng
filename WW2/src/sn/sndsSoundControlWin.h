#pragma once

#include "include/sysTypes.h"
#include "sn/sndsSoundControl.h"

/////////////////////////////
// UINT32 return functions
/////////////////////////////
extern UINT32 sndsGetMaxNumberOfSounds(void);

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
extern LOGICAL sndsPlatformCommitDeferredSettings();
extern LOGICAL sndsPlatformCreateSound(snSound* lpOld, snSound* lpNew);
extern LOGICAL sndsPlatformDriverConfig(void* hints);
extern LOGICAL	sndsPlatformDuplicateSound(snSound* to, const snSound* from);
extern LOGICAL sndsPlatformFreeSound(snSound* snd);
extern LOGICAL sndsPlatformGetVolume(snSound* snd, SINT32* vol);
extern LOGICAL sndsPlatformInit(void);
extern LOGICAL sndsPlatformIsPlaying(snSound* snd);
extern LOGICAL sndsPlatformKill(void);
extern LOGICAL sndsPlatformLoadSound(snSound* snd);
extern LOGICAL sndsPlatformOverLimit(UINT32 numPlaying);
extern LOGICAL sndsPlatformReloadSound(snSound* oldSound, snSound* newSound);
extern LOGICAL sndsPlatformReset(void);
extern LOGICAL sndsPlatformSetConeAngles(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetConeOrientation(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetCurrentPosition(snSound* snd);
extern LOGICAL sndsPlatformSetFrequency(snSound* snd);
extern LOGICAL sndsPlatformSetListener3DPos(piVECTOR* r, UINT32 unused);
extern LOGICAL sndsPlatformSetListener3DVel(piVECTOR* v, UINT32 unused);
extern LOGICAL sndsPlatformSetMaxDistance(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetMinDistance(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetOrientation(piVECTOR* at, piVECTOR* up, UINT32 unused);
extern LOGICAL sndsPlatformSetPanPosition(snSound* snd);
extern LOGICAL sndsPlatformSetSound3DPos(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetSound3DVel(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetSound3DMode(snSound* snd, UINT32 unused);
extern LOGICAL sndsPlatformSetVolume(snSound* snd);
extern LOGICAL sndsPlatformStartSound(snSound* snd);
extern LOGICAL sndsPlatformStopSound(snSound* snd);
