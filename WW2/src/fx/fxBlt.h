#pragma once

#include "ob/obcd.h"
#include "pi/pi.h"

#include <map>

#define blt_decal_is_blood_splat    0x00000001
#define blt_decal_is_ground_blood   0x00000002

enum ENUMBULLETTEXTURES
{
    IDE_BULLET_CONCRETE,
    IDE_BULLET_GROUND,
    IDE_BULLET_METALDENT,
    IDE_BULLET_METALHOLE,
    IDE_BULLET_WALLHOLE,
    IDE_BULLET_RICOCHET,
    IDE_BULLET_GLASS,
    IDE_BULLET_SKIN,
    BLOOD_SPAT_DECAL,
    GROUND_BLOOD_DECAL,
    IDE_MAX_BULLET_TEXTURES
};

struct FXBULLET
{
    FXBULLET* next;
    FXBULLET* prev;

    piVECTOR original[4];
    piVECTOR decal[4];
    piVECTOR n;
    piVECTOR relpos;
    piVECTOR oldrelpos;
    piVECTOR velocity;

    FLOAT64VECTOR worldpos;

    FLOAT64ATT vecAtt;

    LOGICAL isStreak;
    LOGICAL isWater;

    piUV uv[2];

    std::multimap<void* , SINT32>::iterator mapIter;

    struct vehVehicle* lpVehAttach;

    struct OBJECT* o;
    struct OBJECT* lpTop;

    UINT32 uVehicleType;
    UINT32 uAmmoSubType;
    UINT32 starttime;
    UINT32 lastupdate;

    UINT32 flags;

    SINT32 index;
    SINT32 collidepoly;
    SINT32 penetrate;

    int type;

    FLOAT32 radius;
    FLOAT32 dis2;
    FLOAT32 scale;
};

struct FXBULLETCREATE
{
    FLOAT32 fEnergy;
    FLOAT32 fRadius;

    struct OBJECT* lpAttachedObj;
    struct OBJECT* lpTopLevelObj;

    SINT32 penetrate;
    SINT32 iPolyNum;

    piVECTOR vecDelta;

    UINT32 uIsTracer;
    UINT32 uAmmoSubType;
    UINT32 flags;

    FLOAT64VECTOR vecPos;
    FLOAT64VECTOR vecVel;

    void* pVehicle;
};

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
extern LOGICAL fxBltAddBullet(FXBULLET* lpBlt);
extern LOGICAL fxBltCheckForBullets();
extern LOGICAL fxBltClip(FXBULLET* lpBlt, piVECTOR* pVec);
extern LOGICAL fxBltCollideObject(piVECTOR* pV1,
                           piVECTOR* pV2,
                           FXBULLET* lpBlt,
                           UINT32* pNumCls,
                           FASTCOLLIDE cldArray[]);
extern LOGICAL fxBltCreate(FXBULLETCREATE* lpCreate);
extern LOGICAL fxBltCreateBulletDecal(FXBULLETCREATE* lpCreate);
extern LOGICAL fxBltCreateParticles(FXBULLET* lpBlt, FXBULLETCREATE* lpCreate);
extern LOGICAL fxBltCreateRotationMatrix(FXBULLET* lpBlt,
                                  piMATRIX* lpMat,
                                  piVECTOR* pvecShot);
extern LOGICAL fxBltCreateSmoke(FXBULLET* b);
extern LOGICAL fxBltDetachBlt(struct OBJECT* lpObj);
extern LOGICAL fxBltDraw();
extern LOGICAL fxBltFreeBullet(FXBULLET* lpBlt);
extern LOGICAL fxBltGetBullet(FXBULLET** lpBlt);
extern LOGICAL fxBltGetBulletTexture(FXBULLET* lpBlt);
extern LOGICAL fxBltGetHitSound(FXBULLET* lpBlt, SCHAR szBuff[]);
extern LOGICAL fxBltGetObjectForBullet(FXBULLET* lpBlt, struct OBJECT* lpObj);
extern LOGICAL fxBltHandleCollision(FASTCOLLIDE cldArray[],
                             UINT32 uNumCollisions,
                             FXBULLET* lpBlt,
                             SINT32 iIndex);
extern LOGICAL fxBltInit();
extern LOGICAL fxBltIsWater(FXBULLET* lpBlt);
extern void fxBltKill();
extern LOGICAL fxBltPlayHitSound(FXBULLET* lpBlt);
extern LOGICAL fxBltReleaseBullet(FXBULLET* lpBlt);
extern LOGICAL fxBltSetBltValues(FXBULLET* lpBlt, FXBULLETCREATE* lpCreate);
extern LOGICAL fxBltSetBulletVector(FXBULLET* lpBlt, FASTCOLLIDE* lpCld, SINT32 i);
extern LOGICAL fxBltSetCollisionVectors(piVECTOR* pv1,
                                 piVECTOR* pv2,
                                 piMATRIX* lpMat,
                                 FXBULLET* lpBlt);
extern LOGICAL fxBltUpdateVehicleBullet(FXBULLET* lpBlt);

/////////////////////////////
// struct OBJECT* return functions
/////////////////////////////
extern struct OBJECT* fxBltGetObject(struct OBJECT* lpObj);

/////////////////////////////
// void return functions
/////////////////////////////
extern void fxBltFreeAll();
extern void fxBltSetColor(piCOLORVALUE* lpColor);
extern void fxBltUpdate();

/////////////////////////////
// SINT32 return functions
/////////////////////////////
extern bool fxBltPlaySound(FLOAT64VECTOR* pos, const OBJECT* o);
