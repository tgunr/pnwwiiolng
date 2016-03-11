#pragma once

#include "fx/fxMgr.h"
#include "trn/trncd.h"
#include "veh/vehmain.h"

extern LOGICAL fxUtlInit();
extern LOGICAL fxUtlTerrainCollide(FASTCOLLIDE* pFastCollision);
extern LOGICAL fxUtlGetRandomHitSound(const SCHAR* material,
                               const SCHAR** sound,
                               FLOAT32* minvolume,
                               FLOAT32* maxvolumeenergy);

extern void fxUtlApplyDrag(piVECTOR* lpVel, FLOAT32* pDragCoef, FLOAT32* pLastUpdate);
extern void fxUtlApplyVelocity(piVECTOR* lpVel,
                        FLOAT64VECTOR* pPos,
                        FLOAT32* pLastUpdate,
                        UINT32 uFlags);
extern void fxUtlFromToRotation(piVECTOR* lpFrom, piVECTOR* lpTo, piMATRIX* lpMat);
extern void fxUtlGetDOFDirection(OBJECTDOF* lpDOF, piMATRIX* lpMat, piVECTOR* pDir);
extern void fxUtlGetDOFPos(OBJECTDOF* lpDOF, piMATRIX* lpMat, FLOAT64VECTOR* pPos);
extern void fxUtlGetGroundPos(piVECTOR* pFrom, FLOAT64VECTOR* pGound);
extern void fxUtlGetTextureUV(piUV[2], SINT32 iSpriteNum);
extern void fxUtlGetTextureUV(piUV uv[2], const char* str);
extern void fxUtlInterpolateColor(INTFLOAT32* lpTime, INTCOLOR* lpIn, piCOLORVALUE* lpOut);
extern void fxUtlInterpolateRadius(INTFLOAT32* lpTime, INTFLOAT32* lpIn, FLOAT32* pOut);
extern void fxUtlInterpolateAlpha(INTFLOAT32* lpTime, INTCOLOR* lpIn, piCOLORVALUE* lpOut);
extern void fxUtlInterpolateVector(INTFLOAT32* lpTime, INTVECTOR* lpIn, piVECTOR* lpOut);
extern void fxUtlKill();
extern void fxUtlRotateZ(piVECTOR vecOut[], piMATRIX* lpRot);
extern void fxUtlSetMatrixInTree(OBJECT* o, const piMATRIX* mat);
extern void fxUtlSetTexture();
extern void fxUtlUpdatePosition(piVECTOR* pvecPos, FLOAT64VECTOR* pvecOriginal);

extern const char* fxUtlGetBulletEffectFromMaterial(const char* material);

extern FLOAT32 fxUtlGetCos(SINT32 iDegrees);
extern FLOAT32 fxUtlGetSin(SINT32 iDegrees);
extern FLOAT32 fxUtlGetDistanceSqrd(piVECTOR* pvecPos);

extern OBJECT* fxUtlGetObject(OBJECT* lpObj);

extern SINT32 fxUtlGetBillowSmoke();
extern SINT32 fxUtlGetFire();
extern SINT32 fxUtlGetFireC();
extern SINT32 fxUtlGetHitSmoke();
extern SINT32 fxUtlGetVehiclePos(vehVehicle*pVeh, FLOAT64VECTOR* pPos);
extern SINT32 fxUtlGetVehicleVel(vehVehicle*pVeh, FLOAT64VECTOR* pVec);
extern SINT32 fxUtlGetBulletType(const SCHAR* material);
