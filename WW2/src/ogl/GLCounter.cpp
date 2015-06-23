#include <syscfg.h> // Path is project specific.
/*
 *  GLCounter.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 6/20/11.
 *  Copyright 2011
 *
 */

#include "GLCounter.h"

static GLCounter* GLCounterInstance = nullptr;

extern int ft1000msticks;

void GLCounterKill()
{
    if(GLCounterInstance)
        delete GLCounterInstance;
}

GLCounter& GLCounter::instance()
{
    if ( GLCounterInstance == nullptr )
    {
        GLCounterInstance = new GLCounter();
    }
    return *GLCounterInstance;
}

void GLCounter::startFrame()
{
    if ( frames == 0 )
    {
        memset(&countArray[0], 0, countArray.size()*sizeof(size_t));
    }
}

void GLCounter::endFrame()
{
    frames++;
    if ( ft1000msticks == 0 )
    {
        size_t total = 0;
        for ( int i = GLC_start; i< GLC_count; i++ )
        {
            total += countArray[i];
        }
        float callsPerFrame = total / (float) frames;
        recentTotal = (int(callsPerFrame*100.0f))/100.0f;
        frames = 0;
    }
}

GLCounter::GLCounter() : frames(0), recentTotal(0.0)
{
    countArray.resize(GLC_count);

    indexToString.push_back("glDrawArrays");
    indexToString.push_back("glDrawArraysInstanced");
    indexToString.push_back("glDrawElements");
    indexToString.push_back("glDrawElementsInstanced");
    indexToString.push_back("glDrawPixels");
    indexToString.push_back("glDrawRangeElements");
	indexToString.push_back("glCallList");
    assert( indexToString.size() ==  GLC_count );
}

void GLCounter::GLC_callCounter(GLCounter_Index index )
{
    countArray[index]++;
}

const std::string& GLCounter::asString( GLCounter_Index index )
{
    return indexToString[index];
}

