#pragma once
/*
 *  TextureFont.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 2/12/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

struct CharData
{
    short id,x,y,width,height,xoffset,yoffset,xadvance, page;
};

#include <unordered_map>
using CharDataMap = std::unordered_map<int,CharData>;

class TextureFont
{
    friend class FTFontWestern;
    friend class FTFontChina;
    friend class FontAdaptor;

public:
    TextureFont( const char* name, int size );
private:

    int nominalSize;
    GLuint texID;
    int textureID;
    CharDataMap cmap;
    float lineheight;
    float baseline;
    float baseFontSize;
    float imageWidth;
    float imageHeight;
};
