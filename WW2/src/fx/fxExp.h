#pragma once

#include "pi/pi.h"

#include "fx/fxMgr.h"

enum ENUMEXPTYPE
{
    IDE_EXPTYPE_NOTUSED,
    IDE_EXPTYPE_AMMOAIR,
    IDE_EXPTYPE_AMMOGROUND,
    IDE_EXPTYPE_VEHICLECRASH,
    IDE_EXPTYPE_SATCHEL,
    NUMBER_OF_EXP_TYPES
};

enum ENUMEXPTEXTURES
{
    IDE_EXP_NOTUSED,
    IDE_EXP_SHOCKWAVE,
    IDE_MAX_EXP_TEXTURES
};

struct EXPSHAKE
{
    FLOAT32 fTimeToStart;
    FLOAT32 fShakeCurrent;
    FLOAT32 fShakeStart;
    FLOAT32 fShakeTime;

    LOGICAL bShake;

};

struct FXEXPLOSION
{
    FXEXPLOSION* next;
    FXEXPLOSION* prev;

    piVECTOR vecPt;
    piVECTOR n;
    piVECTOR vecWave[4];
    piVECTOR vecVel;

    FLOAT64VECTOR vecPos;

    FLOAT64 fEnergy;

    FLOAT32 fRadius;
    FLOAT32 fDistanceSqrd;
    FLOAT32 fPowEnergy;

    piMATRIX matSize;

    struct vehVehicle* lpVehAttach;

    struct OBJECT* lpEmitterObj;
    struct OBJECT* lpTop;

    piCOLORVALUE piColor;

    SINT32 iIndex;

    UINT32 uSubType;
    UINT32 uFlags;

    LOGICAL bShockWave;
    LOGICAL bIsWater;

    INTCOLOR expWave;

    EXPSHAKE expShake;

    INTFLOAT32 waveTime;
    INTFLOAT32 expTime;

    ENUMEXPTYPE enumType;

    const struct weapAmmo* ammo;
};

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
extern LOGICAL fxExpCreateGenericExp(FLOAT64VECTOR* pPos, FLOAT64 fEnergy);
extern LOGICAL fxExpDraw(SINT32 iIndex);
extern LOGICAL fxExpInit();
extern LOGICAL fxExpKill();

/////////////////////////////
// void return functions
/////////////////////////////
extern void fxExpFreeAll();
extern void fxExpGetList(void* lpExpSort, SINT32* pIndex);
extern void fxExpUpdate();

///////////////////////
// void set functions
///////////////////////
extern void fxExpInitExplosion(SINT32 iID);
extern void fxExpPopFlags(SINT32 iID, UINT32 uFlags);
extern void fxExpPushFlags(SINT32 iID, UINT32 uFlags);
extern void fxExpSetColor(piCOLORVALUE* lpColor);
extern void fxExpSetEmitterObject(SINT32 iID, struct OBJECT* lpObj);
extern void fxExpSetEnergy(SINT32 iID, FLOAT64 fEnergy);
extern void fxExpSetIsWater(SINT32 iID, LOGICAL bIsWater);
extern void fxExpSetNormal(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxExpSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxExpSetPowEnergy(SINT32 iID, FLOAT32 fEnergy);
extern void fxExpSetPt1Time(SINT32 iID, FLOAT32 fTime);
extern void fxExpSetPt2Time(SINT32 iID, FLOAT32 fTime);
extern void fxExpSetPt3Time(SINT32 iID, FLOAT32 fTime);
extern void fxExpSetPt4Time(SINT32 iID, FLOAT32 fTime);
extern void fxExpSetRadius(SINT32 iID, FLOAT32 fRadius);
extern void fxExpSetSubType(SINT32 iID, UINT32 uSubType);
extern void fxExpSetTopLevelObject(SINT32 iID, struct OBJECT* lpObj);
extern void fxExpSetType(SINT32 iID, ENUMEXPTYPE enumType);
extern void fxExpSetVehicle(SINT32 iID, struct vehVehicle* lpVeh);
extern void fxExpSetVelocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxExpSetAmmo(SINT32 id, const weapAmmo* ammo);

/////////////////////////////
// SINT32 return functions
/////////////////////////////
extern SINT32 fxExpGetNewExplosion();
