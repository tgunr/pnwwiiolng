#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: rntextLoc.c
// Description: This module should eventually replace rntext.c. This
// module handles localization text.
//////////////////////////////////////////////////////////////////////
#include "rn/rntextloc.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "ld/ldtex.h"

#define DEF_NUM_TEX_PER_FONT    1

#define DEF_NUM_BITMAPS     (NUMBER_OF_WW2_FONTS * DEF_NUM_TEX_PER_FONT)

static SINT32 g_iTextIDS[DEF_NUM_BITMAPS];

static LOCFONTLAYOUT g_Layout[DEF_NUM_BITMAPS];

static piSURFACEDESC g_ddText[DEF_NUM_BITMAPS];

static LOGICAL g_bInitialized = FALSE;

static SCHAR* g_pFontPCXNames[DEF_NUM_BITMAPS]=
{
    "\\fonts\\verdana\\verdana16.bmp",
    "\\fonts\\verdana\\verdana24.bmp",
};

static SCHAR* g_pFontDATNames[DEF_NUM_BITMAPS]=
{
    "\\fonts\\verdana\\verdana16.dat",
    "\\fonts\\verdana\\verdana24.dat",
};

/////////////////////////////////////
// Function: rntextLocInit
// Description: Initialize fonts.
/////////////////////////////////////
LOGICAL rntextLocInit()
{
    int i;
    SINT32 hHandle = -1;
    UINT32 uColor;
   // LOCCHARLAYOUT* lpInfo = NULL;

	g_bInitialized = TRUE;

    if(g_bInitialized)
    {
        return TRUE;
    }

    memset(g_iTextIDS, -1, sizeof(SINT32) * DEF_NUM_BITMAPS);

    uColor = piRGBA_SETALPHA(colorBLACK, 1);

    for(i = 0; i < DEF_NUM_BITMAPS; i++)
    {
        hHandle = fioOpen(g_pFontDATNames[i]);

        if(hHandle == -1)
        {
            Write_Debug("[rntextLocInit] %s FAILED", g_pFontDATNames[i]);
            return FALSE;
        }

        fioRead(hHandle, &g_Layout[i], sizeof(LOCFONTLAYOUT));
        fioClose(hHandle);

		ldpixSetTextureReduction(FALSE);
        g_iTextIDS[i] = ldpixLoadImageFileToSurface(g_pFontPCXNames[i],
                                                    uColor,
                                                    LDPIX_NO_DXT);
      	ldpixSetTextureReduction(TextureReduction);

        if(!piLockTextureMemorySurface(g_iTextIDS[i], 0, &g_ddText[i]))
        {
            Write_Debug("[rntextLocInit] %s FAILED", g_pFontPCXNames[i]);
            return FALSE;
        }

        piUnlockTextureMemorySurface(g_iTextIDS[i], 0);
    }

    g_bInitialized = TRUE;

    return TRUE;
}

/////////////////////////////////////////////
// Function: rntextLocKill
// Description: Kill routine for this module.
/////////////////////////////////////////////
LOGICAL rntextLocKill()
{
    int i;

    if(g_bInitialized == FALSE)
        return TRUE;

    for(i = 0; i < DEF_NUM_BITMAPS; i++)
    {
        piFreeMipTextures(g_iTextIDS[i]);
    }

    g_bInitialized = FALSE;

    return TRUE;
}

/////////////////////////////////////////////
// Function: rntextLocGetHeight
// Description: Return font height.
/////////////////////////////////////////////
SINT32 rntextLocGetHeight(ENUM_WW2_FONTS enumFont)
{
    return g_Layout[enumFont * DEF_NUM_TEX_PER_FONT].iHeight;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLocGetWidth
// Description: Return the string width for a particular font.
//////////////////////////////////////////////////////////////////////
SINT32 rntextLocGetWidth(SCHAR* pszText, ENUM_WW2_FONTS enumFont)
{
    SINT32 i;
    SINT32 iLength = 0;
    SINT32 iWidth = 0;

    iLength = strlen(pszText);

	for(i = 0; i < iLength; i++)
	{
        iWidth += rntextLocGetLayout(enumFont, pszText[i])->iWidth;
    }

	return iWidth;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLocGetCharWidth
// Description: Return the char width for a particular font.
//////////////////////////////////////////////////////////////////////
SINT32 rntextLocGetCharWidth(SCHAR c, ENUM_WW2_FONTS enumFont)
{
    return rntextLocGetLayout(enumFont, c)->iWidth;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLoc3DString
// Description: Wrap 3D vertices around a specific text texture.
//////////////////////////////////////////////////////////////////////
LOGICAL rntextLoc3DString(UCHAR* pStr,
                          UINT32 uColor,
                          ENUM_WW2_FONTS enumFont,
                          LOCCHAR* lpVert,
                          SINT32 iLength)
{
//    SCHAR c = 0;

    SINT32 i = 0;
 //   SINT32 r = 0;
//SINT32 iFontID = -1;

    FLOAT32 fWidth = 0.0f;
    FLOAT32 x = 0.0f;
    FLOAT32 y = (FLOAT32)rntextLocGetHeight(enumFont);

    LOCCHARLAYOUT* lpInfo = NULL;

    if(lpVert == NULL)
        return FALSE;

    if(pStr == NULL)
        return FALSE;

    for(i = 0; i < iLength; i++)
    {
        if(!rntextLocValidChar(pStr[i]))
            continue;

        lpInfo = rntextLocGetLayout(enumFont, pStr[i]);

        if(lpInfo == NULL)
            return FALSE;

        lpVert[i].iTexID = g_iTextIDS[enumFont];

        fWidth = (FLOAT32)lpInfo->iWidth;

        lpVert[i].lVert[0].x = x;
        lpVert[i].lVert[0].y = y;
        lpVert[i].lVert[0].z = 0.0f;
        lpVert[i].lVert[0].specular = colorALPHA100;
        lpVert[i].lVert[0].color = uColor;
        lpVert[i].lVert[0].tu = lpInfo->u1;
        lpVert[i].lVert[0].tv = lpInfo->v2;

        lpVert[i].lVert[1].x = x + fWidth;
        lpVert[i].lVert[1].y = y;
        lpVert[i].lVert[1].z = 0.0f;
        lpVert[i].lVert[1].specular = colorALPHA100;
        lpVert[i].lVert[1].color = uColor;
        lpVert[i].lVert[1].tu = lpInfo->u2;
        lpVert[i].lVert[1].tv = lpInfo->v2;

        lpVert[i].lVert[2].x = x + fWidth;
        lpVert[i].lVert[2].y = 0.0f;
        lpVert[i].lVert[2].z = 0.0f;
        lpVert[i].lVert[2].specular = colorALPHA100;
        lpVert[i].lVert[2].color = uColor;
        lpVert[i].lVert[2].tu = lpInfo->u2;
        lpVert[i].lVert[2].tv = lpInfo->v1;

        lpVert[i].lVert[3].x = x + fWidth;
        lpVert[i].lVert[3].y = 0.0f;
        lpVert[i].lVert[3].z = 0.0f;
        lpVert[i].lVert[3].specular = colorALPHA100;
        lpVert[i].lVert[3].color = uColor;
        lpVert[i].lVert[3].tu = lpInfo->u2;
        lpVert[i].lVert[3].tv = lpInfo->v1;

        lpVert[i].lVert[4].x = x;
        lpVert[i].lVert[4].y = 0.0f;
        lpVert[i].lVert[4].z = 0.0f;
        lpVert[i].lVert[4].specular = colorALPHA100;
        lpVert[i].lVert[4].color = uColor;
        lpVert[i].lVert[4].tu = lpInfo->u1;
        lpVert[i].lVert[4].tv = lpInfo->v1;

        lpVert[i].lVert[5].x = x;
        lpVert[i].lVert[5].y = y;
        lpVert[i].lVert[5].z = 0.0f;
        lpVert[i].lVert[5].specular = colorALPHA100;
        lpVert[i].lVert[5].color = uColor;
        lpVert[i].lVert[5].tu = lpInfo->u1;
        lpVert[i].lVert[5].tv = lpInfo->v2;

        x += fWidth;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLocValidChar
// Description: Add remove chars which are valid.
//////////////////////////////////////////////////////////////////////
LOGICAL rntextLocValidChar(UCHAR c)
{
    switch(c)
    {
    case 10:
    case 13:
        {
            return FALSE;
        }
    };

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLocOutput2d
// Description: Return actual number of characters output
// (ignore spaces and unprintable characters)
//////////////////////////////////////////////////////////////////////
SINT32 rntextLocOutput2d(SCHAR* pStr,
                         SINT32 iXin,
                         SINT32 iYin,
                         FLOAT32 z,
                         SINT32 iWidth,
                         UINT32 uColor,
                         ENUM_WW2_FONTS enumFont)
{
    SINT32 len = strlen(pStr);
    if(len > 256)
        len = 256;
    else if (len == 0)
        return 0;

    SINT32 iHeight;
    SINT32 i, j, vcnt, icnt;
  //  SINT32 r = 0;
//SINT32 iFontID = -1;

    FLOAT32 fLeft;

 //   UCHAR c = 0;

    UINT32 uAlpha;

    static piTLVERTEX tlVert[128];
    static piINDICES Indices[64];
    LOCCHARLAYOUT* lpInfo = NULL;

#ifdef DIRECT3D
	FLOAT32 x = (FLOAT32)iXin - 0.5f;
	FLOAT32 y = (FLOAT32)iYin - 0.5f;
#else
	FLOAT32 x = (FLOAT32)iXin;
	FLOAT32 y = (FLOAT32)iYin;
#endif

	piDisableMaterial();

	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
	piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);

    uAlpha = piRGBA_GETALPHA(uColor);

    if(uAlpha == 0)
    {
       uColor |= colorALPHA100;
    }

    fLeft = x;
    vcnt = icnt = 0;

    iHeight = rntextLocGetHeight(enumFont);

    piSetActiveTexture(g_iTextIDS[enumFont], 0);

    for(i = j = 0; i < len; i++)
    {
        lpInfo = rntextLocGetLayout(enumFont, pStr[i]);

        if(lpInfo == NULL)
        {
	        piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	        piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

            return FALSE;
        }

        iWidth -= lpInfo->iWidth;

        if(iWidth < 0)
            break;

        j++;

        tlVert[vcnt].sx = fLeft;
        tlVert[vcnt].sy = y;
        tlVert[vcnt].sz = z;
        tlVert[vcnt].rhw = 1.0f;
        tlVert[vcnt].color = uColor;
        tlVert[vcnt].specular = colorALPHA100;
        tlVert[vcnt].tu = lpInfo->u1;
        tlVert[vcnt].tv = lpInfo->v1;

        Indices[icnt].v1 = vcnt;
        Indices[icnt+1].v3 = vcnt++;

        tlVert[vcnt].sx = fLeft;
        tlVert[vcnt].sy = y - (FLOAT32)iHeight;
        tlVert[vcnt].sz = z;
        tlVert[vcnt].rhw = 1.0f;
        tlVert[vcnt].color = uColor;
        tlVert[vcnt].specular = colorALPHA100;
        tlVert[vcnt].tu = lpInfo->u1;
        tlVert[vcnt].tv = lpInfo->v2;

        Indices[icnt].v2 = vcnt++;

        tlVert[vcnt].sx = fLeft + (FLOAT32)lpInfo->iWidth;
        tlVert[vcnt].sy = y - (FLOAT32)iHeight;
        tlVert[vcnt].sz = z;
        tlVert[vcnt].rhw = 1.0f;
        tlVert[vcnt].color = uColor;
        tlVert[vcnt].specular = colorALPHA100;
        tlVert[vcnt].tu = lpInfo->u2;
        tlVert[vcnt].tv = lpInfo->v2;

        Indices[icnt++].v3 = vcnt;
        Indices[icnt].v1 = vcnt++;

        tlVert[vcnt].sx = fLeft + (FLOAT32)lpInfo->iWidth;
        tlVert[vcnt].sy = y;
        tlVert[vcnt].sz = z;
        tlVert[vcnt].rhw = 1.0f;
        tlVert[vcnt].color = uColor;
        tlVert[vcnt].specular = colorALPHA100;
        tlVert[vcnt].tu = lpInfo->u2;
        tlVert[vcnt].tv = lpInfo->v1;

        Indices[icnt++].v2 = vcnt++;

        if(vcnt == 96)
        {
            if(!piRenderIndexScene(piPT_TRIANGLELIST,
                                   piVT_TLVERTEX,
                                   tlVert,
                                   vcnt,
                                   (UINT16*)Indices,
                                   icnt * 3))
            {
	            Write_Debug("[rntextLocOutput2d] piRenderIndexScene FAILED");
            }

            vcnt = icnt = 0;
        }

        fLeft += (FLOAT32)lpInfo->iWidth;
    }

	if (vcnt != 0)
	{
		if (!piRenderIndexScene(piPT_TRIANGLELIST,
                                piVT_TLVERTEX,
                                tlVert,
                                vcnt,
                                (UINT16*)Indices,
                                icnt * 3))
		{
            Write_Debug("[rntextLocOutput2d] piRenderIndexScene FAILED");
		}
	}

	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

    piDisableTexture();

	return j;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextLoc3DRender
// Description: Render 3D text vertices. Calling routinues
// are responsible for setting appropriate filtering with...
//
//  piSetRenderStageState(0, piTSS_MINFILTER, piTFN_LINEAR);
//  piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_LINEAR);
//  piSetRenderStageState(0, piTSS_MIPFILTER, 0);
//
// rntextLoc3DRender();
//
// piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
// piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);
// piSetRenderStageState(0, piTSS_MIPFILTER, piTEXTUREMIP_DEFAULT);
//
//////////////////////////////////////////////////////////////////////
LOGICAL rntextLoc3DRender(LOCCHAR* lpVert, SINT32 iLength)
{
    SINT32 i;
    LOGICAL bRetVal = TRUE;

    piDisableTexture();
    piDisableMaterial();

    if(lpVert == NULL)
    {
        Write_Debug("[rntextLoc3DRender] ***** lpVert == NULL *****");
        return FALSE;
    }

    piSetActiveTexture(lpVert[0].iTexID, 0);

    for(i = 0; i < iLength; i++)
    {
        if(!piRenderScene(piPT_TRIANGLELIST,
                          piVT_LVERTEX,
                          lpVert[i].lVert,
                          6))
        {
            Write_Debug("[rntextLoc3DRender] piRenderScene FAILED!");
            bRetVal = FALSE;
            break;
        }
    }

    piDisableTexture();

    return bRetVal;
}

//////////////////////////////////////////////////////////////////////
// Function: rntextGetLayout
// Description: Return proper character layout.
//////////////////////////////////////////////////////////////////////
LOCCHARLAYOUT* rntextLocGetLayout(ENUM_WW2_FONTS enumFont, UCHAR c)
{
    return &g_Layout[enumFont].charLayout[c];
}
