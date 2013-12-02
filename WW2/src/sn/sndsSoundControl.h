#pragma once
//	Platform-independent sound control functions
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	Originating Programmer: Russell J. Mirabelli

// Special debug define to signal special data collection
#define SOUND_DEBUG
#undef SOUND_DEBUG

#ifdef SOUND_DEBUG
	#define SND_CHECK_FILE_LENGTH	32
#endif

#define DEF_IMMEDIATE   0x00000000
#define DEF_DEFERRED    0x00000001

#define DEF_MODE_NORMAL         0x00000000
#define DEF_MODE_HEADRELATIVE   0x00000001
#define DEF_MODE_DISABLE        0x00000002

typedef struct snSound* snSoundPtr;
typedef struct SNDSCREATE* LPSNDSCREATE;

// Return maximum number of sounds allowed by the sound card
extern UINT32 sndsGetMaxNumberOfSounds();

extern LOGICAL sndsInit(void);
// calls any platform-specific system initialization for sounds, allocates
// structures and global variables. This should only be called once during
// a program's execution
// return values:   1 if no errors occurred during startup
//					0 if startup failed for any reason

extern LOGICAL sndsKill(void);
// calls any platform-specific shutdown for sounds, deallocates structures and
// resets any global variables. This should only be called after snInit has been
// called. Any sound calls made after snKill will automatically fail.
// return values:   1 if no errors occurred during shutdown
//					0 if shutdown failed for any reason

extern LOGICAL sndsDriverConfig( void* obj );
// handles any (temporary) UI config of sound driver issues

extern LOGICAL sndsReset(void);
// resets entire sound system to initialization state. All buffers are cleared and
// freed, all currently playing sounds are stopped.
// return values:	1 if no errors occurred during reset
//					0 if reset failed for any reason

extern void sndsUpdateSounds(void);

// this loops through the linked list of sounds, updating 3D effects such as volume3D/pan3D/freqShift3D
// according to the listener pos/vel and each sound pos/vel
//
// it also handles starting delayed sounds if the time to play them has elapsed
//
// it is also a convienent place to put possible resource optomisations such as deleting sounds which have
// finished playing (and are not looping), or doing performance monitoring, etc.

//extern LOGICAL sndsCanCreateNewSound(void);

// this enforces whatever rules we have regarding max number of sounds playing at one time

#ifdef SOUND_DEBUG
	#define sndsLoadSound(a)   sndsLoadSoundDebug(a, __FILE__, __LINE__)
	extern snSound * sndsLoadSoundDebug(LPCREATE lpCreate, SCHAR* file, SINT32 line);
#else
	extern snSound * sndsLoadSound(LPSNDSCREATE lpCreate);
#endif

// creates a new snSound item and loads the sound buffer with the waveform
// found in the file soundName. Waveforms must be in WAV format, but may have
// any standard WAV options beyond that one provision.
// Default values as defined in snApplyDefaultOptions are applied.
//
// returns a pointer to the loaded snSound, or NULL if unsuccessful.

extern LOGICAL sndsFreeSound(snSound* whichSound);
// frees the snSoundBuffer associated with whichSound.
// return values:	1 if no errors occurred during freeing the sound
//					0 if the sound could not be freed

extern LOGICAL sndsStartSound(snSound* whichSound);
// starts playing the sound identified through the snSound pointer whichSound
// with all current options. This does not create a duplicate sound buffer, so
// if the sound might be used again before the current playback is complete,
// duplication is recommended.
// return values:	1 if no errors occurred during starting the sound playback
//					0 if the sound could not be started

extern LOGICAL sndsPlayAfter(snSound* playSound, snSound* afterSound);
// starts playing the sound playSound once afterSound is completed.
// return values:	1 if no errors occurred during starting the sound playback
//					0 if the sound could not be started or the original sound
//						could not be found.

extern LOGICAL sndsStartSoundDelayed(snSound* whichSound, long afterTime);
// begins playing the sound whichSound after a delay of afterTime microseconds
// has passed.
// return values:	1 if no errors occurred during starting the sound playback
//					0 if the sound could not be started

extern LOGICAL sndsSetLooping(snSound* whichSound, bool looping);
// sets the looping attribute of the sound designated by whichSound to the
// value passed in through looping, where 0 is no looping and 1 is looping.
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSetLoopPoints(snSound* whichSound, float loopStart, float loopEnd);
// for sounds which loop in positions other than the beginning and end, those points
// may be set through the loopStart and loopEnd values, which are values in seconds
// from the beginning of the waveform.
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSetDelayedStart( snSound* whichSound, LOGICAL toBeDelayed );
//	sets whether a sound is to be delayed.  if TRUE, the delayTime must also be set

extern LOGICAL sndsSetDelayTime( snSound* whichSound, UINT32 delayTime );
//	sets the time for a sound to be played

extern LOGICAL sndsSetFrequency(snSound* whichSound, FLOAT64 frequencyFactor);
// changes the play frequency to a percentage indicated through frequencyFactor
// off the base sound frequency.
// Note that other factors (such as Doppler shift) can impact the frequency shift
// which is ultimately applied to the sound.  Therefore using this function will
// effect an immeadiate frequency change always, but the actual frequency level may
// be different if doppler shifting is active
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSetVolume(snSound* whichSound, FLOAT64 newVolume);
// changes the base playback volume to the value passed in through newVolume. A
// value of 1.0 is considered to be the base volume from the sound file, and a
// value of 0 is silence. Volumes over 1.0 will overdrive the sound and may have
// unpredictable effects.
// Note that other factors (such as distance attenuation) can impact the volume
// which is ultimately applied to the sound.  Therefore using this function will
// effect an immeadiate volume change always, but the actual volume may
// be different if distance attenuation is active
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSetPanPosition(snSound* whichSound, short panPosition);
// changes the pan balance of the sound to the value passed in through panPosition.
// a 50 value indicates center, values to 0 indicate leftward emphasis,
// and values to 100 indicate rightward emphasis. Stereo sounds will NOT
// be remixed with this command.
// Note that other factors (such as 3D panning) can impact the pan
// which is ultimately applied to the sound.  Therefore using this function will
// effect an immeadiate pan change always, but the actual pan may
// be different if 3D pan is active
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSetSoundPriority(snSound* whichSound, short priority);
// sets a priority level for the sound indicated in whichSound. Priority
// values range from 0 (lowest) to 3 (highest). Priority values will be used
// in determining whether or not a sound can be played when channel availability
// is low.
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsStopSound(snSound* whichSound);
// stops the sound indicated by whichSound from playing. If the sound has loop
// points set, then those loop points will be finished before the sound is
// stopped.
// return values:	1 is the sound was stopped
//					0 if the sound could not be found or was not stopped

extern LOGICAL sndsIsPlaying( snSound* whichSound );
//	returns TRUE if a sound is currently playing

extern LOGICAL sndsOverLimit( UINT32 numSounds );
//	returns TRUE if numSounds is "too many" for the given hardware

extern snSound * sndsDuplicateSound(snSound* whichSound);
// duplicates the designated sound and all associated options currently set
// for that sound.
// returns a pointer to the new snSound, or NULL if not successful

extern LOGICAL sndsApplyDefaultOptions(snSound* whichSound);
// sets all values for the sound indicated in whichSound to the default options
// from load time, that is:
//		looping			false
//		delayedStart	false
//		delayTime		0
//		name			soundName
//		volume			1.0
//		pan				50
//		freqShift		1.0
//		volume3D		1.0
//		pan3D			50
//		freqShift3D		1.0
//		loopStartPoint	0
//		loopEndPoint	0
//		use3DVolume		FALSE
//		use3DPan		FALSE
//		use3DFreqShift	FALSE
//		position		{ 0,0,0 }
//		velocity		{ 0,0,0 }
// return values:	1 if value was set
//					0 if value was not set

extern LOGICAL sndsSet3DPosition( snSound* whichSound, const struct FLOAT64VECTOR* pos );
// sets a 3D position for the sound indicated in whichSound.
// return values:	1 if value was set
//					0 if value was not set


extern LOGICAL sndsSet3DVelocity( snSound* whichSound, const struct FLOAT64VECTOR* vel );
// sets a 3D velocity for the sound indicated in whichSound.
// return values:	1 if value was set
//					0 if value was not set



extern LOGICAL  sndsSet3DDistanceAttenuation( snSound* whichSound, LOGICAL volume3Dactive );
// enables 3D distance attenuation for the sound indicated in whichSound

extern LOGICAL  sndsSet3DPan( snSound* whichSound, LOGICAL pan3Dactive );
// enables 3D panning for the sound indicated in whichSound


extern LOGICAL  sndsSet3DDoppler( snSound* whichSound, LOGICAL dopplerActive );
// enables 3D doppler effects for the sound indicated in whichSound

extern LOGICAL sndsSetCurrentPosition(snSound* whichSound, UINT32 uPosition);
// Sets the current sound position in bytes from the beginning of the buffer.

LOGICAL sndsApply3D(snSound* snd);

extern LOGICAL sndsCommitDeferredSettings();
extern LOGICAL sndsSetListener3DPos(struct piVECTOR* pPos, UINT32 uApply);
extern LOGICAL sndsSetListener3DVel(struct piVECTOR* pVel, UINT32 uApply);
extern LOGICAL sndsSetMaxDistance(snSoundPtr lpSound, FLOAT32 fMax, UINT32 uApply);
extern LOGICAL sndsSetMinDistance(snSoundPtr lpSound, FLOAT32 fMin, UINT32 uApply);
extern LOGICAL sndsSetOrientation(struct piVECTOR* pFront, struct piVECTOR* pTop, UINT32 uApply);
extern LOGICAL sndsSetSound3DMode(snSoundPtr lpSound, UINT32 uMode, UINT32 uApply);
extern LOGICAL sndsSetSound3DPos(snSoundPtr lpSound, struct piVECTOR* pPos, UINT32 uApply);
extern LOGICAL sndsSetSound3DVel(snSoundPtr lpSound, struct piVECTOR* pVel, UINT32 uApply);
extern LOGICAL sndsSetConeAngles(snSoundPtr lpSound,
                                 UINT32 uIn,
                                 UINT32 uOut,
                                 UINT32 uApply);
extern LOGICAL sndsSetConeOrientation(snSoundPtr lpSound, struct piVECTOR* pVec, UINT32 uApply);
extern LOGICAL sndsGetVolume(snSoundPtr lpSound, SINT32* pVolume);

// functions to be defined later:
// -  pitch bending
// -  echo & reverb
// -  filtering
// -  double buffered sounds
// -  mixing sounds

//
// definitions for platform specific functions - description found in the non-platform version unless specified otherwise

// this is for debugging purposes-walks sound list and Write_Debug's the contents
extern void sndsWalkSoundList(void);
