#pragma once

#include "pi/pi.h"
#include "fx/fxMgr.h"

struct FXQUAD
{
    FXQUAD* next;
    FXQUAD* prev;

    LOGICAL bDraw:1;
    LOGICAL bStretch:1;
    LOGICAL stretchFixed:1;

    piUV uv[2];

    SINT32 iIndex;
    SINT32 iTexID;
    SINT32 iPclID;

    UINT32 uFlags;
    UINT32 uSpriteNum;

    FLOAT32 distancesqrd;
    FLOAT32 radius;
    FLOAT32 scale;
    FLOAT32 alpha;
    FLOAT32 dragco;
    FLOAT32 lastupdate;

    FLOAT64 fEnergy;

    piVECTOR relposition;
    piVECTOR quad[4];
    piVECTOR coords[4];
    piVECTOR vecVel;
    piVECTOR vecRot;

    FLOAT64VECTOR worldpos;
    FLOAT64VECTOR lastpos;
    FLOAT64VECTOR groundpos;

    struct OBJECTDOF* lpDOF;

    INTFLOAT32 quadScale;
    INTFLOAT32 quadTime;
    INTVECTOR quadRotate;
    INTVECTOR quadVel;
    INTCOLOR quadColor;

    piCOLORVALUE piColor;

    void (*pFnKill)(void* data);
    void* killData;

};

///////////////////////////
// LOGICAL functions
///////////////////////////
extern LOGICAL fxQuadDetachParticle(struct OBJECT* lpObj);
extern LOGICAL fxQuadDraw();
extern LOGICAL fxQuadInit();
extern LOGICAL fxQuadKill();

///////////////////////////
// void functions
///////////////////////////
extern void fxQuadFreeAll();
extern void fxQuadInitParticle(SINT32 iID);
extern void fxQuadSetColor(piLPCOLORVALUE lpColor);
extern void fxQuadSetInvertView(piLPMATRIX lpInvertView);
extern void fxQuadUpdate();

///////////////////////
// void set functions
///////////////////////
extern void fxQuadPopFlags(SINT32 iID, UINT32 uFlag);
extern void fxQuadPushFlags(SINT32 iID, UINT32 uFlag);
extern void fxQuadSet4PtAlpha(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxQuadSet4PtScale(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxQuadSetCanStretch(SINT32 iID, LOGICAL bStretch);
extern void fxQuadSetDOFObject(SINT32 iID, struct OBJECTDOF* lpDOF);
extern void fxQuadSetDragCoeff(SINT32 iID, FLOAT32 fCoeff);
extern void fxQuadSetEnergy(SINT32 iID, FLOAT64 fEnergy);
extern void fxQuadSetGroundPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxQuadSetKillData(SINT32 iID, void* killData);
extern void fxQuadSetParticleID(SINT32 iID, SINT32 iPclID);
extern void fxQuadSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxQuadSetPt1Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxQuadSetPt2Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxQuadSetPt3Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxQuadSetPt4Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxQuadSetPt1Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt2Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt3Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt4Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt1Time(SINT32 iID, FLOAT32 fTime);
extern void fxQuadSetPt2Time(SINT32 iID, FLOAT32 fTime);
extern void fxQuadSetPt3Time(SINT32 iID, FLOAT32 fTime);
extern void fxQuadSetPt4Time(SINT32 iID, FLOAT32 fTime);
extern void fxQuadSetPt1Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt2Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt3Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetPt4Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetRadius(SINT32 iID, FLOAT32 fRadius);
extern void fxQuadSetScale(SINT32 iID, FLOAT32 fScale);
extern void fxQuadSetSpriteNumber(SINT32 iID, UINT32 uSpriteNum);
extern void fxQuadSetUserKillCallback(SINT32 iID, void (*pFn)(void*));
extern void fxQuadSetVelocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadSetRotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxQuadFixStretch(SINT32 ID);

///////////////////////////
// SINT32 return functions
///////////////////////////
extern SINT32 fxQuadGetNewParticle();
extern SINT32 fxQuadPullParticles(SINT32 iPclID, FXQUAD* lpQuadParts[], SINT32 iMaxSize);

////////////////////////////
// FXQUAD* return functions
////////////////////////////
extern FXQUAD* fxQuadGetQuad(SINT32 iID);
