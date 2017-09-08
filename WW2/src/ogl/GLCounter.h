#pragma once
//  Created by Andrew Wulf on 6/20/11.
//  Copyright 2011

#include "GL/glew.h"
#include "gl/glplat.h"
#include <vector>
#include <string>

#define COUNT_CALLS(f) GLC_callCounter(f)

enum GLCounter_Index
{
    GLC_start=0,
    GLC_glDrawArrays=0,
    GLC_glDrawArraysInstanced,
    GLC_glDrawElements,
    GLC_glDrawElementsInstanced,
    GLC_glDrawPixels,
    GLC_glDrawRangeElements,
    GLC_glCallList,
    GLC_count
};

class GLCounter
{
public:
    static GLCounter& instance();

    void startFrame();
    void endFrame();
    float getRecentTotal() { return recentTotal; }

    std::vector<size_t> copyCount() { return countArray; }
    const std::string& asString( GLCounter_Index index );

    void inline call_glCallList (GLuint list)
    {
        COUNT_CALLS(GLC_glCallList);
        glCallList(list);
    }

    void inline call_glDrawArrays (GLenum mode, GLint first, GLsizei count)
    {
        COUNT_CALLS(GLC_glDrawArrays);
        glDrawArrays(mode, first, count);
    }

    void inline call_glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
    {
        COUNT_CALLS(GLC_glDrawPixels);
        glDrawPixels(width, height, format, type, pixels);
    }

    void inline call_glDrawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei primcount )
    {
        COUNT_CALLS(GLC_glDrawArraysInstanced);
        glDrawArraysInstanced( mode, first, count, primcount );
    }

    void inline call_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices )
    {
        COUNT_CALLS(GLC_glDrawElements);
        glDrawElements( mode, count, type, indices );
    }

    void inline call_glDrawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const void*  indices, GLsizei primcount )
    {
        COUNT_CALLS(GLC_glDrawElementsInstanced);
        glDrawElementsInstanced( mode, count, type, indices, primcount );
    }

    void inline call_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices )
    {
        COUNT_CALLS(GLC_glDrawRangeElements);
        glDrawRangeElements( mode, start, end, count, type, indices );
    }

private:
    GLCounter();
    void GLC_callCounter(GLCounter_Index index );

    std::vector<size_t> countArray;
    std::vector<std::string> indexToString;
    size_t frames;
    float recentTotal;
};

extern void GLCounterKill();
