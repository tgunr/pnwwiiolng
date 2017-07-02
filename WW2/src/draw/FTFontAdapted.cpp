#include "syscfg.h" // Path is project specific.
/*
 *  FTFontAdapted.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 2/24/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#include "phui/FTFontReplacement.h"


FTFontAdapted::FTFontAdapted(FTFont* font)
{
    mFont = font;
}

float FTFontAdapted::Advance(const char* str)
{
    return mFont->Advance(str);
}

float FTFontAdapted::Advance(const wchar_t* str)
{
    return mFont->Advance(str);
}

void FTFontAdapted::Render(const wchar_t* str)
{
    mFont->Render(str);
}

void FTFontAdapted::Render(const char* str)
{
    mFont->Render(str);
}

float FTFontAdapted::Ascender() const
{
    return mFont->Ascender();
}

float FTFontAdapted::Descender() const
{
    return mFont->Descender();
}

int FTFontAdapted::Error() const
{
    return mFont->Error();
}

unsigned int FTFontAdapted::GetFaceSize() const
{
    return mFont->FaceSize();
}

bool FTFontAdapted::FaceSize(const unsigned int sz)
{
    return mFont->FaceSize(sz);
}

