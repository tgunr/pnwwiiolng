#include <OpenAL/al.h>
#include <OpenAL/alut.h>
#include <OpenAL/alctypes.h>


#include "sn/sndsSoundControlAL.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "include/vars.h"

/************************************************************************************************
	sndsSoundControlAL
*************************************************************************************************/

#define MAX_ALREFBUF 1024
#define MAX_ALABSSRCS 128
#define MAX_ALRELSRCS 32
#define MAX_ALSRCS (MAX_ALABSSRCS + MAX_ALRELSRCS)

// cheesy ref counted alBuf
typedef struct alRefBuf
{
	ALuint 	buf;
	int 	refCount;
} alRefBuf;

// API-specific implementation structure
typedef struct snImpl
{
	int 		srcIdx;
	ALuint		alSrc;
	alRefBuf*	alBuf;
} snImpl;

static alRefBuf refBuf[MAX_ALREFBUF];
static ALuint	alSrc[MAX_ALSRCS];
static LOGICAL	alBufUsed[MAX_ALSRCS];

static ALenum sndsGetError(snSound* snd, const char* funcname);
static void sndsDump(snSound* snd, const char* funcname);

/************************************************************************************************
	sndsGetMaxNumberOfSounds
*************************************************************************************************/

UINT32 sndsGetMaxNumberOfSounds(void)
{
 	return MaxNumberOfSounds;
}

/************************************************************************************************
	sndsPlatformInit
*************************************************************************************************/

LOGICAL sndsPlatformInit(void)
{
	alutInit(0, NULL);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	// EAX extension stuff would go here

	#if platOS == OS_MACOS
	alDopplerFactor(1);				// current OpenAL used on Mac OS X has doppler issues...
	alDopplerVelocity(340.0);		// so let it stay at its defaults. Working with Apple on this issue

	alSetInteger(ALC_SPATIAL_RENDERING_QUALITY, g_bSoundHighQuality ?
												ALC_SPATIAL_RENDERING_QUALITY_HIGH :
												ALC_SPATIAL_RENDERING_QUALITY_LOW);
	alSetInteger(ALC_RENDER_CHANNEL_COUNT, g_bSoundForceStereo ?
										   ALC_RENDER_CHANNEL_COUNT_STEREO :
										   ALC_RENDER_CHANNEL_COUNT_MULTICHANNEL);
	alSetInteger(ALC_MIXER_MAXIMUM_BUSSES, sndsGetMaxNumberOfSounds());

	if (g_bSoundConvertOnLoad)
		alEnable(ALC_CONVERT_DATA_UPON_LOADING);
	else
		alDisable(ALC_CONVERT_DATA_UPON_LOADING);
	#endif

	alGenSources(MAX_ALSRCS, alSrc);

	return sndsGetError(NULL, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformKill
*************************************************************************************************/

LOGICAL sndsPlatformKill(void)
{
	alutExit();

	return TRUE;
}

/************************************************************************************************
	sndsPlatformReset
*************************************************************************************************/

LOGICAL sndsPlatformReset(void)
{
	return TRUE;
}

/************************************************************************************************
	sndsPlatformReloadSound
*************************************************************************************************/

LOGICAL sndsPlatformReloadSound(snSound* oldSound, snSound* newSound)
{
	if(oldSound == NULL || newSound == NULL || oldSound->impl == NULL) return FALSE;

	newSound->impl = (snImpl*)memAlloc(sizeof(snImpl));
	newSound->impl->srcIdx = -1;
	newSound->impl->alBuf = oldSound->impl->alBuf;
	newSound->impl->alBuf->refCount++;

	return TRUE;
}

/************************************************************************************************
	sndsPlatformLoadSound
*************************************************************************************************/

LOGICAL sndsPlatformLoadSound(snSound* snd)
{
	snImpl* impl = NULL;
	alRefBuf* alBuf = NULL;
	int i;

	int fd, wavLen;
	void* wavData = NULL;

	ALenum format;
	ALsizei size, freq;
	ALvoid* alData = NULL;

	 // get a ref counted alBuffer
	for(i = 0; i < MAX_ALREFBUF;i++)
	{
		if(refBuf[i].refCount <= 0)
		{
			alBuf = &refBuf[i];
			break;
		}
	}
	if(alBuf == NULL)  return FALSE;

	fd = fioOpen(snd->name);
	if(fd < 0) return FALSE;

	// read in WAV
	wavLen = fioFileSize(snd->name);
	wavData = memAlloc(wavLen);

	fioRead(fd, wavData, wavLen);
	fioClose(fd);

	// load into alData
	alutLoadWAVMemory(wavData, &format, &alData, &size, &freq);
	// load into alBuffer
	alGenBuffers(1, &alBuf->buf);
	alBufferData(alBuf->buf, format, alData, size, freq);
	alBuf->refCount = 1;

	alutUnloadWAV(format, alData, size, freq);
	memFree(wavData);

	// alloc implementation struct
	impl = (snImpl*)memAlloc(sizeof(snImpl));
	impl->alBuf = alBuf;
	snd->impl = impl;

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformFreeSound
*************************************************************************************************/

LOGICAL sndsPlatformFreeSound(snSound* snd)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	sndsPlatformStopSound(snd);

	snd->impl->alBuf->refCount--;

	if(snd->impl->alBuf->refCount <= 0)
		alDeleteBuffers(1, &snd->impl->alBuf->buf);

	memFree(snd->impl);
	snd->impl = NULL;

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformDuplicateSound
*************************************************************************************************/

LOGICAL	sndsPlatformDuplicateSound(snSound* to, const snSound* from)
{
	return sndsPlatformReloadSound((snSound*)from, to);
}

/************************************************************************************************
	sndsPlatformCreateSound
*************************************************************************************************/

LOGICAL sndsPlatformCreateSound(snSound* lpOld, snSound* lpNew)
{
	return sndsPlatformReloadSound(lpOld, lpNew);
}

/************************************************************************************************
	sndsPlatformStartSound
*************************************************************************************************/

static int _acquireSource(LOGICAL is3D)
{
	int i;
	int start = (is3D) ? MAX_ALRELSRCS : 0;
	int end = (is3D) ? MAX_ALABSSRCS : MAX_ALRELSRCS;

	for(i = start; i < end; i++)
	{
		if(!alBufUsed[i])
		{
			alBufUsed[i] = TRUE;
			return i;
		}
	}

	return -1;
}

static void _releaseSource(int idx)
{
	if(idx > -1 && idx < MAX_ALSRCS)
		alBufUsed[idx] = FALSE;
}

LOGICAL sndsPlatformStartSound(snSound* snd)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	snd->impl->srcIdx = _acquireSource(snd->bIs3D);
	if(snd->impl->srcIdx < 0) return FALSE;

	snd->impl->alSrc = alSrc[snd->impl->srcIdx];

	alSourcei(snd->impl->alSrc, AL_BUFFER,  snd->impl->alBuf->buf);
	alSourcei(snd->impl->alSrc, AL_LOOPING, snd->looping);
	alSourcei(snd->impl->alSrc, AL_SOURCE_RELATIVE, !snd->bIs3D);

	sndsPlatformSetVolume(snd);
	sndsPlatformSetFrequency(snd);

	if(snd->bIs3D)
	{
		sndsPlatformSetSound3DPos(snd, 0);
		sndsPlatformSetSound3DVel(snd, 0);
		sndsPlatformSetMinDistance(snd, 0);
		sndsPlatformSetMaxDistance(snd, 0);
		sndsPlatformSetConeAngles(snd, 0);
		sndsPlatformSetConeOrientation(snd, 0);
	}

	alSourcePlay(snd->impl->alSrc);

	sndsDump(snd, __FUNCTION__);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformStopSound
*************************************************************************************************/

LOGICAL sndsPlatformStopSound(snSound* snd)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;
	if(snd->impl->srcIdx < 0) return FALSE;

	// DEBUG
	//sndsDump(snd, __FUNCTION__);
	if(sndsPlatformIsPlaying(snd))
		alSourceStop(snd->impl->alSrc);

//	if(snd->impl->alSrc)
//		alDeleteSources(1, &snd->impl->alSrc);

	_releaseSource(snd->impl->srcIdx);
	snd->impl->srcIdx = -1;

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetFrequency
*************************************************************************************************/

LOGICAL sndsPlatformSetFrequency(snSound* snd)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
		alSourcef(snd->impl->alSrc, AL_PITCH, (FLOAT32)snd->freqShift);

	Write_Log(LOG_SOUND, LOG_INFO, "Sound: %s freqShift: %lf", snd->name, snd->freqShift);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetVolume
*************************************************************************************************/

LOGICAL sndsPlatformSetVolume(snSound* snd)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
		alSourcef(snd->impl->alSrc, AL_GAIN, (FLOAT32)snd->volume*VolumeMultiplier);

	Write_Log(LOG_SOUND, LOG_INFO, "Sound: %s volume: %lf", snd->name, snd->volume);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetPanPosition
*************************************************************************************************/

LOGICAL sndsPlatformSetPanPosition(snSound* snd)
{
	// No real need to implement, we can just always use 3D sound since AL is a 3-D API.
	// They provide an example of how to do it if we need to in panning2d.c of the OpenAL
	// demos in the source distribution.
	return TRUE;
}

/************************************************************************************************
	sndsPlatformIsPlaying
*************************************************************************************************/

LOGICAL sndsPlatformIsPlaying(snSound* snd)
{
	ALint state = AL_STOPPED;

	if(snd == NULL || snd->impl == NULL || snd->impl->srcIdx < 0) return FALSE;

	if(snd->impl->srcIdx > -1)
		alGetSourcei(snd->impl->alSrc, AL_SOURCE_STATE, &state);

	sndsGetError(snd, __FUNCTION__);

	return (state == AL_PLAYING);
}

/************************************************************************************************
	sndsPlatformOverLimit
*************************************************************************************************/

LOGICAL sndsPlatformOverLimit(UINT32 numPlaying)
{
	return numPlaying > sndsGetMaxNumberOfSounds();
}

/************************************************************************************************
	sndsPlatformDriverConfig
*************************************************************************************************/

LOGICAL sndsPlatformDriverConfig(void* hints)
{
	return TRUE; // N/A
}

/************************************************************************************************
	sndsPlatformSetCurrentPosition
*************************************************************************************************/

LOGICAL sndsPlatformSetCurrentPosition(snSound* snd)
{
	return TRUE; // not doing any byte offset stuff yet
}

/************************************************************************************************
	sndsPlatformSetListener3DPos
*************************************************************************************************/

LOGICAL sndsPlatformSetListener3DPos(piVECTOR* r, UINT32 unused)
{
	alListener3f(AL_POSITION, r->x, r->y, r->z);

	Write_Log(LOG_SOUND, LOG_INFO,">>>> Listener Position: {%lf, %lf, %lf}", r->x, r->y, r->z);

	return sndsGetError(NULL, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetListener3DVel
*************************************************************************************************/

LOGICAL sndsPlatformSetListener3DVel(piVECTOR* v, UINT32 unused)
{
	alListener3f(AL_VELOCITY, v->x, v->y, v->z);

	Write_Log(LOG_SOUND, LOG_INFO,">>>> Listener Velocity: {%lf, %lf, %lf}", v->x, v->y, v->z);

	return sndsGetError(NULL, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSesndsPlatformSetSound3DPos
*************************************************************************************************/

LOGICAL sndsPlatformSetSound3DPos(snSound* snd, UINT32 unused)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
	{
		alSource3f(snd->impl->alSrc, AL_POSITION,
					(FLOAT32)snd->position.x, (FLOAT32)snd->position.y, (FLOAT32)snd->position.z);

		Write_Log(LOG_SOUND, LOG_INFO,"Sound: %s position: {%lf, %lf, %lf}",
					snd->name, snd->position.x, snd->position.y, snd->position.z);
	}

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetSound3DVel
*************************************************************************************************/

LOGICAL sndsPlatformSetSound3DVel(snSound* snd, UINT32 unused)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
	{
		alSource3f(snd->impl->alSrc, AL_VELOCITY,
					(FLOAT32)snd->velocity.x, (FLOAT32)snd->velocity.y, (FLOAT32)snd->velocity.z);
	}
	Write_Log(LOG_SOUND, LOG_INFO,"Sound: %s velocity: {%lf, %lf, %lf}",
					snd->name, snd->velocity.x, snd->velocity.y, snd->velocity.z);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetSound3DMode
*************************************************************************************************/

LOGICAL sndsPlatformSetSound3DMode(snSound* snd, UINT32 unused)
{
	return TRUE; // N/A, maybe if we ever use DEF_MODE_HEADRELATIVE
}

/************************************************************************************************
	sndsPlatformSetMinDistance
*************************************************************************************************/

LOGICAL sndsPlatformSetMinDistance(snSound* snd, UINT32 unused)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
		alSourcef(snd->impl->alSrc, AL_REFERENCE_DISTANCE, snd->fMinDistance);

	Write_Log(LOG_SOUND, LOG_INFO,"Sound: %s fMinDistance: %lf",
				snd->name, snd->fMinDistance);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetMaxDistance
*************************************************************************************************/

LOGICAL sndsPlatformSetMaxDistance(snSound* snd, UINT32 unused)
{
	if(snd == NULL || snd->impl == NULL) return FALSE;

	if(snd->impl->srcIdx > -1)
		alSourcef(snd->impl->alSrc, AL_MAX_DISTANCE, snd->fMaxDistance);

	Write_Log(LOG_SOUND, LOG_INFO,"Sound: %s fMaxDistance: %lf",
				snd->name, snd->fMaxDistance);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetOrientation
*************************************************************************************************/

LOGICAL sndsPlatformSetOrientation(piVECTOR* at, piVECTOR* up, UINT32 unused)
{
	FLOAT32 orient[6] = {at->x, at->y, at->z, up->x, up->y, up->z};

	alListenerfv(AL_ORIENTATION, orient);
 	Write_Log(LOG_SOUND, LOG_INFO,">>>> Listener Orientation at: {%lf, %lf, %lf} up: {%lf, %lf, %lf}", at->x, at->y, at->z, up->x, up->y, up->z);

	return sndsGetError(NULL, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsPlatformSetConeAngles
*************************************************************************************************/

LOGICAL sndsPlatformSetConeAngles(snSound* snd, UINT32 unused)
{
	return FALSE; // we're not doing any of this stuff yet
}

/************************************************************************************************
	sndsPlatformSetConeOrientation
*************************************************************************************************/

LOGICAL sndsPlatformSetConeOrientation(snSound* snd, UINT32 unused)
{
	return FALSE; // we're not doing any of this stuff yet
}

/************************************************************************************************
	sndsPlatformCommitDeferredSettings
*************************************************************************************************/

LOGICAL sndsPlatformCommitDeferredSettings()
{
	return TRUE; // N/A
}

/************************************************************************************************
	sndsPlatformSetListener3DPos
*************************************************************************************************/

LOGICAL sndsPlatformGetVolume(snSound* snd, SINT32* vol)
{
	*vol = 0;
	if(snd == NULL || snd->impl == NULL) return FALSE;
	if(snd->impl->srcIdx > -1)
		alGetSourcei(snd->impl->alSrc, AL_GAIN, vol);

	return sndsGetError(snd, __FUNCTION__) == AL_NO_ERROR;
}

/************************************************************************************************
	sndsGetError
*************************************************************************************************/

static ALenum sndsGetError(snSound* snd, const char* funcname)
{
	ALenum err = alGetError();
	if(err != AL_NO_ERROR)
	{
		Write_Log(LOG_SOUND, LOG_WARN, "%s alError: 0x%x in %s",
					snd ? snd->name : "", err, funcname);
	}

	return err;
}

/************************************************************************************************
	sndsDump
*************************************************************************************************/

static void sndsDump(snSound* snd, const char* funcname)
{
	if(snd == NULL) return;

	if(funcname)
		Write_Log(LOG_SOUND, LOG_INFO, "function: %s\n{", funcname);

	Write_Log(LOG_SOUND, LOG_INFO, "\t snSound\n\t {");
	Write_Log(LOG_SOUND, LOG_INFO,"\t\t name: %s", snd->name);

	Write_Log(LOG_SOUND, LOG_INFO,"\t\t priority: %hd", snd->priority);
	Write_Log(LOG_SOUND, LOG_INFO,"\t\t looping: %d", snd->looping);

	Write_Log(LOG_SOUND, LOG_INFO,"\t\t volume: %lf", snd->volume);
	Write_Log(LOG_SOUND, LOG_INFO,"\t\t freqShift: %lf", snd->freqShift);
	Write_Log(LOG_SOUND, LOG_INFO,"\t\t pan: %hd", snd->pan);

	Write_Log(LOG_SOUND, LOG_INFO,"\t\t bIs3D: %d", snd->bIs3D);

	if(snd->use3DVolume)
		Write_Log(LOG_SOUND, LOG_INFO,"\t\t volume3D: %lf", snd->volume3D);
	if(snd->use3DFreqShift)
		Write_Log(LOG_SOUND, LOG_INFO,"\t\t freqShift3D: %lf", snd->freqShift3D);
	if(snd->use3DPan)
		Write_Log(LOG_SOUND, LOG_INFO,"\t\t pan: %hd", snd->pan3D);

	Write_Log(LOG_SOUND, LOG_INFO,"\t\t position: {%lf, %lf, %lf}", snd->position.x,snd->position.y, snd->position.z);
	Write_Log(LOG_SOUND, LOG_INFO,"\t\t velocity: {%lf, %lf, %lf}", snd->velocity.x,snd->velocity.y, snd->velocity.z);

	Write_Log(LOG_SOUND, LOG_INFO,"\t\t min/max distance: %lf %lf", snd->fMinDistance,snd->fMaxDistance);

	if(snd->impl && snd->impl->alBuf && snd->impl->srcIdx > -1)
		Write_Log(LOG_SOUND, LOG_INFO,"\t\t alSrc: %d alBuf: %d refCount: %d", snd->impl->alSrc, snd->impl->alBuf->buf, snd->impl->alBuf->refCount);

	Write_Log(LOG_SOUND, LOG_INFO, "\t }");

	if(funcname)
		Write_Log(LOG_SOUND, LOG_INFO, "}");
}