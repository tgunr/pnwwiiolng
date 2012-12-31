#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"
#include "include/vars.h"
#include "list/list.h"

#include "sn/sndsSoundCache.h"
#include "sn/sndsSoundControl.h"
#include "sn/sndsSoundControlAL.h"
#include "sn/sndsSoundTypes.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "sn/audio.h"

#include <queue>
#include <string>

#include "wwii/wwiiHash.h"

#ifdef BETA_RELEASE
#include "ui/uiChat.h"
#endif

USING_EXT_NAMESPACE;

static SINT32 numsounds;
static SINT32 numsndused;

static SOUNDCACHE g_sndCache[MINSOUNDCACHE];
static std::queue<LPSOUNDCACHE> g_sndFree;

typedef stdext::hash_map<std::string, LPSOUNDCACHE> SoundCacheHash;

static SoundCacheHash g_hashSnd;

static LOGICAL g_noLog = FALSE;

#if !defined(XAUDIO)
////////////////////////////////////////////////////////
// Function: GetNewSound
// Description:
////////////////////////////////////////////////////////
static LPSOUNDCACHE GetNewSound(void)
{
    // Not an error
    if(bSoundEnable == FALSE)
        return NULL;

    if(g_sndFree.empty())
    {
        Write_Log(LOG_SOUND, LOG_WARN, "[GetNewSound] ** FAILED ** ");
        return NULL;
    }

    LPSOUNDCACHE lpS = g_sndFree.front();

    g_sndFree.pop();

    return lpS;
}
#endif

////////////////////////////////////////////////////////
// Function: FreeSound
// Description:
////////////////////////////////////////////////////////
static LOGICAL FreeSound(LPSOUNDCACHE lpSnd)
{
    if(lpSnd == NULL)
        return FALSE;

    if(numsndused <= 0)
    {
        const SCHAR* p = "[FreeSound] Attempt to free sound when none are used!";
        Write_Log(LOG_SOUND, LOG_INFO, p);

        return FALSE;
    }

    sndsPlatformFreeSound(&lpSnd->theSound);

    memset(lpSnd, 0, sizeof(SOUNDCACHE));

    numsndused--;

    g_sndFree.push(lpSnd);

	return TRUE;
}

#if !defined(XAUDIO)
////////////////////////////////////////////////////////
// Function: FindNamedSound
// Description:
////////////////////////////////////////////////////////
static LPSOUNDCACHE FindNamedSound(SCHAR* pszName)
{
    std::string szStr = pszName;

    SoundCacheHash::iterator hashFind;

    hashFind = g_hashSnd.find(szStr);

    if(hashFind == g_hashSnd.end())
    {
        return NULL;
    }

    return hashFind->second;
}
#endif

//////////////////////////////////////////
// Function: sndsInitCache
// Desctiption:
//////////////////////////////////////////
LOGICAL sndsInitCache(void)
{
    numsndused = 0;
    numsounds = MINSOUNDCACHE;

    memset(g_sndCache, 0, sizeof(SOUNDCACHE) * MINSOUNDCACHE);

    for(SINT32 i = 0; i < MINSOUNDCACHE; i++)
    {
        g_sndFree.push(&g_sndCache[i]);
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: sndsEmptyCache
// Desctiption:
//////////////////////////////////////////
LOGICAL sndsEmptyCache(void)
{
    // Not an error
    if(bSoundEnable == FALSE)
        return TRUE;

    sndsReset();

    SoundCacheHash::iterator B;
    SoundCacheHash::iterator E;
    SoundCacheHash::iterator Free;

    B = g_hashSnd.begin();
    E = g_hashSnd.end();

    while(B != E)
    {
        Free = B;
        ++B;
        FreeSound(Free->second);
    }

    g_hashSnd.clear();

    while(!g_sndFree.empty())
    {
        g_sndFree.pop();
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: sndsKillCache
// Desctiption:
//////////////////////////////////////////
void sndsKillCache(void)
{
    // Not an error
    if(bSoundEnable == FALSE)
        return;

	sndsEmptyCache();

	numsounds = 0;
}

#if !defined(XAUDIO)
//////////////////////////////////////////
// Function: _preloadSoundCache
// Desctiption:
//////////////////////////////////////////
static LPSOUNDCACHE _preloadSoundCache(const SCHAR* name)
{
#if XAUDIO
    return NULL;
#else
    LPSOUNDCACHE sc = FindNamedSound((char*)name);

    if(sc != NULL)
    {
        // Already cached
        return sc;
    }

    sc = GetNewSound();

    if(!sc) return FALSE;

    numsndused++;

    if(!sndsApplyDefaultOptions(&sc->theSound))
    {
	    FreeSound(sc);
	    return FALSE;
    }

    strncpy(sc->theSound.name, name, MAX_SND_NAME_LENGTH - 1);

	if(!sndsPlatformLoadSound(&sc->theSound))
	{
        Write_Log(LOG_SOUND, LOG_WARN, "** FAILED ** to load %s", name);
		FreeSound(sc);
		return FALSE;
	}

    g_hashSnd.insert(std::make_pair(name, sc));

    if(!g_noLog)
    {
        Write_Log(LOG_SOUND, LOG_INFO, "** PLEASE CACHE %s **", name);
    }

    return sc;
#endif
}
#endif

//////////////////////////////////////////
// Function: sndsPreloadSoundCache
// Desctiption:
//////////////////////////////////////////
#ifdef SOUND_DEBUG
LOGICAL sndsPreloadSoundCacheDebug(const SCHAR* name, const SCHAR* file, SINT32 line)
#else
LOGICAL sndsPreloadSoundCache(const SCHAR* name, float maxattenuation)
#endif
{
    // Not an error
    if(bSoundEnable == FALSE)
        return TRUE;

    if(!name)
        return FALSE;

    std::string str = name;

    std::replace(str.begin(), str.end(), '\\', '/');

#if XAUDIO
    if(!AudioLoadSound(str, maxattenuation))
        return FALSE;

    // Find if sound has a trail sound
    SINT32 pos = str.find(".wav");

    if(pos != -1)
    {
        str.replace(pos, 9, "_trail.wav");

        if(fioAccess(str.c_str()))
        {
            if(!AudioLoadSound(str))
                return FALSE;
        }
    }

    return TRUE;
#else

    g_noLog = TRUE;

    LPSOUNDCACHE sc1 = _preloadSoundCache(str.c_str());

    g_noLog = FALSE;

    if(!sc1) return FALSE;

    // Find if sound has a trail sound
    SINT32 pos = str.find(".wav");
    str.replace(pos, 9, "_trail.wav");

    LOGICAL found = FALSE;

    // For external debuggin
#ifdef BETA_RELEASE
    // Check if there is a sound directory first
    SINT32 fioSwitch = fioQuery();

    if(fioSwitch == ISARCHIVE)
    {
        fioSet(ISDIRECTORY);

        found = fioAccess(str.c_str());

        if(!found)
        {
            fioSet(ISARCHIVE);
            found = fioAccess(str.c_str());
        }
    }
    else
    {
        found = fioAccess(str.c_str());
    }

    fioSet(fioSwitch);

#else
    found = fioAccess(str.c_str());
#endif

    if(!found)
        return TRUE;

    g_noLog = TRUE;

    LPSOUNDCACHE sc2 = _preloadSoundCache(str.c_str());

    g_noLog = FALSE;

    if(!sc2) return FALSE;

    sc1->theSound.hasTrail = true;

    return TRUE;
#endif // XAUDIO
}

//////////////////////////////////////////////////////////////////////
// Function: sndsLoadSoundCache
// Desctiption: Any sounds with trailing sounds must be preloaded with
// sndsPreloadSoundCache for performance reasons
//////////////////////////////////////////////////////////////////////
#ifdef SOUND_DEBUG
	LOGICAL sndsLoadSoundCacheDebug(snSound* newSound, const SCHAR* file, SINT32 line)
#else
	LOGICAL sndsLoadSoundCache(snSound* newSound)
#endif
{
    // Not an error
    if(bSoundEnable == FALSE)
        return TRUE;

#if XAUDIO
    std::string str = newSound->name;
    std::replace(str.begin(), str.end(), '\\', '/');
    strcpy(newSound->name, str.c_str());

    if(!sndsPlatformCreateSound(NULL, newSound))
        return FALSE;

    return TRUE;
#else
    for(unsigned int i = 0; i < MAX_SND_NAME_LENGTH; i++)
    {
        if(newSound->name[i] == '\\')
        {
            newSound->name[i] = '/';
        }
    }

    LPSOUNDCACHE sc = _preloadSoundCache(newSound->name);

    if(!sc)
    {
        return FALSE;
    }

    if(!sndsPlatformCreateSound(&sc->theSound, newSound))
        return FALSE;

    newSound->hasTrail = sc->theSound.hasTrail;

    sc->numuses++;
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: sndsReleaseSoundInCache
// Desctiption: FIXME: This function is not named correctly.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsReleaseSoundInCache(snSound* oldSound)
{
#if XAUDIO
    AudioKillSound(*oldSound);
    return TRUE;
#else
    LOGICAL bReturn = FALSE;
    LPSOUNDCACHE lpSnd = NULL;

    // Not an error
    if(bSoundEnable == FALSE)
        return TRUE;

    lpSnd = FindNamedSound(oldSound->name);

    if(lpSnd != NULL)
	{
        // numuses only incremented on duplicates. Original is 0.
        if(lpSnd->numuses <= 0)
            return TRUE;

        lpSnd->numuses--;
	}
    else
    {
#ifdef BETA_RELEASE
        uiChatPrintLine("ERROR: could not find %s to release", oldSound->name);
#endif
    }

	bReturn = sndsPlatformFreeSound(oldSound);	// frees up platform structure only

	return bReturn;
#endif
}
