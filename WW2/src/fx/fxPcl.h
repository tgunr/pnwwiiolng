#pragma once

#include "pi/pi.h"
#include "fx/fxMgr.h"

enum ENUMPARTICLETYPE
{
    IDE_PARTICLE_NOTUSED,
    IDE_PARTICLE_CALLBACK,
    IDE_PARTICLE_SMOKETRAIL,
    IDE_PARTICLE_DEBRIS,
    IDE_PARTICLE_FIRETRAIL,
    IDE_PARTICLE_SHELLS,
    IDE_PARTICLE_MUZZLEFLASH,
    IDE_PARTICLE_FIRE_FOUNTAIN,
    IDE_PARTICLE_SMOKE_FOUNTAIN,
    IDE_PARTICLE_VAPOR_TRAIL,
    IDE_NUMBER_OF_PARTICLES

};

struct FXPARTICLE
{
    FXPARTICLE* next;
    FXPARTICLE* prev;

    LOGICAL bDraw;

    SINT32 ID;

    UINT32 uFlags;
    UINT32 uAmmoFlags;
    UINT32 uSpriteNum;

    piCOLORVALUE piColor;

    piUV uv[2];

    FLOAT32 fDistanceSqrd;
    FLOAT32 fAlpha;
    FLOAT32 fRadius;
    FLOAT32 fScale;
    FLOAT32 fDragCo;
    FLOAT32 fLastQuadCreate;
    FLOAT32 fLastUpdate;
    FLOAT32 fStallTime;
    FLOAT32 fNumPerSec;
    FLOAT32 fSpacing;
    FLOAT32 fVariance;

    piVECTOR vecPt;
    piVECTOR vecPcl[4];
    piVECTOR vecDraw[4];
    piVECTOR vecRot;
    piVECTOR vecFXVel;
    piVECTOR vecPVel;
    piVECTOR vecDir;

    piMATRIX matWorld;

    FLOAT64VECTOR vecPos;
    FLOAT64VECTOR vecLastPos;
    FLOAT64VECTOR vecOrig;

    struct OBJECT* lpTopLevelObj;
    struct OBJECT* lpEmitterObj;
    struct OBJECT* lpFxObj;

    struct OBJECTDOF* lpDOF;

    INTCOLOR pclColor;
    INTFLOAT32 pclScale;
    INTFLOAT32 pclTime;
    INTFLOAT32 pclOffset;
    INTVECTOR pclRotate;
    INTVECTOR pclVel;

    ENUMPARTICLETYPE enumType;

    void* pAmmo;
    void* pKillData;
    void (*pFnUserKill)(void*);
    void (*pFnUserUpdate)(FXPARTICLE*);

    struct vehVehicle* lpVehicle;
};

struct FXINITFUNC
{
    bool (*pFn)(FXPARTICLE* lpPcl);
};

struct FXUPDATEFUNC
{
    void (*pFn)(FXPARTICLE* lpPcl);
};

///////////////////////////
// LOGICAL functions
///////////////////////////
extern LOGICAL fxPclCreateExplosion(FLOAT64VECTOR* pPos, FLOAT64 fEnergy);
extern LOGICAL fxPclCreateH2OExplosion(FLOAT64VECTOR* pPos, FLOAT64 fEnergy);

extern LOGICAL fxPclCreateSpark(FLOAT64VECTOR* pPos,
                         piVECTOR* pVel,
                         FLOAT32 fRadius,
                         FLOAT32 fTime,
                         UINT32 uFlags,
                         LOGICAL wantStretch,
                         LOGICAL fixStretch,
                         FLOAT32 drag);
extern LOGICAL fxPclDraw(SINT32 ID);
extern LOGICAL fxPclDraw3D();
extern LOGICAL fxPclInit();
extern LOGICAL fxPclInitParticle(SINT32 iID);
extern void fxPclKill();

extern void fxPclDetachAmmo(void const* pAmmo);
extern void fxPclDetachObj(OBJECT* lpObj);
extern void fxPclDetachByID(SINT32 iID);
extern void fxPclDetachVehicle(struct vehVehicle* lpVeh);
extern void fxPclFreeAll();
extern void fxPclGetList(void* lpParticleSort, SINT32* pIndex);
extern void fxPclPlayDirtSound(void* data);
extern void fxPclPlayShellSound(FXPARTICLE* lpPcl);
extern void fxPclRotateObject(FXPARTICLE* lpPcl);
extern void fxPclSetColor(piCOLORVALUE* lpColor);
extern void fxPclSetInvertView(piMATRIX* lpInvertView);
extern void fxPclUpdate();
extern void fxPclPopFlags(SINT32 iID, UINT32 uFlags);
extern void fxPclPushFlags(SINT32 iID, UINT32 uFlags);
extern void fxPclSet4PtAlpha(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxPclSet4PtScale(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxPclSet4PtTimeOffset(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxPclSetAmmo(SINT32 iID, void* pAmmo);
extern void fxPclSetAmmoFlags(SINT32 iID, UINT32 uFlags);
extern void fxPclSetDirection(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetDOFObject(SINT32 iID, OBJECTDOF* lpDOF);
extern void fxPclSetDragCoeff(SINT32 iID, FLOAT32 fDragCoeff);
extern void fxPclSetEmitterObject(SINT32 iID, OBJECT* lpObj);
extern void fxPclSetFXObject(SINT32 iID, OBJECT* lpObj);
extern void fxPclSetFXVel(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetKillData(SINT32 iID, void* pKillData);
extern void fxPclSetNumPerSec(SINT32 iID, FLOAT32 fNumPerSec);
extern void fxPclSetParticleVel(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxPclSetPt1Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxPclSetPt2Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxPclSetPt3Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxPclSetPt4Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxPclSetPt1Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt2Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt3Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt4Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt1Time(SINT32 iID, FLOAT32 fTime);
extern void fxPclSetPt2Time(SINT32 iID, FLOAT32 fTime);
extern void fxPclSetPt3Time(SINT32 iID, FLOAT32 fTime);
extern void fxPclSetPt4Time(SINT32 iID, FLOAT32 fTime);
extern void fxPclSetPt1Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt2Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt3Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetPt4Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetRadius(SINT32 iID, FLOAT32 fRadius);
extern void fxPclSetRotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxPclSetScale(SINT32 iID, FLOAT32 fScale);
extern void fxPclSetSpacing(SINT32 iID, FLOAT32 fSpace);
extern void fxPclSetSpriteNumber(SINT32 iID, UINT32 uNum);
extern void fxPclSetStallTime(SINT32 iID, FLOAT32 fStall);
extern void fxPclSetTopLevelObject(SINT32 iID, OBJECT* lpObj);
extern void fxPclSetType(SINT32 iID, ENUMPARTICLETYPE enumType);
extern void fxPclSetUserKillCallback(SINT32 iID, void (*pFn)(void*));
extern void fxPclSetUserUpdate(SINT32 iID, void (*pFn)(FXPARTICLE*));
extern void fxPclSetVelocityVariance(SINT32 iID, FLOAT32 fVariance);
extern void fxPclSetWorldMatrix(SINT32 iID, piMATRIX* lpMatWorld);
extern void fxPclSetVehicle(SINT32 iID, struct vehVehicle* lpVel);

////////////////////////////
// SINT32 functions
////////////////////////////
extern SINT32 fxPclGetNewParticle();
