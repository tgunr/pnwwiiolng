#pragma once

#include "sn/sndsSoundControl.h"

/////////////////////////////
// UINT32 return functions
/////////////////////////////
UINT32 sndsGetMaxNumberOfSounds();

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
LOGICAL sndsPlatformCommitDeferredSettings();
LOGICAL sndsPlatformCreateSound(snSound* lpOld, snSound* lpNew);
LOGICAL sndsPlatformDriverConfig(void* hints);
LOGICAL	sndsPlatformDuplicateSound(snSound* to, const snSound* from);
LOGICAL sndsPlatformFreeSound(snSound* snd);
LOGICAL sndsPlatformGetVolume(snSound* snd, SINT32* vol);
LOGICAL sndsPlatformInit();
LOGICAL sndsPlatformIsPlaying(snSound* snd);
void sndsPlatformKill();
LOGICAL sndsPlatformLoadSound(snSound* snd);
LOGICAL sndsPlatformOverLimit(UINT32 numPlaying);
LOGICAL sndsPlatformReloadSound(snSound* oldSound, snSound* newSound);
LOGICAL sndsPlatformReset();
LOGICAL sndsPlatformSetConeAngles(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetConeOrientation(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetCurrentPosition(snSound* snd);
LOGICAL sndsPlatformSetFrequency(snSound* snd);
LOGICAL sndsPlatformSetListener3DPos(piVECTOR* r, UINT32 unused);
LOGICAL sndsPlatformSetListener3DVel(piVECTOR* v, UINT32 unused);
LOGICAL sndsPlatformSetMaxDistance(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetMinDistance(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetOrientation(piVECTOR* at, piVECTOR* up, UINT32 unused);
LOGICAL sndsPlatformSetPanPosition(snSound* snd);
LOGICAL sndsPlatformSetSound3DPos(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetSound3DVel(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetSound3DMode(snSound* snd, UINT32 unused);
LOGICAL sndsPlatformSetVolume(snSound* snd);
LOGICAL sndsPlatformStartSound(snSound* snd);
LOGICAL sndsPlatformStopSound(snSound* snd);
