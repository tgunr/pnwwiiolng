#include <syscfg.h> // Path is project specific.
/*
 *  FTFontReplacement.cpp
 *  Replacement for FTGL/Freetype
 *
 *  Created by Andrew Wulf on 11/17/10.
 *
 */

#include "ogl/GLCounter.h"
#include "phui/FTFontReplacement.h"
#include <vector>

static const size_t MAX_CHAR_IN_LINE = 256;

#define xFT_DEBUG

#ifdef FT_DEBUG
#define PRINT_STR(func,x) { printf("%s: %s\n",func,x); }
#define PRINT_WSTR(func,x) { printf("%s: %ls\n",func,x); }
#define PRINT_FUNC(func) { printf("%s\n",func); }
#define PRINT_INT(func,n) { printf("%s: %d\n",func,n); }
#define PRINT_FL(func,n) { printf("%s: %.3f\n",func,n); }
#define PRINT_L(func,n) { printf("%s: %ld\n",func,n); }

#define CHECK_GL { GLenum e = glGetError(); if ( e != 0 ) printf("gl err=%x\n", e ); }


#else
#define PRINT_STR(func,x)
#endif

// ===================================================================================
// Alternative font/text system that uses pre-generated textures and metrics files
// ===================================================================================

#include "pi/piogl.h"
#include "ogl/ogl.h"
#include "ld/ldpix.h"
#include "ld/ldtexture.h"
#include "fio/fio.h"
#include <iostream>
#include <sstream>
#include "wwii/wwiiHash.h"
#include "draw/fontadaptor.h"
#include "draw/TextureFont.h"

using namespace std;

extern Image* ReadDDSFile(const std::string& name);

/*
 format of .fnt file

 info face="WenQuanYi Zen Hei" size=32 bold=0 italic=0 charset="" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1 outline=0
 common lineHeight=32 base=25 scaleW=256 scaleH=256 pages=1 packed=0 alphaChnl=1 redChnl=4 greenChnl=4 blueChnl=4
 page id=0 file="western_0.dds"
 chars count=260
 char id=0    x=14    y=26    width=1     height=1     xoffset=0     yoffset=25    xadvance=0     page=0  chnl=15
 */

//static GLuint texID=0;
//static int textureID=0;

//static CharDataMap cmap;
//static float lineheight;
//static float baseline;
//static float baseFontSize;
//static float imageWidth;
//static float imageHeight;

static bool FontSystemInited = false;

static TextureFont* font10;
static TextureFont* font11;
static TextureFont* font12;
static TextureFont* font13;
static TextureFont* font14;
static TextureFont* font15;
static TextureFont* font16;
static TextureFont* font17;
static TextureFont* font18;
static TextureFont* font20;
static TextureFont* font24;
static TextureFont* font26;

static GLfloat vertexes[(MAX_CHAR_IN_LINE+1)*12];
static GLfloat texes[(MAX_CHAR_IN_LINE+1)*12];

// ------------------------------------------------------------------

ScaledCharData::ScaledCharData( float scale, float imWidth, float imHeight, const CharData& cdata)
{
    width      =  cdata.width     * scale;
    height     =  cdata.height    * scale;
    xoffset    =  cdata.xoffset   * scale;
    yoffset    =  cdata.yoffset   * scale;
    xadvance   =  cdata.xadvance  * scale;
    x          =  cdata.x         / imWidth;
    y          =  cdata.y         / imHeight;
    charWidth  =  cdata.width     / imWidth;
    charHeight =  cdata.height    / imHeight;
}

// ------------------------------------------------------------------

FTFontWestern::FTFontWestern()
{
    if ( !FontSystemInited )
    {
        init(); // done this way because of phui
    }
    mSize = 0;

}

// ------------------------------------------------------------------

#define FONT_WESTERN "Fonts/western"
#define FONT_TYPEWRITER "Fonts/typewriter"

char* use_font = FONT_WESTERN;
bool font_is_old_western = true;

void FTFontWestern::init()
{
    if ( AltFont )
    {
        use_font = FONT_TYPEWRITER;
        font_is_old_western = false;
    }
    font10 = new TextureFont(use_font,10);
    if ( ! font_is_old_western )
    {
        font11 = new TextureFont(use_font,11);
    }
    font12 = new TextureFont(use_font,12);
    font13 = new TextureFont(use_font,13);
    font14 = new TextureFont(use_font,14);
    font15 = new TextureFont(use_font,15);
    font16 = new TextureFont(use_font,16);
    font17 = new TextureFont(use_font,17);
    font18 = new TextureFont(use_font,18);
    font20 = new TextureFont(use_font,20);
    font24 = new TextureFont(use_font,24);
    font26 = new TextureFont(use_font,26);

    FontSystemInited = true;
}

// ------------------------------------------------------------------

float FTFontWestern::Ascender() const
{
    return (mFont->baseline) * fontScale;
}

// ------------------------------------------------------------------

float FTFontWestern::Descender() const
{
    return -(mFont->lineheight-mFont->baseline) * fontScale;
}

// ------------------------------------------------------------------

int FTFontWestern::Error() const
{
    return 0;
}

// ------------------------------------------------------------------

unsigned int FTFontWestern::GetFaceSize() const
{
    return mSize;
}

// ------------------------------------------------------------------

bool FTFontWestern::FaceSize(const unsigned int sz)
{
    assert( mSize == 0 );
    unsigned int adjust = font_is_old_western?2:1;
    unsigned int size = sz + adjust; // make things a bit bigger

    if ( size < 11 )
    {
        mFont = font10;
    }
    else if ( size == 11 && !font_is_old_western )
    {
        mFont = font11;
    }
    else if ( size <= 12 )
    {
        mFont = font12;
    }
    else if ( size == 13 )
    {
        mFont = font13;
    }
    else if ( size == 14 )
    {
        mFont = font14;
    }
    else if ( size == 15 )
    {
        mFont = font15;
    }
    else if ( size == 16 )
    {
        mFont = font16;
    }
    else if ( size == 17 )
    {
        mFont = font17;
    }
    else if ( size <= 20 )
    {
        mFont = font18;
    }
    else if ( size <= 23 )
    {
        mFont = font20;
    }
    else if ( size <= 25 )
    {
        mFont = font24;
    }
    else if ( size > 25 )
    {
        mFont = font26;
    }


    mSize = size;
    fontScale = mSize / mFont->baseFontSize;

    //printf("size:%d scale:%.2f\n",size,fontScale);

    scaledBaseline = mFont->baseline * fontScale;

    for ( CharDataMap::iterator it = mFont->cmap.begin(); it != mFont->cmap.end(); it++ )
    {
        scaledChars.insert( pair<int,ScaledCharData>((*it).first, ScaledCharData( fontScale, mFont->imageWidth, mFont->imageHeight, (*it).second )));
    }

    return true;
}

// ------------------------------------------------------------------

void FTFontWestern::saveState()
{
    assert( mSize != 0 );
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_TEXTURE_BIT);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_INDEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

// ------------------------------------------------------------------

void FTFontWestern::restoreState()
{
    glPopClientAttrib();
    glPopAttrib();
}

// ------------------------------------------------------------------

float FTFontWestern::Advance(const char* str)
{
    float advance = 0.0;

    int len = strlen(str);
    for ( int i=0; i<len; i++ )
    {
        char c = str[i];
        ScaledCharDataMap::iterator it = scaledChars.find(c);
        if ( it != scaledChars.end() )
        {
            ScaledCharData& cdata = (*it).second;
            advance += cdata.xadvance;
        }
    }

    return advance;
}

// ------------------------------------------------------------------

float FTFontWestern::Advance(const wchar_t* str)
{
    float advance = 0.0;

    int len = wcslen(str);
    for ( int i=0; i<len; i++ )
    {
        wchar_t c = str[i];
        ScaledCharDataMap::iterator it = scaledChars.find(c);
        if ( it != scaledChars.end() )
        {
            ScaledCharData& cdata = (*it).second;
            advance += cdata.xadvance;
        }
    }

    return advance;
}
// ------------------------------------------------------------------

#if platOS == OS_MACOS
extern pthread_t mainthread;
#endif

#define ALT_DRAW_TEXT

typedef struct FLPoint { float x,y; } FLPoint;

template<class T>
void FTFontWestern::RenderText( const T* str, int len )
{
    saveState();

    float b = scaledBaseline;

    float advance = 0.0;

    glBindTexture(GL_TEXTURE_2D, mFont->texID);//CHECK_GL;

#ifdef ALT_DRAW_TEXT

    GLfloat* t = &texes[0];
    GLfloat* v = &vertexes[0];
    int tris = 0;

    for ( int i=0; i<len; i++ )
    {
        wchar_t c = (wchar_t)str[i];
        ScaledCharDataMap::iterator it = scaledChars.find(c);
        if ( it != scaledChars.end() ) // skips non-printable character
        {
            ScaledCharData& cdata = (*it).second;

            FLPoint p0 = {advance+cdata.xoffset,b-(cdata.height+cdata.yoffset)};
            FLPoint t0 = {cdata.x,cdata.y+cdata.charHeight};

            FLPoint p1 = {advance+cdata.width+cdata.xoffset,b-(cdata.height+cdata.yoffset)};
            FLPoint t1 = {cdata.x+cdata.charWidth,cdata.y+cdata.charHeight};

            FLPoint p2 = {advance+cdata.width+cdata.xoffset,b-cdata.yoffset};
            FLPoint t2 = {cdata.x+cdata.charWidth,cdata.y};

            FLPoint p3 = {advance+cdata.xoffset,b-cdata.yoffset};
            FLPoint t3 = {cdata.x,cdata.y};

            *t++ = t0.x; *t++ = t0.y;
            *t++ = t1.x; *t++ = t1.y;
            *t++ = t2.x; *t++ = t2.y;
            *t++ = t0.x; *t++ = t0.y;
            *t++ = t2.x; *t++ = t2.y;
            *t++ = t3.x; *t++ = t3.y;

            *v++ = p0.x; *v++ = p0.y;
            *v++ = p1.x; *v++ = p1.y;
            *v++ = p2.x; *v++ = p2.y;
            *v++ = p0.x; *v++ = p0.y;
            *v++ = p2.x; *v++ = p2.y;
            *v++ = p3.x; *v++ = p3.y;

            tris++;
            advance += cdata.xadvance;
        }
        else
        {
            //printf("No char %d (%lc)\n", c, c);  some char=9 in data somewhere
        }
    }

    glVertexPointer( 2, GL_FLOAT, 0, &vertexes[0] );
    glTexCoordPointer( 2, GL_FLOAT, 0, &texes[0] );

//    GLCounter::instance().call_glDrawArrays( GL_QUADS, 0, vertexes.size()/2 );


#if platOS == OS_MACOS
    if ( mainthread && ! pthread_equal( pthread_self(), mainthread ) )
    {
        printf("drawing text not on main thread\n");
    }
#endif
    glDrawArrays( GL_TRIANGLES, 0, tris*6 );

#else

    glBegin(GL_QUADS);

    for ( int i=0; i<len; i++ )
    {
        wchar_t c = (wchar_t)str[i];
        ScaledCharDataMap::iterator it = scaledChars.find(c);
        if ( it != scaledChars.end() ) // skips non-printable character
        {
            ScaledCharData& cdata = (*it).second;

            glTexCoord2f( (cdata.x), (cdata.y+cdata.charHeight) );
            glVertex2f(advance+cdata.xoffset, b-(cdata.height+cdata.yoffset));

            glTexCoord2f( (cdata.x+cdata.charWidth), (cdata.y+cdata.charHeight) );
            glVertex2f(advance+cdata.width+cdata.xoffset, b-(cdata.height+cdata.yoffset));

            glTexCoord2f( (cdata.x+cdata.charWidth), (cdata.y) );
            glVertex2f(advance+cdata.width+cdata.xoffset, b-cdata.yoffset);

            glTexCoord2f( (cdata.x), (cdata.y) );
            glVertex2f(advance+cdata.xoffset, b-cdata.yoffset);

            advance += cdata.xadvance;
        }
        else
        {
            //printf("No char %d (%lc)\n", c, c);
        }
    }

    glEnd();
#endif

    restoreState();
}

// ------------------------------------------------------------------

void FTFontWestern::Render(const wchar_t* str)
{
    size_t len = wcslen(str);
    if ( len >= MAX_CHAR_IN_LINE )
    {
        //printf("More than %lu chars in line = %lu\n", MAX_CHAR_IN_LINE, len);
        len = MAX_CHAR_IN_LINE;
    }

    RenderText( str, len );

}

// ------------------------------------------------------------------

void FTFontWestern::Render(const char* str)
{
    size_t len = strlen(str);
    if ( len >= MAX_CHAR_IN_LINE )
    {
        //printf("More than %lu chars in line = %lu\n", MAX_CHAR_IN_LINE, len );
        len = MAX_CHAR_IN_LINE;
    }

    RenderText( str, len );
}

// ------------------------------------------------------------------

int FontAdaptor::getHeight()
{
    return (int)(font->Ascender() - font->Descender());
}

// ------------------------------------------------------------------

SINT32 FontAdaptor::getStringWidth(const WCHAR*s)
{
    return (int)font->Advance(s);
}

// ------------------------------------------------------------------

LOGICAL FontAdaptor::get3DString(const WCHAR* pszText, UINT32 uColor, piLVERTEX* pVertices, SINT32 iNumVertices)
{
    float b = font->scaledBaseline;
    float advance = 0.0;

    int len = wcslen(pszText);

    for ( int i=0; i<len; i++ )
    {
        int n = -1;
        int m = i*6;
        wchar_t c = (wchar_t)pszText[i];
        ScaledCharDataMap::iterator it = font->scaledChars.find(c);
        if ( it != font->scaledChars.end() ) // skips non-printable character
        {
            ScaledCharData& cdata = (*it).second;

            /*    0   1
             3   2
             */

            //--- 3

            n++;
            pVertices[m + n].x = advance+cdata.xoffset;
            pVertices[m + n].y = b-cdata.yoffset;

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x;
            pVertices[m + n].tv = cdata.y;

            //--- 2

            n++;
            pVertices[m + n].x = advance+cdata.width+cdata.xoffset;
            pVertices[m + n].y = b-cdata.yoffset;

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x+cdata.charWidth;
            pVertices[m + n].tv = cdata.y;

            //--- 0

            n++;
            pVertices[m + n].x = advance+cdata.xoffset;
            pVertices[m + n].y = b-(cdata.height+cdata.yoffset);

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x;
            pVertices[m + n].tv = cdata.y+cdata.charHeight;

            //--- 2

            n++;
            pVertices[m + n].x = advance+cdata.width+cdata.xoffset;
            pVertices[m + n].y = b-cdata.yoffset;

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x+cdata.charWidth;
            pVertices[m + n].tv = cdata.y;

            //--- 1

            n++;
            pVertices[m + n].x = advance+cdata.width+cdata.xoffset;
            pVertices[m + n].y = b-(cdata.height+cdata.yoffset);

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x+cdata.charWidth;
            pVertices[m + n].tv = cdata.y+cdata.charHeight;

            //--- 0

            n++;
            pVertices[m + n].x = advance+cdata.xoffset;
            pVertices[m + n].y = b-(cdata.height+cdata.yoffset);

            pVertices[m + n].z = 0.0f;
            pVertices[m + n].specular = colorALPHA100;
            pVertices[m + n].color = uColor;

            pVertices[m + n].tu = cdata.x;
            pVertices[m + n].tv = cdata.y+cdata.charHeight;

            advance += cdata.xadvance;
        }
    }

    return false;
}

// ------------------------------------------------------------------

SINT32 FontAdaptor::getTextureId()
{
    return font->mFont->textureID; // internal texture id, not opengl
}

// ------------------------------------------------------------------

FontAdaptor::FontAdaptor(FTFontWestern* f)
: font(f)
{
}
