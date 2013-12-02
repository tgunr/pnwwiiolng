#pragma once

#include "sn/sndsSoundControl.h"
#include "sn/sndsSoundTypes.h"

#define TYPE_FINITE_SOUND               200
#define TYPE_INFINITE_SOUND             201
#define TYPE_FINITE_EXTERNAL_SOUND      202
#define TYPE_INFINITE_EXTERNAL_SOUND    203

// Special behavior flags here
#define DEF_SOUND_STOPS_AMBIENCE    0x00000001
#define SOUND_PLAY_IMMEDIATE        0x00000002
#define SOUND_PLAY_NO_TRAIL         0x00000004 // No trail sound
#define SOUND_NO_OVERALL            0x00000008 // Sound not affected by overall variable
#define SOUND_NO_DOPPLER            0x00000010

enum ENUM_PRIORITY
{
    IDE_PRIORITY_NOT_USED,
    IDE_TP_PRIORITY_LOW,
    IDE_FP_PRIORITY_LOW,
    IDE_TP_PRIORITY_MEDIUM,
    IDE_FP_PRIORITY_MEDIUM,
    IDE_TP_PRIORITY_HIGH,
    IDE_FP_PRIORITY_HIGH,
    IDE_TP_PRIORITY_MANDATORY,
    IDE_FP_PRIORITY_MANDATORY,
    IDE_MAX_NUMBER_PRIORITES

};

#define DEF_PLAYTYPE_WORLD  -101
#define DEF_PLAYTYPE_HEAD   -102
#define DEF_PLAYTYPE_OBJECT -103
#define DEF_PLAYTYPE_AMMO   -104

struct FXSOUND
{
    FXSOUND* next;
    FXSOUND* prev;

    SINT32 createTime;
    SINT32 starttime;
    SINT32 timeToStart;
    SINT32 delayedStopTime;
    SINT32 duration;
    SINT32 type;
    SINT32 playType;
    SINT32 ID;
    SINT32 pan;
    SINT32 channelIndex;
    SINT32 updatetimer;

    LOGICAL onchannel;
    LOGICAL bIsExternal;
    LOGICAL bLoop;
    LOGICAL bIs3D;
    LOGICAL bSoundHasStarted;
    LOGICAL bInUse;

    FLOAT32 originalVolume;
    FLOAT32 originalPitch;
    FLOAT32 pitch;
    FLOAT32 oldPitch;
    FLOAT32 volume;
    FLOAT32 oldVolume;
    FLOAT32 maxDistance;
    FLOAT32 minDistance;
    FLOAT32 maxSqrd;
    FLOAT32 minSqrd;
    FLOAT32 distance2;

    FLOAT64VECTOR vecVel;
    FLOAT64VECTOR vecPos;

    struct OBJECT* o;

    void const* pAmmo;
    void* killData;
    void (*userUpdate)(FXSOUND* s);

    struct snSound* s;

    void (*pFn)(void* data);
    UINT32 Flags;

    SCHAR szName[MAX_SND_NAME_LENGTH];

    ENUM_PRIORITY priority;

};

#if defined(USE_OSG)
void fxSndUpdate();
#else
LOGICAL fxSndUpdate();
#endif

/*------------------------------------------------------------------
                            3D PROTOTYPES
-------------------------------------------------------------------*/
//////////////////////////
// void return functions
//////////////////////////
extern void fxSndChangePitch(SINT32 iID, FLOAT32 fNewPitch);
extern void fxSndChangeVolume(SINT32 iID, FLOAT32 fNewVolume);
extern void fxSndDetachAmmo(void const* pAmmo);
extern void fxSndFreeNode(FXSOUND* lpSnd);
extern void fxSndSetPan(SINT32 iID, SINT32 iPan);
extern void fxSndStopSound(SINT32 iID);
extern void fxSndStopSoundDelayed(SINT32 iID, SINT32 time);

////////////////////////////
// LOGICAL return functions
////////////////////////////
extern LOGICAL fxSndAddToChannel(FXSOUND* lpSnd);
extern LOGICAL fxSndAddToEmptyChannel(FXSOUND* lpSnd);
extern LOGICAL fxSndBumpChannel(FXSOUND* lpSnd);
extern LOGICAL fxSndCalculateVolume(FXSOUND* lpSnd);
extern LOGICAL fxSndCanBeAddedToChannel(FXSOUND* lpSnd);
extern LOGICAL fxSndFreeChannel(FXSOUND* lpSnd);
extern LOGICAL fxSndGetSoundPos(FXSOUND* lpSnd, piVECTOR* pVec);
extern LOGICAL fxSndInit();
extern LOGICAL fxSndInitSound(SINT32 iID);
extern LOGICAL fxSndKill();
extern LOGICAL fxSndLoadSound(FXSOUND* lpSnd);
extern LOGICAL fxSndSetExternalVolume(FLOAT32 fVolume);
extern LOGICAL fxSndFreeSoundsIfNecessary();
extern LOGICAL fxSndSetListenerPosAndVel();
extern LOGICAL fxSndSetOverallVolume(FLOAT32 fVolume);
extern LOGICAL fxSndSetSoundPosVel(FXSOUND* lpSnd);
extern LOGICAL fxSndSetSoundPosition(FXSOUND* lpSnd);
extern LOGICAL fxSndSetSoundToFinite(SINT32 iID);
extern LOGICAL fxSndSetSoundVelocity(FXSOUND* lpSnd);
extern LOGICAL fxSndSetTimeToStart(FXSOUND* lpSnd);
extern LOGICAL fxSndSoundFreeCallback(FXSOUND* lpSnd);
extern LOGICAL fxSndSoundFreeSound(FXSOUND* lpSnd);
extern LOGICAL fxSndStartSound(FXSOUND* lpSnd);
extern LOGICAL fxSndStopVehicleSounds(struct vehVehicle*pVehicle);

extern FLOAT32 fxSndGetVolume(SINT32 id);

///////////////////////////
// SINT32 return functions
///////////////////////////
extern SINT32 fxSndGetNewSound();

///////////////////////////
// void set functions
///////////////////////////
extern void fxSndPopFlags(SINT32 iID, UINT32 uFlags);
extern void fxSndPushFlags(SINT32 iID, UINT32 uFlags);
extern void fxSndSetAmmo(SINT32 iID, void const* pAmmo);
extern void fxSndSetKillData(SINT32 iID, void* killData);
extern void fxSndSetCreateTime(SINT32 iID, SINT32 iTime);
extern void fxSndSetEmitterObject(SINT32 iID, OBJECT* lpObj);
extern void fxSndSetIs3D(SINT32 iID, LOGICAL bIs3D);
extern void fxSndSetIsExternal(SINT32 iID, LOGICAL bIsExternal);
extern void fxSndSetKillCallback(SINT32 iID, void (*pFn)(void* killData));
extern void fxSndSetMinDistance(SINT32 iID, FLOAT32 fMin);
extern void fxSndSetMaxDistance(SINT32 iID, FLOAT32 fMax);
extern void fxSndSetPitch(SINT32 iID, FLOAT32 fPitch);
extern void fxSndSetPlayType(SINT32 iID, SINT32 iPlayType);
extern void fxSndSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxSndSetPriority(SINT32 iID, ENUM_PRIORITY enumPriority);
extern void fxSndSetSoundLoop(SINT32 iID, LOGICAL bIsInfinite);
extern void fxSndSetSoundName(SINT32 iID, SCHAR szName[]);
extern void fxSndSetSoundType(SINT32 iID, SINT32 iType);
extern void fxSndSetVolume(SINT32 iID, FLOAT32 fVolume);
extern void fxSndDetachSnd(OBJECT* lpKey);
extern void fxSndSetUserUpdate(SINT32 id, void (*userUpdate)(FXSOUND* s));
extern void fxSndStopSoundsForThisPosition(UINT32 position);
extern void fxSndResetSoundsForThisPosition(UINT32 position);
extern void fxSndStopSoundDelayedFromStart(SINT32 id);

extern int fxSndAddFiniteWorldSound(const char* file, float mindis, float maxdis, int priority, const FLOAT64VECTOR& pos);
extern int fxSndAddFiniteObjectSound(const char* file, int priority, float mindis, float maxdis, const FLOAT64VECTOR& pos, OBJECT* o);
extern int fxSndAddFiniteHeadSound(const char* file, int priority, float volume, float pitch);
extern int fxSndAddInfiniteHeadSound(const char* file, int priority, float volume, float pitch);
extern int fxSndAddInfiniteWorldSound(const char* file, float mindis, float maxdis, int priority, const FLOAT64VECTOR& pos);
