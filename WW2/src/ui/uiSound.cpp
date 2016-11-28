#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: uiSound.cpp
// Description: Wrapper for UI sounds.
//////////////////////////////////////////////////////////////////////
#include "ui/uiSound.h"
#include "sn/sndsSoundControl.h"
#include "client/GameClient.h"
#include "js/jsClient.h"
#include "sn/sndsSoundTypes.h"

#define MAX_UI_SOUNDS   16

struct UiSounds
{
    UiSounds* next;
    UiSounds* prev;

    snSound* s;

    SINT32 ID;
	SCHAR callback[32];
};

static UiSounds sounds[MAX_UI_SOUNDS];
static UiSounds freeH;
static UiSounds freeT;
static UiSounds activeH;
static UiSounds activeT;

//////////////////////////////////////////////////////////////////////
// Function: _addToActive
// Description:
//////////////////////////////////////////////////////////////////////
static void _addToActive(UiSounds* s)
{
    UiSounds* n = activeH.next;

    activeH.next = s;
    s->prev = &activeH;

    s->next = n;
    n->prev = s;
}

//////////////////////////////////////////////////////////////////////
// Function: _getSound
// Description:
//////////////////////////////////////////////////////////////////////
static SINT32 _getSound()
{
    if(freeH.next == &freeT)
    {
        return -1;
    }

    UiSounds* s = freeH.next;

    UiSounds* n = s->next;

    freeH.next = n;
    n->prev = &freeH;

    _addToActive(s);

    return s->ID;
}

//////////////////////////////////////////////////////////////////////
// Function: _releaseSound
// Description:
//////////////////////////////////////////////////////////////////////
static void _releaseSound(UiSounds* s)
{
    UiSounds* p = s->prev;
    UiSounds* n = s->next;

    if(p) p->next = n;
    if(n) n->prev = p;

    s->next = nullptr;
    s->prev = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Function: _freeSound
// Description:
//////////////////////////////////////////////////////////////////////
static void _freeSound(UiSounds* s)
{
    _releaseSound(s);

    UiSounds* n = freeH.next;

    freeH.next = s;
    s->prev = &freeH;

    s->next = n;
    n->prev = s;

	s->s = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Function: uiSoundInit
// Description:
//////////////////////////////////////////////////////////////////////
void uiSoundInit()
{
    memset(sounds, 0, sizeof(UiSounds) * MAX_UI_SOUNDS);

    freeH.next = &freeT;
    freeT.prev = &freeH;

    activeH.next = &activeT;
    activeT.prev = &activeH;

    for(SINT32 i = 0; i < MAX_UI_SOUNDS; i++)
    {
        sounds[i].ID = i + 1;
        _freeSound(&sounds[i]);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: uiPlaySound
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 uiPlaySound(const SCHAR* file, LOGICAL finite, FLOAT64 volume, const SCHAR* callback)
{
    SNDSCREATE sc;

    sc.bIs3D = false;
    sc.pszName = (SCHAR*)file;

    snSound* s = sndsLoadSound(&sc);

    if(!s)
    {
        Write_Log(LOG_OLD, LOG_WARN, "[uiPlaySound] sndsLoadSound(%s) **FAILED**", file);
        return -1;
    }

    SINT32 ID = _getSound();

    if(ID == -1)
    {
        sndsFreeSound(s);

        Write_Log(LOG_OLD, LOG_WARN, "[uiPlaySound] _GetSound() **FAILED**");
        return -1;
    }

    sounds[ID - 1].s = s;

    if(callback)
    {
        strncpy(sounds[ID - 1].callback, callback, 32);
    }
    else
    {
        sounds[ID - 1].callback[0] = 0;
    }

#if XAUDIO
    sndsSetVolume(s, 1.0f);
#else
    sndsSetVolume(s, volume);
#endif

    sndsSetLooping(s, !finite);
    sndsStartSound(s);

    return ID;
}

//////////////////////////////////////////////////////////////////////
// Function: uiSoundUpdate
// Description:
//////////////////////////////////////////////////////////////////////
void uiSoundUpdate()
{
    UiSounds* h = activeH.next;

    while(h && (h != &activeT))
    {
        if(!sndsIsPlaying(h->s))
        {
            if(h->callback[0] != 0)
            {
                PlayerClient()->jsclient->RunScript(h->callback);
            }

            sndsFreeSound(h->s);

            UiSounds* free = h;
            h = h->next;

            _freeSound(free);

            continue;
        }

        h = h->next;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: uiSoundSetVolume
// Description:
//////////////////////////////////////////////////////////////////////
void uiSoundSetVolume(SINT32 ID, FLOAT64 volume)
{
    if(ID <= 0) return;

    SINT32 i = ID - 1;

    sndsSetVolume(sounds[i].s, volume);
}

//////////////////////////////////////////////////////////////////////
// Function: uiSoundStop
// Description:
//////////////////////////////////////////////////////////////////////
void uiSoundStop(SINT32 ID, bool callback)
{
    if(ID <= 0) return;

    SINT32 i = ID - 1;

	if(sounds[i].s)
	{
		if(!callback)
			sounds[i].callback[0] = 0;

		sndsStopSound(sounds[i].s);
	}
}
