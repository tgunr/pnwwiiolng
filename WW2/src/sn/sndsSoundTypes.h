#pragma once

#if platOS == OS_MACOS
#include "sn/SoundBuffer.h"
#endif

#include "pi/pi.h"		// For piVector.

#define MAX_SND_NAME_LENGTH	64

#if XAUDIO
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#endif

struct snSound
{
    snSound* next;
    snSound* prev;

	// "intrinsic" properties
	FLOAT64			volume;				// 0 silence, 1.0 normal volume, > 1.0 overdrive
	FLOAT64			freqShift;			// multiplier of baseFrequency 1.0 no shift, > 1.0 higher, etc.

	FLOAT64			loopStartPt;		// time from beginning of sound sample in seconds
	FLOAT64			loopEndPt;			// time from beginning of sound sample in seconds

	// "3D" effect properties
	FLOAT64			volume3D;			// 0 - no volume, 1.0 same volume, > 1.0 overdrive
	FLOAT64			freqShift3D;		// 1.0 is no shift, > 1.0 higher freq., < 1.0 lower freq

    FLOAT32         fMinDistance;
    FLOAT32         fMaxDistance;

	UINT16			priority;			// how important the sound is relative to others (0 lowest, 3 highest)
	UINT16			pan;				// 0 - full Left, 50 - center, 100 - full Right
	UINT16			pan3D;				// 0 - full Left, 50 - center, 100 - full Right

	// Putting these here because there is a UINT16 missing to make
	// 64-bit alignment so they act as padding.
	bool			looping : 1;
	bool			delayedStart : 1;		// TRUE if the sound is to be started in the future
	bool			use3DVolume : 1;		// TRUE if sound volume is to be changed according to distance from listener
	bool			use3DPan : 1;			// TRUE if sound location relative to listener (pan) is to be effected
	bool			use3DFreqShift : 1;		// TRUE if sound is to be doppler shifted
	bool			use3DDelay : 1;			// TRUE if speed-of-sound and sound-listener distance are to be factored in
	bool			bIs3D : 1;
	bool			hasTrail : 1;          // Has trailing sound (ie, gunshot trail)

	UINT32			frequency;			// original frequency (in Hertz) of sound
    UINT32			delayTime;			// when (in game time units) the sound is to be played
    UINT32			delay_time;			// used only if use3DDelay is active
    UINT32          uCurrentPosition;   // Position in bytes from beginning.
    UINT32          uMode;
    UINT32          uInsideConeAngle;
    UINT32          uOutsideConeAngle;

	FLOAT64VECTOR	position;			// used only if use3DPan or use3DDistFade are active
	FLOAT64VECTOR	velocity;			// used only if use3DDoppler is active

	FLOAT64VECTOR	delay_position;		// used only if use3DDelay is active

    piVECTOR        vecConeOrientation;

    unsigned int handle;

#if platOS == OS_MACOS
	SoundBuffer buffer;
#elif platOS == OS_WIN32
	struct IDirectSoundBuffer*		lpdsb;
#endif

	SCHAR			name[MAX_SND_NAME_LENGTH];

#ifdef SOUND_DEBUG
	SINT32			line;
	SINT8			file[SND_CHECK_FILE_LENGTH+1];
#endif
};

struct SNDSCREATE
{
    SCHAR* pszName;
    LOGICAL bIs3D;
    bool loop;
};

struct SOUNDCACHE
{
    snSound		theSound;
	bool		bUsed : 1;
	SINT32		numuses : 31;

#ifdef SOUND_DEBUG
	SINT32		line;
	SINT8		file[SND_CHECK_FILE_LENGTH+1];
#endif

};

