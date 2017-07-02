#pragma once

#include "pi/pi.h"
#include "ob/obcd.h"			// For FASTCOLLIDE

#include <map>

enum ENUMHITTEXTURES
{
    IDE_HIT_NOTUSED,
    IDE_HIT_TERRAIN,
    IDE_MAX_HIT_TEXTURES
};

struct FXHIT
{
    FXHIT* next;
    FXHIT* prev;

    piVECTOR hitPt[5];

    piVECTOR vecPt;
    piVECTOR vecOldPt;
    piVECTOR n;

    piUV uv[2];

    FLOAT64VECTOR vecPos;

    FLOAT32 fRadius;
    FLOAT32 fDistanceSqrd;
    FLOAT32 fEnergy;

    piMATRIX matSize;

    std::multimap<void* , SINT32>::iterator mapIter;

    struct vehVehicle* lpVehAttach;

    struct OBJECT* lpObj;
    struct OBJECT* lpTop;

    FLOAT32 fCreateTime;

    SINT32 iIndex;

    ENUMHITTEXTURES enumTex;
};

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
extern LOGICAL fxHitAddHit(FXHIT* lpExp);
extern LOGICAL fxHitCanDraw(FXHIT* lpExp);
extern LOGICAL fxHitCheckForHits();
extern LOGICAL fxHitClip(FXHIT* lpExp, piVECTOR* pVec);
extern LOGICAL fxHitCollideObject(piVECTOR* pV1,
                           piVECTOR* pV2,
                           FXHIT* lpHit,
                           UINT32* pNumCls,
                           FASTCOLLIDE cldArray[]);
extern LOGICAL fxHitCreateRotationMatrix(FXHIT* lpExp, piMATRIX* lpMat);
extern LOGICAL fxHitDetachHit(struct OBJECT* lpObj);
extern LOGICAL fxHitDraw();
extern LOGICAL fxHitFreeHit(FXHIT* lpExp);
extern LOGICAL fxHitGetHit(FXHIT** lpExp);
extern LOGICAL fxHitGetHitTexture(FXHIT* lpExp);
extern LOGICAL fxHitGetObjectForHit(FXHIT* lpExp, struct OBJECT* lpObj);
extern LOGICAL fxHitHandleCollision(FASTCOLLIDE cldArray[],
                             UINT32 uNumCollisions,
                             FXHIT* lpExp,
                             SINT32 iIndex);
extern LOGICAL fxHitInit();
extern LOGICAL fxHitIsWater(FXHIT* lpExp);
extern void fxHitKill();
extern LOGICAL fxHitReleaseHit(FXHIT* lpExp);
extern LOGICAL fxHitSetHitVector(FXHIT* lpExp, FASTCOLLIDE* lpCld, SINT32 i);
extern LOGICAL fxHitSetCollisionVectors(piVECTOR* pv1,
                                 piVECTOR* pv2,
                                 piMATRIX* lpMat,
                                 FXHIT* lpExp);
extern LOGICAL fxHitUpdateVehicleHit(FXHIT* lpHit);

/////////////////////////////
// struct OBJECT* return functions
/////////////////////////////
extern struct OBJECT* fxHitGetObject(OBJECT* lpObj);

/////////////////////////////
// void return functions
/////////////////////////////
extern void fxHitFreeAll();
extern void fxHitInitHit(SINT32 iID);
extern void fxHitSetColor(piCOLORVALUE* lpColor);
extern void fxHitUpdate();

//////////////////////
// void set functions
//////////////////////
extern void fxHitSetAttachedObject(SINT32 iID, struct OBJECT* lpPbj);
extern void fxHitSetCreateTime(SINT32 iID, FLOAT32 fTime);
extern void fxHitSetEnergy(SINT32 iID, FLOAT32 fEnergy);
extern void fxHitSetNormal(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxHitSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxHitSetRadius(SINT32 iID, FLOAT32 fRadius);
extern void fxHitSetTopLevelObject(SINT32 iID, struct OBJECT* lpObj);
extern void fxHitSetVehicle(SINT32 iID, vehVehicle* lpVeh);

///////////////////////////
// SINT32 return functions
///////////////////////////
extern SINT32 fxHitGetNewHit();
