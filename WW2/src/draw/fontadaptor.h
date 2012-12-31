/*
 *  fontadaptor.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 1/28/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#include "pi/piogl.h"

class FontAdaptor
{
public:
    int getHeight();
    SINT32 getStringWidth(const WCHAR*);
    LOGICAL get3DString(const WCHAR* pszText, UINT32 uColor, piLVERTEX* plVertices, SINT32 iNumVertices);
    SINT32 getTextureId();

    FontAdaptor( FTFontWestern* f );
    virtual ~FontAdaptor()
    {
        if(font)
            delete font;
    }
private:
    FTFontWestern* font;
};
