#pragma once

#include "pi/pi.h"

#define DEF_AMBFLAG_WIND    0x00000001
#define DEF_AMBFLAG_H2O     0x00000002

// This is associated with the ambient level in settings.exe
enum ENUMFREQUENCY
{
    IDE_AMBIENT_FREQUENT,
    IDE_AMBIENT_INFREQUENT,
    IDE_AMBIENT_NEVER
};

struct REGION
{
    REGION* next;
    REGION* prev;

    SINT32 iLeft;
    SINT32 iTop;
    SINT32 iRight;
    SINT32 iBottom;

};

// Sounds to play only during the day.
enum ENUMDAYAMBIENT
{
    IDE_AMB_BIRD1,
    IDE_AMB_BIRD2,
    IDE_AMB_BIRD3,
    IDE_AMB_BIRD4,
    IDE_AMB_BIRD5,
    IDE_AMB_BIRD6,
    IDE_AMB_BIRD7,
    IDE_AMB_BIRD8,
    IDE_AMB_BIRD9,
    IDE_AMB_DAYFLY1,
    NUMBER_OF_DAY_AMBIENT
};

// Sounds to play only during the night.
enum ENUMNIGHTAMBIENT
{
    IDE_AMB_CRICKET1,
    IDE_AMB_CRICKET2,
    IDE_AMB_OWL1,
    IDE_AMB_OWL2,
    IDE_AMB_NIGHTFLY1,
    NUMBER_OF_NIGHT_AMBIENT
};

// Sounds that always play, even in combat.
enum ENUMALWAYSAMBIENT
{
    IDE_AMB_WIND1,
    IDE_AMB_WIND2,
    NUMBER_OF_ALWAYS_AMBIENT
};

// Sounds played in the city.
enum ENUMCITYAMBIENT
{
    IDE_AMB_DOG1,
    IDE_AMB_DOG2,
    NUMBER_OF_CITY_AMBIENT
};

// Sounds played near the coast.
enum ENUMCOASTAMBIENT
{
    IDE_AMB_WAVES,
    NUMBER_OF_COAST_AMBIENT
};

// Sounds played near the river.
enum ENUMRIVERAMBIENT
{
    IDE_AMB_FROG1,
    IDE_AMB_RIVERFLY1,
    IDE_AMB_RIVERFLY2,
    NUMBER_OF_RIVER_AMBIENT
};

struct AMBIENTSOUND
{
    SINT32 iSoundID;

    UINT32 uNextPlayTime;
    UINT32 uFlags;

    SCHAR szName[64];
};

////////////////////////////
// LOGICAL return functions
////////////////////////////
extern LOGICAL fxAmbInit();
extern LOGICAL fxAmbInitAlways();
extern LOGICAL fxAmbInitBoth();
extern LOGICAL fxAmbInitCity();
extern LOGICAL fxAmbInitCoast();
extern LOGICAL fxAmbInitDay();
extern LOGICAL fxAmbInitNight();
extern LOGICAL fxAmbInitRegion(REGION* lpReg, const SCHAR* pFile);
extern LOGICAL fxAmbInitRiver();

////////////////////////////
// UINT32 return functions
////////////////////////////
extern void fxAmbStopAlwaysCallback(void* data);
extern void fxAmbStopDayCallback(void* data);
extern void fxAmbStopNightCallback(void* data);
extern void fxAmbStopRiverCallback(void* data);
extern void fxAmbStopCoastCallback(void* data);
extern void fxAmbStopCityCallback(void* data);

////////////////////////////
// void return functions
////////////////////////////
extern void fxAmbDayOrNight();
extern void fxAmbKill();
extern void fxAmbOnCity(SINT32 x, SINT32 y, SINT32 z);
extern void fxAmbOnCoast(SINT32 x, SINT32 y, SINT32 z);
extern void fxAmbOnRiver(SINT32 x, SINT32 y, SINT32 z);
extern void fxAmbPlayAmbientSound();
extern void fxAmbPlaySound(AMBIENTSOUND* lpAmb, SINT32 iNum, void (*pFn)(void*));
extern void fxAmbSetAlwaysTimes();
extern void fxAmbSetCityTimes();
extern void fxAmbSetCoastTimes();
extern void fxAmbSetDayTimes();
extern void fxAmbSetNightTimes();
extern void fxAmbSetRiverTimes();
extern void fxAmbSetStartTimes();
extern void fxAmbSetState();
extern void fxAmbStop(bool bStop);
extern void fxAmbUpdate();
extern void fxAmbDisable();
extern void fxAmbEnable();
