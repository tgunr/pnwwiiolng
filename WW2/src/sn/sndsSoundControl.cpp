#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include "sn/sndsSoundCache.h"
#include "sn/sndsSoundControl.h"
#include "sn/sndsSoundTypes.h"
#include "sn/sndsSoundControlAL.h"
#include "mem/mem.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "ml/mlAtmo.h"
#include "veh/vehtype.h"
#include "pers/persmain.h"

#include <string.h>

/*
	Platform-independent sound control functions
	Copyright 1999 Playnet Inc, All rights reserved

	Originating Programmer: Russell J. Mirabelli
	Supporting  Programmer: Andrew Lewis
*/


// ------------------------------------------------------------
// local declarations and variables
// ------------------------------------------------------------
static snSound _activeSoundsHead; // head of the linked list
static snSound _activeSoundsTail; // head of the linked list

#if !defined(XAUDIO)
static void _sndsCalcDoppler(snSound* snd);
static LOGICAL _sndsCalcDistanceFade(snSound* snd);
static void _sndsCalcPan3D(snSound* snd);
static void _sndsCalc3DDelay(snSound* snd);
static void _sndsHowManyPlaying(UINT32* numPlaying, snSound** soundToPreempt);
static void _sndsGetListenerPos(FLOAT64VECTOR* headPos, FLOAT64VECTOR* headLook, FLOAT64VECTOR* headUp, FLOAT64VECTOR* headRight, FLOAT64VECTOR* headVel);
#endif
static void _sndsFreeRemainingSounds();
static void _sndsRemoveSoundFromList(snSound* removeSound);


#include <deque>
static std::deque<snSound*> _freesounds;

// ------------------------------------------------------------
// externally visible functions
// ------------------------------------------------------------
LOGICAL sndsInit(void)
{
#if platOS == OS_MACOS
    b3DSoundEnable = TRUE;
#endif

	if (bSoundEnable == FALSE)
	{
		Write_Log(LOG_SOUND, LOG_INFO, "Sounds have been disabled in the config file");
		return TRUE;
	}

    _activeSoundsHead.next = &_activeSoundsTail;
    _activeSoundsTail.prev = &_activeSoundsHead;

	if (!sndsInitCache())
	{
		return(FALSE);
	}

#if XAUDIO
    Write_Debug("Running XAUDIO");
#else
    Write_Debug("Running DirectSound");
#endif

	LOGICAL rVal = sndsPlatformInit();
	return rVal;
}

// calls any platform-specific system initialization for sounds, allocates
// structures and global variables. This should only be called once during
// a program's execution
// return values:   0 if no errors occurred during startup
//					1 if startup failed for any reason

LOGICAL sndsKill(void)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

    while(!_freesounds.empty())
    {
        snSound* sound = _freesounds.front();
        _freesounds.pop_front();

        memFree(sound);
    }

	sndsKillCache();
	sndsReset();

	LOGICAL rval = sndsPlatformKill();

    _sndsFreeRemainingSounds();

    Write_Log(LOG_OLD, LOG_INFO, "[sndsKill]");

    return rval;
}

// does any driver configuration (such as on windows, enumerating sound drivers and picking
// one in a dialog box)
LOGICAL sndsDriverConfig(void* obj)
{
//	if (bSoundEnable == FALSE)
//	{
//		return TRUE;
//	}
	return sndsPlatformDriverConfig(obj);
}


// calls any platform-specific shutdown for sounds, deallocates structures and
// resets any global variables. This should only be called after snInit has been
// called. Any sound calls made after snKill will automatically fail.
// return values:   0 if no errors occurred during shutdown
//					1 if shutdown failed for any reason

LOGICAL sndsReset(void)
{
LOGICAL		retval;
snSound*		eachSound;

	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

	eachSound = _activeSoundsHead.next;

#ifdef SOUND_DEBUG
	if (eachSound != NULL)
	{
		Write_Log(LOG_SOUND, LOG_INFO, "Sounds still allocated...");
	}
#endif

	while(eachSound != NULL && eachSound != &_activeSoundsTail)
	{
#ifdef SOUND_DEBUG
		Write_Log(LOG_SOUND, LOG_INFO, "Sound: %s' - File: %s, Line: %d",
			eachSound->name,
			eachSound->file,
			eachSound->line);
#endif
		if(sndsIsPlaying(eachSound))
			sndsStopSound(eachSound);

		eachSound = eachSound->next;
	}

	retval = sndsPlatformReset();

	return retval;
}

// resets entire sound system to initialization state. All buffers are cleared and
// freed, all currently playing sounds are stopped.
// return values:	0 if no errors occurred during reset
//					1 if reset failed for any reason

void sndsUpdateSounds(void)
{
snSound*	listSound;

	if (bSoundEnable == FALSE)
	{
		return;
	}
	listSound = _activeSoundsHead.next;

	while(listSound != &_activeSoundsTail)
	{
		if(sndsIsPlaying(listSound))
		{
			sndsApply3D(listSound);
		}

		listSound = listSound->next;
	}
}

static void _sndsFreeRemainingSounds()
{
#if(0)
    snSound* s = _BaseSound.next;
    snSound* free = NULL;

    while(s != NULL)
    {
        free = s;
        s = s->next;

        _sndsRemoveSoundFromList(free);
        memFree(free);
    }
#endif
}

// adds the sound to the end of the list
static void sndsAddSoundToList(snSound* newSound)
{
	if (bSoundEnable == FALSE)
	{
		return;
	}

    newSound->prev = &_activeSoundsHead;
    newSound->next = _activeSoundsHead.next;

    _activeSoundsHead.next->prev = newSound;
    _activeSoundsHead.next = newSound;
}

// walks the list looking for the sound, yanks it from list (but does not delete it)
static void _sndsRemoveSoundFromList(snSound* removeSound)
{
	if(bSoundEnable == FALSE)
	{
		return;
	}

    if(!removeSound->next && !removeSound->prev)
    {
        // Sound already removed
        return;
    }

    snSound* next = removeSound->next;
    snSound* prev = removeSound->prev;

    prev->next = next;
    next->prev = prev;

    removeSound->next = NULL;
    removeSound->prev = NULL;
}

//		this is a debugging routine useful for examining the contents of the soundlist at any given time
void sndsWalkSoundList()
{
snSound*	listSound;

	if (bSoundEnable == FALSE)
	{
		return;
	}
	Write_Log(LOG_SOUND, LOG_INFO, "BaseSoundPtr = %x", _activeSoundsHead.next);
	listSound = _activeSoundsHead.next;
	while(listSound != &_activeSoundsTail)
	{
		Write_Log(LOG_SOUND, LOG_INFO, "Sound %s, addr = %x, next = %x, prev = %x", listSound->name, listSound, listSound->next, listSound->prev);
		listSound = listSound->next;
	}
	Write_Log(LOG_SOUND, LOG_INFO, "");
}

// this loops through the linked list of sounds, updating 3D effects such as volume3D/pan3D/freqShift3D
// according to the listener pos/vel and each sound pos/vel
//
// it also handles starting delayed sounds if the time to play them has elapsed
//
// it is also a convienent place to put possible resource optomisations such as deleting sounds which have
// finished playing (and are not looping), or doing performance monitoring, etc.
#ifdef SOUND_DEBUG
	snSound * sndsLoadSoundDebug(LPSNDSCREATE lpCreate, SCHAR* file, SINT32 line)
#else
	snSound * sndsLoadSound(LPSNDSCREATE lpCreate)
#endif
{
snSound* newSound = NULL;
LOGICAL	success;

	if (bSoundEnable == FALSE)
	{
		return NULL;
	}

    if(lpCreate == NULL)
        return NULL;

    if(lpCreate->pszName == NULL)
        return NULL;

    if(!_freesounds.empty())
    {
        newSound = _freesounds.front();
        _freesounds.pop_front();
    }
    else
    {
        newSound = (snSound*)memAlloc(sizeof(snSound));
    }

    if(newSound != NULL)
    {
        memset(newSound, 0, sizeof(snSound));

        strncpy(newSound->name, lpCreate->pszName, MAX_SND_NAME_LENGTH - 1);

		success = sndsApplyDefaultOptions(newSound);

		if(success)
		{
            newSound->bIs3D = lpCreate->bIs3D;

#ifdef SOUND_DEBUG
			success = sndsLoadSoundCacheDebug(newSound, file, line);
#else
			success = sndsLoadSoundCache(newSound);
#endif
		}

		if(success)
		{
			sndsAddSoundToList(newSound);

#ifdef SOUND_DEBUG

			{
			SINT8*	ptr;

				ptr = strrchr(file, '\\');
				if (ptr != NULL)
				{
					ptr += 1;
				}
				else
				{
					ptr = file;
				}

				strncpy(newSound->file, ptr, SND_CHECK_FILE_LENGTH);
				newSound->line = line;
			}

#endif
		}
		else // something failed along the way, so free the structure
		{
            _freesounds.push_back(newSound);
            return NULL;
		}
	}

	return newSound;
}

LOGICAL sndsFreeSound(snSound* whichSound)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

	_sndsRemoveSoundFromList(whichSound);

	LOGICAL rVal = sndsReleaseSoundInCache(whichSound);

	if(rVal)
	{
        _freesounds.push_back(whichSound);
	}

	return rVal;
}


LOGICAL sndsStartSound(snSound* whichSound)
{
UINT32		numSoundsPlaying = 0;
snSound*	soundToDie = NULL;

	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if !XAUDIO
	_sndsHowManyPlaying(&numSoundsPlaying, &soundToDie);
#endif
	if(sndsOverLimit(numSoundsPlaying + 1) == TRUE)
	{
		sndsStopSound(soundToDie);
	}

	// give 3D effects an initial set. Otherwise they are applied in sndsUpdateSounds
#if(0)
    // Applied in fxSnd
	sndsApply3D(whichSound);
#endif

	return sndsPlatformStartSound(whichSound);
}

LOGICAL sndsIsPlaying(snSound* whichSound)
{
	if (bSoundEnable == FALSE)
	{
		return FALSE;
	}
	return sndsPlatformIsPlaying(whichSound);
}

LOGICAL sndsOverLimit(UINT32 numSounds)
{
	if (bSoundEnable == FALSE)
	{
		return FALSE;
	}
	return sndsPlatformOverLimit(numSounds);
}

// starts playing the sound identified through the snSound pointer whichSound
// with all current options. This does not create a duplicate sound buffer, so
// if the sound might be used again before the current playback is complete,
// duplication is recommended.
// return values:	0 if no errors occurred during starting the sound playback
//					1 if the sound could not be started

LOGICAL sndsPlayAfter(snSound* playSound, snSound* afterSound)
{
	return FALSE;
}

// starts playing the sound playSound once afterSound is completed.
// return values:	0 if no errors occurred during starting the sound playback
//					1 if the sound could not be started or the original sound
//						could not be found.

LOGICAL sndsStartSoundDelayed(snSound* whichSound, long afterTime)
{
	return FALSE;
}

// begins playing the sound whichSound after a delay of afterTime microseconds
// has passed.
// return values:	0 if no errors occurred during starting the sound playback
//					1 if the sound could not be started

LOGICAL sndsStopSound(snSound* whichSound)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

	return sndsPlatformStopSound(whichSound);
}

// stops the sound indicated by whichSound from playing. If the sound has loop
// points set, then those loop points will be finished before the sound is
// stopped.
// return values:	0 is the sound was stopped
//					1 if the sound could not be found or was not stopped

LOGICAL sndsSetLooping(snSound* whichSound, bool looping)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		whichSound->looping = looping;
		return TRUE;
	}
	return FALSE;
}

LOGICAL sndsSetLoopPoints(snSound* whichSound, float loopStart, float loopEnd)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if((whichSound != NULL) && (loopEnd >= loopStart))
	{
		whichSound->loopStartPt = loopStart;
		whichSound->loopEndPt = loopEnd;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL sndsSetDelayedStart(snSound* whichSound, LOGICAL toBeDelayed)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if(whichSound != NULL)
	{
		whichSound->delayedStart = toBeDelayed;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL sndsSetDelayTime(snSound* whichSound, UINT32 delayTime)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if(whichSound != NULL)
	{
		whichSound->delayTime = delayTime;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL sndsSetFrequency(snSound* whichSound, FLOAT64 frequencyFactor)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		whichSound->freqShift = frequencyFactor;
		LOGICAL rVal = sndsPlatformSetFrequency(whichSound);
		return rVal;
	}
	return FALSE;
}

LOGICAL sndsSetVolume(snSound* whichSound, FLOAT64 newVolume)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		whichSound->volume = newVolume;
		LOGICAL rVal = sndsPlatformSetVolume(whichSound);
		return rVal;
	}
	return FALSE;
}

LOGICAL sndsSetPanPosition(snSound* whichSound, short panPosition)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if((whichSound != NULL) && (panPosition >= 0) && (panPosition <= 100))
	{
		whichSound->pan = panPosition;
		LOGICAL rVal = sndsPlatformSetPanPosition(whichSound);
		return rVal;
	}
	return FALSE;
#endif
}

LOGICAL sndsSetSoundPriority(snSound* whichSound, short priority)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if((whichSound != NULL) && (priority >= 0) && (priority <= 3))
	{
		whichSound->priority = priority;
		return TRUE;
	}
	return FALSE;
#endif
}

snSound * sndsDuplicateSound(snSound* whichSound)
{
snSound*	newSound = NULL;
LOGICAL	success;

	if (bSoundEnable == FALSE)
	{
		return NULL;
	}
	if(whichSound != NULL)
	{
		newSound = (snSound*)memAlloc(sizeof(snSound));
		if(newSound != NULL)
		{
			memcpy(newSound, whichSound, sizeof(snSound));
			success = sndsPlatformDuplicateSound(newSound, whichSound);
			// if everything got created OK, insert new sound into linked list
			if(success)
			{
				newSound->next = whichSound->next;
				newSound->prev = whichSound;
				whichSound->next = newSound;
				if(newSound->next != NULL)
					newSound->next->prev = newSound;
			} // end linked list insertion
			else
			{
                _freesounds.push_back(newSound);
			}
		} // end if(newSound != NULL)
	} // end if(whichSound != NULL)
	return newSound;
}

LOGICAL sndsApplyDefaultOptions(snSound* whichSound)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		FLOAT64VECTOR nullVec = { 0,0,0 };

		whichSound->priority = 0;	// lowest priority
		whichSound->pan = 50;
		whichSound->pan3D = 50;
		whichSound->loopStartPt = 0;
		whichSound->loopEndPt = 0;
		whichSound->delayTime = 0;
		whichSound->delay_time = 0;
        whichSound->uCurrentPosition = 0;
		whichSound->volume = 1.0;
		whichSound->freqShift = 1.0;
		whichSound->volume3D = 1.0;
		whichSound->freqShift3D = 1.0;
		whichSound->looping = false;
		whichSound->delayedStart = false;
		whichSound->use3DVolume = false;
		whichSound->use3DPan = false;
		whichSound->use3DFreqShift = false;
		whichSound->use3DDelay = false;
		whichSound->position = nullVec;
		whichSound->velocity = nullVec;
		whichSound->delay_position = nullVec;
        whichSound->handle = 0;

		return TRUE;
	}
	return FALSE;
}

LOGICAL sndsSet3DPosition(snSound* whichSound, const FLOAT64VECTOR* pos)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		whichSound->position = *pos;
		return TRUE;
	}
	return FALSE;
}

LOGICAL sndsSet3DVelocity(snSound* whichSound, const FLOAT64VECTOR* vel)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
	if(whichSound != NULL)
	{
		whichSound->velocity = *vel;
		return TRUE;
	}
	return FALSE;
}

LOGICAL  sndsSet3DDistanceAttenuation(snSound* whichSound, LOGICAL volume3Dactive)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if(whichSound != NULL)
	{
		whichSound->use3DVolume = volume3Dactive;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL  sndsSet3DPan(snSound* whichSound, LOGICAL pan3Dactive)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if(whichSound != NULL)
	{
		whichSound->use3DPan = pan3Dactive;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL  sndsSet3DDoppler(snSound* whichSound, LOGICAL dopplerActive)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}
#if XAUDIO
    return TRUE;
#else
	if(whichSound != NULL)
	{
		whichSound->use3DFreqShift = dopplerActive;
		return TRUE;
	}
	return FALSE;
#endif
}

LOGICAL sndsSetCurrentPosition(snSound* pWhichSound, UINT32 uPosition)
{
    if(bSoundEnable == FALSE)
        return TRUE;
#if XAUDIO
    return TRUE;
#else

    if(pWhichSound != NULL)
    {
        pWhichSound->uCurrentPosition = uPosition;
        LOGICAL rVal = sndsPlatformSetCurrentPosition(pWhichSound);
		return rVal;
	}
	return FALSE;
#endif
}

// ------------------------------------------------------------
// statically visible functions
// ------------------------------------------------------------
void _sndsCalcDoppler(snSound* snd)
{
	if (bSoundEnable == FALSE)
	{
		return;
	}
#if XAUDIO
#else
	if(snd != NULL && snd->use3DFreqShift == true)
	{
	FLOAT64VECTOR listenerPos, listenerVel, listenerLook, listenerUp, listenerRight, distSourceListen;
	FLOAT64 velSource, velListen;
	FLOAT64 velSpeedSound;

		_sndsGetListenerPos(&listenerPos, &listenerLook, &listenerUp, &listenerRight, &listenerVel);

        mathSubVectors(&distSourceListen, &snd->position, &listenerPos);
        distSourceListen.z = -distSourceListen.z;

        mathNormalize(&distSourceListen);

        // calculate the velocity (scalar value) component of the sound source and listener along the vector
		// lying between them by doing a dot product of the vel vector and a unit vector between
		velSource = mathVectDotProduct(&(snd->velocity), &distSourceListen);

		velListen = mathVectDotProduct(&listenerVel, &distSourceListen);

        mlGetAltSpeedOfSound(&velSpeedSound, snd->position.z);

        snd->freqShift3D = (velSpeedSound + velListen) / (velSpeedSound + velSource);
 	}
#endif
}

LOGICAL _sndsCalcDistanceFade(snSound* snd)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

#if XAUDIO
#else
	if(snd != NULL && snd->use3DVolume == true)
	{
#if(0)
        FLOAT32 minDist2 = snd->fMinDistance * snd->fMinDistance;	// meters - wild guess for now to be applied to all sounds
        FLOAT32 maxDist2 = snd->fMaxDistance * snd->fMaxDistance;
        FLOAT32 dist2 = 0.0f;

        FLOAT64VECTOR listenerPos, listenerVel, listenerLook, listenerUp, listenerRight, dsl;

		_sndsGetListenerPos(&listenerPos, &listenerLook, &listenerUp, &listenerRight, &listenerVel);
		mathSubVectors(&dsl, &(snd->position), &listenerPos);

        dist2 = (FLOAT32)mathVectDotProduct(&dsl, &dsl);

        snd->volume3D = 0;

        if(dist2 < maxDist2)
        {
		    snd->volume3D = minDist2 / dist2;

            if(snd->volume3D > 1.0f)
            {
                snd->volume3D = 1.0f;
            }
        }
#else
        float min2 = snd->fMinDistance * snd->fMinDistance;
        float max2 = snd->fMaxDistance * snd->fMaxDistance;

        FLOAT64VECTOR listner, vel, look, up, right, v;

		_sndsGetListenerPos(&listner, &look, &up, &right, &vel);

        mathSubVectors(&v, &snd->position, &listner);

        double distance = mathExactVectorLength(&v);

        if(distance > snd->fMaxDistance)
            snd->volume3D = 0.0f;
        else if(distance <= snd->fMinDistance)
            snd->volume3D = 1.0f;
        else
        {
            float min2 = snd->fMinDistance * snd->fMinDistance;
            float max2 = snd->fMaxDistance * snd->fMaxDistance;

            FLOAT64VECTOR listner, vel, look, up, right, v;

		    _sndsGetListenerPos(&listner, &look, &up, &right, &vel);

            mathSubVectors(&v, &snd->position, &listner);

            double distance = mathExactVectorLength(&v);

            // Calculate attenuation (volume is halved each min distance)
            float numhalves = 0;

            // Get distance from min distance.
            distance -= snd->fMinDistance;

            if(snd->fMinDistance > 0.0f)
            {
                numhalves = (float)distance / snd->fMinDistance;
            }

            snd->volume3D = 1.0f / pow(2.0f, numhalves);
        }
#endif
	}
#endif // XAUDIO
    return TRUE;
}

void _sndsCalcPan3D(snSound* snd)
{
	if (bSoundEnable == FALSE)
	{
		return;
	}
#if XAUDIO
#else
	if(snd != NULL && snd->use3DPan == true)
	{
        FLOAT64VECTOR listenerPos, listenerVel, listenerLook, listenerUp, listenerRight, distSourceListen;
        FLOAT64 costheta;

		_sndsGetListenerPos(&listenerPos, &listenerLook, &listenerUp, &listenerRight, &listenerVel);
		mathSubVectors(&distSourceListen, &(snd->position), &listenerPos);

		mathNormalize(&distSourceListen);
		mathNormalize(&listenerRight);

		// this will be the cosine of the angle between the right ear and the sound, between 1.0 and -1.0
		costheta = mathVectDotProduct(&distSourceListen, &listenerRight);

		// to have angular change map linearly to the pan, one must get the actual theta value
		// this is a first attempt which is less computationally intensive and may suffice - AJL
		snd->pan3D = (short)(50.0 * costheta) + 50;
	}
#endif
}

void _sndsCalc3DDelay(snSound* snd)
{
	if (bSoundEnable == FALSE)
	{
		return;
	}

#if XAUDIO
#else
UINT32			curTime = 0;	// this must be set somehow
UINT32			deltaTime;
FLOAT64			velSpeedSound;
FLOAT64VECTOR	listenerPos, listenerVel, listenerLook, listenerUp, listenerRight, dsl;
LOGICAL			withinSphere = FALSE;

	if(snd != NULL && snd->use3DDelay == true && sndsIsPlaying(snd) == FALSE)
	{
		deltaTime = curTime - snd->delay_time;
		mlGetAltSpeedOfSound(&velSpeedSound, snd->delay_position.x);
		_sndsGetListenerPos(&listenerPos, &listenerLook, &listenerUp, &listenerRight, &listenerVel);
		mathSubVectors(&dsl, &(snd->delay_position), &listenerPos);

		// play the sound if it's travelled dist (speedsound*time) is greater than the distance between sound origin and listener
		if((velSpeedSound * velSpeedSound * deltaTime * deltaTime) > (dsl.x * dsl.x + dsl.y * dsl.y + dsl.x * dsl.z))
		{
			withinSphere = TRUE;
			sndsStartSound(snd);
			// once the sound has started its delayed play, we shut off it's delayed property
			// because if it were to be left on, we'd just start it up again as soon as it was done playing
			snd->use3DDelay = false;
		}
	}
#endif
}

//	Applies the 3D effects to the sound if they are specified
//
// volume, pan, and freqshift have both an "intrinsic" value and a "3D" effect value
// we calculate the 3D value from positions and velocities of sound and listener
// in the _sndsCalc functions, and then apply the changes in the sndsPlatform functions
// the "intrinsic" values are not changed by this process
LOGICAL sndsApply3D(snSound* snd)
{
	if (bSoundEnable == FALSE)
	{
		return TRUE;
	}

#if XAUDIO
#else
    if(snd == NULL)
        return TRUE;

	if(snd->use3DVolume)
	{
		if(!_sndsCalcDistanceFade(snd))
            return FALSE;

		sndsPlatformSetVolume(snd);
	}

	if(snd->use3DPan)
	{
		_sndsCalcPan3D(snd);
		sndsPlatformSetPanPosition(snd);
	}

	if(snd->use3DFreqShift)
	{
		_sndsCalcDoppler(snd);
		sndsPlatformSetFrequency(snd);
	}

	if(snd->use3DDelay)
	{
		_sndsCalc3DDelay(snd);
	}
#endif
    return TRUE;
}

// returns number of sounds playing.  Also returns pointer to the first and lowest priority playing sound found
// which is the one which will be shut off if need be
void _sndsHowManyPlaying(UINT32* numPlaying, snSound** soundToPreempt)
{
	if (bSoundEnable == FALSE)
	{
		return;
	}

#if XAUDIO
#else
snSound*				listSound;
UINT16					pri = 4;

	*numPlaying = 0;
	listSound = _activeSoundsHead.next;
	while(listSound != &_activeSoundsTail)
	{
		if(sndsPlatformIsPlaying(listSound))
		{
			(*numPlaying)++;
			if(listSound->priority < pri)
			{
				pri = listSound->priority;
				*soundToPreempt = listSound;
			}
		}
		listSound = listSound->next;
	}
#endif
}

void _sndsGetListenerPos(FLOAT64VECTOR* headPos, FLOAT64VECTOR* headLook, FLOAT64VECTOR* headUp, FLOAT64VECTOR* headRight, FLOAT64VECTOR* headVel)
{
    piVECTOR vel;

    if (bSoundEnable == FALSE)
	{
		return;
	}

    headPos->x = -playpos.x;
    headPos->y = -playpos.y;
    headPos->z = -playpos.z;

#if FRES
	piVECTOR up = {0,0,-1};
	piVECTOR front = {0,1,0};
	piVECTOR right = {1,0,0};

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	if(Vehicle && Vehicle->ModelData)
	{
		math3dMultiplyRotation(&front, &front, &Vehicle->ModelData->VehicleAttMat);
		math3dMultiplyRotation(&up, &up, &Vehicle->ModelData->VehicleAttMat);
		math3dMultiplyRotation(&right, &right, &Vehicle->ModelData->VehicleAttMat);

		headUp->x = up.x;
		headUp->y = up.y;
		headUp->z = up.z;

		headLook->x =  front.x;
		headLook->y =  front.y;
		headLook->z = -front.z;

		headRight->x = right.x;
		headRight->y = right.y;
		headRight->z = right.z;
	}

#else
    headUp->x = playup.x;
    headUp->y = playup.y;
    headUp->z = playup.z;

    headLook->x =  playdir.x;
    headLook->y =  playdir.y;
    headLook->z = -playdir.z;

    headRight->x = playright.x;
    headRight->y = playright.y;
    headRight->z = playright.z;
#endif

    persGetPlayerVelocity(&vel);

#ifdef DOROTHY
	if(persIsPersonaVehicleType(VEH_TYPE_PERSON) || persIsPersonaVehicleType(VEH_TYPE_MECH))
#else
    if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
#endif
    {
        // Even our velocities are turned around. This makes direct sound
        // pitch correctly. This was discovered by working on 2D sounds.
        vel.x = -vel.x;
        vel.z = -vel.z;
    }

    headVel->x = vel.x;
    headVel->y = vel.y;
    headVel->z = vel.z;

#if(0)
	// fill in the FLOAT64VECTORs from whereever
	headPos->x = headPos->y = headPos->z = 0;
	headVel->x = headVel->y = headVel->z = 0;
	headRight->x = 1;
	headRight->y = headRight->z = 0;
	headUp->x = headUp->z = 0;
	headUp->y = 1;
	headLook->x = headLook->y = 0;
	headLook->z = -1;
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: sndsSetListener3DPos
// Description: Set the listener (Player) position.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsSetListener3DPos(piVECTOR* pPos, UINT32 uApply)
{
    if(pPos == NULL)
        return FALSE;

    return sndsPlatformSetListener3DPos(pPos, uApply);
}

//////////////////////////////////////////////////////////////////////
// Function: sndsSetListener3DVel
// Description: Set the listener (Player) velocity.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsSetListener3DVel(piVECTOR* pVel, UINT32 uApply)
{
    if(pVel == NULL)
        return FALSE;

    return sndsPlatformSetListener3DVel(pVel, uApply);
}

//////////////////////////////////////////////////////////////////////
// Function: sndsSetSound3DPos
// Description: Set the sound position.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsSetSound3DPos(snSoundPtr lpSound, piVECTOR* pPos, UINT32 uApply)
{
    if(lpSound == NULL || pPos == NULL)
        return FALSE;

    lpSound->position.x = pPos->x;
    lpSound->position.y = pPos->y;
    lpSound->position.z = pPos->z;

    return sndsPlatformSetSound3DPos(lpSound, uApply);
}

//////////////////////////////////////////////////////////////////////
// Function: sndsSetSound3DVel
// Description: Set the sound velocity.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsSetSound3DVel(snSoundPtr lpSound, piVECTOR* pVel, UINT32 uApply)
{
    if(lpSound == NULL || pVel == NULL)
        return FALSE;

    lpSound->velocity.x = pVel->x;
    lpSound->velocity.y = pVel->y;
    lpSound->velocity.z = pVel->z;

    return sndsPlatformSetSound3DVel(lpSound, uApply);
}

//////////////////////////////////////////////////////////////////////
// Function: sndsSetSound3DMode
// Description: Set the sound mode (NORMAL, DISABLE, HEAD_RELATIVE.)
//////////////////////////////////////////////////////////////////////
LOGICAL sndsSetSound3DMode(snSoundPtr lpSound, UINT32 uMode, UINT32 uApply)
{
#if XAUDIO
    return TRUE;
#else
    if(lpSound == NULL)
        return FALSE;

    lpSound->uMode = uMode;

    return sndsPlatformSetSound3DMode(lpSound, uApply);
#endif
}

//////////////////////////////////////////////////////////
// Function: sndsCommitDeferredSettings
// Description: Commit deferred settings for the listener.
//////////////////////////////////////////////////////////
LOGICAL sndsCommitDeferredSettings()
{
    return sndsPlatformCommitDeferredSettings();
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetMinDistance
// Description: Set sound buffer minimum distance.
//////////////////////////////////////////////////////////
LOGICAL sndsSetMinDistance(snSoundPtr sound, FLOAT32 mindist, UINT32 apply)
{
    if(sound == NULL)
        return FALSE;

    sound->fMinDistance = mindist;

    return sndsPlatformSetMinDistance(sound, apply);
}

//////////////////////////////////////////////////////////
// Function: sndsSetMaxDistance
// Description: Set sound buffer maximum distance.
//////////////////////////////////////////////////////////
LOGICAL sndsSetMaxDistance(snSoundPtr sound, FLOAT32 maxdist, UINT32 apply)
{
    if(sound == NULL)
        return FALSE;

    sound->fMaxDistance = maxdist;

    return sndsPlatformSetMaxDistance(sound, apply);
}

//////////////////////////////////////////////////////////
// Function: sndsSetOrientation
// Description: Set listener orientation.
//////////////////////////////////////////////////////////
LOGICAL sndsSetOrientation(piVECTOR* pFront, piVECTOR* pTop, UINT32 uApply)
{
    return sndsPlatformSetOrientation(pFront, pTop, uApply);
}

//////////////////////////////////////////////////////////
// Function: sndsSetConeAngles
// Description: Set inside and outside cone volume angles.
//////////////////////////////////////////////////////////
LOGICAL sndsSetConeAngles(snSoundPtr lpSound, UINT32 uIn, UINT32 uOut, UINT32 uApply)
{
    if(lpSound == NULL)
        return FALSE;

#if XAUDIO
    return TRUE;
#else
    lpSound->uInsideConeAngle = uIn;
    lpSound->uOutsideConeAngle = uOut;

    return sndsPlatformSetConeAngles(lpSound, uApply);
#endif
}

//////////////////////////////////////////////////////////
// Function: sndsSetConeOrientation
// Description: Set the cone orientation.
//////////////////////////////////////////////////////////
LOGICAL sndsSetConeOrientation(snSoundPtr lpSound, piVECTOR* pVec, UINT32 uApply)
{
    if(lpSound == NULL || pVec == NULL)
        return FALSE;

#if XAUDIO
    return TRUE;
#else
    lpSound->vecConeOrientation = *pVec;

    return sndsPlatformSetConeOrientation(lpSound, uApply);
#endif
}

//////////////////////////////////////////////////////////
// Function: sndsGetVolume
// Description: Get volume for specific sound buffer.
//////////////////////////////////////////////////////////
LOGICAL sndsGetVolume(snSoundPtr lpSound, SINT32* pVolume)
{
    if(lpSound == NULL || pVolume == NULL)
        return FALSE;

    return sndsPlatformGetVolume(lpSound, pVolume);
}
