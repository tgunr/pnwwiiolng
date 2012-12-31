#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ld/ldbmp.h"
#include "ld/ldmdl.h"
#include "ob/obmain.h"
#include "ob/obsph.h"
#include "math/math3d.h"
#include "str/strpool.h"

LOGICAL obsphCreate(const char* name, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color, OBJECT* lpobj)
{
	int	        vo, io;
	int         i, j;
	int         NumSphereVertices, NumSphereIndices, TotalVertices, TotalIndices;
	float       di, dj;
	float       x, y, z;
	piVECTOR    vec;
	UINT16*		ptris;
	piVECTOR	loc;


	if (MeshSize < 2)
	{
		return(FALSE);
	}

	NumSphereVertices	= (MeshSize * 2 + 1) * (MeshSize + 1);
	if (NumSphereVertices > piGetNumberVertices())
	{
		Write_Debug("Sphere has too many vertices: %d; max is %d", NumSphereVertices, piGetNumberVertices());
		return(FALSE);
	}

	NumSphereIndices	= (MeshSize * MeshSize * 2) * 6;
	di = PI / MeshSize;
	dj = PI / MeshSize;


	lpobj->flags = 0;
	lpobj->flags |= OBJECT_LITE;
	lpobj->flags |= OBJECT_DYNALITE;
	lpobj->flags |= OBJECT_VISIBLE;
	lpobj->flags |= OBJECT_COLLISION;
	lpobj->flags |= OBJECT_INDEXED;
	lpobj->flags |= OBJECT_TRIANGLELIST;
	if (name != NULL)
		lstrcpy(lpobj->name, name);

	lpobj->numverts = NumSphereVertices;
	lpobj->numtris  = NumSphereIndices / 3;
	lpobj->ptris    = (piINDICES*)memAlloc(NumSphereIndices * sizeof(UINT16));
	lpobj->psnorig  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->psnorms  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->pscdon   = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->pscdn    = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->mat 		= IdentityMatrix;

	ptris = (UINT16*)lpobj->ptris;
	lpobj->CreateStaticData();

	x = Location->x * ScaleFactor;
	y = Location->y * ScaleFactor;
	z = Location->z * ScaleFactor;

	TotalVertices = 0;

	piVectorPtr spos 	= lpobj->StaticPositions();
	piVectorPtr snorm 	= lpobj->StaticNormals();
	piTexCoordPtr stex 	= lpobj->StaticTexCoords();
	piVertexColorPtr sc = lpobj->StaticColors();

#if defined(OPENGL3D) && platOS == OS_WIN32
	Color = piRGBA_MAKE(piRGBA_GETBLUE(Color), piRGBA_GETGREEN(Color), piRGBA_GETRED(Color), piRGBA_GETALPHA(Color));
#endif

	for (i=0 ; i <= MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;

		for (j=0 ; j <= MeshSize * 2 ; j++)
		{
			vec.x = (float)cos(PI / 2 + j * dj) * (float)sin(i * di);
			vec.y = (float)sin(PI / 2 + j * dj) * (float)sin(i * di);
			vec.z = (float)cos(i * di);

			spos[vo + j].x 	= x + vec.x * Scale * ScaleFactor;
			spos[vo + j].y	= y + vec.y * Scale * ScaleFactor;
			spos[vo + j].z	= z + vec.z * Scale * ScaleFactor;

			sc[vo + j].color    = Color;
			sc[vo + j].specular = colorALPHA100;

			stex[vo + j].u	= (float)j / (float)(MeshSize * 2);
			stex[vo + j].v	= (float)i / (float)(MeshSize);

			snorm[vo + j].x	= vec.x;
			snorm[vo + j].y	= vec.y;
			snorm[vo + j].z	= vec.z;

			TotalVertices += 1;
		}
	}

	lpobj->SyncStaticData();
	lpobj->SyncMutableData();

	if (TotalVertices != NumSphereVertices)
	{
		Write_Debug("Sphere vertex count was incorrect");
		return(FALSE);
	}

	TotalIndices = 0;
	for (i=0 ; i < MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;
		io = (MeshSize * 2) * i * 6;
		for (j=0 ; j < MeshSize * 2 ; j++)
		{
			ptris[6 * j + io]      = vo + j;
			ptris[6 * j + 2 + io]  = vo + j + 1;
			ptris[6 * j + 1 + io]  = vo + j + (MeshSize * 2 + 1);
			TotalIndices += 3;

			ptris[6 * j + io + 3]      = vo + j + (MeshSize * 2 + 1);
			ptris[6 * j + 2 + io + 3]  = vo + j + 1;
			ptris[6 * j + 1 + io + 3]  = vo + j + (MeshSize * 2 + 1) + 1;
			TotalIndices += 3;
		}
	}

	if (TotalIndices != NumSphereIndices)
	{
		Write_Debug("Sphere index count was incorrect");
		return(FALSE);
	}

	if (!ldmdlCalcSurfaceNormals(lpobj) ||
	  	!ldmdlGetBounds(lpobj))
	{
		return(FALSE);
	}

	lpobj->loc.x = Location->x * ScaleFactor;
	lpobj->loc.y = Location->y * ScaleFactor;
	lpobj->loc.z = Location->z * ScaleFactor;

	if (!ldmdlGetObjectBounds(lpobj, &loc, lpobj->obox, &lpobj->minrad, &lpobj->maxrad))
	{
		return(FALSE);
	}

	Write_Debug("Sphere '%s' created with %d triangles comprised of %d vertices %d indices", name, NumSphereIndices / 3, NumSphereVertices, TotalIndices);

	return(TRUE);
}

LOGICAL obsphCreateHemi(const char* name, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color, OBJECT* lpobj)
{
int	        vo, io;
int         i, j;
int         NumSphereVertices, NumSphereIndices, TotalVertices, TotalIndices;
float       di, dj;
float       x, y, z;
piVECTOR    vec;
UINT16*		ptris;
piVECTOR	loc;


  if (MeshSize < 2)
  {
    return(FALSE);
  }

  NumSphereVertices	= (MeshSize + 1) * (MeshSize * 2 + 1);
  if (NumSphereVertices > piGetNumberVertices())
  {
    Write_Debug("Sphere has too many vertices: %d; max is %d", NumSphereVertices, piGetNumberVertices());
    return(FALSE);
  }

  NumSphereIndices	= (MeshSize * MeshSize * 2) * 6;
  di = PI / MeshSize / 2.0f;
  dj = PI / MeshSize;

	lpobj->flags = 0;
	lpobj->flags |= OBJECT_LITE;
	lpobj->flags |= OBJECT_DYNALITE;
	lpobj->flags |= OBJECT_VISIBLE;
	lpobj->flags |= OBJECT_COLLISION;
	lpobj->flags |= OBJECT_INDEXED;
	lpobj->flags |= OBJECT_TRIANGLELIST;
	if (name != NULL) lstrcpy(lpobj->name, name);
	lpobj->numverts = NumSphereVertices;
	lpobj->numtris  = NumSphereIndices / 3;
	lpobj->ptris    = (piINDICES*)memAlloc(NumSphereIndices * sizeof(UINT16));;
	lpobj->psnorig  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->psnorms  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->pscdon   = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->pscdn    = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->mat = IdentityMatrix;

	lpobj->CreateStaticData();
	ptris  = (UINT16*)lpobj->ptris;


	x = Location->x * ScaleFactor;
	y = Location->y * ScaleFactor;
	z = Location->z * ScaleFactor;

	piVectorPtr spos 	= lpobj->StaticPositions();
	piVectorPtr snorm 	= lpobj->StaticNormals();
	piTexCoordPtr stex 	= lpobj->StaticTexCoords();
	piVertexColorPtr sc = lpobj->StaticColors();

#if defined(OPENGL3D) && platOS == OS_WIN32
	Color = piRGBA_MAKE(piRGBA_GETBLUE(Color), piRGBA_GETGREEN(Color), piRGBA_GETRED(Color), piRGBA_GETALPHA(Color));
#endif

	TotalVertices = 0;
	for (i=0 ; i <= MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;
		for (j=0 ; j <= MeshSize * 2 ; j++)
		{
			vec.x = (float)cos(PI / 2 + j * dj) * (float)sin(i * di);
			vec.y = (float)sin(PI / 2 + j * dj) * (float)sin(i * di);
			vec.z = -(float)cos(i * di) * Ellipse;
			spos[vo + j].x      = x + vec.x * Scale * ScaleFactor;
			spos[vo + j].y      = y + vec.y * Scale * ScaleFactor;
			spos[vo + j].z      = z + vec.z * Scale * ScaleFactor;
			sc[vo + j].color    = Color;
			sc[vo + j].specular = colorALPHA100;
			stex[vo + j].u 	= (float)j / (float)(MeshSize * 2);
			stex[vo + j].v  = (float)(MeshSize - i) / (float)(MeshSize);
			snorm[vo + j].x	= vec.x;
			snorm[vo + j].y	= vec.y;
			snorm[vo + j].z	= vec.z;


			TotalVertices += 1;
		}
	}

	lpobj->SyncStaticData();
	lpobj->SyncMutableData();

	if (TotalVertices != NumSphereVertices)
	{
		Write_Debug("Hemisphere vertex count was incorrect");
		return(FALSE);
	}

	TotalIndices = 0;
	for (i=0 ; i < MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;
		io = (MeshSize * 2) * i * 6;
		for (j=0 ; j < MeshSize * 2 ; j++)
		{
			ptris[6 * j + io]      = vo + j;
			ptris[6 * j + 1 + io]  = vo + j + 1;
			ptris[6 * j + 2 + io]  = vo + j + (MeshSize * 2 + 1);
			TotalIndices += 3;

			ptris[6 * j + io + 3]      = vo + j + (MeshSize * 2 + 1);
			ptris[6 * j + 1 + io + 3]  = vo + j + 1;
			ptris[6 * j + 2 + io + 3]  = vo + j + (MeshSize * 2 + 1) + 1;
			TotalIndices += 3;
		}
	}

	if (TotalIndices != NumSphereIndices)
	{
		Write_Debug("Hemisphere index count was incorrect");
		return(FALSE);
	}

	if (!ldmdlCalcSurfaceNormals(lpobj) ||
		!ldmdlGetBounds(lpobj))
	{
		return(FALSE);
	}

	lpobj->loc.x = Location->x * ScaleFactor;
	lpobj->loc.y = Location->y * ScaleFactor;
	lpobj->loc.z = Location->z * ScaleFactor;

	if (!ldmdlGetObjectBounds(lpobj, &loc, lpobj->obox, &lpobj->minrad, &lpobj->maxrad))
	{
		return(FALSE);
	}

	Write_Debug("Hemisphere '%s' created with %d triangles comprised of %d vertices %d indices", name, NumSphereIndices / 3, NumSphereVertices, TotalIndices);

	return(TRUE);
}

LOGICAL obsphCreateCylinder(const char* name, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color, OBJECT* lpobj)
{
int	        vo, io;
int         i, j;
int         NumSphereVertices, NumSphereIndices, TotalVertices, TotalIndices;
float       di, dj;
float       x, y, z;
piVECTOR    vec;
UINT16*		ptris;
piVECTOR loc;


	if (MeshSize < 2)
	{
		return(FALSE);
	}

	NumSphereVertices	= (MeshSize * 2 + 1) * (MeshSize + 1);
	if (NumSphereVertices > piGetNumberVertices())
	{
		Write_Debug("Sphere has too many vertices: %d; max is %d", NumSphereVertices, piGetNumberVertices());
		return(FALSE);
	}

	NumSphereIndices	= (MeshSize * MeshSize * 2) * 6;
	di = PI / MeshSize;
	dj = PI / MeshSize;

	lpobj->flags = 0;
	lpobj->flags |= OBJECT_LITE;
	lpobj->flags |= OBJECT_DYNALITE;
	lpobj->flags |= OBJECT_VISIBLE;
	lpobj->flags |= OBJECT_COLLISION;
	lpobj->flags |= OBJECT_INDEXED;
	lpobj->flags |= OBJECT_TRIANGLELIST;
	if (name != NULL) lstrcpy(lpobj->name, name);

	lpobj->numverts = NumSphereVertices;
	lpobj->numtris  = NumSphereIndices / 3;
	lpobj->ptris    = (piINDICES*)memAlloc(NumSphereIndices * sizeof(UINT16));
	lpobj->psnorig  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->psnorms  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->pscdon   = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->pscdn    = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->mat = IdentityMatrix;

	lpobj->CreateStaticData();
 	ptris  = (UINT16*)lpobj->ptris;

	x = Location->x * ScaleFactor;
	y = Location->y * ScaleFactor;
	z = Location->z * ScaleFactor;

	piVectorPtr spos 	= lpobj->StaticPositions();
	piVectorPtr snorm 	= lpobj->StaticNormals();
	piTexCoordPtr stex 	= lpobj->StaticTexCoords();
	piVertexColorPtr sc = lpobj->StaticColors();

#if defined(OPENGL3D) && platOS == OS_WIN32
	Color = piRGBA_MAKE(piRGBA_GETBLUE(Color), piRGBA_GETGREEN(Color), piRGBA_GETRED(Color), piRGBA_GETALPHA(Color));
#endif

	TotalVertices = 0;
	for (i=0 ; i <= MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;
		for (j=0 ; j <= MeshSize * 2 ; j++)
		{
			vec.x = (float)cos(PI / 2 + j * dj);// * (float)sin(i * di);
			vec.y = (float)sin(PI / 2 + j * dj);// * (float)sin(i * di);
			vec.z = (float)cos(i * di) * Ellipse;
			spos[vo + j].x        = x + vec.x * Scale * ScaleFactor;
			spos[vo + j].y        = y + vec.y * Scale * ScaleFactor;
			spos[vo + j].z        = z + vec.z * Scale * ScaleFactor;
			sc[vo + j].color    = Color;
			sc[vo + j].specular = colorALPHA100;
			stex[vo + j].u       = (float)j / (float)(MeshSize * 2);
			stex[vo + j].v       = (float)i / (float)(MeshSize);
			snorm[vo + j].x         = vec.x;
			snorm[vo + j].y         = vec.y;
			snorm[vo + j].z         = vec.z;


			TotalVertices += 1;
		}
	}

	lpobj->SyncStaticData();
	lpobj->SyncMutableData();

	if (TotalVertices != NumSphereVertices)
	{
		Write_Debug("Sphere vertex count was incorrect");
		return(FALSE);
	}

  	TotalIndices = 0;
	for (i=0 ; i < MeshSize ; i++)
	{
		vo = (MeshSize * 2 + 1) * i;
		io = (MeshSize * 2) * i * 6;
		for (j=0 ; j < MeshSize * 2 ; j++)
		{
			ptris[6 * j + io]      = vo + j;
			ptris[6 * j + 2 + io]  = vo + j + 1;
			ptris[6 * j + 1 + io]  = vo + j + (MeshSize * 2 + 1);
			TotalIndices += 3;

			ptris[6 * j + io + 3]      = vo + j + (MeshSize * 2 + 1);
			ptris[6 * j + 2 + io + 3]  = vo + j + 1;
			ptris[6 * j + 1 + io + 3]  = vo + j + (MeshSize * 2 + 1) + 1;
			TotalIndices += 3;
		}
	}

	if (TotalIndices != NumSphereIndices)
	{
		Write_Debug("Sphere index count was incorrect");
		return(FALSE);
	}

  if (!ldmdlCalcSurfaceNormals(lpobj) ||
      !ldmdlGetBounds(lpobj))
  {
    return(FALSE);
  }

	lpobj->loc.x = Location->x * ScaleFactor;
	lpobj->loc.y = Location->y * ScaleFactor;
	lpobj->loc.z = Location->z * ScaleFactor;

	if (!ldmdlGetObjectBounds(lpobj, &loc, lpobj->obox, &lpobj->minrad, &lpobj->maxrad))
	{
		return(FALSE);
	}

  Write_Debug("Sphere '%s' created with %d triangles comprised of %d vertices %d indices", name, NumSphereIndices / 3, NumSphereVertices, TotalIndices);

  return(TRUE);
}

static LOGICAL CreateQuarter(const char* name, int Quarter, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color, OBJECT* lpobj)
{
int	        vo, io, ia, ib;
int         i, j;
int         NumSphereVertices, NumSphereIndices, TotalVertices, TotalIndices;
float       di, dj, qi;
float       x, y, z, ta, tb;
piVECTOR    vec;
UINT16*		ptris;


	if (MeshSize < 2)
	{
		return(FALSE);
	}

	NumSphereVertices	= ((MeshSize / 2 + 1) * (MeshSize + 1));
	if (NumSphereVertices > piGetNumberVertices())
	{
		Write_Debug("Sphere has too many vertices: %d; max is %d", NumSphereVertices, piGetNumberVertices());
		return(FALSE);
	}

	NumSphereIndices	= ((MeshSize / 2) * MeshSize) * 6;
	di = (PI / 2.0f) / (MeshSize / 2);
	dj = PI / MeshSize;
	qi = PI / 2.0f * (float)Quarter;

	lpobj->flags = 0;
	lpobj->flags |= OBJECT_LITE;
	lpobj->flags |= OBJECT_DYNALITE;
	lpobj->flags |= OBJECT_VISIBLE;
	lpobj->flags |= OBJECT_COLLISION;
	lpobj->flags |= OBJECT_INDEXED;
	lpobj->flags |= OBJECT_TRIANGLELIST;
	if (name != NULL) lstrcpy(lpobj->name, name);
	lpobj->numverts = NumSphereVertices;
	lpobj->numtris  = NumSphereIndices / 3;
	lpobj->ptris    = (piINDICES*)memAlloc(NumSphereIndices * sizeof(UINT16));
	lpobj->psnorig  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->psnorms  = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
	lpobj->pscdon   = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->pscdn    = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
	lpobj->mat = IdentityMatrix;

	lpobj->CreateStaticData();
  	ptris  = (UINT16*)lpobj->ptris;

	x = Location->x * ScaleFactor;
	y = Location->y * ScaleFactor;
	z = Location->z * ScaleFactor;

	switch (Quarter)
	{
	case 0:
		ta = 0.0f;
		tb = 0.0f;
		break;
	case 1:
		ta = 0.0f;
		tb = 0.5f;
		break;
	case 2:
		ta = 0.5f;
		tb = 0.0f;
		break;
	case 3:
		ta = 0.5f;
		tb = 0.5f;
		break;
	}

	piVectorPtr spos 	= lpobj->StaticPositions();
	piVectorPtr snorm 	= lpobj->StaticNormals();
	piTexCoordPtr stex 	= lpobj->StaticTexCoords();
	piVertexColorPtr sc = lpobj->StaticColors();

#if defined(OPENGL3D) && platOS == OS_WIN32
	Color = piRGBA_MAKE(piRGBA_GETBLUE(Color), piRGBA_GETGREEN(Color), piRGBA_GETRED(Color), piRGBA_GETALPHA(Color));
#endif

	TotalVertices = 0;
	for (i=0 ; i <= (MeshSize / 2) ; i++)
	{
		vo = (MeshSize + 1) * i;
		for (j=0 ; j <= MeshSize ; j++)
		{
			vec.x = (float)cos(PI / 2 + j * dj) * (float)sin(i * di + qi);
			vec.y = (float)sin(PI / 2 + j * dj) * (float)sin(i * di + qi);
			vec.z = (float)cos(i * di + qi);
			spos[vo + j].x        = x + vec.x * Scale * ScaleFactor;
			spos[vo + j].y        = y + vec.y * Scale * ScaleFactor;
			spos[vo + j].z        = z + vec.z * Scale * ScaleFactor;
			sc[vo + j].color    = Color;
			sc[vo + j].specular = colorALPHA100;
			if (Quarter < 2)
			{
			stex[vo + j].u     = (ta + (float)j / (float)(MeshSize * 2));
			stex[vo + j].v     = (tb + (float)i / (float)(MeshSize));
			}
			else
			{
			stex[vo + j].u     = (ta + (float)j / (float)(MeshSize * 2));
			stex[vo + j].v     = 1.0f - (tb + (float)i / (float)(MeshSize));
			}

			snorm[vo + j].x         = vec.x;
			snorm[vo + j].y         = vec.y;
			snorm[vo + j].z         = vec.z;


			TotalVertices += 1;
		}
	}

	lpobj->SyncStaticData();
	lpobj->SyncMutableData();

	if (TotalVertices != NumSphereVertices)
	{
		Write_Debug("Sphere vertex count was incorrect");
		return(FALSE);
	}

	// now generate the triangle indices
	if (Quarter < 2)
	{
		ia = 1;
		ib = 2;
	}
	else
	{
		ia = 2;
		ib = 1;
	}

  TotalIndices = 0;
	for (i=0 ; i < (MeshSize / 2) ; i++)
  {
    vo = (MeshSize + 1) * i;
		io = MeshSize * i * 6;
		for (j=0 ; j < MeshSize ; j++)
    {
			ptris[6 * j + io]      = vo + j;
			ptris[6 * j + ib + io]  = vo + j + 1;
			ptris[6 * j + ia + io]  = vo + j + (MeshSize + 1);
      TotalIndices += 3;

			ptris[6 * j + io + 3]      = vo + j + (MeshSize + 1);
			ptris[6 * j + ib + io + 3]  = vo + j + 1;
			ptris[6 * j + ia + io + 3]  = vo + j + (MeshSize + 1) + 1;
      TotalIndices += 3;
		}
	}

	if (TotalIndices != NumSphereIndices)
	{
		Write_Debug("Sphere index count was incorrect");
		return(FALSE);
	}

  if (!ldmdlCalcSurfaceNormals(lpobj) ||
      !ldmdlGetBounds(lpobj))
  {
    return(FALSE);
  }

	lpobj->loc.x = Location->x * ScaleFactor;
	lpobj->loc.y = Location->y * ScaleFactor;
	lpobj->loc.z = Location->z * ScaleFactor;

  Write_Debug("Quarter Sphere '%s' created with %d triangles comprised of %d vertices %d indices", name, NumSphereIndices / 3, NumSphereVertices, TotalIndices);

  return(TRUE);
}

LOGICAL obsphCreateQuarter(const char* name, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color, OBJECT* lpmain)
{
OBJECT*	lpobj;

	if (MeshSize & 1)
	{
		MeshSize += 1;
	}

	if (!CreateQuarter(name, 0, MeshSize, Location, Scale, Color, lpmain))
	{
		return(FALSE);
	}

	lpobj = obmainGetNewObject();
	if (!CreateQuarter(name, 1, MeshSize, Location, Scale, Color, lpobj))
	{
		return(FALSE);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(lpmain->texname);
	lpobj->texid = lpmain->texid;

	obmainAttachObject(lpmain, lpobj);

	lpobj = obmainGetNewObject();
	if (!CreateQuarter(name, 2, MeshSize, Location, Scale, Color, lpobj))
	{
		return(FALSE);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(lpmain->texname);
	lpobj->texid = lpmain->texid;

	obmainAttachObject(lpmain, lpobj);

	lpobj = obmainGetNewObject();
	if (!CreateQuarter(name, 3, MeshSize, Location, Scale, Color, lpobj))
	{
		return(FALSE);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(lpmain->texname);
	lpobj->texid = lpmain->texid;

	obmainAttachObject(lpmain, lpobj);

  return(TRUE);
}

LOGICAL obsphCreateVoxel(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, float Depth, UINT32 Color, OBJECT* lpobj)
{

return FALSE;
/*
char  localname[256];
UCHAR*  bits;
int   i, j, index, rows, cols, width, height, avg;
float wdelta, hdelta, maxheight, hmulti;
float	minrad, maxrad;
piBITMAPINFO*  bmi;
piVECTOR		loc;
piVERTEX*	pvorig, pverts;
piLVERTEX*	pslverts, pdlverts;

  if (!obsphCreate(name, MeshSize, Location, Scale, Color, lpobj))
  {
    return(FALSE);
  }

  // TEMPORARY
//  return(TRUE);

  strcpy(localname, texname);
  strcat(localname, "_height.bmp");

  if (!ldbmpLoadBMPFile(localname, &bmi))
  {
    return(FALSE);
  }

  if (bmi->bmiHeader.biBitCount != 8)
  {
    memFree(bmi);
    return(FALSE);
  }

	bits = (UCHAR*)bmi + bmi->bmiHeader.biSize;
  if (bmi->bmiHeader.biClrUsed == 0)
  {
    bits += 256 * sizeof(piRGBQUAD);
  }
  else
  {
    bits += bmi->bmiHeader.biClrUsed * sizeof(piRGBQUAD);
  }

  width = bmi->bmiHeader.biWidth;
  height = bmi->bmiHeader.biHeight;

  // Average the points around the top
  avg = 0;
  for (i=0 ; i < width ; i++)
  {
    avg += bits[i];
  }
  avg /= width;
  for (i=0 ; i < width ; i++)
  {
    bits[i] = avg;
  }

  // Average the points around the bottom
  avg = 0;
  index = (height - 1) * width;
  for (i=0 ; i < width ; i++)
  {
    avg += bits[index + i];
  }
  avg /= width;
  for (i=0 ; i < width ; i++)
  {
    bits[index + i] = avg;
  }

  // Average the points at the seam
  index = width;
  for (i=0 ; i < height - 1 ; i++)
  {
    avg = bits[index] + bits[index + width - 1];
    bits[index] = bits[index + width - 1] = avg / 2;
    index += width;
  }

  rows = MeshSize + 1;
  cols = MeshSize * 2 + 1;

  if (lpobj->numverts != (rows * cols))
  {
	memFree(bmi);
    return(TRUE);
  }

  hdelta = (float)(bmi->bmiHeader.biHeight - 1) / (float)(MeshSize);
  wdelta = (float)(bmi->bmiHeader.biWidth - 1) / (float)(MeshSize * 2);

  maxheight = lpobj->obox[1].x * Depth / 255.0f;// * ScaleFactor;

  pvorig = lpobj->pvorig;
  pverts = lpobj->pverts;
  pslverts = lpobj->pslverts;
  pdlverts = lpobj->pdlverts;
  for (i=0 ; i < rows ; i++)
  {
    for (j=0 ; j < cols ; j++)
    {
      index = (int)((float)((int)((float)i * hdelta) * width) + (float)j * wdelta);
      hmulti = (float)bits[index] * maxheight;

      pvorig->x += pvorig->nx * hmulti;
      pvorig->y += pvorig->ny * hmulti;
      pvorig->z += pvorig->nz * hmulti;
      pverts->x += pvorig->nx * hmulti;
      pverts->y += pvorig->ny * hmulti;
      pverts->z += pvorig->nz * hmulti;
      pslverts->x += pvorig->nx * hmulti;
      pslverts->y += pvorig->ny * hmulti;
      pslverts->z += pvorig->nz * hmulti;
      pdlverts->x += pvorig->nx * hmulti;
      pdlverts->y += pvorig->ny * hmulti;
      pdlverts->z += pvorig->nz * hmulti;

      pvorig++;
      pverts++;
      pslverts++;
      pdlverts++;
    }
  }

  memFree(bmi);

	if (!ldmdlGetBounds(lpobj) ||
			!ldmdlGetObjectBounds(lpobj, &loc, lpobj->obox, &minrad, &maxrad) ||
      !ldmdlCalcNormals(lpobj))
	{
		return(FALSE);
	}

  return(TRUE);*/
}
