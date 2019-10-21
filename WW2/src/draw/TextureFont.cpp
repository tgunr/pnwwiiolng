#include "syscfg.h" // Path is project specific.
/*
 *  TextureFont.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 2/12/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */

#include "pi/piogl.h"
#include "ogl/ogl.h"
#include "ld/ldpix.h"
#include "ld/ldtexture.h"
#include "fio/fio.h"
#include <iostream>
#include <sstream>
#include "draw/TextureFont.h"
#include "mem/mem.h"
#include "client/client.h"

using namespace std;

TextureFont::TextureFont( const char* name, int size )
{
    char path[128];
    strprintf(path, "%s%d.dds", name, size);
    textureID = ldLoadTexture( path, 0, LDPIX_SET_FONT_PARMS );

    char* text;
    unsigned int filesize = 0;
    strprintf(path, "%s%d.fnt", name, size);
    fioLoad(path, text, &filesize );
    if ( filesize == 0 || textureID <= 0 )
    {
        clientErrorLog("western font data not found");
        exit(-1); // TODO: Find a better way to report this error, this happens when archives are missing
    }

    nominalSize = size;

    TEXTURE*t = oglGetTextureData2(textureID );

    imageWidth = (float)t->imagewidth[0];
    imageHeight = (float)t->imageheight[0];
    texID = t->genlist;

    stringstream ss (text,stringstream::in);
    char line[256];
    ss.getline (line,256); // skip

    ss.getline (line,256);
    sscanf(line, "common lineHeight=%f base=%f", &lineheight, &baseline);

    baseFontSize = lineheight; // * 26.0/32.0; // apparent font size heuristic from the texture

    ss.getline (line,256); // skip

    ss.getline (line,256);
    int charcount=0;
    sscanf(line, "chars count=%d", &charcount);

    for ( int i=0; i<charcount; i++ )
    {
        ss.getline (line,256);
        CharData cd;
        sscanf(line, "char id=%hd    x=%hd    y=%hd    width=%hd     height=%hd     xoffset=%hd     yoffset=%hd    xadvance=%hd      page=%hd",
               &cd.id,&cd.x,&cd.y,&cd.width,&cd.height,&cd.xoffset,&cd.yoffset,&cd.xadvance, &cd.page
               );
        cmap.insert(pair<int,CharData>(cd.id,cd));
    }

    if(text)
    {
        memFree(text);
        text = nullptr;
    }
}

