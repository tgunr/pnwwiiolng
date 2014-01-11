#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: fxAmb.cpp
// Description: Handles ambient sounds. These sounds are played continuously,
// unless stopped temporarily with fxAmbStop. i.e., cricket sounds may stop
// temporarily when a bomb explodes nearby.
//////////////////////////////////////////////////////////////////////
#include "fx/fxAmb.h"
#include "fx/fxMgr.h"
#include "fx/fxSnd.h"

#include "include/vars.h"
#include "ui/uiMain.h"
#include "pers/persmain.h"
#include "trn/trntod.h"
#include "math/mathnum.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"

#define DEF_SECONDS_COMBAT_STOP     30000   // 30 seconds

#define DEF_NIGHT   3
#define DEF_DAY     5
#define DEF_COAST   3
#define DEF_RIVER   3
#define DEF_CITY    5
#define DEF_ALWAYS  2

#define DEF_PLAY_NIGHT  0x00000001
#define DEF_PLAY_DAY    0x00000002
#define DEF_PLAY_CITY   0x00000004
#define DEF_PLAY_RIVER  0x00000008
#define DEF_PLAY_COAST  0x00000010

#define DEF_SECONDS_DELAY_DAY       300000  // 5 minutes
#define DEF_SECONDS_DELAY_NIGHT     180000  // 3 minutes
#define DEF_SECONDS_DELAY_ALWAYS    120000  // 2 minutes
#define DEF_SECONDS_DELAY_CITY      300000  // 5 minutes
#define DEF_SECONDS_DELAY_COAST     180000  // 3 minutes
#define DEF_SECONDS_DELAY_RIVER     180000  // 3 minutes

static AMBIENTSOUND g_ambDay[NUMBER_OF_DAY_AMBIENT];
static AMBIENTSOUND g_ambNight[NUMBER_OF_NIGHT_AMBIENT];
static AMBIENTSOUND g_ambAlways[NUMBER_OF_ALWAYS_AMBIENT];
static AMBIENTSOUND g_ambRiver[NUMBER_OF_RIVER_AMBIENT];
static AMBIENTSOUND g_ambCoast[NUMBER_OF_COAST_AMBIENT];
static AMBIENTSOUND g_ambCity[NUMBER_OF_CITY_AMBIENT];

static REGION g_regCity;
static REGION g_regRiver;
static REGION g_regCoast;

static UINT32 g_uStopTime = 0;
static UINT32 g_uState = 0x0;
static UINT32 g_uDayDelay = 0;
static UINT32 g_uNightDelay = 0;
static UINT32 g_uAlwaysDelay = 0;
static UINT32 g_uCityDelay = 0;
static UINT32 g_uCoastDelay = 0;
static UINT32 g_uRiverDelay = 0;

static SINT32 g_iAmbDelay = 1;

static LOGICAL g_bStopSounds = FALSE;
static LOGICAL g_bFirstTime = TRUE;
static LOGICAL g_oldCanopyExposed=TRUE;
static LOGICAL g_oldHatchExposed=TRUE;

///////////////////////////////////
// Function: fxAmbInit
// Description:
///////////////////////////////////
LOGICAL fxAmbInit()
{
    g_iAmbDelay = g_iAmbLevel == IDE_AMBIENT_FREQUENT ? 1 :
                  g_iAmbLevel == IDE_AMBIENT_INFREQUENT ? 2 : 0;

    // Selected never to play ambient sounds on settings.exe
    if(g_iAmbDelay == 0 || !bSoundEnable)
        return TRUE;

    memset(&g_regCity, 0, sizeof(REGION));
    memset(&g_regRiver, 0, sizeof(REGION));
    memset(&g_regCoast, 0, sizeof(REGION));

    if(!fxAmbInitRegion(&g_regCity, "sounds\\ambience\\city_region.txt"))
        return FALSE;

    if(!fxAmbInitRegion(&g_regRiver, "sounds\\ambience\\river_region.txt"))
        return FALSE;

    if(!fxAmbInitRegion(&g_regCoast, "sounds\\ambience\\coast_region.txt"))
        return FALSE;

    fxAmbInitDay();
    fxAmbInitNight();
    fxAmbInitAlways();
    fxAmbInitRiver();
    fxAmbInitCoast();
    fxAmbInitCity();

    g_uStopTime = (UINT32)frametime;

    g_uDayDelay = (UINT32)(DEF_SECONDS_DELAY_DAY * g_iAmbDelay);
    g_uNightDelay = (UINT32)(DEF_SECONDS_DELAY_NIGHT * g_iAmbDelay);
    g_uAlwaysDelay = (UINT32)(DEF_SECONDS_DELAY_ALWAYS * g_iAmbDelay);
    g_uCityDelay = (UINT32)(DEF_SECONDS_DELAY_CITY * g_iAmbDelay);
    g_uCoastDelay = (UINT32)(DEF_SECONDS_DELAY_COAST * g_iAmbDelay);
    g_uRiverDelay = (UINT32)(DEF_SECONDS_DELAY_RIVER * g_iAmbDelay);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitRegion
// Description:
///////////////////////////////////
LOGICAL fxAmbInitRegion(REGION* lpReg, SCHAR* pFile)
{
    SINT32 hHandle = -1;
    SCHAR szBuff[80];
    REGION* lpNode = nullptr;
    REGION* lpTail = nullptr;

    if(!bSoundEnable)
        return TRUE;

    hHandle = fioOpen(pFile);

    if(hHandle == -1)
        return FALSE;

    while(fioGetString(hHandle, szBuff, 79))
    {
        lpNode = (REGION*)memAlloc(sizeof(REGION));

        if(lpNode == nullptr)
        {
            Write_Log(LOG_SOUND, LOG_FATAL, "[fxAmbInitRegion] Out of memory");
            fioClose(hHandle);
            return FALSE;
        }

        sscanf(szBuff,
               "%d %d %d %d%*c",
               &lpNode->iLeft,
               &lpNode->iTop,
               &lpNode->iRight,
               &lpNode->iBottom);

        if(lpReg->next == nullptr)
        {
            lpReg->next = lpNode;
            lpNode->prev = lpReg;
            lpTail = lpNode;
        }
        else
        {
            lpTail->next = lpNode;
            lpNode->prev = lpTail;
            lpTail = lpNode;
        }
    }

    fioClose(hHandle);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitRiver
// Description:
///////////////////////////////////
LOGICAL fxAmbInitRiver()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambRiver, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_RIVER_AMBIENT);

    strncpy(g_ambRiver[IDE_AMB_FROG1].szName, "sounds\\ambience\\frog1.wav", 64);
    strncpy(g_ambRiver[IDE_AMB_RIVERFLY1].szName, "sounds\\ambience\\fly1.wav", 64);
    strncpy(g_ambRiver[IDE_AMB_RIVERFLY2].szName, "sounds\\ambience\\fly2.wav", 64);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitCoast
// Description:
///////////////////////////////////
LOGICAL fxAmbInitCoast()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambCoast, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_COAST_AMBIENT);

    strncpy(g_ambCoast[IDE_AMB_WAVES].szName, "sounds\\ambience\\ocean.wav", 64);

    g_ambAlways[IDE_AMB_WAVES].uFlags |= DEF_AMBFLAG_H2O;

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitCity
// Description:
///////////////////////////////////
LOGICAL fxAmbInitCity()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambCity, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_CITY_AMBIENT);

    strncpy(g_ambCity[IDE_AMB_DOG1].szName, "sounds\\ambience\\dog1.wav", 64);
    strncpy(g_ambCity[IDE_AMB_DOG2].szName, "sounds\\ambience\\dog2.wav", 64);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitAlways
// Description:
///////////////////////////////////
LOGICAL fxAmbInitAlways()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambAlways, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_ALWAYS_AMBIENT);

    strncpy(g_ambAlways[IDE_AMB_WIND1].szName, "sounds\\ambience\\gust1.wav", 64);
    strncpy(g_ambAlways[IDE_AMB_WIND2].szName, "sounds\\ambience\\gust2.wav", 64);

    g_ambAlways[IDE_AMB_WIND1].uFlags |= DEF_AMBFLAG_WIND;
    g_ambAlways[IDE_AMB_WIND2].uFlags |= DEF_AMBFLAG_WIND;

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitDay
// Description:
///////////////////////////////////
LOGICAL fxAmbInitDay()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambDay, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_DAY_AMBIENT);

    strncpy(g_ambDay[IDE_AMB_BIRD1].szName, "sounds\\ambience\\bird1.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD2].szName, "sounds\\ambience\\bird2.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD3].szName, "sounds\\ambience\\bird3.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD4].szName, "sounds\\ambience\\bird4.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD5].szName, "sounds\\ambience\\bird5.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD6].szName, "sounds\\ambience\\bird6.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD7].szName, "sounds\\ambience\\bird7.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD8].szName, "sounds\\ambience\\bird8.wav", 64);
    strncpy(g_ambDay[IDE_AMB_BIRD9].szName, "sounds\\ambience\\bird9.wav", 64);
    strncpy(g_ambDay[IDE_AMB_DAYFLY1].szName, "sounds\\ambience\\fly1.wav", 64);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbInitNight
// Description:
///////////////////////////////////
LOGICAL fxAmbInitNight()
{
    if(!bSoundEnable)
        return TRUE;

    memset(g_ambNight, 0, sizeof(AMBIENTSOUND) * NUMBER_OF_NIGHT_AMBIENT);

    strncpy(g_ambNight[IDE_AMB_CRICKET1].szName, "sounds\\ambience\\cricket1.wav", 64);
    strncpy(g_ambNight[IDE_AMB_CRICKET2].szName, "sounds\\ambience\\cricket2.wav", 64);
    strncpy(g_ambNight[IDE_AMB_OWL1].szName, "sounds\\ambience\\owl1.wav", 64);
    strncpy(g_ambNight[IDE_AMB_OWL2].szName, "sounds\\ambience\\owl2.wav", 64);
    strncpy(g_ambNight[IDE_AMB_NIGHTFLY1].szName, "sounds\\ambience\\fly1.wav", 64);

    return TRUE;
}

///////////////////////////////////
// Function: fxAmbSetStartTimes
// Description:
///////////////////////////////////
void fxAmbSetStartTimes()
{
    if(g_iAmbDelay == 0 || !bSoundEnable)
        return;

    fxAmbSetDayTimes();
    fxAmbSetNightTimes();
    fxAmbSetAlwaysTimes();
    fxAmbSetCoastTimes();
    fxAmbSetCityTimes();
    fxAmbSetRiverTimes();

    if(g_bFirstTime)
    {
        g_bFirstTime = FALSE;
        fxAmbStop(TRUE);
    }
}

///////////////////////////////////
// Function: fxAmbSetCityTimes
// Description:
///////////////////////////////////
void fxAmbSetCityTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iCity = DEF_CITY * g_iAmbDelay;

    if(!bSoundEnable) return;

    for(i = 0; i < NUMBER_OF_CITY_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iCity) % g_uCityDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambCity[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbSetCoastTimes
// Description:
///////////////////////////////////
void fxAmbSetCoastTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iCoast = DEF_COAST * g_iAmbDelay;

    if(!bSoundEnable) return;

    for(i = 0; i < NUMBER_OF_COAST_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iCoast) % g_uCoastDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambCoast[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbSetRiverTimes
// Description:
///////////////////////////////////
void fxAmbSetRiverTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iRiver = DEF_RIVER * g_iAmbDelay;

    if(!bSoundEnable) return;

    for(i = 0; i < NUMBER_OF_RIVER_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iRiver) % g_uRiverDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambRiver[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbSetAlwaysTimes
// Description:
///////////////////////////////////
void fxAmbSetAlwaysTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iAlways = DEF_ALWAYS * g_iAmbDelay;

    if(!bSoundEnable) return;

    for(i = 0; i < NUMBER_OF_ALWAYS_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iAlways) % g_uAlwaysDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambAlways[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbSetDayTimes
// Description:
///////////////////////////////////
void fxAmbSetDayTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iDay = DEF_DAY * g_iAmbDelay;

    if(!bSoundEnable) return;
    if(g_uDayDelay == 0) return;

    for(i = 0; i < NUMBER_OF_DAY_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iDay) % g_uDayDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambDay[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbSetNightTimes
// Description:
///////////////////////////////////
void fxAmbSetNightTimes()
{
    SINT32 iNextTime = 0;
    SINT32 i;
    SINT32 iNight = DEF_NIGHT * g_iAmbDelay;

    if(!bSoundEnable) return;

    for(i = 0; i < NUMBER_OF_NIGHT_AMBIENT; i++)
    {
        iNextTime = (mathGetChaoticNumber() * iNight) % g_uNightDelay;

        if(iNextTime < 0) iNextTime = -iNextTime;

        g_ambNight[i].uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
    }
}

///////////////////////////////////
// Function: fxAmbUpdate
// Description:
///////////////////////////////////
void fxAmbUpdate()
{
    if(!bSoundEnable) return;

    if(g_iAmbDelay)
    {
        fxAmbSetState();
    }
    if(!uiInGame()) return;

    // Don't want ambient sounds for the following.
    if(persIsPersonaVehicleType(VEH_TYPE_AIR)) return;
    if(persIsPersonaVehicleType(VEH_TYPE_NONE)) return;
    if(persIsPersonaVehicleType(VEH_TYPE_FLOATER)) return;

    if(g_iAmbDelay && persIsPersonaVehicleType(VEH_TYPE_SEA))
    {
        g_uState = 0;
        fxAmbPlayAmbientSound();
        return;
    }

    // Only want ambient sounds if we are poking our head out.
    if(persIsPersonaVehicleType(VEH_TYPE_GROUND))
    {
        UINT32 pos = 0;
        persGetPersonaPosition(&pos);
        LOGICAL canopyExposed = FALSE;
        LOGICAL hatchExposed = FALSE;

        canopyExposed = persIsCanopyExposed(pos);
        hatchExposed = persIsHatchExposed(pos);

        // Canopy open when previously closed
        if(canopyExposed && !g_oldCanopyExposed)
        {
            fxAmbEnable();
        }
        // Hatch open when previously closed
        else if(hatchExposed && !g_oldHatchExposed)
        {
            fxAmbEnable();
        }

        // Canopy closed when previously opened
        if(!canopyExposed && g_oldCanopyExposed)
        {
            fxAmbDisable();
        }
        // Hatch closed when previously opened
        else if(!hatchExposed && g_oldHatchExposed)
        {
            fxAmbDisable();
        }

        g_oldCanopyExposed = canopyExposed;
        g_oldHatchExposed = hatchExposed;
    }

    // VEH_TYPE_PERSON always has ambient sounds.

    if(g_iAmbDelay)
    {
        fxAmbPlayAmbientSound();
    }
}

///////////////////////////////////
// Function: fxAmbSetState
// Description:
///////////////////////////////////
void fxAmbSetState()
{
  //  UINT32 uSec = 0;

    SINT32 x, y, z;

    if(!bSoundEnable) return;

    x = (SINT32)WorldOriginOffset.x - (SINT32)playpos.x;
    y = (SINT32)WorldOriginOffset.y - (SINT32)playpos.y;
    z = (SINT32)WorldOriginOffset.z - (SINT32)playpos.z;

    fxAmbOnCity(x, y, z);
    fxAmbOnCoast(x, y, z);
    fxAmbOnRiver(x, y, z);

    fxAmbDayOrNight();
}

///////////////////////////////////
// Function: fxAmbDayOrNight
// Description:
///////////////////////////////////
void fxAmbDayOrNight()
{
    UINT32 uSec = 0;
    UINT32 uOldState = g_uState;

    if(!bSoundEnable) return;

    // Check if we need to switch to night sounds.
    trnTODGetTimeOfDay(&uSec);

    // Daytime for this is 0600 to 1800 hours
    if(uSec > 0 && uSec < 21600)
    {
        g_uState &= ~DEF_PLAY_DAY;
        g_uState |= DEF_PLAY_NIGHT;
    }
    else if(uSec > 64800 && uSec < 86400)
    {
        g_uState &= ~DEF_PLAY_DAY;
        g_uState |= DEF_PLAY_NIGHT;
    }
    else
    {
        g_uState &= ~DEF_PLAY_NIGHT;
        g_uState |= DEF_PLAY_DAY;
    }

    // Reset start times so we don't get all sounds at once.
    if(uOldState != g_uState)
    {
        if(g_uState & DEF_PLAY_DAY)
            fxAmbSetDayTimes();

        if(g_uState & DEF_PLAY_NIGHT)
            fxAmbSetNightTimes();
    }
}

///////////////////////////////////
// Function: fxAmbOnCity
// Description:
///////////////////////////////////
void fxAmbOnCity(SINT32 x, SINT32 y, SINT32 z)
{
    static REGION* lpCity = nullptr;
    static REGION* lpMark = nullptr;
    LOGICAL bOnCity = FALSE;

    if(!bSoundEnable) return;

    if(g_regCity.next != nullptr)
    {
        if(lpCity == nullptr) lpCity = g_regCity.next;

        if(lpMark != nullptr)
        {
            if(x > lpMark->iLeft && x < lpMark->iRight)
            {
                if(y > lpMark->iTop && y < lpMark->iBottom)
                {
                    bOnCity = TRUE;
                }
            }

            if(bOnCity == TRUE)
            {
                g_uState |= DEF_PLAY_CITY;
            }
            else
            {
                lpMark = nullptr;
                g_uState &= ~DEF_PLAY_CITY;
            }
        }
        else if(x > lpCity->iLeft && x < lpCity->iRight)
        {
            if(y > lpCity->iTop && y < lpCity->iBottom)
            {
                lpMark = lpCity;
                g_uState |= DEF_PLAY_CITY;
            }
        }

        lpCity = lpCity->next;
    }
}

///////////////////////////////////
// Function: fxAmbOnRiver
// Description:
///////////////////////////////////
void fxAmbOnRiver(SINT32 x, SINT32 y, SINT32 z)
{
    static REGION* lpRiver = nullptr;
    static REGION* lpMark = nullptr;
    LOGICAL bOnRiver = FALSE;

    if(!bSoundEnable) return;

    if(g_regRiver.next != nullptr)
    {
        if(lpRiver == nullptr) lpRiver = g_regRiver.next;

        if(lpMark != nullptr)
        {
            if(x > lpMark->iLeft && x < lpMark->iRight)
            {
                if(y > lpMark->iTop && y < lpMark->iBottom)
                {
                    bOnRiver = TRUE;
                }
            }

            if(bOnRiver == TRUE)
            {
                g_uState |= DEF_PLAY_RIVER;
            }
            else
            {
                lpMark = nullptr;
                g_uState &= ~DEF_PLAY_RIVER;
            }
        }
        else if(x > lpRiver->iLeft && x < lpRiver->iRight)
        {
            if(y > lpRiver->iTop && y < lpRiver->iBottom)
            {
                lpMark = lpRiver;
                g_uState |= DEF_PLAY_RIVER;
            }
        }

        lpRiver = lpRiver->next;
    }
}

///////////////////////////////////
// Function: fxAmbOnCoast
// Description:
///////////////////////////////////
void fxAmbOnCoast(SINT32 x, SINT32 y, SINT32 z)
{
    static REGION* lpCoast = nullptr;
    static REGION* lpMark = nullptr;
    LOGICAL bOnCoast = FALSE;

    if(!bSoundEnable) return;

    if(g_regCoast.next != nullptr)
    {
        if(lpCoast == nullptr) lpCoast = g_regCoast.next;

        if(lpMark != nullptr)
        {
            if(x > lpMark->iLeft && x < lpMark->iRight)
            {
                if(y > lpMark->iTop && y < lpMark->iBottom)
                {
                    bOnCoast = TRUE;
                }
            }

            if(bOnCoast == TRUE)
            {
                g_uState |= DEF_PLAY_COAST;
            }
            else
            {
                lpMark = nullptr;
                g_uState &= ~DEF_PLAY_COAST;
            }
        }
        else if(x > lpCoast->iLeft && x < lpCoast->iRight)
        {
            if(y > lpCoast->iTop && y < lpCoast->iBottom)
            {
                lpMark = lpCoast;
                g_uState |= DEF_PLAY_COAST;
            }
        }

        lpCoast = lpCoast->next;
    }
}

///////////////////////////////////
// Function: fxAmbKill
// Description:
///////////////////////////////////
void fxAmbKill()
{
    REGION* lpFree = nullptr;
    REGION* lpReg = nullptr;

    if(!bSoundEnable) return;

    fxAmbStop(TRUE);

    lpReg = g_regCity.next;

    while(lpReg != nullptr)
    {
        lpFree = lpReg;
        lpReg = lpReg->next;

        memFree(lpFree);
    }

    lpReg = g_regRiver.next;

    while(lpReg != nullptr)
    {
        lpFree = lpReg;
        lpReg = lpReg->next;

        memFree(lpFree);
    }

    lpReg = g_regCoast.next;

    while(lpReg != nullptr)
    {
        lpFree = lpReg;
        lpReg = lpReg->next;

        memFree(lpFree);
    }

    memset(&g_regCity, 0, sizeof(REGION));
    memset(&g_regRiver, 0, sizeof(REGION));
    memset(&g_regCoast, 0, sizeof(REGION));
}

///////////////////////////////////
// Function: fxAmbStop
// Description:
///////////////////////////////////
void fxAmbStop(LOGICAL bStop)
{
    if(g_iAmbDelay == 0) return;
    if(!bSoundEnable) return;

    g_bStopSounds = bStop;

    if(bStop == FALSE)
    {
        fxAmbSetStartTimes();
    }
    else
    {
        g_uStopTime = (UINT32)frametime;
    }
}

///////////////////////////////////
// Function: fxAmbPlayAmbientSound
// Description:
///////////////////////////////////
void fxAmbPlayAmbientSound()
{
    if(!bSoundEnable) return;

    // Sounds that play even in combat...like wind
    fxAmbPlaySound(g_ambAlways, NUMBER_OF_ALWAYS_AMBIENT, fxAmbStopAlwaysCallback);

    if((UINT32)frametime - g_uStopTime < DEF_SECONDS_COMBAT_STOP) return;

    if(g_bStopSounds)
    {
        fxAmbStop(FALSE);
    }

    if(g_uState & DEF_PLAY_NIGHT)
        fxAmbPlaySound(g_ambNight, NUMBER_OF_NIGHT_AMBIENT, fxAmbStopNightCallback);

    if(g_uState & DEF_PLAY_DAY)
        fxAmbPlaySound(g_ambDay, NUMBER_OF_DAY_AMBIENT, fxAmbStopDayCallback);

    if(g_uState & DEF_PLAY_RIVER)
        fxAmbPlaySound(g_ambRiver, NUMBER_OF_RIVER_AMBIENT, fxAmbStopRiverCallback);

    if(g_uState & DEF_PLAY_COAST)
        fxAmbPlaySound(g_ambCoast, NUMBER_OF_COAST_AMBIENT, fxAmbStopCoastCallback);

    if(g_uState & DEF_PLAY_CITY)
        fxAmbPlaySound(g_ambCity, NUMBER_OF_CITY_AMBIENT, fxAmbStopCityCallback);
}

////////////////////////////////////////////////
// Function: fxAmbPlaySound
// Description:
////////////////////////////////////////////////
void fxAmbPlaySound(AMBIENTSOUND* lpAmb, SINT32 iNum, void (*pFn)(void*))
{
    SINT32 i;

    if(!bSoundEnable) return;

    FLOAT64 x;
    FLOAT64 y;
    FLOAT64 z;

    SINT32 iDir = 0;
    SINT32 iID = 0;

    for(i = 0; i < iNum; i++)
    {
        // Already playing
        if(lpAmb[i].iSoundID > 0) continue;

        if((UINT32)frametime < lpAmb[i].uNextPlayTime) continue;

        iDir = mathGetChaoticNumber() & 1;	// Fast modulo 2

        if(iDir == 0)
            iDir = 1;

        iID = fxSndGetNewSound();

        if(iID == -1)
            return;

        x = -playpos.x;
        y = -playpos.y;
        z = -playpos.z;

        lpAmb[i].iSoundID = iID;

        fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
        fxSndSetPitch(iID, 1.0f);
        fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
        fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
        fxSndSetKillCallback(iID, pFn);
        fxSndSetSoundName(iID, lpAmb[i].szName);
        fxSndSetKillData(iID, &lpAmb[i]);

        if(lpAmb[i].uFlags & DEF_AMBFLAG_WIND || lpAmb[i].uFlags & DEF_AMBFLAG_H2O)
        {
            fxSndSetVolume(iID, mathGetFloat32ChaoticNumber());
            z = -z;

            x += WorldOriginOffset.x;
            y += WorldOriginOffset.y;
            z += WorldOriginOffset.z;
       }
        else
        {
            x = (FLOAT64)iDir * x;
            y = (FLOAT64)iDir * y;
            z = -z;

            x += WorldOriginOffset.x;
            y += WorldOriginOffset.y;
            z += WorldOriginOffset.z;

            x += mathGetFloat32ChaoticNumber() * 50.0f;
            y += mathGetFloat32ChaoticNumber() * 50.0f;
            z += mathGetFloat32ChaoticNumber() * 50.0f;

            fxSndSetVolume(iID, 1.0f);
        }

        fxSndSetPosition(iID, x, y, z);
        fxSndSetMaxDistance(iID, 100.0f);
        fxSndSetMinDistance(iID, 10.0f);

        fxSndInitSound(iID);
    }
}

////////////////////////////////////////////////
// Function: fxAmbStopDayCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopDayCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
    SINT32 iDay = DEF_DAY * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * iDay) % g_uDayDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbStopNightCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopNightCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
    SINT32 iNight = DEF_NIGHT * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * iNight) % g_uNightDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbStopAlwaysCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopAlwaysCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
    SINT32 iAlways = DEF_ALWAYS * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * iAlways) % g_uAlwaysDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbStopCoastCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopCoastCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
    SINT32 iCoast = DEF_COAST * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * iCoast) % g_uCoastDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbStopRiverCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopRiverCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
  //  SINT32 iRiver = DEF_RIVER * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * DEF_RIVER) % g_uRiverDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbStopCityCallback
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void fxAmbStopCityCallback(void* data)
{
    AMBIENTSOUND* lpAmbSnd = (AMBIENTSOUND*)data;
    SINT32 iCity = DEF_CITY * g_iAmbDelay;

    if(lpAmbSnd == nullptr || !bSoundEnable)
        return;

    SINT32 iNextTime = 0;

    iNextTime = (mathGetChaoticNumber() * iCity) % g_uCityDelay;

    if(iNextTime < 0) iNextTime = -iNextTime;

    lpAmbSnd->iSoundID = -1;
    lpAmbSnd->uNextPlayTime = (UINT32)frametime + (UINT32)iNextTime;
}

////////////////////////////////////////////////
// Function: fxAmbDisable
// Description:
////////////////////////////////////////////////
void fxAmbDisable()
{
    g_iAmbDelay = 0;
}

////////////////////////////////////////////////
// Function: fxAmbEnable
// Description:
////////////////////////////////////////////////
void fxAmbEnable()
{
    if(!bSoundEnable)
        return;

    g_iAmbDelay = g_iAmbLevel == IDE_AMBIENT_FREQUENT ? 1 :
                  g_iAmbLevel == IDE_AMBIENT_INFREQUENT ? 2 : 0;

    fxAmbSetStartTimes();
}
