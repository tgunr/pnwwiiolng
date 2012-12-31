#pragma once

extern void uiSoundInit();
extern void uiSoundUpdate();
extern void uiSoundSetVolume(SINT32 ID, FLOAT64 volume);
extern void uiSoundStop(SINT32 ID, bool callback = true);

extern SINT32 uiPlaySound(const SCHAR* file, LOGICAL finite, FLOAT64 volume, const SCHAR* callback = 0);
