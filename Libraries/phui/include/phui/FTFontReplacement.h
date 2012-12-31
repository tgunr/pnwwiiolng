/*
 *  FTFontReplacement.h
 *  phui
 *
 *  Created by Andrew Wulf on 11/17/10.
 *
 */

// generic font wrapper

class FTFontAdaptor
{
public:
    FTFontAdaptor() {}
    virtual float Advance(const char* str)=0;
    virtual float Advance(const wchar_t* str)=0;
    virtual void Render(const wchar_t* str)=0;
    virtual void Render(const char* str)=0;  
    virtual float Ascender() const=0;
    virtual float Descender() const=0;
    virtual int Error() const=0;
    virtual unsigned int GetFaceSize() const=0;
    virtual bool FaceSize(const unsigned int)=0;
};

//----------------------------------------------
//  old font system
//----------------------------------------------

#include "FTGL/ftgl.h"

// adapted version of the regular FTFont

class FTFontAdapted : public FTFontAdaptor
{
public:
    FTFontAdapted(FTFont*font);
    float Advance(const char* str);
    float Advance(const wchar_t* str);
    void Render(const wchar_t* str);
    void Render(const char* str);  
    
    float Ascender() const;
    float Descender() const;
    int Error() const;
    unsigned int GetFaceSize() const;
    bool FaceSize(const unsigned int);
private:
    FTFont* mFont;
};

//----------------------------------------------
//  china font system (works on non-china also)
//----------------------------------------------

#ifndef __APPLE__
# if !defined(WIN32)
#  define WIN32
# endif
#endif

#ifdef __GNUC__
#include <ext/hash_map>
#include <ext/hash_set>
namespace stdext = ::__gnu_cxx ;
#elif defined(WIN32) || defined(WIN64)
#include <hash_map>
#include <hash_set>
#endif

class FTFontChina : public FTFontAdaptor
{
public:
    FTFontChina();
    float Advance(const char* str);
    float Advance(const wchar_t* str);
    void Render(const wchar_t* str);
    void Render(const char* str);  
    
    float Ascender() const;
    float Descender() const;
    int Error() const;
    unsigned int GetFaceSize() const;
    bool FaceSize(const unsigned int);
    
private:
    void init();
    void saveState();
    void restoreState();
    template<class T> void RenderText( const T* str, int len );
    template<class T> float AdvanceText( const T* str, int len );
    
    int mSize;
};

//----------------------------------------------
//  non-china font system (very minimal ram)
//----------------------------------------------

struct CharData;
class FontAdaptor;
class TextureFont;

typedef struct ScaledCharData
{
    float width,height,xoffset,yoffset,xadvance; // vector
    float x, y, charWidth, charHeight; // texture
    int page;
    
    ScaledCharData( float scale, float imageWidth, float imageHeight, const CharData& cdata );
    
} ScaledCharData;

typedef stdext::hash_map<int,ScaledCharData> ScaledCharDataMap;

class FTFontWestern : public FTFontAdaptor 
{
public:
    FTFontWestern();
    float Advance(const char* str);
    float Advance(const wchar_t* str);
    void Render(const wchar_t* str);
    void Render(const char* str);  
    
    float Ascender() const;
    float Descender() const;
    int Error() const;
    unsigned int GetFaceSize() const;
    bool FaceSize(const unsigned int);
    
    friend class FontAdaptor;
    
protected:
    TextureFont* mFont;
    
private:
    static void init();
    void saveState();
    void restoreState();
    template<class T> void RenderText( const T* str, int len );
    
    unsigned int mSize;
    float fontScale;
    float scaledBaseline;
    GLint imageUnits; // 4 is the minimum of any card we support
    
    ScaledCharDataMap scaledChars;
};
