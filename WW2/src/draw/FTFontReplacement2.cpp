#include <syscfg.h> // Path is project specific.
/*
 *  FTFontReplacement2.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 2/12/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#include "phui/FTFontReplacement.h"

// ===================================================================================
// Alternative font/text system that updates a texture on the fly based on used chars
// ===================================================================================
#if FONT_SYSTEM == 2

#include "ogl/ogl.h"
#include "ld/ldpix.h"
#include "ld/ldtexture.h"
#include "fio.h"
#include <iostream>
#include <sstream>
#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "draw/fontadaptor.h"

using namespace std;
USING_EXT_NAMESPACE;

extern SINT32 frametime;

struct CharacterInfo
{
    unsigned long boxNumber;
    float pixWidth, pixHeight, pixX, pixY; // in texture coord
    float advance, width, height, offset; // in pixels
};

using CharacterInfoMap = stdext::hash_map<unsigned int,CharacterInfo>;

class FTFontMaster
{
private:

    static const int TextureWidth = 2048;
    static const int TextureHeight = 2048;
    static const float TextureWidthF = TextureWidth*1.0;
    static const float TextureHeightF = TextureHeight*1.0;
    static const int CannedFontSquare = 34;
    static const float CannedFontSquareWidthF = CannedFontSquare / (TextureWidth*1.0);
    static const float CannedFontSquareHeightF = CannedFontSquare / (TextureHeight*1.0);
    static const int MaxBoxesAcrossInTexture = TextureWidth/CannedFontSquare;
    static const int MaxBoxesInTexture = TextureWidth/CannedFontSquare * TextureHeight/CannedFontSquare;

    GLuint mTexID;

    CharacterInfoMap mCharacterTextureInfo;
    unsigned int mBoxCount;

    FT_Library mLibrary;
    FT_Face mFace;

    float ascender, descender, height;

    static FTFontMaster* fontMaster;

public:

    static const int CannedFontSize = 32;
    static const int MaxCharactersInLine = 128;

    static FTFontMaster* getFontMaster();

    GLuint getTexID() { return mTexID; }

    float getAscender() { return ascender; }    // at CannedFontSize
    float getDescender() { return descender; }  // at CannedFontSize
    float getHeight() { return height; }        // at CannedFontSize

    const CharacterInfo* getCharacterInfo( unsigned int c )
    {
        CharacterInfoMap::iterator iter = mCharacterTextureInfo.find(c);
        if ( iter == mCharacterTextureInfo.end() )
        {
            assert(false);
            return NULL; // should never happen!
        }
        return &(*iter).second;
    }

    FTFontMaster(const char* fontPath)
    {
        mBoxCount = 0;

        GLenum e = glGetError();

        glGenTextures( 1, &mTexID );

        printf("id=%d\n",mTexID);

        e = glGetError();
        glBindTexture(GL_TEXTURE_2D, mTexID);
        e = glGetError();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); e = glGetError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); e = glGetError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); e = glGetError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); e = glGetError();

        GLvoid* tex = malloc(TextureWidth*TextureHeight*sizeof(long));
        memset(tex, 255, TextureWidth*TextureHeight*sizeof(long) );

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TextureWidth, TextureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, tex ); e = glGetError();

        free(tex);

        int error = FT_Init_FreeType( &mLibrary );

        assert(error==0);

        char* buffer;
        unsigned int bufferSize;
        if ( ! fioLoad(fontPath, buffer, &bufferSize) )
        {
            throw "Can't load font";
        }

        error = FT_New_Memory_Face( mLibrary, (unsigned char*)buffer, bufferSize, 0, &mFace );
        assert(error==0);
        error = FT_Set_Pixel_Sizes( mFace, 0, CannedFontSize );
        assert(error==0);

        ascender = mFace->ascender / (float)mFace->units_per_EM * CannedFontSize;
        descender = mFace->descender / (float) mFace->units_per_EM * CannedFontSize;
        height = mFace->height / (float)mFace->units_per_EM * CannedFontSize;

        int x =1;
    }

    template<class T>
    void updateCharacters( const T* str )
    {
        const T* p = str;
        int notFoundCount = 0, n = 0;
        bool notFountArray[MaxCharactersInLine];

        while (*p!=0 && n < MaxCharactersInLine)
        {
            unsigned int c = (unsigned int)*p;

            CharacterInfoMap::iterator iter = mCharacterTextureInfo.find(c);
            if ( iter == mCharacterTextureInfo.end() )
            {
                notFoundCount++;
                notFountArray[n] = true;
            }
            else
            {
                notFountArray[n] = false;
            }
            p++;
            n++;
        }

        if ( notFoundCount )
        {
            if ( notFoundCount+mBoxCount >= MaxBoxesInTexture ) // out of room
            {
                mBoxCount = 0; // start over
                notFoundCount = n;
                mCharacterTextureInfo.clear();
                printf("START OVER\n");
            }

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mTexID);

            assert( notFoundCount+mBoxCount >= MaxBoxesInTexture );

            GLenum err = glGetError();
            assert(err == 0 );

            for (int i=0; i<n; i++ )
            {
                if ( notFoundCount == n || notFountArray[i] )
                {
                    unsigned int c = str[i];
                    CharacterInfoMap::iterator iter = mCharacterTextureInfo.find(c);
                    if ( iter != mCharacterTextureInfo.end() ) // already did this char in this loop
                    {
                        continue;
                    }

                    //render character

                    int error = FT_Load_Char( mFace, c, FT_LOAD_RENDER );
                    assert(error==0);

                    int ascentOffset = mFace->ascender >> 6;

                    assert( mFace->glyph->bitmap.pitch <= CannedFontSquare );
                    assert( mFace->glyph->bitmap.rows + ascentOffset <= CannedFontSquare );

                    // copy to square buffer

                    unsigned long* buffer = (unsigned long*)calloc(CannedFontSquare*CannedFontSquare, sizeof(unsigned long) ); // rgba = 4 bytes
                    unsigned long* destrow = buffer;

                    unsigned char* p = mFace->glyph->bitmap.buffer;
                    for ( int i=0; i<mFace->glyph->bitmap.rows; i++ )
                    {
                        unsigned char* from = p+i*mFace->glyph->bitmap.pitch;
                        unsigned char* to = (unsigned char*)(destrow+i*CannedFontSquare);
                        for ( int j=0; j<mFace->glyph->bitmap.width; j++ )
                        {
                            unsigned char c = *from++;
                            *to++ = 255;
                            *to++ = 255;
                            *to++ = 255;
                            *to++ = c;
                        }
                    }

                    // copy to texture

                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    (mBoxCount%MaxBoxesAcrossInTexture)*CannedFontSquare,
                                    (mBoxCount/MaxBoxesAcrossInTexture)*CannedFontSquare,
                                    CannedFontSquare,
                                    CannedFontSquare,
                                    GL_BGRA,
                                    GL_UNSIGNED_BYTE,
                                    buffer);

                    err = glGetError();
                    assert(err == 0 );

                    // update data structure

                    CharacterInfo ci;

                    ci.boxNumber = mBoxCount;
                    ci.pixWidth = mFace->glyph->bitmap.width / TextureWidthF;
                    ci.pixHeight = (mFace->glyph->bitmap.rows) / TextureHeightF;
                    ci.pixX = ((mBoxCount%MaxBoxesAcrossInTexture)*CannedFontSquare) / TextureWidthF;
                    ci.pixY = ((mBoxCount/MaxBoxesAcrossInTexture)*CannedFontSquare) / TextureHeightF;
                    ci.advance = mFace->glyph->metrics.horiAdvance >> 6;
                    ci.width = mFace->glyph->metrics.width >> 6;
                    ci.height = mFace->glyph->metrics.height >> 6;
                    ci.offset = mFace->glyph->metrics.vertBearingY >> 6;

                    mCharacterTextureInfo[c] = ci;

                    free(buffer);
                    mBoxCount++;
                }
            }

            glPopClientAttrib();
            glPopAttrib();
        }

    }

};

FTFontMaster* FTFontMaster::fontMaster = NULL;

FTFontMaster* FTFontMaster::getFontMaster()
{
    if ( FTFontMaster::fontMaster == NULL )
    {
        FTFontMaster::fontMaster = new FTFontMaster("Fonts/wqy-zenhei.ttf");
    }
    return FTFontMaster::fontMaster;
}

// ------------------------------------------------------------------

FTFont::FTFont()
{
    FTFontMaster::getFontMaster();
    mSize = 0;
}

// ------------------------------------------------------------------

FTGLTextureFont::FTGLTextureFont(unsigned char* , size_t ) : FTFont()
{
}

// ------------------------------------------------------------------

void FTFont::init()
{
}

// ------------------------------------------------------------------

float FTFont::Ascender()
{
    float f= FTFontMaster::getFontMaster()->getAscender();
    return f;
}

// ------------------------------------------------------------------

float FTFont::Descender()
{
    float f= FTFontMaster::getFontMaster()->getDescender();
    return f;
}

// ------------------------------------------------------------------

int FTFont::Error()
{
    return 0;
}

// ------------------------------------------------------------------

unsigned int FTFont::FaceSize()
{
    return mSize;
}

// ------------------------------------------------------------------

bool FTFont::FaceSize(int size)
{
    assert( mSize == 0 );

    mSize = size;

    return true;
}

// ------------------------------------------------------------------

void FTFont::saveState()
{
    assert( mSize != 0 );
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

// ------------------------------------------------------------------

void FTFont::restoreState()
{
    glPopClientAttrib();
    glPopAttrib();
}

// ------------------------------------------------------------------

template<class T>
float FTFont::AdvanceText( const T* str, int len )
{
    float advance = 0;

    for (int i=0; i<len; i++ )
    {
        unsigned int c = (unsigned int)str[i];
        const CharacterInfo* ci;
        if ( ci=FTFontMaster::getFontMaster()->getCharacterInfo(c) )
        {
            advance += ci->advance;
        }
    }

    return advance;
}

// ------------------------------------------------------------------

float FTFont::Advance(const char* str)
{
    FTFontMaster::getFontMaster()->updateCharacters(str);
    return AdvanceText( str, strlen(str) );
}

// ------------------------------------------------------------------

float FTFont::Advance(const wchar_t* str)
{
    FTFontMaster::getFontMaster()->updateCharacters(str);
    return AdvanceText( str, wcslen(str) );
}

// ------------------------------------------------------------------

template<class T>
void FTFont::RenderText( const T* str, int len )
{
    saveState();

    float advance = 0.0;
    float b = 0;

    glBindTexture(GL_TEXTURE_2D, FTFontMaster::getFontMaster()->getTexID());
	glBegin(GL_QUADS);

    for (int i=0; i<len; i++ )
    {
        unsigned int c = str[i];
        const CharacterInfo* ci;
        if ( ci=FTFontMaster::getFontMaster()->getCharacterInfo(c) )
        {
            glTexCoord2f( (ci->pixX), (ci->pixY+ci->pixHeight) );
            glVertex2f(advance, b-(ci->height)+ci->offset);

            glTexCoord2f( (ci->pixX+ci->pixWidth), (ci->pixY+ci->pixHeight) );
            glVertex2f(advance+ci->width, b-(ci->height)+ci->offset);

            glTexCoord2f( (ci->pixX+ci->pixWidth), (ci->pixY) );
            glVertex2f(advance+ci->width, b+ci->offset);

            glTexCoord2f( (ci->pixX), (ci->pixY) );
            glVertex2f(advance, b+ci->offset);

            advance += ci->advance;
        }
    }

    glEnd();

    restoreState();
}

// ------------------------------------------------------------------

void FTFont::Render(const wchar_t* str)
{
    FTFontMaster::getFontMaster()->updateCharacters(str);

    int len = wcslen(str);
    if ( len >= FTFontMaster::MaxCharactersInLine )
    {
        printf("More than %d chars in line = %d\n", FTFontMaster::MaxCharactersInLine, len );
        len = FTFontMaster::MaxCharactersInLine;
    }

    RenderText( str, len );
}

// ------------------------------------------------------------------

void FTFont::Render(const char* str)
{
    FTFontMaster::getFontMaster()->updateCharacters(str);

    int len = strlen(str);
    if ( len >= FTFontMaster::MaxCharactersInLine )
    {
        printf("More than %d chars in line = %d\n", FTFontMaster::MaxCharactersInLine, len );
        len = FTFontMaster::MaxCharactersInLine;
    }

    RenderText( str, len );
}


#endif

