#pragma once
/*
 *  ldwwo.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 7/18/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 */
#ifdef SUPPORT_WWO

#include <string>
#include <map>
#include <vector>
#include <set>

class LdWWO
{
public:
    LdWWO(std::string path);
    bool load();

private:

    enum commands
    {
        c_blank=0,   //
        c_comment,   // # comment
        c_c,         // data comment
        c_ma,        // material ambient color
        c_mb,        // material begin [name]
        c_md,        // material diffuse color
        c_me,        // material end
        c_mem,       // material emissive
        c_mid,       // material reference [from 1]
        c_ms,        // material specular
        c_msh,       // material shinyness
        c_mt,        // texture reference [from 1]
        c_ob,        // object begin [name] - nests
        c_oe,        // object end
        c_t,         // triangle [references from vertext/uv/normal counting from 1 in file]
        c_tx,        // texture [path]
        c_v,         // vertex [x y z]
        c_vn,        // normal [x y z]
        c_vt,        // texture uv [x y]
        c_ww,        // first line of file [version]
        c_count      //

    };

    struct classcomp
    {
        bool fncomp (const char*x, const char*y) {return strcmp(x,y)<0;}
    };

    using CommandMap = std::map<const char*, commands,classcomp>;

    bool loadFromText( );
    void process( const char* start, const char* end );

    std::string mPath;
    CommandMap commentMap;

    const char* mText;
};


#endif
