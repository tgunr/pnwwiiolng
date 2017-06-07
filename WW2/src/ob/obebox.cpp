#include "syscfg.h" // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ld/ldmdl.h"
#include "ld/ldpix.h"
#include "ob/obmain.h"
#include "ob/obebox.h"
#include "str/strpool.h"

static const char*         suffix[6]={"s","e","n","w","u","d"};
static piVECTOR     norms[6]={{ 0.0f,-1.0f, 0.0f},
                              {-1.0f, 0.0f, 0.0f},
                              { 0.0f, 1.0f, 0.0f},
                              { 1.0f, 0.0f, 0.0f},
                              { 0.0f, 0.0f, 1.0f},
                              { 0.0f, 0.0f,-1.0f}};


LOGICAL obeboxCreate(const char* name, float distance)
{
char        tname[32];
char        texname[64];
int         i, j;
int         texid;
OBJECT* lpobj, *lplast;
piVECTOR*  psnorms;
piINDICES* ptris;
piVECTOR	  faces[6][4]={{{-1, 1, 1},
						  	{-1, 1,-1},
							{ 1, 1,-1},
							{ 1, 1, 1}},
							{{ 1, 1, 1},
							{ 1, 1,-1},
							{ 1,-1,-1},
							{ 1,-1, 1}},
  							{{-1,-1, 1},
					  		{-1,-1,-1},
						  	{-1, 1,-1},
							{-1, 1, 1}},
							{{ 1, 1,-1},
							{-1, 1,-1},
							{-1,-1,-1},
							{ 1,-1,-1}},
							{{-1, 1, 1},
							{ 1, 1, 1},
							{ 1,-1, 1},
							{-1,-1, 1}}};

	for (i=0 ; i<6 ; i++)
	{
		for (j=0 ; j<4 ; j++ )
		{
			faces[i][j].x *= distance;
			faces[i][j].y *= distance;
			faces[i][j].z *= distance;
		}
	}

  for (i=0 ; i < 6 ; i++)
  {
    lstrcpy(tname, name);
    lstrcat(tname, suffix[i]);
    lstrcpy(texname, tname);

    lpobj = obmainGetNewObject();
    if (lpobj == nullptr)
    {
      return(FALSE);
    }

    texid = ldpixLoadImageFileToSurface(texname, colorWHITEALPHA, LDPIX_NONE);
    if (!piIsTexture(texid, 0))
    {
      return(FALSE);
    }

    lpobj->flags = 0;
	lpobj->flags |= OBJECT_VISIBLE;
    lpobj->flags |= OBJECT_TRIANGLEFAN;
    lpobj->flags |= OBJECT_NOFILTER;
    lstrcpy(lpobj->name, name);
    lpobj->numverts = 4;
    lpobj->numtris  = 2;
    lpobj->ptris    = ptris = (piINDICES*)memAlloc(2 * sizeof(piINDICES));
	lpobj->psnorms  = psnorms = (piVECTOR*)memAlloc(2 * sizeof(piVECTOR));
    lpobj->matid = -1;
    lpobj->texid = texid;
    lpobj->texname = strpool(texname);

    lpobj->CreateStaticData();

	piVectorPtr spos    = lpobj->StaticPositions();
	piVectorPtr snorm   = lpobj->StaticNormals();
	piVertexColorPtr sc = lpobj->StaticColors();
	piTexCoordPtr stex  = lpobj->StaticTexCoords();

    for (j=0 ; j < 4 ; j++)
    {
      	spos[j].x  = faces[i][j].x * ScaleFactor;
     	spos[j].y  = faces[i][j].y * ScaleFactor;
     	spos[j].z  = faces[i][j].z * ScaleFactor;
     	snorm[j].x = norms[i].x;
      	snorm[j].y = norms[i].y;
      	snorm[j].z = norms[i].z;

      	sc[j].color = colorWHITEALPHA;
      	sc[j].specular = colorALPHA100;
    }

    psnorms[0].x = norms[i].x;
    psnorms[0].y = norms[i].y;
    psnorms[0].z = norms[i].z;
    psnorms[1].x = norms[i].x;
    psnorms[1].y = norms[i].y;
    psnorms[1].z = norms[i].z;

    ptris[0].v1 = 0;
    ptris[0].v2 = 1;
    ptris[0].v3 = 3;
    ptris[1].v1 = 1;
    ptris[1].v2 = 2;
    ptris[1].v3 = 3;

    stex[0].u = 0.0f;
    stex[0].v = 0.0f;
    stex[1].u = 0.0f;
    stex[1].v = 1.0f;
    stex[2].u = 1.0f;
    stex[2].v = 1.0f;
    stex[3].u = 1.0f;
    stex[3].v = 0.0f;

	lpobj->SyncStaticData();
	lpobj->SyncMutableData();

    if (!ldmdlGetBounds(lpobj))
    {
      return(FALSE);
    }

		if (i != 0)
		{
			lplast->attached = lpobj;
			lpobj->flags |= OBJECT_ATTACHED;
		}

		lplast = lpobj;

    Write_Debug("Created environment object: %s", tname);
  }

  return(TRUE);
}
