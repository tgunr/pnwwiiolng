#include <syscfg.h> // Path is project specific.
//
#include <math.h>
#include <wchar.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldpix.h"
#include "ld/ldtex.h"
#include "rn/rntext.h"
#include "fio/fio.h"
//#include "rn/rntextLoc.h"
#include "ld/ldtexture.h"

static int westernGetHeight(int f);
static int westernInit(void);
static void westernKill(void);
static SINT32 westernGetStringWidth(const WCHAR*s, SINT32 f);
static LOGICAL western3DString(const WCHAR* pszText, UINT32 uColor, SINT32 f, piLVERTEX* plVertices, SINT32 iNumVertices);
static SINT32 westernGetTexId(SINT32 f);
static int chinaInit(void);
static void chinaKill(void);
static int chinaGetHeight(int Font);
#if(0)
static int chinaGetWidth(const WCHAR* Text, int Font);
static SINT32 chinaGetCharWidth(const WCHAR c, SINT32 f);
static int chinaOutput2d(const WCHAR* Text, int x, int y, float z, int Width, UINT32 Color, int Font);
static int chinaOutputToTexture(int texid, int miplevel, const WCHAR* Text, int x, int y, int Width, UINT32 TransparentColor, int Font);
static int chinaColorTextToTexture(SINT32 iTexID, SINT32 iMipLevel, WCHAR* pszText, SINT32 x, SINT32 y, SINT32 iWidth, UINT32 uTransparentColor, SINT32 iFont, UINT32 uTextColor);
static int chinaOutputToSurface(piSURFACEDESC* piwork, const WCHAR* Text, int x, int y, int Width, UINT32 TransparentColor, int Font);
#endif
static SINT32 chinaGetStringWidth(const WCHAR* s, SINT32 f);
static LOGICAL china3DString(const WCHAR* pszText, UINT32 uColor, SINT32 iFont, piLVERTEX* pVertices, SINT32 iNumVertices);
static SINT32 chinaGetTexId(SINT32 iFont);

static bool isChina = false;

int rntextGetHeight(int Font)
{
    if ( isChina )
    {
        return chinaGetHeight(Font);
    }
    else
    {
        return westernGetHeight(Font);
    }
}

int rntextInit(void)
{
    isChina = gLanguage==5;
    if ( isChina )
    {
        return chinaInit();
    }
    else
    {
        return westernInit();
    }
}

void rntextKill(void)
{
    if ( isChina )
    {
        chinaKill();
    }
    else
    {
        westernKill();
    }
}

SINT32 rntextGetStringWidth(const WCHAR*s, SINT32 n)
{
    if ( isChina )
    {
        return chinaGetStringWidth(s,n);
    }
    else
    {
        return westernGetStringWidth(s,n);
    }
}

LOGICAL rntext3DString(const WCHAR* pszText, UINT32 uColor, SINT32 iFont, piLVERTEX* plVertices, SINT32 iNumVertices)
{
    if ( isChina )
    {
        return china3DString(pszText,uColor,iFont,plVertices,iNumVertices);
    }
    else
    {
        return western3DString(pszText,uColor,iFont,plVertices,iNumVertices);
    }
}

SINT32 rntextGetTexId(SINT32 iFont)
{
    if ( isChina )
    {
        return chinaGetTexId(iFont);
    }
    else
    {
        return westernGetTexId(iFont);
    }
}


// *************** new 3d text drawing

#include "draw/fontadaptor.h"

static FontAdaptor* iconfontAdaptor = nullptr;
static FontAdaptor* gunsightfontAdaptor = nullptr;

static int westernGetHeight(int f)
{
    FontAdaptor* fa = (f == 1) ? gunsightfontAdaptor : iconfontAdaptor;
    return fa->getHeight();
}

static int westernInit(void)
{
    FTFontWestern* f = new FTFontWestern( );
    f->FaceSize( 26 );
    iconfontAdaptor = new FontAdaptor(f);

    f = new FTFontWestern( );
    f->FaceSize( 18 );
    gunsightfontAdaptor = new FontAdaptor(f);
    return 1;
}

static void westernKill(void)
{
    if(iconfontAdaptor)
        delete iconfontAdaptor;

    if(gunsightfontAdaptor)
        delete gunsightfontAdaptor;
}

static SINT32 westernGetStringWidth(const WCHAR*s, SINT32 f)
{
    FontAdaptor* fa = (f == 1) ? gunsightfontAdaptor : iconfontAdaptor;
    return fa->getStringWidth(s);
}

// ALWAYS 6 vertex per char - TRIANGLE list

static LOGICAL western3DString(const WCHAR* pszText, UINT32 uColor, SINT32 f, piLVERTEX* plVertices, SINT32 iNumVertices)
{
    FontAdaptor* fa = (f == 1) ? gunsightfontAdaptor : iconfontAdaptor;
    return fa->get3DString(pszText, uColor, plVertices, iNumVertices);
}

static SINT32 westernGetTexId(SINT32 f)
{
    FontAdaptor* fa = (f == 1) ? gunsightfontAdaptor : iconfontAdaptor;
    return fa->getTextureId();
}


#define TEXTURE_V1_FUDGE	0.001f

static LOGICAL textInitted = FALSE;
static SCHAR* FontPCXName = "Fonts\\iconfont.bmp";
static SCHAR* FontDATName = "Fonts\\iconfont.dat";

static SINT32 FontTexID = -1;
static FONTLAYOUT FontLayout;
static piSURFACEDESC ddtext;

static int chinaInit(void)
{
	if (textInitted)
		return(TRUE);

	UINT32 color = piRGBA_SETALPHA(colorBLACK,1);

	ldpixSetTextureReduction (FALSE);

	SINT32 stream = fioOpen(FontDATName);
	if (stream == -1)
	{
		Write_Debug("Could not find the text font data file");
		return(FALSE);
	}

	fioRead(stream, &FontLayout, sizeof(FONTLAYOUT));
	fioClose(stream);

	FontTexID = ldpixLoadImageFileToSurface(FontPCXName, color, LDPIX_FLIP);

	textInitted = TRUE;

	return(TRUE);
}

static void chinaKill(void)
{
	if(!textInitted)
		return;

	piFreeMipTextures(FontTexID);

	FontTexID = -1;
	textInitted = FALSE;

    Write_Log(LOG_OLD, LOG_INFO, "[rntextKill]");
}

static int chinaGetHeight(int Font)
{
	return(FontLayout.Height);
}

#if(0)
static int chinaGetWidth(const WCHAR* Text, int Font)
{
	int len = wcslen(Text);
	int width = 0;
	UINT16 ch = 0;

	for(int i = 0 ; i < len ; i++)
	{
		ch = chatGetHandleCharacterSymbol(Text[i]);

		if(ch < MAX_CHAR_LAYOUT)
			width += FontLayout.CharLayout[ch].width;
	}

	return(width);
}
#endif

#if(0)
static SINT32 chinaGetCharWidth(const WCHAR c, SINT32 f)
{
	UINT16 index = 0;
	SINT32 width = 0;

	index = chatGetHandleCharacterSymbol(c);

	if(index < MAX_CHAR_LAYOUT)
		width = FontLayout.CharLayout[index].width;

	return width;
}
#endif

static SINT32 chinaGetStringWidth(const WCHAR* s, SINT32 f)
{
	SINT32 width = 0;
	UINT16 charIdx;

	for(; *s; s++)
	{
		charIdx = chatGetHandleCharacterSymbol(*s);
		if(charIdx < MAX_CHAR_LAYOUT)
			width += FontLayout.CharLayout[charIdx].width;
	}

    wprintf(L"rntextGetStringWidth s=%ls f=%d\n",s,width);
	return width;
}

///////////////////////////////////////////////////
// FUNCTION: rntext3DString
// This function sets the vertices UV coordinates
// to the correct UV coordinates for the font.
// The calling routine will set the texture as the
// active texture, and render these vertices. The
// number of vertices should be 6 * number of characters.
///////////////////////////////////////////////////
static LOGICAL china3DString(const WCHAR* pszText,
                       UINT32 uColor,
                       SINT32 iFont,
                       piLVERTEX* pVertices,
                       SINT32 iNumVertices)
{
    UINT16 c = 0;
    SINT32 i = 0;
    SINT32 iCharIndex = 0;

    FLOAT32 fWidth = 0.0f;
    FLOAT32 x = 0.0f;
    FLOAT32 y = (FLOAT32)rntextGetHeight(iFont);

    CHARLAYOUT* pCharLayout = nullptr;

    if(pVertices == nullptr)
        return FALSE;

    if(pszText == nullptr)
        return FALSE;

    if(iNumVertices == 0)
        return FALSE;

    for(i = 0; i < iNumVertices; i += 6)
    {
		c = chatGetHandleCharacterSymbol(pszText[iCharIndex++]);

        if(c >= MAX_CHAR_LAYOUT)
			continue;

        pCharLayout = &FontLayout.CharLayout[c];

        if(pCharLayout == nullptr)
            return FALSE;

        fWidth = (FLOAT32)pCharLayout->width;

        pVertices[i + 0].x = x;
        pVertices[i + 0].y = y;
        pVertices[i + 0].z = 0.0f;
        pVertices[i + 0].specular = colorALPHA100;
        pVertices[i + 0].color = uColor;
        pVertices[i + 0].tu = pCharLayout->u1;
        pVertices[i + 0].tv = pCharLayout->v2;

        pVertices[i + 1].x = x + fWidth;
        pVertices[i + 1].y = y;
        pVertices[i + 1].z = 0.0f;
        pVertices[i + 1].specular = colorALPHA100;
        pVertices[i + 1].color = uColor;
        pVertices[i + 1].tu = pCharLayout->u2;
        pVertices[i + 1].tv = pCharLayout->v2;

        pVertices[i + 2].x = x + fWidth;
        pVertices[i + 2].y = 0.0f;
        pVertices[i + 2].z = 0.0f;
        pVertices[i + 2].specular = colorALPHA100;
        pVertices[i + 2].color = uColor;
        pVertices[i + 2].tu = pCharLayout->u2;
        pVertices[i + 2].tv = pCharLayout->v1;

        pVertices[i + 3].x = x + fWidth;
        pVertices[i + 3].y = 0.0f;
        pVertices[i + 3].z = 0.0f;
        pVertices[i + 3].specular = colorALPHA100;
        pVertices[i + 3].color = uColor;
        pVertices[i + 3].tu = pCharLayout->u2;
        pVertices[i + 3].tv = pCharLayout->v1;

        pVertices[i + 4].x = x;
        pVertices[i + 4].y = 0.0f;
        pVertices[i + 4].z = 0.0f;
        pVertices[i + 4].specular = colorALPHA100;
        pVertices[i + 4].color = uColor;
        pVertices[i + 4].tu = pCharLayout->u1;
        pVertices[i + 4].tv = pCharLayout->v1;

        pVertices[i + 5].x = x;
        pVertices[i + 5].y = y;
        pVertices[i + 5].z = 0.0f;
        pVertices[i + 5].specular = colorALPHA100;
        pVertices[i + 5].color = uColor;
        pVertices[i + 5].tu = pCharLayout->u1;
        pVertices[i + 5].tv = pCharLayout->v2;

        x += fWidth;
    }

    return TRUE;
}

// Return actual number of characters output (ignore spaces and unprintable characters)
#if(0)
static int chinaOutput2d(const WCHAR* Text, int x, int y, float z, int Width, UINT32 Color, int Font)
{
	int len = wcslen(Text);
	if (len > 256)
	{
		len = 256;
	}
	else if (len == 0)
	{
		return(0);
	}

	static piTLVERTEX  Vertices[128];
	static piINDICES	Indices[64];

	UINT16 ch;
	int i, j, left, height, vcnt, icnt;
	CHARLAYOUT*  cl;

	piSetActiveTexture(FontTexID, 0);
	piDisableMaterial();
	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
	piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);

	if (piRGBA_GETALPHA(Color) == 0)
	{
		Color |= colorALPHA100;
	}

	left = x;
	height = FontLayout.Height;
	vcnt = icnt = 0;
	for (i=j=0 ; i < len ; i++)
	{
		ch = chatGetHandleCharacterSymbol(Text[i]);

		if (ch >= MAX_CHAR_LAYOUT)
			continue;

		cl = &FontLayout.CharLayout[ch];
		Width -= cl->width;
		if (Width < 0)
		{
			break;
		}

		j += 1;	// times 2 equals number of triangles; times 6 = number of vertices

		Vertices[vcnt].sx = (float)left;
		Vertices[vcnt].sy = (float)y;
		Vertices[vcnt].sz = z;
		Vertices[vcnt].rhw = 1.0f;
		Vertices[vcnt].color = Color;
		Vertices[vcnt].specular = colorALPHA100;
		Vertices[vcnt].tu = cl->u1;
		Vertices[vcnt].tv = cl->v1 - TEXTURE_V1_FUDGE;
		Indices[icnt].v1 = vcnt;
		Indices[icnt+1].v3 = vcnt++;

		Vertices[vcnt].sx = (float)left;
		Vertices[vcnt].sy = (float)(y - height);
		Vertices[vcnt].sz = z;
		Vertices[vcnt].rhw = 1.0f;
		Vertices[vcnt].color = Color;
		Vertices[vcnt].specular = colorALPHA100;
		Vertices[vcnt].tu = cl->u1;
		Vertices[vcnt].tv = cl->v2;
		Indices[icnt].v2 = vcnt++;

		Vertices[vcnt].sx = (float)(left + cl->width);
		Vertices[vcnt].sy = (float)(y - height);
		Vertices[vcnt].sz = z;
		Vertices[vcnt].rhw = 1.0f;
		Vertices[vcnt].color = Color;
		Vertices[vcnt].specular = colorALPHA100;
		Vertices[vcnt].tu = cl->u2;
		Vertices[vcnt].tv = cl->v2;
		Indices[icnt++].v3 = vcnt;
		Indices[icnt].v1 = vcnt++;

		Vertices[vcnt].sx = (float)(left + cl->width);
		Vertices[vcnt].sy = (float)y;
		Vertices[vcnt].sz = z;
		Vertices[vcnt].rhw = 1.0f;
		Vertices[vcnt].color = Color;
		Vertices[vcnt].specular = colorALPHA100;
		Vertices[vcnt].tu = cl->u2;
		Vertices[vcnt].tv = cl->v1 - TEXTURE_V1_FUDGE;
		Indices[icnt++].v2 = vcnt++;


		if (vcnt == 96)
		{
			if (!piRenderIndexScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, vcnt, (UINT16*)Indices, icnt * 3))
			{
				Write_Debug("piRenderIndexScene failed in rntextOutput");
			}

			vcnt = icnt = 0;
		}

		left += cl->width;
	}

	if (vcnt != 0)
	{
		if (!piRenderIndexScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, vcnt, (UINT16*)Indices, icnt * 3))
		{
			Write_Debug("piRenderIndexScene failed in rntextOutput");
		}
	}

	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

	return(j);
}
#endif

#if(0)
static int chinaOutputToTexture(int texid, int miplevel, const WCHAR* Text, int x, int y, int Width, UINT32 TransparentColor, int Font)
{
UINT16 ch;
int   len;
int   i, height;
int   lsrc, ldest;
CHARLAYOUT*  cl;
piSURFACEDESC	piwork;

	len = wcslen(Text);
	if (len > 256)
	{
		len = 256;
	}
	else
	if (len == 0)
	{
		return(0);
	}

	ldtexOpenWork(texid, miplevel, &piwork);

	ldest = (piwork.Height - y) * piwork.Width + x;
	height = FontLayout.Height;
	for (i=0 ; i < len ; i++)
	{
		ch = chatGetHandleCharacterSymbol(Text[i]);

		if(ch >= MAX_CHAR_LAYOUT)
			continue;

		cl = &FontLayout.CharLayout[ch];
		Width -= cl->width;
		if (Width < 0)
		{
			break;
		}

		lsrc = (ddtext.Height - cl->y) * ddtext.Width + cl->x;
		ldtexCopySubsurfaceToSubsurface(cl->width, height, lsrc, &ddtext, ldest, &piwork, TransparentColor);
		ldest += cl->width;
	}

	ldtexCloseWork(texid, miplevel);

	return(i);
}
#endif

#if(0)
static int chinaColorTextToTexture(SINT32 iTexID,
						     SINT32 iMipLevel,
						     WCHAR* pszText,
						     SINT32 x,
						     SINT32 y,
						     SINT32 iWidth,
						     UINT32 uTransparentColor,
						     SINT32 iFont,
						     UINT32 uTextColor)
{
	UINT16 iCharacter = 0;
	SINT32 iLength = 0;
	SINT32 i = 0;
	SINT32 iHeight = 0;
	SINT32 iSource = 0;
	SINT32 iDestination = 0;
	SINT32 iCharWidth = 0;
	LOGICAL bDone = FALSE;
	LOGICAL bFirstTime = TRUE;
	CHARLAYOUT* pCharLayout = nullptr;
	piSURFACEDESC piWork;

	memset(&piWork, 0, sizeof(piSURFACEDESC));

	iLength = wcslen(pszText);

	if(iLength > 256)
		iLength = 256;

	else if(iLength == 0)
		return 0;

	ldtexOpenWork(iTexID, iMipLevel, &piWork);

	iDestination = ((piWork.Height - 1) - y) * piWork.Width + x;
	iHeight = FontLayout.Height;

	for(i = 0; i < iLength; i++)
	{
		iCharacter = chatGetHandleCharacterSymbol(pszText[i]);

		if(iCharacter >= MAX_CHAR_LAYOUT)
			continue;

		pCharLayout = &FontLayout.CharLayout[iCharacter];

        if(pCharLayout == nullptr)
            return 0;

        iWidth -= pCharLayout->width;

		if(iWidth < 0)
		{
			iCharWidth = iWidth + pCharLayout->width;
			bDone = TRUE;
		}
		else
		{
			iCharWidth = pCharLayout->width;
		}

		iSource = (ddtext.Height - pCharLayout->y) *
			       ddtext.Width + pCharLayout->x;

		if(x < 0 && bFirstTime)
		{
			iDestination -= x;
			iSource -= x;
		}

		ldtexCopyColorTextToSubsurface(iCharWidth,
			                           iHeight,
									   iSource,
									   &ddtext,
									   iDestination,
									   &piWork,
									   uTransparentColor,
									   uTextColor);

		if(bFirstTime)
		{
			bFirstTime = FALSE;

            if(x < 0)
			    iDestination += x;
		}

		if(bDone)
		{
			break;
		}

		iDestination += pCharLayout->width;
	}

	bFirstTime = TRUE;

	ldtexCloseWork(iTexID, iMipLevel);

	return i;
}
#endif

#if(0)
static int chinaOutputToSurface(piSURFACEDESC* piwork, const WCHAR* Text, int x, int y, int Width, UINT32 TransparentColor, int Font)
{
UINT16 ch;
int   len;
int   i, height;
int   lsrc, ldest;
CHARLAYOUT*  cl;

	len = wcslen(Text);
	if (len > 256)
	{
		len = 256;
	}
	else
	if (len == 0)
	{
		return(0);
	}

	ldest = (piwork->Height - y) * piwork->Width + x;
	height = FontLayout.Height;
	for (i=0 ; i < len ; i++)
	{
		ch = chatGetHandleCharacterSymbol(Text[i]);

		if(ch >= MAX_CHAR_LAYOUT)
			continue;

		cl = &FontLayout.CharLayout[ch];
		Width -= cl->width;
		if (Width < 0)
		{
			break;
		}

		lsrc = (ddtext.Height - cl->y) * ddtext.Width + cl->x;
		ldtexCopySubsurfaceToSubsurface(cl->width, height, lsrc, &ddtext, ldest, piwork, TransparentColor);
		ldest += cl->width;
	}

	return(i);
}
#endif

static SINT32 chinaGetTexId(SINT32 iFont)
{
	return FontTexID;
}
