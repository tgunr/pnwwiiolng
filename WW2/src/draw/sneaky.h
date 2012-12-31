#pragma once
/*
 *  sneaky.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 3/17/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 */

#include "ogl/ogl.h"

void gl_GenTextures( GLsizei n, GLuint* textures );
GLhandleARB gl_CreateProgram();

const static union
{
    float          f;
    unsigned int   ui;
} altMask = { -1.123f };

template<class T>
class AltPointer
{
private:
    T* ptr;

    inline void hide( T* p )
    {
        ptr = (T*)(((unsigned int)p)^altMask.ui);
    }

    inline T* recover( )
    {
        return (T*)(((unsigned int)ptr)^altMask.ui);
    }

public:
    AltPointer():ptr(NULL)
    {
    }

    AltPointer( T* p )
    {
        hide(p);
    }

    const unsigned int getMask();

    inline T* get()
    {
        return ptr==NULL ? NULL : recover();
    }

    inline void set( T* p )
    {
        hide(p);
    }

    inline T& operator*()
    {
        assert(ptr!=NULL);
        return *recover();
    }

    std::string desc()
    {
        char s[32];
        sprintf(s,"%x", (unsigned int)ptr );
        return std::string(s);
    }
};
