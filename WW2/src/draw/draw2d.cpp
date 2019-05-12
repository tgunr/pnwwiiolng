#include "syscfg.h" // Path is project specific.
//

#include "pi/pi.h"
#include "include/vars.h"
#include "draw/draw2d.h"

#if USE_STATESET
#include "ogl/States.h"
static ref_ptr<RenderStateSet> disablestateset;
static ref_ptr<RenderStateSet> texturestateset;

static void InitStateSetIfNecessary()
{
    if(!disablestateset.valid())
    {
        disablestateset = new RenderStateSet;
        disablestateset->SetMode(GL_LIGHTING, 0);
    }

    if(!texturestateset.valid())
    {
        texturestateset = new RenderStateSet;
        texturestateset->SetRenderState(new TextureUnit);
        texturestateset->SetMode(GL_LIGHTING, 0);
    }
}
#endif

LOGICAL draw2dPoint(SINT32 x, SINT32 y, FLOAT32 z, UINT32 Color)
{
piTLVERTEX Vertex;

	Vertex.sx = (FLOAT32)x;
	Vertex.sy = (FLOAT32)y;
	Vertex.sz = z;
	Vertex.rhw = 1.0f;
	Vertex.color = Color;
	Vertex.specular = colorALPHA100;
	Vertex.tu = 0.0f;
	Vertex.tv = 0.0f;

#if USE_STATESET
    InitStateSetIfNecessary();
    disablestateset->Apply();
    piRenderScene(piPT_POINTLIST, piVT_TLVERTEX, &Vertex, 1);
#else
	piDisableTexture();
	piDisableMaterial();
	if (!piRenderScene(piPT_POINTLIST, piVT_TLVERTEX, &Vertex, 1))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL draw2dLine(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color)
{
piTLVERTEX Vertices[2];

	Vertices[0].sx = (FLOAT32)x1;
	Vertices[0].sy = (FLOAT32)y1;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].sx = (FLOAT32)x2;
	Vertices[1].sy = (FLOAT32)y2;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

#if USE_STATESET
    InitStateSetIfNecessary();
    disablestateset->Apply();
    piRenderScene(piPT_LINESTRIP, piVT_TLVERTEX, Vertices, 2);
#else
	piDisableTexture();
	piDisableMaterial();
	if (!piRenderScene(piPT_LINESTRIP, piVT_TLVERTEX, Vertices, 2))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL draw2dRectangle(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color)
{
piTLVERTEX Vertices[5];

	Vertices[0].sx = (FLOAT32)x1;
	Vertices[0].sy = (FLOAT32)y1;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].sx = (FLOAT32)x2;
	Vertices[1].sy = (FLOAT32)y1;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

	Vertices[2].sx = (FLOAT32)x2;
	Vertices[2].sy = (FLOAT32)y2;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = 0.0f;
	Vertices[2].tv = 0.0f;

	Vertices[3].sx = (FLOAT32)x1;
	Vertices[3].sy = (FLOAT32)y2;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = Color;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = 0.0f;
	Vertices[3].tv = 0.0f;

	Vertices[4].sx = (FLOAT32)x1;
	Vertices[4].sy = (FLOAT32)y1;
	Vertices[4].sz = z;
	Vertices[4].rhw = 1.0f;
	Vertices[4].color = Color;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = 0.0f;
	Vertices[4].tv = 0.0f;

#if USE_STATESET
    InitStateSetIfNecessary();
    disablestateset->Apply();
    piRenderScene(piPT_LINESTRIP, piVT_TLVERTEX, Vertices, 5);
#else
	piDisableTexture();
	piDisableMaterial();
	if (!piRenderScene(piPT_LINESTRIP, piVT_TLVERTEX, Vertices, 5))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL draw2dTransparentRectangle(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color)
{
piTLVERTEX Vertices[4];

	// The Z for these vertices is set to something small instead of 0.0.
	// This will put the rectangle just behind other 2D items.
	Vertices[0].sx = (FLOAT32)x1;
	Vertices[0].sy = (FLOAT32)y1;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].sx = (FLOAT32)x2;
	Vertices[1].sy = (FLOAT32)y1;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 1.0f;
	Vertices[1].tv = 0.0f;

	Vertices[2].sx = (FLOAT32)x2;
	Vertices[2].sy = (FLOAT32)y2;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = 1.0f;
	Vertices[2].tv = 1.0f;

	Vertices[3].sx = (FLOAT32)x1;
	Vertices[3].sy = (FLOAT32)y2;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = Color;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = 0.0f;
	Vertices[3].tv = 1.0f;

#if USE_STATESET
    InitStateSetIfNecessary();
    disablestateset->Apply();
    piRenderScene(piPT_TRIANGLEFAN, piVT_TLVERTEX, Vertices, 4);
#else
	piDisableTexture();
	piDisableMaterial();
	if (!piRenderScene(piPT_TRIANGLEFAN, piVT_TLVERTEX, Vertices, 4))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

//------------------------------------
// Fix for 'off-by-one-half' DirectX bug.
// This fix is for texel alignment. The rule is to bias the screen
// coordinates by -0.5 to align correctly with the texel coordinates
// which have this bias.
//------------------------------------
LOGICAL draw2dTexture(SINT32 iTexID,
					  SINT32 iMipLevel,
					  SINT32 iXin,
					  SINT32 iYin,
					  FLOAT32 z,
					  SINT32 iWidth,
					  SINT32 iHeight,
					  UINT32 uColor)
{
	FLOAT32 u1, u2, v1, v2;
	piTLVERTEX Vertices[6];

#ifdef DIRECT3D
	FLOAT32 x = (FLOAT32)iXin - 0.5f;
	FLOAT32 y = (FLOAT32)iYin - 0.5f;
#else
	FLOAT32 x = (FLOAT32)iXin;
	FLOAT32 y = (FLOAT32)iYin;
#endif

	FLOAT32 fWidth = (FLOAT32)iWidth;
	FLOAT32 fHeight = (FLOAT32)iHeight;

	if (!piGetTextureFactors(iTexID, iMipLevel, &u2, &v2))
	{
		return FALSE;
	}

	u1 = 0.0f;			// Left
	v1 = 1.0f - v2;		// Bottom
	v2 = 1.0f;			// Top

	Vertices[0].sx = x;
	Vertices[0].sy = y;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = uColor;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = u1;
	Vertices[0].tv = v1;

	Vertices[1].sx = x;
	Vertices[1].sy = y - fHeight;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = uColor;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = u1;
	Vertices[1].tv = v2;

	Vertices[2].sx = x + fWidth;
	Vertices[2].sy = y - fHeight;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = uColor;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = u2;
	Vertices[2].tv = v2;

	Vertices[3].sx = x + fWidth;
	Vertices[3].sy = y - fHeight;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = uColor;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = u2;
	Vertices[3].tv = v2;

	Vertices[4].sx = x + fWidth;
	Vertices[4].sy = y;
	Vertices[4].sz = z;
	Vertices[4].rhw = 1.0f;
	Vertices[4].color = uColor;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = u2;
	Vertices[4].tv = v1;

	Vertices[5].sx = x;
	Vertices[5].sy = y;
	Vertices[5].sz = z;
	Vertices[5].rhw = 1.0f;
	Vertices[5].color = uColor;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = u1;
	Vertices[5].tv = v1;

#if USE_STATESET
    InitStateSetIfNecessary();

    TextureUnit* tu = dynamic_cast<TextureUnit*>(texturestateset->GetRenderState(RS_TEXTURE_UNIT0));
    tu->SetTexture(iTexID);

    tu->SetDirty(true);
    texturestateset->Apply();
    tu->SetDirty(false);

    piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6);
#else
	piDisableMaterial();
	piSetActiveTexture(iTexID, iMipLevel);
	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
	piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);

	if (!piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6))
	{
		WriteDebug("piRenderScene failed in draw2dTexture");
	}

	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);
#endif

	return TRUE;
}

LOGICAL draw2dTexture2(SINT32 texid, SINT32 miplevel, SINT32 x, SINT32 y, FLOAT32 z, SINT32 Width, SINT32 Height, UINT32 Color)
{
FLOAT32		u1, u2, v1, v2, sx, sy, fw, fh;
piTLVERTEX  Vertices[6];

#ifdef DIRECT3D
	sx = (FLOAT32)x - 0.5f;
	sy = (FLOAT32)y - 0.5f;
#else
	sx = (FLOAT32)x;
	sy = (FLOAT32)y;
#endif

	fw = (FLOAT32)Width;
	fh = (FLOAT32)Height;

	// The important part of the texture must be in the upper left!
	if(!piGetTextureFactors(texid, miplevel, &u2, &v2))
	{
		return(FALSE);
	}

    u1 = 0.0f;
    v1 = 0.0f;

	Vertices[0].sx = sx;
	Vertices[0].sy = sy;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = u1;
	Vertices[0].tv = v1;

	Vertices[1].sx = sx;
	Vertices[1].sy = sy - fh;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = u1;
	Vertices[1].tv = v2;

	Vertices[2].sx = sx + fw;
	Vertices[2].sy = sy - fh;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = u2;
	Vertices[2].tv = v2;

	Vertices[3].sx = sx + fw;
	Vertices[3].sy = sy - fh;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = Color;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = u2;
	Vertices[3].tv = v2;

	Vertices[4].sx = sx + fw;
	Vertices[4].sy = sy;
	Vertices[4].sz = z;
	Vertices[4].rhw = 1.0f;
	Vertices[4].color = Color;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = u2;
	Vertices[4].tv = v1;

	Vertices[5].sx = sx;
	Vertices[5].sy = sy;
	Vertices[5].sz = z;
	Vertices[5].rhw = 1.0f;
	Vertices[5].color = Color;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = u1;
	Vertices[5].tv = v1;

#if USE_STATESET
    InitStateSetIfNecessary();

    TextureUnit* tu = dynamic_cast<TextureUnit*>(texturestateset->GetRenderState(RS_TEXTURE_UNIT0));
    tu->SetTexture(texid);

    tu->SetDirty(true);
    texturestateset->Apply();
    tu->SetDirty(false);

    piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6);
#else
	piDisableMaterial();
	piSetActiveTexture(texid, miplevel);

	if (!piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6))
	{
		WriteDebug("piRenderScene failed in draw2dTexture");
	}
#endif

	return(TRUE);
}

LOGICAL draw2dSubTexture(SINT32 texid, SINT32 miplevel, SINT32 x, SINT32 y, piRECT* wrect, FLOAT32 z, piRECT* rect, UINT32 Color)
{
SINT32		wWidth, wHeight, Width, Height;
FLOAT32		u1, u2, v1, v2;
FLOAT32		x1, x2, y1, y2;
FLOAT32		sx, sy, fw, fh;
piTLVERTEX  Vertices[6];

	// The important part of the texture must be in the upper left!
	if (!piGetTextureFactors(texid, miplevel, &u2, &v2))
	{
		return(FALSE);
	}

	wWidth = wrect->right - wrect->left + 1;
	wHeight = wrect->bottom - wrect->top + 1;

	Width = rect->right - rect->left + 1;
	Height = rect->bottom - rect->top + 1;

	x += rect->left;
	y -= wHeight - (rect->top + Height);

#ifdef DIRECT3D
	sx = (FLOAT32)x - 0.5f;
	sy = (FLOAT32)y - 0.5f;
#else
	sx = (FLOAT32)x;
	sy = (FLOAT32)y;
#endif

	fw = (FLOAT32)Width;
	fh = (FLOAT32)Height;

	x1 = (FLOAT32)rect->left / (FLOAT32)wWidth;
	x2 = (FLOAT32)rect->right / (FLOAT32)wWidth;
	y1 = (FLOAT32)rect->bottom / (FLOAT32)wHeight;
	y2 = (FLOAT32)rect->top / (FLOAT32)wHeight;

	u1 = x1 * u2;			// Left
	u2 = x2 * u2;			// Right
	v1 = 1.0f - y1 * v2;	// Bottom
	v2 = 1.0f - y2 * v2;	// Top

	Vertices[0].sx = sx;
	Vertices[0].sy = sy;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = u1;
	Vertices[0].tv = v1;

	Vertices[1].sx = sx;
	Vertices[1].sy = sy - fh;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = u1;
	Vertices[1].tv = v2;

	Vertices[2].sx = sx + fw;
	Vertices[2].sy = sy - fh;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = u2;
	Vertices[2].tv = v2;

	Vertices[3].sx = sx + fw;
	Vertices[3].sy = sy - fh;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = Color;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = u2;
	Vertices[3].tv = v2;

	Vertices[4].sx = sx + fw;
	Vertices[4].sy = sy;
	Vertices[4].sz = z;
	Vertices[4].rhw = 1.0f;
	Vertices[4].color = Color;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = u2;
	Vertices[4].tv = v1;

	Vertices[5].sx = sx;
	Vertices[5].sy = sy;
	Vertices[5].sz = z;
	Vertices[5].rhw = 1.0f;
	Vertices[5].color = Color;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = u1;
	Vertices[5].tv = v1;

#if USE_STATESET
    InitStateSetIfNecessary();

    TextureUnit* tu = dynamic_cast<TextureUnit*>(texturestateset->GetRenderState(RS_TEXTURE_UNIT0));
    tu->SetTexture(texid);

    tu->SetDirty(true);
    texturestateset->Apply();
    tu->SetDirty(false);

    piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6);
#else
	piDisableMaterial();
	piSetActiveTexture(texid, miplevel);
	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
	piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);

	if (!piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6))
	{
		WriteDebug("piRenderScene failed in draw2dTexture");
	}

	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);
#endif

	return(TRUE);
}

//////////////////////////////////////////////////////////////////////
// Function: draw2dByUV
// Description: The user is responsible for setting the texture by
// piSetActiveTexture. Use this function to draw a 2D portion of a
// texture based on it's UV coordinates.
//////////////////////////////////////////////////////////////////////
LOGICAL draw2dByUV(FLOAT32 x,
                   FLOAT32 y,
                   FLOAT32 z,
                   piUV uvCoords[2],
                   SINT32 iWidth,
                   SINT32 iHeight,
                   UINT32 uColor)
{
	piTLVERTEX Vertices[6];

    FLOAT32 fWidth = (FLOAT32)iWidth;
	FLOAT32 fHeight = (FLOAT32)iHeight;

#ifdef DIRECT3D
	x -= 0.5f;
	y -= 0.5f;
#endif

	Vertices[0].sx = x;
	Vertices[0].sy = y;
	Vertices[0].sz = z;
	Vertices[0].rhw = 1.0f;
	Vertices[0].color = uColor;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = uvCoords[0].u;
	Vertices[0].tv = uvCoords[0].v;

	Vertices[1].sx = x;
	Vertices[1].sy = y - fHeight;
	Vertices[1].sz = z;
	Vertices[1].rhw = 1.0f;
	Vertices[1].color = uColor;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = uvCoords[0].u;
	Vertices[1].tv = uvCoords[1].v;

	Vertices[2].sx = x + fWidth;
	Vertices[2].sy = y - fHeight;
	Vertices[2].sz = z;
	Vertices[2].rhw = 1.0f;
	Vertices[2].color = uColor;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = uvCoords[1].u;
	Vertices[2].tv = uvCoords[1].v;

	Vertices[3].sx = x + fWidth;
	Vertices[3].sy = y - fHeight;
	Vertices[3].sz = z;
	Vertices[3].rhw = 1.0f;
	Vertices[3].color = uColor;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = uvCoords[1].u;
	Vertices[3].tv = uvCoords[1].v;

	Vertices[4].sx = x + fWidth;
	Vertices[4].sy = y;
	Vertices[4].sz = z;
	Vertices[4].rhw = 1.0f;
	Vertices[4].color = uColor;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = uvCoords[1].u;
	Vertices[4].tv = uvCoords[0].v;

	Vertices[5].sx = x;
	Vertices[5].sy = y;
	Vertices[5].sz = z;
	Vertices[5].rhw = 1.0f;
	Vertices[5].color = uColor;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = uvCoords[0].u;
	Vertices[5].tv = uvCoords[0].v;

	if(!piRenderScene(piPT_TRIANGLELIST, piVT_TLVERTEX, Vertices, 6))
	{
		WriteDebug("[draw2dByUV] piRenderScene failed");
	}

    return TRUE;
}
