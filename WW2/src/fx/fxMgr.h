#pragma once

#include "pi/pi.h"

#define DEF_MAX_EFFECTS     2048

#define DEF_FXFLAG_RISE             0x00000001 // Particle rises (fireball/smoke,etc...)
#define DEF_FXFLAG_INFINITE         0x00000002 // Run infinitely
#define DEF_FXFLAG_INTVELOCITY      0x00000004 // Interpolate velocity
#define DEF_FXFLAG_INTCOLOR         0x00000008 // Interpolate color
#define DEF_FXFLAG_INTALPHA         0x00000010 // Interpolate alpha
#define DEF_FXFLAG_INTRADIUS        0x00000020 // Interpolate radius
#define DEF_FXFLAG_GLOW             0x00000040 // Particle glows
#define DEF_FXFLAG_MYCOLOR          0x00000080 // Ignore game color.
#define DEF_FXFLAG_INTATTENUATION   0x00000100 // Interpolate light attenuation
#define DEF_FXFLAG_ROTATE           0x00000200 // Particle has rotation
#define DEF_FXFLAG_USEGRAVITY       0x00000400 // Use gravity
#define DEF_FXFLAG_USESPRITENUM     0x00000800 // User sprite number to be used
#define DEF_FXFLAG_ATTACHAMMO       0x00001000 // FX has ammo structure.
#define DEF_FXFLAG_OBJECT           0x00002000 // Particle is a 3D OBJECT*.
#define DEF_FXFLAG_USEDRAG          0x00004000 // FX particle has drag.
#define DEF_FXFLAG_DOF              0x00008000 // Updated with DOF.
#define DEF_FXFLAG_INTROTATE        0x00010000 // Interpolate rotate vector.
#define DEF_FXFLAG_NOPHYSICS        0x00020000 // Do not apply velocity/drag, etc...
#define DEF_FXFLAG_STOPATGROUND     0x00040000 // Stop at ground position
#define DEF_FXFLAG_USEMATERIAL      0x00080000 // Use material
#define DEF_FXFLAG_LOCKDOFONEFRAME  0x00100000 // Lock draw to DOF for one frame
#define DEF_FXFLAG_VARYVELOCITY     0x00200000 // Add varying velocity per update.
#define DEF_FXFLAG_FLAT             0x00400000 // Particle flag on xy plane

enum ENUMFXTYPE
{
    IDE_FXTYPE_NOT_USED,
    IDE_FXTYPE_WATER,
    IDE_FXTYPE_EXPLOSION,
    IDE_FXTYPE_LIGHT,
    IDE_FXTYPE_PARTICLE,
    NUMBER_OF_FXTYPES
};

struct FXSORT
{
    ENUMFXTYPE type;
    SINT32 index;
    FLOAT32 dist2;

};

// Interpolate UINT32
struct INTUINT32
{
    UINT32 p1;  // Initial state
    UINT32 p2;  // Second state
    UINT32 p3;  // Third state
    UINT32 p4;  // Final state
};

// Interpolate FLOAT32
struct INTFLOAT32
{
    FLOAT32 p1;  // Initial state
    FLOAT32 p2;  // Second state
    FLOAT32 p3;  // Third state
    FLOAT32 p4;  // Final state
};

// Interpolate color
struct INTCOLOR
{
    piCOLORVALUE p1;
    piCOLORVALUE p2;
    piCOLORVALUE p3;
    piCOLORVALUE p4;
};

// Interpolate vector
struct INTVECTOR
{
    piVECTOR p1;
    piVECTOR p2;
    piVECTOR p3;
    piVECTOR p4;
};

////////////////////////////
// LOGICAL return functions
////////////////////////////
extern LOGICAL fxMgrCreateMaterial();
extern LOGICAL fxMgrDetach(struct OBJECT* lpObj);
extern LOGICAL fxMgrDraw(LOGICAL bBeforeAlpha);
extern LOGICAL fxMgrInit();
extern void fxMgrKill();
extern LOGICAL fxMgrUpdate();

///////////////////
// void functions
///////////////////
extern void fxMgrDetachAmmo(void const* pAmmo);

extern void fxMgrFreeAll();
extern void fxMgrSetInvertView();
extern void fxMgrSetLight();
extern void fxMgrSetWind(FLOAT32 x, FLOAT32 y, FLOAT32 z);
extern void fxMgrSortFX();
extern void fxMgrUpdateSound();

/////////////////////
// SINT32 functions
/////////////////////
extern SINT32 fxMgrGetMaterial();

////////////////////////
// piVECTOR* functions
////////////////////////
extern piVECTOR* fxMgrGetWind();
