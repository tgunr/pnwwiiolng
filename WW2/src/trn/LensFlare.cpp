#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
//  CLensFlare Class
//
//  (c) 2003 IDV, Inc.
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      1233 Washington St. Suite 610
//      Columbia, SC 29201
//      Voice: (803) 799-1699
//      Fax:   (803) 931-0320
//      Web:   http://www.idvinc.com
//


//////////////////////////////////////////////////////////////////////
//  Preprocessor
#include "math/math3d.h"
#include "trn/LensFlare.h"
#include <cmath>
#include "IdvVector.h"
#include "trn/Filename.h"
#include "include/vars.h"
#include "pi/pi.h"
#include "pi/piinfo.h"
#include "math/mathnum.h"
#include "ob/obmain.h"
#include "trn/trnsky.h"
#include "ogl/oglvars.h"
#include "ob/obdraw.h"
#include "ogl/ogl.h"
#include "ogl/oglGlobalStates.h"
#include "osgImport/FrameBufferObject.h"
#include "osgImport/Texture2D.h"
#include "osgImport/TextureRectangle.h"
#include "osgImport/BufferObject.h"
#include "ld/ldtexture.h"
#include "ld/ldpix.h"
#include "ogl/oglMaterial.h"
#include "cfg/cfgCodes.h"
#include "ogl/States.h"

#include "fx/fxWeather.h"

#define TEST_PBO    0

#define BUFFER_OFFSET(i) ((char*)nullptr + (i))

static ref_ptr<FrameBufferObject> sunfbo;
static ref_ptr<Texture> suntex;

#if USE_STATESET
static ref_ptr<RenderStateSet> glareMaterial;
static ref_ptr<RenderStateSet> afterflareMaterial;
static ref_ptr<FrameBufferObject> depth;
#else
static ref_ptr<oglMaterial> glareMaterial;
static ref_ptr<oglMaterial> afterflareMaterial;
#endif

#if TEST_PBO
#define MAX_PBO 2
static ref_ptr<PixelDataBufferObject> pbo[MAX_PBO];
#endif

static OBJECT* g_sun = nullptr;

//static float c_fHalfMaxBright = 0.45f;
//static float c_fDistanceScale = 1.0f;
static int DepthTestDimension = 20;

#if USE_STATESET
static ref_ptr<RenderStateSet> flarestateset;
static ref_ptr<RenderStateSet> brightstateset;
#endif

CLensFlare g_lensFlare;

static bool canUseTexture2D = false;

//////////////////////////////////////////////////////////////////////
//  Variables

#if USE_STATESET
// flare texture map filenames
static std::string g_strFiles[] =
{
    "fx/sunflare/flare2.dds",
    "fx/sunflare/flare1.dds",
    "fx/sunflare/flare2.dds",
    "fx/sunflare/flare1.dds",
    "fx/sunflare/flare6.dds",
    "fx/sunflare/flare4.dds",
    "fx/sunflare/flare2.dds",
    "fx/sunflare/flare3.dds",
    ""
};
#else
// flare texture map filenames
static std::string g_strFiles[] =
{
    "flare2.dds",
    "flare1.dds",
    "flare2.dds",
    "flare1.dds",
    "flare6.dds",
    "flare4.dds",
    "flare2.dds",
    "flare3.dds",
    ""
};
#endif

// relative positions of each of the flare textures
static float g_fPosition[] =
{
    -0.55f,
    -0.5f,
    -0.45f,
    0.05f,
    0.25f,
    0.95f,
    0.9f,
    1.0f
};

#if USE_STATESET
// relative sizes of each of the flare textures
static float g_fWidth[] =
{
    100.0f,//20.0f,
    100.0f,//32.0f,
    20.0f,
    32.0f,
    50.0f,
    32.0f,
    20.0f,
    150.0f
};
#else
// relative sizes of each of the flare textures
static float g_fWidth[] =
{
    100.0f,//20.0f,
    100.0f,//32.0f,
    20.0f,
    32.0f,
    50.0f,
    32.0f,
    20.0f,
    150.0f
};
#endif

// colors of each of the flare textures
static float g_afColors[ ][4] =
{
    { 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 0.8f },
    { 0.3f, 0.5f, 1.0f, 0.9f },
    { 0.3f, 0.5f, 1.0f, 0.6f },
    { 1.0f, 0.6f, 0.9f, 0.4f },
    { 1.0f, 0.0f, 0.0f, 0.5f },
    { 1.0f, 0.6f, 0.3f, 0.4f }
};


void LensFlareKill()
{
    sunfbo = nullptr;
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::CLensFlare

CLensFlare::CLensFlare( ) :
    SunSize(0),
    m_fBeforeBright(0.0f),
    AfterBright(0.0f),
    m_bFlareVisible(false),
    m_bDrawFlare(true),
    m_bDrawBrightScreen(true),
    m_bEnabled(true),
    m_bShowMainFlare(true),
    m_fMaxBrightness(1.0f),
    m_fGlowBright(1.0f),
    m_fObstructionFactor(1.0f)
{
    m_pControlPixels = new GLfloat[DepthTestDimension * DepthTestDimension];
    m_pTestPixels = new GLfloat[DepthTestDimension * DepthTestDimension];
    afColor[0] = afColor[1] = afColor[2] = 1.0f;
    depthread = false;

#ifdef USE_OCCLUSION_QUERY
    query = 0;

    sun.push_back(400.0f);
    sun.push_back(0.0f);
    sun.push_back(-400.0f);

    sun.push_back(-400.0f);
    sun.push_back(0.0f);
    sun.push_back(-400.0f);

    sun.push_back(-400.0f);
    sun.push_back(0.0f);
    sun.push_back(400.0f);

    sun.push_back(400.0f);
    sun.push_back(0.0f);
    sun.push_back(400.0f);
#endif
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::~CLensFlare

CLensFlare::~CLensFlare()
{
    delete[] m_pControlPixels;
    delete[] m_pTestPixels;

#ifdef USE_OCCLUSION_QUERY
    if(GLEW_ARB_occlusion_query)
    {
        glDeleteQueriesARB(1, &query);
    }

    sun.clear();
#endif
}

float CLensFlare::getAfterBright() const
{
    return AfterBright;
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::DrawBeforeFlareDeprecated

void CLensFlare::DrawBeforeFlareDeprecated()
{
#if(0)
    if (!m_bFlareVisible || !m_bEnabled || !m_bShowMainFlare)
        return;

    UINT32 lightOn = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
    UINT32 depthTest = piGetEngineRenderState(piRENDERSTATE_ZENABLE);
    UINT32 alphaTest = piGetEngineRenderState(piRENDERSTATE_ALPHATESTENABLE);
    UINT32 alphaBlend = piGetEngineRenderState(piRENDERSTATE_ALPHABLENDENABLE);
    UINT32 shadeModel = piGetEngineRenderState(piRENDERSTATE_SHADEMODE);

    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    piSetRenderState(piRENDERSTATE_ZENABLE, FALSE);
    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, FALSE);
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);
    piSetRenderState(piRENDERSTATE_SHADEMODE, piSHADE_FLAT);

    oglSetMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

    oglSetMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if(0)// (m_fBeforeBright != 0.0f && m_bDrawFlare && m_bDrawBrightScreen && false)
    {
	    oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1.0f, 1.0f, 1.0f, m_fBeforeBright));
        piDisableTexture();
        glBegin(GL_TRIANGLE_STRIP);
            glVertex2f(0.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(1.0f, 1.0f);
        glEnd( );
    }

    piEnableTexture();
    glTranslatef(m_afFlarePos[0], m_afFlarePos[1], 0.0f);

    float fAspectRatio = pViewport[2] / static_cast<float>(pViewport[3]);
    float fHeight = m_fSunSize * fAspectRatio;

    // sun
    TextureState::Bind(m_uiSunFlareTex, GL_TEXTURE_2D);
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-m_fSunSize, -fHeight);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-m_fSunSize, fHeight);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(m_fSunSize, -fHeight);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(m_fSunSize, fHeight);
    glEnd( );

    // sun over bright
    float fIncrease = 0.5f + AfterBright;

    TextureState::Bind(m_uiAfterFlare, GL_TEXTURE_2D);

	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, AfterBright));
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-m_fSunSize * fIncrease, -fHeight * fIncrease);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-m_fSunSize * fIncrease, fHeight * fIncrease);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(m_fSunSize * fIncrease, -fHeight * fIncrease);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(m_fSunSize * fIncrease, fHeight * fIncrease);
    glEnd( );

    oglSetMatrixMode(GL_PROJECTION);
    glPopMatrix( );

    oglSetMatrixMode(GL_MODELVIEW);
    glPopMatrix( );

    piSetRenderState(piRENDERSTATE_LIGHTING, lightOn);
    piSetRenderState(piRENDERSTATE_ZENABLE, depthTest);
    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, alphaTest);
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, alphaBlend);
    piSetRenderState(piRENDERSTATE_SHADEMODE, shadeModel);
#endif
}

static void drawquad(float x1, float y1, float x2, float y2, unsigned int color = colorWHITEALPHA)
{
    piLVERTEX v[4];

	v[0].x = x1;
	v[0].y = y1;
	v[0].z = zdepthLAYER0;
	v[0].color = color;
	v[0].specular = colorALPHA100;
	v[0].tu = 0.0f;
	v[0].tv = 0.0f;

	v[1].x = x2;
	v[1].y = y1;
	v[1].z = zdepthLAYER0;
	v[1].color = color;
	v[1].specular = colorALPHA100;
	v[1].tu = 1.0f;
	v[1].tv = 0.0f;

	v[2].x = x2;
	v[2].y = y2;
	v[2].z = zdepthLAYER0;
	v[2].color = color;
	v[2].specular = colorALPHA100;
	v[2].tu = 1.0f;
	v[2].tv = 1.0f;

	v[3].x = x1;
	v[3].y = y2;
	v[3].z = zdepthLAYER0;
	v[3].color = color;
	v[3].specular = colorALPHA100;
	v[3].tu = 0.0f;
	v[3].tv = 1.0f;

    piINDICES indices[2];

    indices[0].v1 = 0;
    indices[0].v2 = 1;
    indices[0].v3 = 3;
    indices[1].v1 = 1;
    indices[1].v2 = 2;
    indices[1].v3 = 3;

    oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_LVERTEX,
                        v,
                        4,
                        (unsigned short*)indices,
                        6);
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::DrawBeforeFlare

void CLensFlare::DrawBeforeFlare()
{
    if(!m_bFlareVisible || !m_bEnabled || !m_bShowMainFlare)
        return;

#if USE_STATESET
#else
    piMATRIX view = *oglGetViewMatrix();
    piMATRIX proj = *oglGetProjectionMatrix();

    oglSetRenderState(oglRENDERSTATE_ZENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);

    oglSetViewMatrix(&IdentityMatrix);

    piMATRIX world = IdentityMatrix;

    float left = 0.0f;
    float right = 1.0f;
    float bottom = 0.0f;
    float top = 1.0f;
    float znear = -1.0f;
    float zfar = 1.0f;

    world._41 = FlarePos[0];
    world._42 = FlarePos[1];

    oglSetOrthoProjectionMatrix(right, left, top, bottom, znear, zfar);
    oglSetWorldMatrix(&world);

    float AspectRatio = (float)WantedWidth / (float)WantedHeight;
    float Height = SunSize * AspectRatio;

    glareMaterial->Apply();

    unsigned int color = piRGBA(1, 1, 1, AfterBright);

    drawquad(-SunSize, -Height, SunSize, Height, color);

    // sun over bright
    float Increase = 0.5f + AfterBright;

    float xx = SunSize * Increase;
    float yy = Height * Increase;

    drawquad(-xx, -yy, xx, yy, color);

    oglSetRenderState(oglRENDERSTATE_ZENABLE, bZEnable);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

    oglSetViewMatrix(&view);
    oglSetProjectionMatrix(&proj);
#endif
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::DrawAfterFlare
//  Called from DrawFlare. This should only be called from DrawFlare.
void CLensFlare::DrawAfterFlare(float brightness)
{
    if(obdrawIsRenderPass(RENDER_REFLECTION))
    {
        return;
    }

    if(g_postRenderFilter && sunfbo.valid())
    {
        return;
    }

    if (m_bEnabled && brightness != 0.0f && m_bDrawBrightScreen)
    {
        if(ogl::UseDeprecatedPath())
        {
#if USE_STATESET
            brightstateset->Apply();

            drawquad(0,
                     0,
                     WantedWidth,
                     WantedHeight,
                     piRGBA(1.0f, 1.0f, 1.0f, brightness));
#else
            // screen brightness
	        oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(afColor[0], afColor[1], afColor[2], brightness * 0.5f));
            glBegin(GL_TRIANGLE_STRIP);
                glVertex2i(0, 0);
                glVertex2i(0, WantedHeight);
                glVertex2i(WantedWidth, 0);
                glVertex2i(WantedWidth, WantedHeight);
            glEnd();
#endif
        }
        else
        {
            afterflareMaterial->Apply();
            drawquad(0, 0, WantedWidth, WantedHeight, piRGBA(afColor[0], afColor[1], afColor[2], brightness * 0.5f));
        }
    }
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::SetMainFlare

void CLensFlare::SetMainFlare(std::string strSunFile, float fSunSize)
{
    if (m_bEnabled && m_bShowMainFlare)
    {
        SunSize = fSunSize;

        //m_uiAfterFlare = ldpixLoadImageFileToSurface("sunglare1.bmp", colorBLACK, LDPIX_NONE);
        //m_uiAfterFlare = oglGetTextureData(m_uiAfterFlare)->genlist;
    }
}

#if USE_STATESET
//////////////////////////////////////////////////////////////////////
//  CLensFlare::DrawFlare

void CLensFlare::DrawFlare()
{
    piMATRIX proj = *oglGetProjectionMatrix();
    piMATRIX view = *oglGetViewMatrix();

    if(m_bEnabled && m_bFlareVisible && AfterBright > 0.0f)
    {
        oglSetOrthoProjectionMatrix(WantedWidth, 0.0f, WantedHeight, 0.0f, -1.0f, 1.0f);
        oglSetViewMatrix(&IdentityMatrix);

        DrawAfterFlare(AfterBright);
    }

    flarestateset->GetRenderState(RS_FRAMEBUFFEROBJECT)->SetEnabled(g_postRenderFilter);
    flarestateset->GetRenderState(RS_VIEWPORT)->SetEnabled(g_postRenderFilter);
    flarestateset->Apply();

    // Clear render texture
    if(g_postRenderFilter)
    {
        static ref_ptr<ClearNode> clear = new ClearNode(GL_COLOR_BUFFER_BIT, 1, Vec4f(0, 0, 0, 1));
        clear->apply();
    }

    if(m_bEnabled && m_bFlareVisible && AfterBright > 0.0f)
    {
        if(fxWeatherIsCloudy())
        {
            // Interpolate
            float t = (playpos.z - RAIN_CLOUD_HEIGHT) / INTEROPLATE_DIST;
            t = t < 0 ? 0 : t > 1 ? 1 : t;

            AfterBright = t * AfterBright;
        }

        int x, y;

        mathFloatToInt(&x, FlareWinPos[0]);
        mathFloatToInt(&y, FlareWinPos[1]);

        if(AfterBright > 0.0)
        {
            if(!g_postRenderFilter)
                AfterBright *= 2.0f;

            AfterBright = math3dFastSqrt(AfterBright);
        }

        m_cFlare.DrawDeprecated(AfterBright,
                                x,
                                y);

        oglSetProjectionMatrix(&proj);
        oglSetViewMatrix(&view);
    }
}
#else
//////////////////////////////////////////////////////////////////////
//  CLensFlare::DrawFlare

void CLensFlare::DrawFlare()
{
    if(sunfbo.valid() && g_postRenderFilter)
    {
        sunfbo->apply();
	    oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, piRGBA(0, 0, 0, 1));
        oglClearTargets(oglCLEAR_VIEWPORT);
        oglSetViewport(0, 0, suntex->getTextureWidth(), suntex->getTextureHeight());
    }

    if (m_bEnabled &&
        m_bFlareVisible &&
        m_bDrawFlare &&
        depthread &&
        AfterBright != 0.0f)
    {
        if(ogl::UseDeprecatedPath())
        {
            piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
            piSetRenderState(piRENDERSTATE_ZENABLE, FALSE);
            piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);
            piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, FALSE);
            piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, piBLEND_SRCALPHA);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, piBLEND_ONE);

            piMATRIX proj = *oglGetProjectionMatrix();
            piMATRIX view = *oglGetViewMatrix();

            float oldback = BackClip;
            float oldfront = FrontClip;

            oglSetOrthoProjectionMatrix(WantedWidth, 0.0f, WantedHeight, 0.0f, -1.0f, 1.0f);
            oglSetViewMatrix(&IdentityMatrix);

            // Ticket: 5172
            oglDisableTexturesAboveAndIncluding(0);
            ProgramState::Bind(0);

            if(fxWeatherIsCloudy())
            {
                // Interpolate
                float t = (playpos.z - RAIN_CLOUD_HEIGHT) / INTEROPLATE_DIST;
                t = t < 0 ? 0 : t > 1 ? 1 : t;

                AfterBright = t * AfterBright;
            }

            DrawAfterFlare(AfterBright);

            piEnableTexture();

            int x, y;

            mathFloatToInt(&x, FlareWinPos[0]);
            mathFloatToInt(&y, FlareWinPos[1]);

            m_cFlare.DrawDeprecated(math3dFastSqrt(AfterBright),
                                    x,
                                    y);

            piDisableTexture();
            piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
            piSetRenderState(piRENDERSTATE_ZENABLE, bZEnable);
            piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
            piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
            piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);

            oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));

            BackClip = oldback;
            FrontClip = oldfront;

            oglSetProjectionMatrix(&proj);
            oglSetViewMatrix(&view);
        }
        else
        {
            oglSetRenderState(oglRENDERSTATE_ZENABLE, FALSE);
            oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);
            oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, FALSE);
            oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);

            piMATRIX proj = *oglGetProjectionMatrix();
            piMATRIX view = *oglGetViewMatrix();

            oglSetWorldMatrix(&IdentityMatrix);
            oglSetViewMatrix(&IdentityMatrix);

            float left = 0.0f;
            float right = WantedWidth;
            float bottom = 0.0f;
            float top = WantedHeight;
            float znear = -1.0f;
            float zfar = 1.0f;

            oglSetOrthoProjectionMatrix(right, left, top, bottom, znear, zfar);

            if(fxWeatherIsCloudy())
            {
                // Interpolate
                float t = (playpos.z - RAIN_CLOUD_HEIGHT) / INTEROPLATE_DIST;
                t = t < 0 ? 0 : t > 1 ? 1 : t;

                AfterBright = t * AfterBright;
            }

            DrawAfterFlare(AfterBright);

            int x, y;

            mathFloatToInt(&x, FlareWinPos[0]);
            mathFloatToInt(&y, FlareWinPos[1]);

            m_cFlare.Draw(math3dFastSqrt(AfterBright),
                          x,
                          y);

            oglSetRenderState(oglRENDERSTATE_ZENABLE, bZEnable);
            oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
            oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
            oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

            oglSetProjectionMatrix(&proj);
            oglSetViewMatrix(&view);
        }
    }

    if(sunfbo.valid() && g_postRenderFilter)
    {
        sunfbo->UnApply();
        oglSetViewport(0, 0, WantedWidth, WantedHeight);
    }
}
#endif

Texture* CLensFlare::GetSunTexture()
{
    if(!sunfbo.valid())
    {
        return nullptr;
    }

    return suntex.get();
}

#if USE_STATESET
void CLensFlare::Initialize(std::string strPath)
{
    if (m_bEnabled)
        m_cFlare.Init(strPath);

#ifdef USE_OCCLUSION_QUERY
    if(GLEW_ARB_occlusion_query)
    {
        // Don't use firstread with occlusion query, so set it here.
        depthread = true;
        glGenQueriesARB(1, &query);
    }
#else
#if TEST_PBO
    for(int i = 0; i < MAX_PBO; i++)
    {
        pbo[i] = new PixelDataBufferObject;

        if(pbo[i]->isPBOSupported(0))
        {
            int size = sizeof(float) * c_nDepthTestDimension * c_nDepthTestDimension;
            pbo[i]->setDataSize(size);
        }
    }
#endif
#endif
    brightstateset = new RenderStateSet;
    brightstateset->SetMode(GL_BLEND, 1);
    brightstateset->SetMode(GL_LIGHTING, 0);
    brightstateset->SetMode(GL_CULL_FACE, 0);
    brightstateset->SetRenderState(new DepthMask(false));

    flarestateset = new RenderStateSet;
    flarestateset->SetMode(GL_CULL_FACE, 0);
    flarestateset->SetMode(GL_LIGHTING, 0);
    flarestateset->SetMode(GL_BLEND, 1);

    flarestateset->SetRenderState(new TextureUnit);
    flarestateset->SetRenderState(new Program("glare"));
    flarestateset->SetRenderState(new DepthMask(false));

    BlendFunc* bf = new BlendFunc;

    bf->SetSrcRGB(GL_SRC_ALPHA);
    bf->SetDstRGB(GL_ONE);
    bf->SetSrcAlpha(GL_SRC_ALPHA);
    bf->SetDstAlpha(GL_ONE);

    flarestateset->SetRenderState(bf);

    canUseTexture2D = oglCanUseTexture2D(WantedWidth, WantedHeight);

    sunfbo = new FrameBufferObject;

    if(canUseTexture2D)
    {
        suntex = new Texture2D;
        Texture2D* t = dynamic_cast<Texture2D*>(suntex.get());
        sunfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(t));
        t->setTextureSize(WantedWidth / 4, WantedHeight / 4);
    }
    else if(GLEW_ARB_texture_rectangle)
    {
        suntex = new TextureRectangle;
        TextureRectangle* t = dynamic_cast<TextureRectangle*>(suntex.get());
        sunfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(t));
        t->setTextureSize(WantedWidth / 4, WantedHeight / 4);
    }

    flarestateset->SetRenderState(sunfbo.get());
    flarestateset->SetRenderState(new Viewport(0, 0, suntex->getTextureWidth(), suntex->getTextureHeight()));

    suntex->setInternalFormat(GL_RGBA);
    suntex->setSourceFormat(GL_RGBA);
    suntex->setSourceType(GL_UNSIGNED_SHORT);
    suntex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    suntex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

    if(!ogl::UseDeprecatedPath())
    {
        glareMaterial = new RenderStateSet;

        SunFlareTex = ldpixLoadImageFileToSurface("sunglare1.bmp", colorBLACK, LDPIX_NONE);

        glareMaterial->SetRenderState(new TextureUnit(0, SunFlareTex));
        glareMaterial->SetRenderState(new Program("glare"));

        afterflareMaterial = new RenderStateSet;
        afterflareMaterial->SetRenderState(new Program("basiccolor"));
    }
}
#else
//////////////////////////////////////////////////////////////////////
//  CLensFlare::Initialize

void CLensFlare::Initialize(std::string strPath)
{
    if (m_bEnabled)
        m_cFlare.Init(strPath);

#ifdef USE_OCCLUSION_QUERY
    if(GLEW_ARB_occlusion_query)
    {
        // Don't use firstread with occlusion query, so set it here.
        depthread = true;
        glGenQueriesARB(1, &query);
    }
#else
#if TEST_PBO
    for(int i = 0; i < MAX_PBO; i++)
    {
        pbo[i] = new PixelDataBufferObject;

        if(pbo[i]->isPBOSupported(0))
        {
            int size = sizeof(float) * c_nDepthTestDimension * c_nDepthTestDimension;
            pbo[i]->setDataSize(size);
        }
    }
#endif
#endif

    bool supported = piinfoIsSupportedPref(cfgPOST_RENDER_FILTER);

    if(!supported)
    {
        return;
    }

    canUseTexture2D = oglCanUseTexture2D(WantedWidth, WantedHeight);

    sunfbo = new FrameBufferObject;

    if(canUseTexture2D)
    {
        suntex = new Texture2D;
        Texture2D* t = dynamic_cast<Texture2D*>(suntex.get());
        sunfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(t));
        t->setTextureSize(WantedWidth / 4, WantedHeight / 4);
    }
    else if(GLEW_ARB_texture_rectangle)
    {
        suntex = new TextureRectangle;
        TextureRectangle* t = dynamic_cast<TextureRectangle*>(suntex.get());
        sunfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(t));
        t->setTextureSize(WantedWidth / 4, WantedHeight / 4);
    }
    else
    {
        return;
    }

    suntex->setInternalFormat(GL_RGBA);
    suntex->setSourceFormat(GL_RGBA);
    suntex->setSourceType(GL_UNSIGNED_SHORT);
    suntex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    suntex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

    if(!ogl::UseDeprecatedPath())
    {/*
        glareMaterial = new oglMaterial;

        SunFlareTex = ldpixLoadImageFileToSurface("sunglare1.bmp", colorBLACK, LDPIX_NONE);

        glareMaterial->AddTexture(0, SunFlareTex);
        glareMaterial->SetShader("glare");

        afterflareMaterial = new oglMaterial;
        afterflareMaterial->SetShader("basiccolor");
        */
    }
}
#endif

//////////////////////////////////////////////////////////////////////
//  CLensFlare::SetFlareLocation

void CLensFlare::SetFlareLocation(GLdouble x, GLdouble y)
{
    if (m_bEnabled)
    {
        FlareWinPos[0] = float(x);
        FlareWinPos[1] = float(y);

        FlarePos[0] = float(x) / (float)WantedWidth;
        FlarePos[1] = float(y) / (float)WantedHeight;
    }
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::SetBrightnesses

void CLensFlare::SetBrightnesses(float fBeforeBright, float fAfterBright)
{
    if (m_bEnabled)
    {
        m_fBeforeBright = fBeforeBright;
        AfterBright = fAfterBright;

        ClampBrightness( );
    }
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::ReadControlPixels

void CLensFlare::ReadControlPixels( )
{
#ifdef USE_OCCLUSION_QUERY
    if(GLEW_ARB_occlusion_query)
    {
        return;
    }
#else
    if(m_bEnabled && m_bFlareVisible)
    {
#if(0)// USE_STATESET
        depthread = true;
#else
        ReadDepthPixels(m_pControlPixels);
#endif
    }
#endif
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::ReadObstructionPixels

void CLensFlare::ReadObstructionPixels( )
{
    if (m_bEnabled && m_bFlareVisible)
    {
        ReadDepthPixels(m_pTestPixels);

        int nDifferent = 0;
        for (int i = 0; i < DepthTestDimension * DepthTestDimension; ++i)
            if (m_pTestPixels[i] != m_pControlPixels[i])
                ++nDifferent;

        m_fObstructionFactor = 1.0f - ((static_cast<float>(nDifferent) / (DepthTestDimension * DepthTestDimension)));
    }
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::AdjustBrightness
void CLensFlare::AdjustBrightness( )
{
    static bool samplequeried = false;
    if(m_bEnabled)// && m_bFlareVisible)
    {
        if(!g_sun)
        {
            g_sun = (OBJECT*)trnskyGetSunObject();
        }

        GLuint different = 0;

#ifdef USE_OCCLUSION_QUERY
        GLfloat percent = 0;

        if(GLEW_ARB_occlusion_query)
        {
#if USE_STATESET

            static ref_ptr<RenderStateSet> ss;

            if(!ss.valid())
            {
                ss = new RenderStateSet;
                ss->SetRenderState(new ColorMask(false, false, false, false));
                ss->SetRenderState(new DepthMask(false));
                ss->SetMode(GL_LIGHTING, 0);
            }

            if(samplequeried)
            {
                samplequeried = false;

                int available = 0;

                do
                {
                    glGetQueryObjectivARB(query, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
                } while(!available);

                glGetQueryObjectuivARB(query, GL_QUERY_RESULT_ARB, &different);

                percent = 1.0f;

                if(different > 0)
                {
                    percent = 1.0f - ((GLfloat)different / 2000.0f); //2000 WAG
                    percent = percent > 1.0f ? percent : percent < 0.0f ? 0.0f : percent;
                }

                AfterBright *= (1.0f - percent) * m_fObstructionFactor;
                m_fGlowBright = (AfterBright * 0.5f + percent) * m_fObstructionFactor * 0.85f;
            }

            OBJECT* o = g_sun->child;
            ss->Apply();

            // Render Sun (2 triangles)
            piSetWorldMatrix(&o->mat);

            ogl::SetVertexPointer(3, GL_FLOAT, 0, (GLvoid*)&sun[0]);

            glBeginQueryARB(GL_SAMPLES_PASSED_ARB, query);

			ogl::DrawArrays(GL_QUADS, 0, 4);

            glEndQueryARB(GL_SAMPLES_PASSED_ARB);

            samplequeried = true;
#else
            if(samplequeried)
            {
                samplequeried = false;

                int available = 0;

                do
                {
                    glGetQueryObjectivARB(query, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
                } while(!available);

                glGetQueryObjectuivARB(query, GL_QUERY_RESULT_ARB, &different);

                percent = 1.0f;

                if(different > 0)
                {
                    percent = 1.0f - ((GLfloat)different / 2000.0f); //2000 WAG
                    percent = percent > 1.0f ? percent : percent < 0.0f ? 0.0f : percent;
                }

                AfterBright *= (1.0f - percent) * m_fObstructionFactor;
                m_fGlowBright = (AfterBright * 0.5f + percent) * m_fObstructionFactor * 0.85f;
            }

            OBJECT* o = g_sun->child;

            UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
            UINT32 zwriteon = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);

            piSetRenderState(piRENDERSTATE_COLORMASK, 0);
            piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
            piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
            piDisableTexture();

            piMATRIX oldworld = *piGetWorldMatrix();

            // Render Sun (2 triangles)
            piSetWorldMatrix(&o->mat);

            ogl::SetVertexPointer(3, GL_FLOAT, 0, (GLvoid*)&sun[0]);

            glBeginQueryARB(GL_SAMPLES_PASSED_ARB, query);

			ogl::DrawArrays(GL_QUADS, 0, 4);

            glEndQueryARB(GL_SAMPLES_PASSED_ARB);

            samplequeried = true;

            piSetRenderState(piRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));
            piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
            piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwriteon);

            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                // Do nothing
            }
            else
            {
                AfterBright *= (1.0f - percent) * m_fObstructionFactor;
            }

            m_fGlowBright = (AfterBright * 0.5f + percent) * m_fObstructionFactor * 0.85f;

            piSetWorldMatrix(&oldworld);
#endif
        }
        else
#endif // USE_OCCLUSION_QUERY
        {
            if(m_bFlareVisible)
                ReadDepthPixels(m_pTestPixels);

            for (int i = 0; i < DepthTestDimension * DepthTestDimension; ++i)
                if (m_pTestPixels[i] != m_pControlPixels[i])
                    ++different;

            float fPercent = (static_cast<float>(different) / (DepthTestDimension * DepthTestDimension));
            float fAdjust = fPercent; //sqrtf(fPercent) * 0.85f;

            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                // Do nothing
            }
            else
            {
                AfterBright *= (1.0f - fAdjust) * m_fObstructionFactor;
            }

            m_fGlowBright = (AfterBright * 0.5f + fAdjust) * m_fObstructionFactor * 0.85f;
        }
    }
}

void CLensFlare::ReadDepthPixels(GLfloat* pPixels)
{
#ifdef USE_OCCLUSION_QUERY
    if(GLEW_ARB_occlusion_query)
    {
        return;
    }
    else
#endif
    {
        int nHalfSize = DepthTestDimension / 2;

        if (FlareWinPos[0] > nHalfSize &&
            FlareWinPos[1] > nHalfSize &&
            FlareWinPos[0] < piGetWidth() - nHalfSize &&
            FlareWinPos[1] < piGetHeight() - nHalfSize)
        {
            depthread = true;

#if TEST_PBO
            static int a = 0;
            static int b = 1;

            if(pbo[a]->isPBOSupported(0))
            {
                pbo[a]->bindBufferInWriteMode();

                // Read from current buffer
                glReadPixels(GLint(m_afFlareWinPos[0] - nHalfSize),
                            GLint(m_afFlareWinPos[1] - nHalfSize),
                            c_nDepthTestDimension, c_nDepthTestDimension,
                            GL_DEPTH_COMPONENT, GL_FLOAT, BUFFER_OFFSET(0));

                // Now retrieve data from the previous buffer
                pbo[b]->bindBufferInWriteMode();

                void* data = glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
                memcpy(pPixels, data, sizeof(float) * c_nDepthTestDimension * c_nDepthTestDimension);

                glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
                pbo[a]->unbindBuffer(0);

                // Swap 'a' and 'b' for next read
                a ^= b;
                b ^= a;
                a ^= b;
            }
            else
#endif
            {
                glReadPixels(GLint(FlareWinPos[0] - nHalfSize),
                            GLint(FlareWinPos[1] - nHalfSize),
                            DepthTestDimension, DepthTestDimension,
                            GL_DEPTH_COMPONENT, GL_FLOAT, pPixels);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////
//  CLensFlare::CharacterizeFlare

void CLensFlare::CharacterizeFlare(bool bEnabled, bool bShowMainFlare, float fMaxBrightness, float* pColor)
{
    memcpy(afColor, pColor, 3 * sizeof(float));
    m_bEnabled = bEnabled;
    m_bShowMainFlare = bShowMainFlare;
    m_fMaxBrightness = fMaxBrightness;
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::setFlareColor

void CLensFlare::setFlareColor(float* color)
{
    memcpy(afColor, color, 3 * sizeof(float));

    for(int i = 0; i < 7; i++)
    {
        g_afColors[i][0] = color[0];
        g_afColors[i][1] = color[1];
        g_afColors[i][2] = color[2];
    }
}

//////////////////////////////////////////////////////////////////////
//  CLensFlare::ClampBrightness

void CLensFlare::ClampBrightness( )
{
    // before
    if (m_fBeforeBright < 0.0f)
        m_fBeforeBright = 0.0f;
    else if (m_fBeforeBright > 1.0f)
        m_fBeforeBright = 1.0f;
    m_fBeforeBright *= m_fMaxBrightness;

    if (AfterBright < 0.0f)
        AfterBright = 0.0f;
    else if (AfterBright > 1.0f)
        AfterBright = 1.0f;
    AfterBright *= m_fMaxBrightness;
}




//////////////////////////////////////////////////////////////////////
//  CFlare implementation
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//  CFlare::CFlare

CFlare::CFlare( )
{
}


//////////////////////////////////////////////////////////////////////
//  CFlare::~CFlare

CFlare::~CFlare()
{
}


//////////////////////////////////////////////////////////////////////
//  CFlare::Init

#if USE_STATESET
void CFlare::Init(std::string strPath)
{
    int i = 0;
    while(g_strFiles[i] != "")
    {
        SFlarePiece cPiece;

        int id = ldpixLoadImageFileToSurface(g_strFiles[i].c_str(), colorBLACK, LDPIX_MIPMAP);
        cPiece.m_dlTex = ldTextureGetTextureObject(id);
        cPiece.m_fPosition = g_fPosition[i];
        cPiece.m_fWidth = g_fWidth[i];
        cPiece.m_pColor = g_afColors[i];
        m_vFlares.push_back(cPiece);
        i++;
    }
}
#else
void CFlare::Init(std::string strPath)
{
    int i = 0;
    while (g_strFiles[i] != "")
    {
        SFlarePiece cPiece;

        std::string str = strPath + "/" + std::string(g_strFiles[i]);
        int id = ldTextureLoad(str.c_str());
        cPiece.m_dlTex = ldTextureGetTextureObject(id);
        cPiece.m_fPosition = g_fPosition[i];
        cPiece.m_fWidth = g_fWidth[i];
        cPiece.m_pColor = g_afColors[i];
        m_vFlares.push_back(cPiece);
        i++;
    }
}
#endif

#if USE_STATESET
void CFlare::DrawDeprecated(float BrightScale, int nX, int nY)
{
    extern Texture* obdrawGetSceneDepthTexture();

    float centerX, centerY;

    TextureUnit* tu = dynamic_cast<TextureUnit*>(flarestateset->GetRenderState(RS_TEXTURE_UNIT0));

    for(unsigned int i = 0; i < m_vFlares.size(); i++)
    {
        if(i == 0 || i == 1 || i == 7)
        {
            centerX = (float)nX;
            centerY = (float)nY;

            float fW = m_vFlares[i].m_fWidth;// * BrightScale;

            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

            if(!obdrawIsRenderPass(RENDER_REFLECTION))
            {
                r = m_vFlares[i].m_pColor[0] * BrightScale;
                g = m_vFlares[i].m_pColor[1] * BrightScale;
                b = m_vFlares[i].m_pColor[2] * BrightScale;
                a = m_vFlares[i].m_pColor[3] * BrightScale;
            }

            tu->SetTexture(m_vFlares[i].m_dlTex, GL_TEXTURE_2D);
            tu->apply();

            drawquad(centerX - fW,
                     centerY - fW,
                     centerX + fW,
                     centerY + fW,
                     piRGBA(r, g, b, a));
        }
    }
}
#else
//////////////////////////////////////////////////////////////////////
//  CFlare::DrawDeprecated

void CFlare::DrawDeprecated(float BrightScale, int nX, int nY)
{
    float centerX, centerY;

    for (unsigned int i = 0; i < m_vFlares.size( ); i++)
    {
        switch(i)
        {
            // Sun glare
        case 0:
        case 1:
        case 7:
            {
                centerX = (float)nX;
                centerY = (float)nY;
                break;
            }
        default:
            // Save for lens flare
            continue;
            // Lens flare
            //centerX = float(nX) - (m_vFlares[i].m_fPosition + 1.0f)*fDX;
            //centerY = float(nY) - (m_vFlares[i].m_fPosition + 1.0f)*fDY;
        }

        float fW = m_vFlares[i].m_fWidth;// * fBrightScale;

        float afColor[4];

        if(obdrawIsRenderPass(RENDER_REFLECTION))
        {
            afColor[0] = 1;
            afColor[1] = 1;
            afColor[2] = 1;
            afColor[3] = 1;
        }
        else
        {
            afColor[0] = m_vFlares[i].m_pColor[0] * BrightScale;
            afColor[1] = m_vFlares[i].m_pColor[1] * BrightScale;
            afColor[2] = m_vFlares[i].m_pColor[2] * BrightScale;
            afColor[3] = m_vFlares[i].m_pColor[3] * BrightScale;
        }

        TextureState::Bind(m_vFlares[i].m_dlTex, GL_TEXTURE_2D);

        piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_MODULATE);

        oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(afColor[0], afColor[1], afColor[2], afColor[3]));
        glBegin(GL_POLYGON);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(centerX - fW, centerY - fW);

            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(centerX + fW, centerY - fW);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(centerX + fW, centerY + fW);

            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(centerX - fW, centerY + fW);
        glEnd( );
    }

    piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
    piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
}
#endif

void CFlare::Draw(float BrightScale, int X, int Y)
{
    oglSetRenderState(oglRENDERSTATE_SRCBLEND, piBLEND_SRCALPHA);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, piBLEND_ONE);

    float centerX, centerY;

    glareMaterial->Apply();

    for(unsigned int q =  m_vFlares.size(); q != 0; --q)
    {
        int idx = q - 1;

        if(idx == 0 || idx == 1 || idx == 7)
        {
            centerX = (float)X;
            centerY = (float)Y;

            float fW = m_vFlares[idx].m_fWidth;// * fBrightScale;

            float afColor[4];

            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                afColor[0] = 1;
                afColor[1] = 1;
                afColor[2] = 1;
                afColor[3] = 1;
            }
            else
            {
                afColor[0] = m_vFlares[idx].m_pColor[0] * BrightScale;
                afColor[1] = m_vFlares[idx].m_pColor[1] * BrightScale;
                afColor[2] = m_vFlares[idx].m_pColor[2] * BrightScale;
                afColor[3] = m_vFlares[idx].m_pColor[3] * BrightScale;
            }

            drawquad(centerX - fW,
                     centerY - fW,
                     centerX + fW,
                     centerY + fW,
                     piRGBA(afColor[0], afColor[1], afColor[2], afColor[3]));
        }

    }

    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
}
