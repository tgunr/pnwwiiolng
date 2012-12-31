#include <syscfg.h> // Path is project specific.
/*
 *  sneaky.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 3/17/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#include "sneaky.h"
#include <vector>
#include <algorithm>

EXTERN LOGICAL			Online;

typedef std::vector<GLuint> NameVector;
static NameVector names;
static const int ChunkSize=1000;

void gl_GenTextures( GLsizei n, GLuint* textures )
{
    if ( ! Online )
    {
        glGenTextures( n, textures );
        return;
    }

    if ( (size_t)n>names.size() )
    {
        names.resize( ChunkSize>=n ? ChunkSize : n );
        glGenTextures( ChunkSize, &names[0] );
        random_shuffle( names.begin(), names.end() );
    }

    int count = names.size();
    GLuint* from = &names[count-n];
    memcpy( textures, from, sizeof(GLuint) * n );
    names.resize( count-n );
}

GLhandleARB gl_CreateProgram()
{
    if(!Online)
        return glCreateProgram();

#define CHUNCK  256

    typedef std::vector<GLhandleARB> ProgramVector;
    static ProgramVector programs;
    static size_t idx = 0;

    if(idx >= programs.size())
    {
        size_t start = programs.size();

        programs.resize(programs.size() + CHUNCK);

        for(unsigned int i = start; i < programs.size(); i++)
            programs[i] = glCreateProgram();

        random_shuffle(programs.begin() + start, programs.end());
    }

    return programs[idx++];
}