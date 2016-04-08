/*
 *  ldwwo.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 7/18/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */
#ifdef SUPPORT_WWO
#include <syscfg.h> // Path is project specific.
#include "ldwwo.h"

LdWWO::LdWWO(std::string path)
: mPath(path)
{
}

void LdWWO::process( const char* start, const char* end )
{
}

bool LdWWO::loadFromText()
{
    commands current = c_blank;
    char c;
    bool comment = false;
    const char* p = mText;
    const char* start = p;

    while ( c = *p )
    {
        if ( c == '#' )
        {
            comment = true;
        }
        if ( c == 10 )
        {
            if ( !comment )
            {
                process( start, p );
            }
            comment = false;
            start = p+1;
        }
        else if ( c == 13 && *(p+1) == 10 || *(p+1)==0)
        {
            if ( !comment )
            {
                process( start, p );
            }
            comment = false;
            if ( *(p+1) == 10 )
            {
                p++;
            }
            start = p+1;
        }
        p++;
    }
    return false;
}

bool LdWWO::load()
{
	int fd = fioOpen(mPath.c_str());
	if(fd < 0) return false;
    fioClose(fd);

    char* text;
    unsigned int size = 0;
    if ( fioLoad(mPath.c_str(), text, &size) )
    {
        mText = text;
        bool result = loadFromText();
        memFree( text );
        if ( result )
        {
            return true;
        }
    }

    return false;
}

#endif
