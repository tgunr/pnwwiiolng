#pragma once

#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obmain.h"

#define DEF_MAX_CHAR_LAYOUT 256

//////////////////////////
// ENUMERATION types here.
//////////////////////////
enum ENUM_WW2_FONTS
{
    IDE_WW2_FONT_VERDANA16,
    IDE_WW2_FONT_VERDANA24,
    NUMBER_OF_WW2_FONTS
};

/////////////////////////
// STRUCTURES here.
/////////////////////////
struct LOCCHARLAYOUT
{
    SINT32 x;
    SINT32 y;
    SINT32 iWidth;

    FLOAT32 u1;
    FLOAT32 u2;
    FLOAT32 v1;
    FLOAT32 v2;

};

struct LOCFONTLAYOUT
{
    SINT32 iAvgWidth;
    SINT32 iHeight;

    LOCCHARLAYOUT charLayout[DEF_MAX_CHAR_LAYOUT];
};

struct LOCCHAR
{
    piLVERTEX lVert[6];

    SINT32 iTexID;

};

/////////////////////////////
// LOGICAL return functions
/////////////////////////////
extern LOGICAL rntextLoc3DRender(LOCCHAR* lpVert, SINT32 iLength);
extern LOGICAL rntextLoc3DString(UCHAR* pStr,
                          UINT32 uColor,
                          ENUM_WW2_FONTS enumFont,
                          LOCCHAR* lpVert,
                          SINT32 iNumVert);
extern LOGICAL rntextLocInit();
extern LOGICAL rntextLocKill();
extern LOGICAL rntextLocValidChar(UCHAR c);

///////////////////////////
// SINT32 return functions
///////////////////////////
extern SINT32 rntextLocGetCharWidth(SCHAR c, ENUM_WW2_FONTS enumFont);
extern SINT32 rntextLocGetHeight(ENUM_WW2_FONTS enumFont);
extern SINT32 rntextLocGetWidth(SCHAR* pszText, ENUM_WW2_FONTS enumFont);

extern SINT32 rntextLocOutput2d(SCHAR* pStr,
                         SINT32 x,
                         SINT32 y,
                         FLOAT32 z,
                         SINT32 iWidth,
                         UINT32 uColor,
                         ENUM_WW2_FONTS enumFont);

/////////////////////////////////
// LOCCHARLAYOUT* return functions
/////////////////////////////////
extern LOCCHARLAYOUT* rntextLocGetLayout(ENUM_WW2_FONTS enumFont, UCHAR c);
