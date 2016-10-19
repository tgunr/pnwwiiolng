#include <syscfg.h> // Path is project specific.
////////////////////////////////////////////////
// Module: fxSnd.cpp
// Description: Module handles all FX Sounds.
////////////////////////////////////////////////
#include <math.h>

#include "fx/fxSnd.h"
#include "fx/fxAmb.h"
#include "fx/fxMgr.h"
#include "fx/fxUtl.h"

#include "math/mathvect.h"
#include "pers/persmain.h"
#include "math/math3d.h"
#include "include/vars.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "fio/fio.h"
#include "ui/uiMain.h"
#include "sn/sndsSoundTypes.h"

#if XAUDIO
#include "sn/audio.h"
#endif

#ifdef _DEBUG
#define OUTPUT_SOUND_NAMES_TO_CHAT_BUFFER
#undef OUTPUT_SOUND_NAMES_TO_CHAT_BUFFER

#ifdef OUTPUT_SOUND_NAMES_TO_CHAT_BUFFER
#include "rn/rnmain.h"
#endif
#endif

#include <vector>

#define SOUND_ARRAY_SIZE    1024

#if XAUDIO
#define MAX_CHANNEL_SOUNDS 256
#else
#define MAX_CHANNEL_SOUNDS 128
#endif

static FXSOUND Sounds[SOUND_ARRAY_SIZE];

static std::multimap<OBJECT*, SINT32> mappedObjects;

static std::vector<FXSOUND*> Channels;

static FLOAT32 _overallVol;
static FLOAT32 _externalVol;

static FXSOUND g_freeHead;
static FXSOUND g_freeTail;

static FXSOUND g_activeHead;
static FXSOUND g_activeTail;

static LOGICAL  _startSound(FXSOUND* s);

static void _addToFreeList(FXSOUND* s);
static void _addToActiveList(FXSOUND* s);
static void _releaseActiveSound(FXSOUND* s);
static void _playTrailSound(FXSOUND* s);
static void _freeSound(SINT32 ID);

#if platOS == OS_WIN32
static LOGICAL _calculateVolumeAndPan(FXSOUND* s);
#endif

static LOGICAL _playImmediateSound(FXSOUND* s);

static FXSOUND* _getNewSound();

static UINT32 _stopsoundposition[ML_MAX_CREW_POSITIONS] = {0};

////////////////////////////////
// Function: fxSndKill
// Description: Kill routine
////////////////////////////////
void fxSndKill()
{
    memset(_stopsoundposition, 0, sizeof(UINT32) * ML_MAX_CREW_POSITIONS);

    FXSOUND* B = g_activeHead.next;
    FXSOUND* E = &g_activeTail;
    FXSOUND* FREE = nullptr;

    if(!B)
        return;

    // Free all active sounds
    while(B != E)
    {
        FREE = B;
        B = B->next;

        _freeSound(FREE->ID);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxSndInit
// Description: Init routine.
//////////////////////////////////////////////////////////////////////
LOGICAL fxSndInit()
{
#if XAUDIO
    b3DSoundEnable = TRUE;
#endif

    SINT32 i;

#if XAUDIO
    Channels.resize(MAX_CHANNEL_SOUNDS);
#else
    Channels.resize(MaxNumberOfSounds >= MAX_CHANNEL_SOUNDS ? MAX_CHANNEL_SOUNDS : MaxNumberOfSounds);
#endif

	memset(Sounds, 0, sizeof(FXSOUND) * SOUND_ARRAY_SIZE);

    memset(&g_freeHead, 0, sizeof(FXSOUND));
    memset(&g_freeTail, 0, sizeof(FXSOUND));

    memset(&g_activeHead, 0, sizeof(FXSOUND));
    memset(&g_activeTail, 0, sizeof(FXSOUND));

    g_freeHead.next = &g_freeTail;
    g_freeTail.prev = &g_freeHead;

    g_activeHead.next = &g_activeTail;
    g_activeTail.prev = &g_activeHead;

    for(i = 0; i < SOUND_ARRAY_SIZE; i++)
    {
        Sounds[i].ID = i + 1;
        _addToFreeList(&Sounds[i]);
    }

    _overallVol = 1.0f;
    _externalVol = 1.0f;

	return TRUE;
}

/*------------------------------------------------------------
Function: _findSound
Description:
-------------------------------------------------------------*/
inline FXSOUND* _findSound(SINT32 id)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= SOUND_ARRAY_SIZE) return nullptr;

    return &Sounds[index];
}

/*------------------------------------------------------------
Function: FindDistanceSqrd
Description:
-------------------------------------------------------------*/
static LOGICAL FindDistanceSqrd(FXSOUND* s)
{
    piVECTOR player;
    piVECTOR soundpos;

    switch(s->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            s->distance2 = 0.0f;
            return TRUE;
        }
    }

    // Play position has not been set yet.
    if(playpos.x == 0.0f && playpos.y == 0.0f && playpos.z == 0.0f)
    {
        return FALSE;
    }

    player.set(-playpos.x, -playpos.y, -playpos.z);

    if(!fxSndGetSoundPos(s, &soundpos))
        return FALSE;

    if(!b3DSoundEnable)
    {
        math3dVectorMultiply(&soundpos, &soundpos, piGetViewMatrix());
        s->distance2 = math3dDotProduct(&soundpos, &soundpos);
        return TRUE;
    }

    piVECTOR diff;
    math3dSubtractVectors(&diff, &soundpos, &player);

    s->distance2 = mathVectDotProduct(&diff, &diff);

    return TRUE;
}

/*------------------------------------------------------------
Function: FreeIfNecessary
Description:
-------------------------------------------------------------*/
static LOGICAL FreeIfNecessary(FXSOUND* s)
{
    if(!s->bSoundHasStarted)
        return FALSE;

    LOGICAL IsPlaying = sndsIsPlaying(s->s);

    if(!IsPlaying)
    {
        return TRUE;
    }

    if(s->delayedStopTime)
    {
        if((frametime - s->starttime) >= s->delayedStopTime)
        {
            if(s->type == TYPE_FINITE_SOUND)
            {
                if(!IsPlaying)
                {
                    return TRUE;
                }
            }
            else
            {
                return TRUE;
            }
        }
    }

    if(s->distance2 > s->maxSqrd)
        return TRUE;

    return FALSE;
}

////////////////////////////////////////////////////////
// Function: _addToFreeList
// Description:
////////////////////////////////////////////////////////
void _addToFreeList(FXSOUND* s)
{
    SINT32 ID = s->ID;
    memset(s, 0, sizeof(FXSOUND));
    s->ID = ID;

    FXSOUND* n = g_freeHead.next;

    g_freeHead.next = s;
    s->prev = &g_freeHead;

    s->next = n;
    n->prev = s;
}

////////////////////////////////////////////////////////
// Function: fxSndInitSound
// Description:
////////////////////////////////////////////////////////
LOGICAL fxSndInitSound(SINT32 ID)
{
    if(!bSoundEnable)
        return FALSE;

	FXSOUND* sound = _findSound(ID);

	if(!sound) return FALSE;

      // Some sounds have no name attached (error!)
    // Check here before we continue.
    if(!sound->szName[0])
    {
        _freeSound(sound->ID);
        return FALSE;
    }

    sound->pitch = sound->originalPitch;
    sound->pan = 50;
    sound->volume = sound->originalVolume;

    switch(sound->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            sound->bIs3D = FALSE;
            break;
        }
    default:
        {
#if XAUDIO
            sound->bIs3D = true;
#else
            sound->bIs3D = b3DSoundEnable;
#endif
        }
    }

    switch(sound->type)
    {
    case TYPE_INFINITE_SOUND:
    case TYPE_INFINITE_EXTERNAL_SOUND:
        {
            sound->bLoop = TRUE;
            break;
        }
    }

    if(sound->minDistance <= 0.0001f)
    {
        sound->minDistance = 1.0f;
    }

    if(sound->maxDistance <= 0.0001f)
    {
        sound->maxDistance = 3218.0f;
    }

    sound->maxSqrd = sound->maxDistance * sound->maxDistance;
    sound->minSqrd = sound->minDistance * sound->minDistance;

    switch(sound->type)
    {
    case TYPE_FINITE_EXTERNAL_SOUND:
        {
            sound->type = TYPE_FINITE_SOUND;
            sound->bIsExternal = TRUE;
            break;
        }
    case TYPE_INFINITE_EXTERNAL_SOUND:
        {
            sound->type = TYPE_INFINITE_SOUND;
            sound->bIsExternal = TRUE;
            break;
        }
    default:
        {
            sound->bIsExternal = FALSE;
        }
    }

    sound->createTime = frametime;
    sound->updatetimer = frametime;

    if(sound->Flags & SOUND_PLAY_IMMEDIATE)
    {
        return _playImmediateSound(sound);
    }
    else
    {
        if(!fxSndSetTimeToStart(sound))
        {
            _freeSound(sound->ID);
            return FALSE;
        }
    }

#ifdef OUTPUT_SOUND_NAMES_TO_CHAT_BUFFER
    static int count = 0;
    rnmainListText("[%d] %s", count++, sound->szName);
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxSndSetTimeToStart
// Description: Set time to start for sounds at far distances.
// Hardcode speed of sound at sea level for now.
//////////////////////////////////////////////////////////////////////
LOGICAL fxSndSetTimeToStart(FXSOUND* lpSnd)
{
    static FLOAT32 SpeedOfSound = 344.0f;

    switch(lpSnd->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            lpSnd->timeToStart = 0;
            return TRUE;
        }
    }

    FindDistanceSqrd(lpSnd);

    if(lpSnd->distance2 > lpSnd->maxSqrd)
    {
        return FALSE;
    }

    lpSnd->timeToStart = FloatToInt(math3dFastSqrt(lpSnd->distance2) / SpeedOfSound) * 1000;

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxSndCalculateVolume
// Description: Calculate volume of the sound source.
////////////////////////////////////////////////////////
LOGICAL fxSndCalculateVolume(FXSOUND* s)
{
    FLOAT32 dist2 = 0.0f;

#if platOS == OS_WIN32
    if(!b3DSoundEnable)
    {
        return _calculateVolumeAndPan(s);
    }
#endif

    switch(s->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            if(s->Flags & SOUND_NO_OVERALL)
            {
                s->volume = s->originalVolume;
            }
            else
            {
	            s->volume = s->originalVolume  * _overallVol;
            }

            break;
        }
    default:
        {
            FindDistanceSqrd(s);
#if XAUDIO
#else
            if(dist2 > s->maxSqrd)
            {
                return FALSE;
            }
#endif
            if(s->Flags & SOUND_NO_OVERALL)
            {
                s->volume = s->originalVolume;
            }
            else
            {
	            s->volume = s->originalVolume  * _overallVol;
            }

            if(s->bIsExternal)
            {
                s->volume *= _externalVol;
            }
        }
    }

    UINT32 position = persGetPersonaPosition();

    if(_stopsoundposition[position])
    {
        s->volume = 0.0f;
    }

    if(s->s != nullptr && (s->volume != s->oldVolume))
    {
        s->oldVolume = s->volume;
        sndsSetVolume(s->s, s->volume);
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxSndSetListenerPos
// Description: Set the listeners position and velocity.
////////////////////////////////////////////////////////
LOGICAL fxSndSetListenerPosAndVel()
{
    logTimerBeginMacro();

    if(!b3DSoundEnable)
    {
        return TRUE;
    }

    piVECTOR vecVel;
    piVECTOR vecPos;

    persGetPlayerVelocity(&vecVel);

	if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
    {
        // Even our velocities are turned around. This makes direct sound
        // pitch correctly. This was discovered by working on 2D sounds.
        vecVel.x = -vecVel.x;
        vecVel.z = -vecVel.z;
    }

    vecPos.x = -playpos.x;
    vecPos.y = -playpos.y;
    vecPos.z = -playpos.z;

    sndsSetListener3DVel(&vecVel, DEF_DEFERRED);
    sndsSetListener3DPos(&vecPos, DEF_DEFERRED);

    piVECTOR vecFront = playdir;

#if(platOS != OS_WIN32)
    vecFront.x = -vecFront.x;
    vecFront.y = -vecFront.y;
    vecFront.z = -vecFront.z;
#endif

#if XAUDIO
    sndsSetOrientation(&playdir, &playup, DEF_DEFERRED);
#else
	sndsSetOrientation(&vecFront, &playup, DEF_DEFERRED);
#endif // XAUDIO
    logTimerEndMacro();

    return TRUE;
}

////////////////////////////////////////////////
// Function: fxSndUpdate
// Description: Game loop update for FXSounds.
///////////////////////////////////////////////
LOGICAL fxSndUpdate()
{
    if(bSoundEnable == FALSE)
        return FALSE;

    fxSndSetListenerPosAndVel();

#if XAUDIO
    AudioUpdate();
#endif

	std::vector<SINT32> removeList;

    FXSOUND* B = g_activeHead.next;
    FXSOUND* E = &g_activeTail;

    FXSOUND* s = nullptr;

    for(; B != E; B = B->next)
    {
        s = &Sounds[B->ID - 1];

        if(s->userUpdate)
        {
            s->userUpdate(s);
        }

        if(!fxSndCalculateVolume(s))
        {
            removeList.push_back(B->ID);
            continue;
        }

        if(!fxSndAddToChannel(s))
        {
            removeList.push_back(B->ID);
            continue;
        }

        if(!fxSndSetSoundPosVel(s))
        {
            removeList.push_back(B->ID);
            continue;
        }

        if(FreeIfNecessary(s))
        {
            removeList.push_back(B->ID);
            continue;
        }

        if(s->Flags & DEF_SOUND_STOPS_AMBIENCE)
        {
            // Set flag to prevent ambient sounds from playing.
            if(s->volume > 0.3f)
                fxAmbStop(true);
        }

        if(s->onchannel)
        {
            if(!s->bSoundHasStarted)
            {
                if(!_startSound(s))
                {
                    removeList.push_back(B->ID);
                    continue;
                }
            }
        }
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
        _freeSound(removeList[u]);
    }

    if(b3DSoundEnable)
    {
        // Required for 3D DirectSound
        sndsCommitDeferredSettings();
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: _startSound
// Description:
////////////////////////////////////////////////////////
LOGICAL _startSound(FXSOUND* s)
{
    if(s->s == nullptr)
        return FALSE;

#if !XAUDIO
    if(sndsIsPlaying(s->s))
        return FALSE;
#endif

    sndsSetLooping(s->s, s->bLoop);

    if(!fxSndStartSound(s))
    {
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxSndCanBeAddedToChannel
// Description: Is this sound ready to play?
////////////////////////////////////////////////////////
LOGICAL fxSndCanBeAddedToChannel(FXSOUND* lpSnd)
{
    if(lpSnd->onchannel)
        return FALSE;

    SINT32 timeToStart = lpSnd->createTime + lpSnd->timeToStart;

    if(timeToStart - frametime <= 0)
        return TRUE;

    return FALSE;
}

////////////////////////////////////////////////////////
// Function: fxSndAddToChannel
// Description: Add/Bump sounds off based on priority.
////////////////////////////////////////////////////////
LOGICAL fxSndAddToChannel(FXSOUND* lpSnd)
{
    if(!fxSndCanBeAddedToChannel(lpSnd))
    {
        return TRUE;
    }

    if(fxSndAddToEmptyChannel(lpSnd))
    {
        if(!fxSndLoadSound(lpSnd))
        {
            return FALSE;
        }

        return TRUE;
    }

    if(fxSndBumpChannel(lpSnd))
    {
        if(!fxSndLoadSound(lpSnd))
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////
// Function: fxSndLoadSound
// Description: Load the sound buffer.
////////////////////////////////////////////////////////
LOGICAL fxSndLoadSound(FXSOUND* s)
{
    SNDSCREATE sndsCreate;

    sndsCreate.pszName = s->szName;
    sndsCreate.bIs3D = s->bIs3D;
    sndsCreate.loop = s->bLoop;

    s->s = sndsLoadSound(&sndsCreate);

    if(s->s == nullptr)
    {
        return FALSE;
    }

    sndsSetMaxDistance(s->s, s->maxDistance, DEF_DEFERRED);
    sndsSetMinDistance(s->s, s->minDistance, DEF_DEFERRED);
    sndsSetSound3DMode(s->s, DEF_MODE_NORMAL, DEF_DEFERRED);

    if(!sndsCreate.bIs3D)
    {
        if(!b3DSoundEnable)
        {
            if(s->playType != DEF_PLAYTYPE_HEAD)
            {
                sndsSet3DDoppler(s->s, s->Flags & SOUND_NO_DOPPLER ? FALSE : TRUE);
                sndsSet3DPan(s->s, TRUE);
                sndsSet3DDistanceAttenuation(s->s, TRUE);
            }

            // Have to update here, so volume is correct before starting.
            // Kind of a hack, but it is only necessary for 2D sounds.
            fxSndCalculateVolume(s);
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxSndAddToEmptyChannel
// Description: Add sound to empty slot.
////////////////////////////////////////////////////////
LOGICAL fxSndAddToEmptyChannel(FXSOUND* lpSnd)
{
    UINT32 num;
    UINT32 q;

#if XAUDIO
    num = MAX_CHANNEL_SOUNDS;
#else
    num = MaxNumberOfSounds >= MAX_CHANNEL_SOUNDS ? MAX_CHANNEL_SOUNDS : MaxNumberOfSounds;
#endif

    for(q = 0; q < num; q++)
    {
        if(Channels[q] == nullptr)
        {
            lpSnd->onchannel = TRUE;
            lpSnd->channelIndex = (UINT32)q;
            Channels[q] = lpSnd;

            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////
// Function: PrioritySort
// Description:
////////////////////////////////////////////////////////
static int PrioritySort(FXSOUND* l, FXSOUND* r)
{
    return l->priority < r->priority;
}

////////////////////////////////////////////////////////
// Function: fxSndBumpChannel
// Description: Bump sounds off based on priority.
////////////////////////////////////////////////////////
LOGICAL fxSndBumpChannel(FXSOUND* WantSound)
{
    unsigned int num = 0;

#if XAUDIO
    num = MAX_CHANNEL_SOUNDS;
#else
    num = MaxNumberOfSounds >= MAX_CHANNEL_SOUNDS ? MAX_CHANNEL_SOUNDS : MaxNumberOfSounds;
#endif

    std::sort(Channels.begin(), Channels.end(),  PrioritySort);

    for(unsigned int i = 0; i < num; i++)
    {
        if(Channels[i]->priority < WantSound->priority)
        {
            if(!sndsIsPlaying(Channels[i]->s))
            {
                _freeSound(Channels[i]->ID);

                WantSound->onchannel = TRUE;
                WantSound->channelIndex = i;
                Channels[i] = WantSound;

                return TRUE;
            }
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxSndSetSoundPosVel
// Description: Set position and velocity. These are only done on
// the sounds currently on the play channel.
//////////////////////////////////////////////////////////////////////
LOGICAL fxSndSetSoundPosVel(FXSOUND* lpSnd)
{
    if(!b3DSoundEnable)
        return TRUE;

    if(!lpSnd->onchannel)
        return TRUE;

    switch(lpSnd->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            return TRUE;
        }
    }

    if(!fxSndSetSoundPosition(lpSnd))
        return FALSE;

    if(!fxSndSetSoundVelocity(lpSnd))
        return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////////
// Function: fxSndSetSoundVelocity
// Description: Set the velocity of the sound object.
/////////////////////////////////////////////////////
LOGICAL fxSndSetSoundVelocity(FXSOUND* lpSnd)
{
    if(!b3DSoundEnable)
        return TRUE;

    if(lpSnd->playType == DEF_PLAYTYPE_OBJECT)
    {
        std::multimap<OBJECT*, SINT32>::iterator I = mappedObjects.find(lpSnd->o);

        if(I == mappedObjects.end())
            return FALSE;

        // Get velocity object
        OBJECT* o = fxUtlGetObject(I->first);

        if(!o)
            return FALSE;

        sndsSetSound3DVel(lpSnd->s, &o->vel, DEF_DEFERRED);
    }
    else if(lpSnd->playType == DEF_PLAYTYPE_AMMO)
    {
        weapAmmo* lpAmmo = (weapAmmo*)lpSnd->pAmmo;
        piVECTOR vecVel;

        if(lpAmmo == nullptr)
            return FALSE;

        vecVel.x = (FLOAT32) lpAmmo->Velocity.x;
        vecVel.y = (FLOAT32) lpAmmo->Velocity.y;
        vecVel.z = (FLOAT32)-lpAmmo->Velocity.z;

        sndsSetSound3DVel(lpSnd->s, &vecVel, DEF_DEFERRED);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxSndGetSoundPos
// Description: Retrieve sound position for world and object sounds.
//////////////////////////////////////////////////////////////////////
LOGICAL fxSndGetSoundPos(FXSOUND* lpSnd, piVECTOR* pVec)
{
    switch(lpSnd->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            if(!b3DSoundEnable)
            {
                pVec->x = (FLOAT32)lpSnd->vecPos.x;
                pVec->y = (FLOAT32)lpSnd->vecPos.y;
                pVec->z = (FLOAT32)lpSnd->vecPos.z;
            }

            return TRUE;
        }
    }

    if(lpSnd->playType == DEF_PLAYTYPE_WORLD)
    {
        FLOAT64VECTOR pos;

        pos = lpSnd->vecPos;
        pos.z = -pos.z;

        mathSubVectors(&pos, &pos, &WorldOriginOffset);

        pVec->set((float)pos.x, (float)pos.y, (float)pos.z);
    }
    else if(lpSnd->playType == DEF_PLAYTYPE_AMMO)
    {
        FLOAT64VECTOR pos;
        weapAmmo* lpAmmo = (weapAmmo*)lpSnd->pAmmo;

        if(lpAmmo == nullptr)
            return FALSE;

        pos = lpAmmo->Position;
        pos.z = -pos.z;

        mathSubVectors(&pos, &pos, &WorldOriginOffset);

        pVec->set((float)pos.x, (float)pos.y, (float)pos.z);
    }
    else if(lpSnd->playType == DEF_PLAYTYPE_OBJECT)
    {
        FLOAT64VECTOR pos;

        if(lpSnd->o == nullptr)
            return FALSE;

        std::multimap<OBJECT*, SINT32>::iterator I = mappedObjects.find(lpSnd->o);

        if(I == mappedObjects.end())
            return FALSE;

        OBJECT* o = I->first;

        mathVectorMultiply64(&pos, &lpSnd->vecPos, &o->mat);

        pVec->set((float)pos.x, (float)pos.y, (float)pos.z);
    }

    return TRUE;
}

//////////////////////////////////////////////////
// Function: fxSndSetSoundPosition
// Description:
//////////////////////////////////////////////////
LOGICAL fxSndSetSoundPosition(FXSOUND* lpSnd)
{
    if(!b3DSoundEnable)
        return TRUE;

    piVECTOR vecPosition;

    switch(lpSnd->playType)
    {
    case DEF_PLAYTYPE_HEAD:
        {
            break;
        }
    default:
        {
            if(!fxSndGetSoundPos(lpSnd, &vecPosition))
                return FALSE;

            sndsSetSound3DPos(lpSnd->s, &vecPosition, DEF_DEFERRED);
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////
// Function: fxSndDetachSnd
// Description: Remove a ALL sounds attached to lpObject
/////////////////////////////////////////////////////////
void fxSndDetachSnd(OBJECT* lpKey)
{
    if(lpKey == nullptr) return;
    if(!(lpKey->flag2 & OBJECT_HAS_FX)) return;
    if(mappedObjects.empty()) return;

    std::multimap<OBJECT*, SINT32>::iterator I = mappedObjects.find(lpKey);
    std::multimap<OBJECT*, SINT32>::iterator FREE;

    std::vector<SINT32> removeList;

    while(I != mappedObjects.end())
    {
		if(I->first != lpKey) break;

        FREE = I;
        ++I;

        removeList.push_back(FREE->second);
        mappedObjects.erase(FREE);
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
        Sounds[removeList[u] - 1].o = nullptr;
        _freeSound(removeList[u]);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxSndDetachAmmo
// Description: Detach sound from ammo.
//////////////////////////////////////////////////////////////////////
void fxSndDetachAmmo(void const* pAmmo)
{
    if(pAmmo == nullptr) return;

    weapAmmo* lpAmmo = (weapAmmo*)pAmmo;

    fxSndStopSound(lpAmmo->SoundID);
}

//////////////////////////////////////////////////////////
// Function: fxSndChangePitch
// Description: Set new original volume.
//////////////////////////////////////////////////////////
void fxSndChangePitch(SINT32 id, FLOAT32 pitch)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    pitch = pitch > 1.0f ? 1.0f : pitch < 0.0f ? 0.0f : pitch;

    sound->originalPitch = pitch;

    if(sound->originalPitch != sound->oldPitch)
    {
        if(!sndsSetFrequency(sound->s, pitch))
        {
            return;
        }

        sound->oldPitch = pitch;
    }
}

//////////////////////////////////////////////////////////
// Function: fxSndChangeVolume
// Description: Set new original volume.
//////////////////////////////////////////////////////////
void fxSndChangeVolume(SINT32 id, FLOAT32 newvolume)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    newvolume = newvolume > 1.0f ? 1.0f :
                newvolume < 0.000001f ? 0.0f :
                newvolume;

    sound->originalVolume = newvolume;
}

//////////////////////////////////////////////////
// Function: _freeSound
// Description: Free the sound and clear the node.
//////////////////////////////////////////////////
void _freeSound(SINT32 id)
{
	FXSOUND* sound = nullptr;

    SINT32 index = id - 1;

    if(index < 0 || index >= SOUND_ARRAY_SIZE) return;

    sound = &Sounds[index];

    if(sound->s != nullptr)
    {
        sndsStopSound(sound->s);

        if(sound->s->hasTrail)
        {
            if(!(sound->Flags & SOUND_PLAY_NO_TRAIL))
            {
                _playTrailSound(sound);
            }
        }
    }

    fxSndSoundFreeCallback(sound);
    fxSndSoundFreeSound(sound);
    fxSndFreeChannel(sound);
    fxSndFreeNode(sound);
}

////////////////////////////////////////
// Function: _playTrailSound
// Description:
////////////////////////////////////////
void _playTrailSound(FXSOUND* s)
{
    SCHAR str[64];

    strncpy(str, s->szName, 63);

    SCHAR* p = strchr(str, '.');

    if(!p) return;

    *p = 0;

    SCHAR path[64];

    strncpy(path, str, 63);
    strcat(path, "_trail.wav");

    SINT32 ID = fxSndGetNewSound();

    if(ID == -1) return;

    fxSndSetSoundName(ID, path);
    fxSndSetPlayType(ID, s->playType);
    fxSndSetEmitterObject(ID, s->o);
    fxSndSetMinDistance(ID, s->minDistance);
    fxSndSetMaxDistance(ID, s->maxDistance);
    fxSndSetPosition(ID, s->vecPos.x, s->vecPos.y, s->vecPos.z);
    fxSndSetPriority(ID, s->priority);
    fxSndSetPitch(ID, 1.0f);
	fxSndSetVolume(ID, 1.0f);
    fxSndPushFlags(ID, s->Flags);
    fxSndPushFlags(ID, SOUND_PLAY_IMMEDIATE);
    fxSndSetSoundType(ID, s->playType);

    fxSndInitSound(ID);
}

////////////////////////////////////////
// Function: _playImmediateSound
// Description:
////////////////////////////////////////
inline LOGICAL _playImmediateSound(FXSOUND* s)
{
    if(!fxSndAddToChannel(s))
    {
        _freeSound(s->ID);
        return FALSE;
    }

    if(!fxSndCalculateVolume(s))
    {
        _freeSound(s->ID);
        return FALSE;
    }

    if(!fxSndSetSoundPosVel(s))
    {
        _freeSound(s->ID);
        return FALSE;
    }

    if(!_startSound(s))
    {
        _freeSound(s->ID);
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////
// Function: fxSndSoundFreeChannel
// Description:
////////////////////////////////////////
LOGICAL fxSndFreeChannel(FXSOUND* s)
{
    if(s->channelIndex < 0)
        return FALSE;

    if(s->onchannel == FALSE)
        return FALSE;

    UINT32 num;

#if XAUDIO
    num = MAX_CHANNEL_SOUNDS;
#else
    num = MaxNumberOfSounds >= MAX_CHANNEL_SOUNDS ? MAX_CHANNEL_SOUNDS : MaxNumberOfSounds;
#endif

    if(s->channelIndex >= (SINT32)num)
        return FALSE;

    Channels[s->channelIndex] = nullptr;

    return TRUE;
}

////////////////////////////////////////
// Function: fxSndSoundFreeCallback
// Description:
////////////////////////////////////////
LOGICAL fxSndSoundFreeCallback(FXSOUND* lpSnd)
{
    if(lpSnd->pFn == nullptr)
        return FALSE;

    lpSnd->pFn(lpSnd->killData);

    return TRUE;
}

////////////////////////////////////////
// Function: fxSndSoundFreeSound
// Description:
////////////////////////////////////////
LOGICAL fxSndSoundFreeSound(FXSOUND* lpSnd)
{
    if(lpSnd->s == nullptr)
        return FALSE;

    sndsFreeSound(lpSnd->s);
    lpSnd->s = nullptr;

    return TRUE;
}

////////////////////////////////////////
// Function: fxSndStopSound
// Description: Free sound.
////////////////////////////////////////
void fxSndStopSound(SINT32 ID)
{
	_freeSound(ID);
}

///////////////////////////////////////////////////
// Function: fxSndStopSoundDelayed
// Description: Stops the sound when the sound
// ending would reach you, based on speed of sound.
//////////////////////////////////////////////////
void fxSndStopSoundDelayed(SINT32 id, SINT32 time)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->duration = time;
    sound->delayedStopTime = time;
}

///////////////////////////////////////////////////
// Function: fxSndSetSoundToFinite
// Description: Set a sound to a finite play type.
//////////////////////////////////////////////////
LOGICAL fxSndSetSoundToFinite(SINT32 id)
{
	FXSOUND* sound = _findSound(id);

	if(!sound)
	{
		return FALSE;
	}

    sound->bLoop = FALSE;

    if(!sndsSetLooping(sound->s, false))
        return FALSE;

    if(sound->onchannel)
    {
        if(sndsIsPlaying(sound->s))
        {
            sndsStopSound(sound->s);
            fxSndStartSound(sound);
        }
    }

    return TRUE;
}

///////////////////////////////////////////
// Function: fxSndStartSound
// Description:
///////////////////////////////////////////
LOGICAL fxSndStartSound(FXSOUND* s)
{
    if(sndsStartSound(s->s))
    {
        sndsSetVolume(s->s, s->volume);
        sndsSetFrequency(s->s, s->pitch);

        if(!b3DSoundEnable)
        {
            sndsSetPanPosition(s->s, (SINT16)s->pan);

            if(!sndsApply3D(s->s))
                return FALSE;
        }

        s->bSoundHasStarted = TRUE;
        s->starttime = frametime;
    }

    return TRUE;
}

//////////////////////////////////////////////////////
// Function: fxSndGetNewSound
// Description: Find 'available' slot.
//////////////////////////////////////////////////////
SINT32 fxSndGetNewSound()
{
    if(bSoundEnable == FALSE)
        return -1;

    if(!uiInGame())
    {
        return -1;
    }

    FLOAT32 fMaxBrainOxy = 0.0f;
    FLOAT32 fBlackoutFullPercent = 0.0f;

    persGetMaxBrainOxy(&fMaxBrainOxy, &fBlackoutFullPercent);

    // We are dead. No need to create new sounds.
    if(fMaxBrainOxy <= fBlackoutFullPercent)
        return -1;

    FXSOUND* s = _getNewSound();

    if(s == nullptr)
        return -1;

    // To prevent freeing more than once
    s->bInUse = TRUE;

    return s->ID;
}

#if platOS == OS_WIN32
//////////////////////////////////////////////////////
// Function: _calculateVolumeAndPan
// Description:
//////////////////////////////////////////////////////
LOGICAL _calculateVolumeAndPan(FXSOUND* s)
{
    piVECTOR pos;

    s->pitch = s->originalPitch;

    UINT32 position = persGetPersonaPosition();

    if(s->playType == DEF_PLAYTYPE_HEAD)
    {
        s->pan = 50;

        if(s->s != nullptr)
        {
            if(s->Flags & SOUND_NO_OVERALL)
            {
                s->volume = s->originalVolume;
            }
            else
            {
                s->volume = s->originalVolume * _overallVol;
            }

            if(_stopsoundposition[position])
            {
                s->volume = 0.0f;
            }

            sndsSetVolume(s->s, s->volume);
            sndsSetPanPosition(s->s, (SINT16)s->pan);
        }

        return TRUE;
    }

    if(_stopsoundposition[position])
    {
        s->volume = 0.0f;

        if(s->s)
        {
            sndsSetVolume(s->s, s->volume);
        }

        return TRUE;
    }

    fxSndGetSoundPos(s, &pos);

    FLOAT64VECTOR v = {pos.x, pos.y, pos.z};

    sndsSet3DPosition(s->s, &v);

    if(s->playType == DEF_PLAYTYPE_OBJECT)
    {
        OBJECT* o = s->o;

        if(o)
        {
            o = fxUtlGetObject((OBJECT*)o);
        }

        if(!o) return FALSE;

        FLOAT64VECTOR vel = {o->vel.x, o->vel.y, o->vel.z};
        sndsSet3DVelocity(s->s, &vel);
    }
    else if(s->playType == DEF_PLAYTYPE_AMMO)
    {
        weapAmmo* a = (weapAmmo*)s->pAmmo;

        sndsSet3DVelocity(s->s, &a->Velocity);
    }
    else
    {
        FLOAT64VECTOR vel = {0.0f, 0.0f, 0.0f};
        sndsSet3DVelocity(s->s, &vel);
    }

    if(s->Flags & SOUND_NO_OVERALL)
    {
        s->volume = s->originalVolume;
    }
    else
    {
        s->volume = s->originalVolume * _overallVol;
    }

    if(s->bIsExternal)
    {
        s->volume *= _externalVol;
    }

    sndsSetVolume(s->s, s->volume);

    if(!sndsApply3D(s->s))
        return FALSE;

    return TRUE;
}
#endif

//////////////////////////////////////////////////////
// Function: _getNewSound
// Description:
//////////////////////////////////////////////////////
FXSOUND* _getNewSound()
{
	FXSOUND* sound = nullptr;

    if(g_freeHead.next == nullptr)
        return nullptr;

    if(g_freeHead.next == &g_freeTail)
        return nullptr;

    sound = g_freeHead.next;
    FXSOUND* n = sound->next;

    g_freeHead.next = n;
    n->prev = &g_freeHead;

    sound->next = nullptr;
    sound->prev = nullptr;

    _addToActiveList(sound);

    return sound;
}

//////////////////////////////////////////////////////
// Function: _addToActiveList
// Description:
//////////////////////////////////////////////////////
void _addToActiveList(FXSOUND* s)
{
    FXSOUND* n = g_activeHead.next;

    g_activeHead.next = s;
    s->prev = &g_activeHead;

    n->prev = s;
    s->next = n;
}

//////////////////////////////////////////////////////
// Function: fxSndFreeNode
// Description:
//////////////////////////////////////////////////////
void fxSndFreeNode(FXSOUND* s)
{
    // Sound already freed
    if(s->bInUse == FALSE)
        return;

    if(s->o)
    {
        std::multimap<OBJECT*, SINT32>::iterator I = mappedObjects.find(s->o);

        if(I != mappedObjects.end())
        {
            mappedObjects.erase(I);
        }
    }

    _releaseActiveSound(s);
    _addToFreeList(s);
}

//////////////////////////////////////////////////////
// Function: _releaseActiveSound
// Description:
//////////////////////////////////////////////////////
void _releaseActiveSound(FXSOUND* s)
{
    FXSOUND* p = s->prev;
    FXSOUND* n = s->next;

    p->next = n;
    n->prev = p;

    s->next = nullptr;
    s->prev = nullptr;
}

//////////////////////////////////////////////////////
// Function: fxSndSetOverallVolume
// Description:
//////////////////////////////////////////////////////
LOGICAL fxSndSetOverallVolume(FLOAT32 volume)
{
    _overallVol = volume > 1.0f ? 1.0f :
                  volume < 0.0f ? 0.0f :
                  volume;

    return TRUE;
}

//////////////////////////////////////////////////////
// Function: fxSndSetExternalVolume
// Description:
//////////////////////////////////////////////////////
LOGICAL fxSndSetExternalVolume(FLOAT32 volume)
{
    _externalVol = volume > 1.0f ? 1.0f :
                   volume < 0.0f ? 0.0f :
                   volume;

    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: fxSndStopVehicleSounds
// Description: Stop all sounds associated with a vehicle.
// This function is here because when you were killed, the
// vehicle sounds would still play.
//////////////////////////////////////////////////////////
LOGICAL fxSndStopVehicleSounds(vehVehicle*pVehicle)
{
    SINT32 i, j;

    if(pVehicle == nullptr)
        return FALSE;

    for(i = 0; i < ML_MAX_ENGINES; i++)
    {
        for(j = 0; j < 4; j++)
        {
	        if(pVehicle->StateAnimation.EngineSoundId[i][j] > 0)
    	    {
        	    fxSndStopSound(pVehicle->StateAnimation.EngineSoundId[i][j]);
            	pVehicle->StateAnimation.EngineSoundId[i][j] = -1;
        	}
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetPan
// Description: Set pan from outside this module.
// 0 <= pan <= 100. This should only be done on sounds
// with DEF_PLAYTYPE_SOUND, since other types are overidden.
//////////////////////////////////////////////////////////
void fxSndSetPan(SINT32 id, SINT32 pan)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->pan = pan;
}

//////////////////////////////////////////////////////////
// Function: fxSndPopFlags
// Description:
//////////////////////////////////////////////////////////
void fxSndPopFlags(SINT32 id, UINT32 Flags)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->Flags &= ~Flags;
}

//////////////////////////////////////////////////////////
// Function: fxSndPushFlags
// Description:
//////////////////////////////////////////////////////////
void fxSndPushFlags(SINT32 id, UINT32 Flags)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->Flags |= Flags;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetAmmo
// Description:
//////////////////////////////////////////////////////////
void fxSndSetAmmo(SINT32 id, void const* pAmmo)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->pAmmo = pAmmo;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetKillData
// Description:
//////////////////////////////////////////////////////////
void fxSndSetKillData(SINT32 id, void* killData)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->killData = killData;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetEmitterObject
// Description:
//////////////////////////////////////////////////////////
void fxSndSetEmitterObject(SINT32 id, OBJECT* o)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    if(!o) return;

    if(obmainValidateObject(o))
    {
        sound->o = o;

        o->flag2 |= OBJECT_HAS_FX;

        mappedObjects.insert(std::make_pair(o, id));
    }
    else
    {
        sound->o = nullptr;
    }
}

//////////////////////////////////////////////////////////
// Function: fxSndSetIs3D
// Description:
//////////////////////////////////////////////////////////
void fxSndSetIs3D(SINT32 id, LOGICAL bIs3D)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->bIs3D = bIs3D;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetKillCallback
// Description:
//////////////////////////////////////////////////////////
void fxSndSetIsExternal(SINT32 id, LOGICAL bIsExternal)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->bIsExternal = bIsExternal;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetKillCallback
// Description:
//////////////////////////////////////////////////////////
void fxSndSetKillCallback(SINT32 id, void (*pFn)(void* killData))
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->pFn = pFn;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetMinDistance
// Description:
//////////////////////////////////////////////////////////
void fxSndSetMinDistance(SINT32 id, FLOAT32 fMin)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->minDistance = fMin;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetMaxDistance
// Description:
//////////////////////////////////////////////////////////
void fxSndSetMaxDistance(SINT32 id, FLOAT32 fMax)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->maxDistance = fMax;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetPitch
// Description:
//////////////////////////////////////////////////////////
void fxSndSetPitch(SINT32 id, FLOAT32 pitch)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->originalPitch = pitch;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetPlayType
// Description:
//////////////////////////////////////////////////////////
void fxSndSetPlayType(SINT32 id, SINT32 playType)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->playType = playType;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetPosition
// Description:
//////////////////////////////////////////////////////////
void fxSndSetPosition(SINT32 id, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->vecPos.x = x;
    sound->vecPos.y = y;
    sound->vecPos.z = z;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetPriority
// Description:
//////////////////////////////////////////////////////////
void fxSndSetPriority(SINT32 id, ENUM_PRIORITY priority)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->priority = priority;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetCreateTime
// Description:
//////////////////////////////////////////////////////////
void fxSndSetCreateTime(SINT32 id, SINT32 iTime)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->createTime = iTime;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetSoundName
// Description:
//////////////////////////////////////////////////////////
void fxSndSetSoundName(SINT32 id, const char* soundName)
{
	if (soundName)
	{
		FXSOUND* sound = _findSound(id);
		if (sound)
			strncpy(sound->szName, soundName, MAX_SND_NAME_LENGTH - 1);
	}
}

//////////////////////////////////////////////////////////
// Function: fxSndSetSoundType
// Description:
//////////////////////////////////////////////////////////
void fxSndSetSoundType(SINT32 id, SINT32 type)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->type = type;

    switch(type)
    {
    case TYPE_FINITE_EXTERNAL_SOUND:
    case TYPE_INFINITE_EXTERNAL_SOUND:
        {
            sound->bIsExternal = TRUE;
            break;
        }
     default:
        {
            sound->bIsExternal = FALSE;
        }
    }
}

//////////////////////////////////////////////////////////
// Function: fxSndSetSoundLoop
// Description:
//////////////////////////////////////////////////////////
void fxSndSetSoundLoop(SINT32 id, LOGICAL bIsInfinite)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->bLoop = bIsInfinite;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetVolume
// Description:
//////////////////////////////////////////////////////////
void fxSndSetVolume(SINT32 id, FLOAT32 volume)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->originalVolume = volume;
}

//////////////////////////////////////////////////////////
// Function: fxSndGetVolume
// Description:
//////////////////////////////////////////////////////////
FLOAT32 fxSndGetVolume(SINT32 id)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return 0.0f;

    return sound->originalVolume;
}

//////////////////////////////////////////////////////////
// Function: fxSndSetUserUpdate
// Description:
//////////////////////////////////////////////////////////
void fxSndSetUserUpdate(SINT32 id, void (*userUpdate)(FXSOUND* s))
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

	sound->userUpdate = userUpdate;
}

//////////////////////////////////////////////////////////
// Function: fxSndStopSoundsForThisPosition
// Description:
//////////////////////////////////////////////////////////
void fxSndStopSoundsForThisPosition(UINT32 position)
{
    _stopsoundposition[position] = 1;
}

//////////////////////////////////////////////////////////
// Function: fxSndResetSoundsForThisPosition
// Description:
//////////////////////////////////////////////////////////
void fxSndResetSoundsForThisPosition(UINT32 position)
{
    _stopsoundposition[position] = 0;
}

//////////////////////////////////////////////////////////
// Function: fxSndStopSoundDelayedFromStart
// Description: Stops sound after it's supposed to play.
// Example: Player shoots gun and it takes 3 seconds for sound to arrive.
// Player stops shooting after 1 second.
// The sound should stop 1 second after the 3 second delay.
//////////////////////////////////////////////////////////
void fxSndStopSoundDelayedFromStart(SINT32 id)
{
	FXSOUND* sound = _findSound(id);

	if(!sound) return;

    sound->delayedStopTime = frametime - sound->createTime;

    if(sound->delayedStopTime <= 0)
    {
        sound->delayedStopTime = 1;
    }
}

//////////////////////////////////////////////////////////
// Function: fxSndAddFiniteWorldSound
// Description:
//////////////////////////////////////////////////////////
int fxSndAddFiniteWorldSound(const char* file,
                             float mindis,
                             float maxdis,
                             int priority,
                             const FLOAT64VECTOR& pos)
{
    int id = fxSndGetNewSound();

    if(id == -1) return 0;

    fxSndSetSoundName(id, (char*)file);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMinDistance(id, mindis);
    fxSndSetMaxDistance(id, maxdis);
    fxSndSetPosition(id, pos.x, pos.y, pos.z);
    fxSndSetPriority(id, (ENUM_PRIORITY)priority);
    fxSndSetPitch(id, 1.0f);
	fxSndSetVolume(id, 1.0f);
    fxSndSetSoundType(id, TYPE_FINITE_SOUND);

    if(!fxSndInitSound(id))
    {
        fxSndStopSound(id);
        return 0;
    }

    return id;
}

//////////////////////////////////////////////////////////
// Function: fxSndAddFiniteObjectSound
// Description:
//////////////////////////////////////////////////////////
int fxSndAddFiniteObjectSound(const char* file,
                              int priority,
                              float mindis,
                              float maxdis,
                              const FLOAT64VECTOR& pos,
                              OBJECT* o)
{
    int id = fxSndGetNewSound();

    if(id == -1) return 0;

    fxSndSetSoundName(id, (char*)file);
    fxSndSetPlayType(id, DEF_PLAYTYPE_OBJECT);
    fxSndSetMinDistance(id, mindis);
    fxSndSetMaxDistance(id, maxdis);
    fxSndSetPosition(id, pos.x, pos.y, pos.z);
    fxSndSetPitch(id, 1.0f);
	fxSndSetVolume(id, 1.0f);
    fxSndSetPriority(id, (ENUM_PRIORITY)priority);
    fxSndSetSoundType(id, TYPE_FINITE_SOUND);
    fxSndSetEmitterObject(id, o);

    if(!fxSndInitSound(id))
    {
        fxSndStopSound(id);
        return 0;
    }

    return id;
}

//////////////////////////////////////////////////////////
// Function: fxSndAddFiniteObjectSound
// Description:
//////////////////////////////////////////////////////////
int fxSndAddFiniteHeadSound(const char* file, int priority, float volume, float pitch)
{
    int id = fxSndGetNewSound();

    if(id == -1) return 0;

    fxSndSetPitch(id, pitch);
    fxSndSetVolume(id, volume);
    fxSndSetPriority(id, (ENUM_PRIORITY)priority);
    fxSndSetSoundName(id, (char*)file);
    fxSndSetSoundType(id, TYPE_FINITE_SOUND);
    fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);

    if(!fxSndInitSound(id))
    {
        fxSndStopSound(id);
        return 0;
    }

    return id;
}

//////////////////////////////////////////////////////////
// Function: fxSndAddInfiniteObjectSound
// Description:
//////////////////////////////////////////////////////////
int fxSndAddInfiniteHeadSound(const char* file, int priority, float volume, float pitch)
{
    int id = fxSndGetNewSound();

    if(id == -1) return 0;

    fxSndSetPitch(id, pitch);
    fxSndSetVolume(id, volume);
    fxSndSetPriority(id, (ENUM_PRIORITY)priority);
    fxSndSetSoundName(id, (char*)file);
    fxSndSetSoundType(id, TYPE_INFINITE_SOUND);
    fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);

    if(!fxSndInitSound(id))
    {
        fxSndStopSound(id);
        return 0;
    }

    return id;
}
