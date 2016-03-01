#pragma once
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

#include <float.h>
#include "pi/pi.h"
#include "osgImport/Texture.h"

#include <string>
#include <vector>

#define USE_OCCLUSION_QUERY
#if platOS != OS_MACOS
#if !USE_STATESET
#undef USE_OCCLUSION_QUERY
#endif
#endif

//////////////////////////////////////////////////////////////////////
//  Constants

class CLensFlare;


//////////////////////////////////////////////////////////////////////
//  CFlare

class CFlare
{
friend class CLensFlare;

public:
    void DrawDeprecated(float fBrightScale, int nX, int nY);

    void Draw(float fBrightScale, int nX, int nY);
    void Init(std::string strPath);

    CFlare();
    virtual ~CFlare();


private:
        struct SFlarePiece
        {
            SFlarePiece( ) :
            m_dlTex(0),
            m_fPosition(0.0f),
            m_fWidth(0.0f),
            m_pColor(nullptr)
        {
        }

            GLuint  m_dlTex;
            float   m_fPosition;    // -1.0 = light location, 0.0 = center, 1.0 = far end of flare
            float   m_fWidth;       // height = width
            float*  m_pColor;
        };

        std::vector<SFlarePiece>    m_vFlares;
};


//////////////////////////////////////////////////////////////////////
//  CLensFlare

class CLensFlare
{
public:
    CLensFlare( );
    ~CLensFlare( );

    void DrawBeforeFlare();
    void DrawBeforeFlareDeprecated();

        void            DrawAfterFlare(float brightness);
        void            DrawFlare();

        void            SetMainFlare(std::string strSunFile, float fSunSize);
        void            Initialize(std::string strPath);

        void            SetFlareLocation(GLdouble dX, GLdouble dY);
        void            setFlareColor(float* color);
        void            SetVisible(bool bState)    { m_bFlareVisible = bState; }
        void            SetBrightnesses(float fBeforeBright, float fAfterBright);

        void            ReadControlPixels( );
        void            ReadObstructionPixels( );
        void            AdjustBrightness( );

        void            CharacterizeFlare(bool bEnabled, bool bShowMainFlare, float fMaxBrightness, float* pColor);

        float           getAfterBright() const;
        bool            isVisible() {return m_bFlareVisible;}
        Texture*        GetSunTexture();

private:
    float SunSize;
    float FlarePos[2];
    float FlareWinPos[2];

        float           m_fBeforeBright, AfterBright;
        bool            m_bFlareVisible, m_bDrawFlare, m_bDrawBrightScreen;
        bool            depthread;
        CFlare          m_cFlare;
        GLfloat*        m_pControlPixels;
        GLfloat*        m_pTestPixels;
        bool            m_bEnabled;
        bool            m_bShowMainFlare;
        float           m_fMaxBrightness;
        float           afColor[4];
        float           m_fGlowBright;
        float           m_fObstructionFactor;
#ifdef USE_OCCLUSION_QUERY
        GLuint query;
        std::vector<float> sun;
#endif
        void            ReadDepthPixels(GLfloat* pPixels);
        void            ClampBrightness( );
};

extern CLensFlare g_lensFlare;

