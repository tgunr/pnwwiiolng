/*
 *  GenerateHostTerrain.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 8/1/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */
#include <syscfg.h> // Path is project specific.
#ifdef MAKE_HOST_TERRAIN
#include "GenerateHostTerrain.h"
#include "sdm.h"
#include <limits.h>

extern SINT32 dot_move(const char* line);

bool GeneratingHostTerrain = false;

static int currentSupercellX = -26;
static int currentSupercellY = 210;
static bool inited = false;
static SINT32 stable_frametime;

// whole map -26, 210, 79 wide 72 high = 79*72 = 5688 or 1,456,128 octets or 931,921 km^2
// at 50meter grid = 256 grids per octet or 372,768,768 grids

void generateHostTerrain()
{
    GeneratingHostTerrain = !GeneratingHostTerrain;
}

static void getposition(float&x, float&y)
{
    x = currentSupercellX * SDM_SUPERCELL_SIZE;
    y = currentSupercellY * SDM_SUPERCELL_SIZE;
}

void updateGeneratingHostTerrain()
{
    if ( ! GeneratingHostTerrain )
    {
        return;
    }

    float x,y;
    char buffer[256];

    if ( ! inited )
    {
        inited = true;
        getposition(x,y);
        strprintf(buffer, "move %.0f %.0f 400",x,y);
        dot_move(buffer);
        stable_frametime = frametime+2000;
    }

    if ( frametime > stable_frametime )
    {
        printf("looking at %d,%d\n",currentSupercellX,currentSupercellY);

        currentSupercellX+=1;
        if ( currentSupercellX >= -26+79 )
        {
            currentSupercellX = -26;
            GeneratingHostTerrain = false;
        }
        else
        {
            getposition(x,y);
            strprintf(buffer, "move %.0f %.0f 400",x,y);
            dot_move(buffer);
            stable_frametime = frametime+2000;
        }
    }
}

/*
void generateHostTerrain()
{
    for ( int x = -26; x<-26+79; x++ )
    {
        for ( int y = 210; y<210+72; y++ )
        {
            float posx = x*SDM_SUPERCELL_SIZE;
            float posy = y*SDM_SUPERCELL_SIZE;

            for ( int ox = 0; ox<16; ox++ )
            {
                for ( int oy = 0; oy<16; oy++ )
                {
                }
            }
        }
    }
}
*/
#endif
