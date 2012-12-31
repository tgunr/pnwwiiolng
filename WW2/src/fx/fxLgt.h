#pragma once

#include "pi/pi.h"
#include "fx/fxMgr.h"

enum LIGHTTYPE
{
    IDE_LIGHT_DONTUSE,
    IDE_LIGHT_MUZZLEFLASH,
    IDE_LIGHT_BOMB,
    NUMBER_OF_LIGHT_TYPES
} ;

struct FXLIGHTS
{
    FXLIGHTS* next;
    FXLIGHTS* prev;

    LOGICAL bDraw;
    LOGICAL bOn;

    piLIGHT lgtLight;

    SINT32 iIndex;

    UINT32 uFlags;

    piCOLORVALUE piColor;

    INTFLOAT32 lgtTime;

    INTFLOAT32 lgtAttn;

    INTCOLOR lgtColor;

    FLOAT32 fDistanceSqrd;
    FLOAT32 fAttenuation;

    FLOAT64VECTOR vecPos;

    piVECTOR vecPt;
    piVECTOR direction;

    LIGHTTYPE enumType;

};

////////////////////////////
// LOGICAL return functions
////////////////////////////
extern LOGICAL fxLgtAddLight(FXLIGHTS* lpLgt);
extern LOGICAL fxLgtCreateMuzzleLight(FLOAT64VECTOR* pPos, piVECTOR* dir);
extern LOGICAL fxLgtDraw(SINT32 iIndex);
extern LOGICAL fxLgtGetLight(FXLIGHTS** lpLgt);
extern LOGICAL fxLgtInit();
extern LOGICAL fxLgtUpdateCallback(LIGHT* lpLgt);

////////////////////////////
// void return functions
////////////////////////////
extern void fxLgtGetList(void* lpLgtSort, SINT32* pIndex);
extern void fxLgtInitLight(SINT32 iID);
extern void fxLgtInterpolateLight(FXLIGHTS* lpLgt);
extern void fxLgtKill();
extern void fxLgtSetInvertView(piMATRIX* lpView);
extern void fxLgtUpdate();

///////////////////////
// void set functions
///////////////////////
extern void fxLgtPopFlags(SINT32 iID, UINT32 uFlags);
extern void fxLgtPushFlags(SINT32 iID, UINT32 uFlags);
extern void fxLgtSet4PtAlpha(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxLgtSet4PtAttenuation(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4);
extern void fxLgtSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z);
extern void fxLgtSetPt1Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxLgtSetPt2Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxLgtSetPt3Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxLgtSetPt4Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b);
extern void fxLgtSetPt1Time(SINT32 iID, FLOAT32 fTime);
extern void fxLgtSetPt2Time(SINT32 iID, FLOAT32 fTime);
extern void fxLgtSetPt3Time(SINT32 iID, FLOAT32 fTime);
extern void fxLgtSetPt4Time(SINT32 iID, FLOAT32 fTime);
extern void fxLgtSetType(SINT32 iID, LIGHTTYPE enumType);
extern void fxLgtSetDirection(SINT32 ID, piVECTOR* dir);
extern void fxLgtSetLights();

///////////////////////////
// SINT32 return functions
///////////////////////////
extern SINT32 fxLgtGetNewLight();
