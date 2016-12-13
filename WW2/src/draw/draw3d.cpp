#include <syscfg.h> // Path is project specific.
/*
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA
*/

#include "pi/pi.h"
#include "draw/draw3d.h"
#include "trn/trnvars.h"
#include "math/mathvect.h"
#include "include/vars.h"
#include "ogl/oglGlobalStates.h"

static bool _lineDrawing = false;

/************************************************************************************************
	draw3dPoint
*************************************************************************************************/

LOGICAL draw3dPoint(piVECTOR* point, UINT32 Color)
{
	piLVERTEX  Vertex;

	Vertex.x = point->x;
	Vertex.y = point->y;
	Vertex.z = point->z;
	Vertex.color = Color;
	Vertex.specular = colorALPHA100;
	Vertex.tu = 0.0f;
	Vertex.tv = 0.0f;

#if USE_STATESET
#else
	piDisableTexture();
	piDisableMaterial();
	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
	if (!piRenderScene(piPT_POINTLIST, piVT_LVERTEX, &Vertex, 1))
	{
		return(FALSE);
	}

  return(TRUE);
}

/************************************************************************************************
	draw3dLine
*************************************************************************************************/

LOGICAL draw3dLine(const piVECTOR* point1, const piVECTOR* point2, UINT32 Color)
{
	piLVERTEX  Vertices[2];

	Vertices[0].x = point1->x;
	Vertices[0].y = point1->y;
	Vertices[0].z = point1->z;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = point2->x;
	Vertices[1].y = point2->y;
	Vertices[1].z = point2->z;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

#if USE_STATESET
#else
	oglDisableTexturesAboveAndIncluding(0);

	piDisableMaterial();
 	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif

	if (!piRenderScene(piPT_LINELIST, piVT_LVERTEX, Vertices, 2))
	{
		return(FALSE);
	}

	return(TRUE);
}


/************************************************************************************************
	draw3dColoredLine
*************************************************************************************************/

LOGICAL draw3dColoredLine(piVECTOR* point1, UINT32 Color1, piVECTOR* point2, UINT32 Color2)
{
	piLVERTEX  Vertices[2];

	Vertices[0].x = point1->x;
	Vertices[0].y = point1->y;
	Vertices[0].z = point1->z;
	Vertices[0].color = Color1;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = point2->x;
	Vertices[1].y = point2->y;
	Vertices[1].z = point2->z;
	Vertices[1].color = Color2;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

#if USE_STATESET
#else
	piDisableTexture();
	piDisableMaterial();
 	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
 	if (!piRenderScene(piPT_LINELIST, piVT_LVERTEX, Vertices, 2))
	{
		return(FALSE);
	}

	return(TRUE);
}

/************************************************************************************************
	draw3dTexturedTriangle
*************************************************************************************************/

LOGICAL draw3dTexturedTriangle(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3,
                            int upperleft, UINT32 Color)
{
	piLVERTEX  Vertices[3];

	Vertices[0].x = point1->x;
	Vertices[0].y = point1->y;
	Vertices[0].z = point1->z;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = point2->x;
	Vertices[1].y = point2->y;
	Vertices[1].z = point2->z;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	if (upperleft)
	{
		Vertices[1].tu = 0.0f;
		Vertices[1].tv = 1.0f;
	}
	else
	{
		Vertices[1].tu = 1.0f;
		Vertices[1].tv = 0.0f;
	}

	Vertices[2].x = point3->x;
	Vertices[2].y = point3->y;
	Vertices[2].z = point3->z;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = 1.0f;
	Vertices[2].tv = 1.0f;

#if USE_STATESET
#else
	piDisableMaterial();
 	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif

	if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 3))
	{
		return(FALSE);
	}

  return(TRUE);
}

/************************************************************************************************
	draw3dTriangle
*************************************************************************************************/

LOGICAL draw3dTriangle(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3,
                    UINT32 Color1, UINT32 Color2, UINT32 Color3)
{
	piLVERTEX  Vertices[3];

	Vertices[0].x = point1->x;
	Vertices[0].y = point1->y;
	Vertices[0].z = point1->z;
	Vertices[0].color = Color1;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = point2->x;
	Vertices[1].y = point2->y;
	Vertices[1].z = point2->z;
	Vertices[1].color = Color2;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

	Vertices[2].x = point3->x;
	Vertices[2].y = point3->y;
	Vertices[2].z = point3->z;
	Vertices[2].color = Color3;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = 0.0f;
	Vertices[2].tv = 0.0f;

    if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
#else
	    piDisableTexture();
	    piDisableMaterial();
 	    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
 	    if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 3))
	    {
		    return(FALSE);
	    }
    }
    else
    {
        piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 3);
    }

  return(TRUE);
}

/************************************************************************************************
	draw3dTexturedQuad
*************************************************************************************************/

LOGICAL draw3dTexturedQuad(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3, piVECTOR* point4,
						UINT32 Color)
{
	piLVERTEX  Vertices[6];

	Vertices[0].x = point1->x;
	Vertices[0].y = point1->y;
	Vertices[0].z = point1->z;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = point2->x;
	Vertices[1].y = point2->y;
	Vertices[1].z = point2->z;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 1.0f;

	Vertices[2].x = point3->x;
	Vertices[2].y = point3->y;
	Vertices[2].z = point3->z;
	Vertices[2].color = Color;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = 1.0f;
	Vertices[2].tv = 1.0f;

	Vertices[3].x = point3->x;
	Vertices[3].y = point3->y;
	Vertices[3].z = point3->z;
	Vertices[3].color = Color;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = 1.0f;
	Vertices[3].tv = 1.0f;

	Vertices[4].x = point4->x;
	Vertices[4].y = point4->y;
	Vertices[4].z = point4->z;
	Vertices[4].color = Color;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = 1.0f;
	Vertices[4].tv = 0.0f;

	Vertices[5].x = point1->x;
	Vertices[5].y = point1->y;
	Vertices[5].z = point1->z;
	Vertices[5].color = Color;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = 0.0f;
	Vertices[5].tv = 0.0f;

    if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
#else
	    piDisableMaterial();
 	    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
 	    if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 6))
	    {
		    return(FALSE);
	    }
    }
    else
    {
        // Shader set by calling function
        piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 6);
    }

	return(TRUE);
}

//////////////////////////////////////////////////////////////////////
// Function: Draw3dTexture
// Description: The user sets the texture handle (piSetActiveTexture) and
// uses this function to draw a portion of the texture, or the whole
// texture. Usefull for displaying a texture with mulitiple icons, for
// example.
//////////////////////////////////////////////////////////////////////
LOGICAL draw3dTexture(SINT32 iTexID, piUV uvCoords[2], piVECTOR vecPts[4], UINT32 uColor)
{
    piLVERTEX  Vertices[6];

	Vertices[0].x = vecPts[0].x;
	Vertices[0].y = vecPts[0].y;
	Vertices[0].z = vecPts[0].z;
	Vertices[0].color = uColor;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = uvCoords[0].u;
	Vertices[0].tv = uvCoords[0].v;

	Vertices[1].x = vecPts[1].x;
	Vertices[1].y = vecPts[1].y;
	Vertices[1].z = vecPts[1].z;
	Vertices[1].color = uColor;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = uvCoords[0].u;
	Vertices[1].tv = uvCoords[1].v;

	Vertices[2].x = vecPts[2].x;
	Vertices[2].y = vecPts[2].y;
	Vertices[2].z = vecPts[2].z;
	Vertices[2].color = uColor;
	Vertices[2].specular = colorALPHA100;
	Vertices[2].tu = uvCoords[1].u;
	Vertices[2].tv = uvCoords[1].v;

	Vertices[3].x = vecPts[2].x;
	Vertices[3].y = vecPts[2].y;
	Vertices[3].z = vecPts[2].z;
	Vertices[3].color = uColor;
	Vertices[3].specular = colorALPHA100;
	Vertices[3].tu = uvCoords[1].u;
	Vertices[3].tv = uvCoords[1].v;

	Vertices[4].x = vecPts[3].x;
	Vertices[4].y = vecPts[3].y;
	Vertices[4].z = vecPts[3].z;
	Vertices[4].color = uColor;
	Vertices[4].specular = colorALPHA100;
	Vertices[4].tu = uvCoords[1].u;
	Vertices[4].tv = uvCoords[0].v;

	Vertices[5].x = vecPts[0].x;
	Vertices[5].y = vecPts[0].y;
	Vertices[5].z = vecPts[0].z;
	Vertices[5].color = uColor;
	Vertices[5].specular = colorALPHA100;
	Vertices[5].tu = uvCoords[0].u;
	Vertices[5].tv = uvCoords[0].v;

#if USE_STATESET
#else
    LOGICAL lightOn;
#endif

    if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
#else
	    piSetActiveTexture(iTexID, 0);
        piDisableMaterial();

        lightOn = piGetEngineRenderState(piRENDERSTATE_LIGHTING);

        piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif

        piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 6);

#if USE_STATESET
#else
        piSetRenderState(piRENDERSTATE_LIGHTING, lightOn);
#endif
    }
    else
    {
        piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 6);
    }

    return TRUE;
}

/************************************************************************************************
	draw3dDebugMarkPoint
*************************************************************************************************/

LOGICAL draw3dDebugMarkPoint (piVECTOR* point, UINT32 color, FLOAT32 size)
{
	piVECTOR V1, V2;

	if(!_lineDrawing && !VehicleObjectAnalysis) return TRUE;

	V2 = V1 = *point;
	V2.x -= size/2.0f;
	V1.x += size;
	draw3dLine(&V2, &V1, color);
	V2 = V1 = *point;
	V2.y -= size/2.0f;
	V1.y += size;
	draw3dLine(&V2, &V1, color);
	V2 = V1 = *point;
	V2.z -= size/2.0f;
	V1.z += size;
	draw3dLine(&V2, &V1, color);

	return TRUE;
}

void draw3dBoxTop(piVECTOR const* corner1, piVECTOR const* corner2, UINT32 color)
{
	piVECTOR v1,v2;

    v1 = v2 = *corner1;
    v2.x = corner2->x;
	draw3dLine(&v1, &v2, color);

    v1 = v2;
    v2.x = corner2->x;
    v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

    v1 = v2;
    v2.x = corner1->x;
    v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

    v1 = v2;
    v2 = *corner1;
	draw3dLine(&v1, &v2, color);
}

/************************************************************************************************
	draw3dBox
*************************************************************************************************/

LOGICAL draw3dBox(piVECTOR const* corner1, piVECTOR const* corner2, UINT32 color)
{
	piVECTOR v1,v2;

// top
	v1.x = corner1->x;
	v1.y = corner1->y;
	v1.z = corner1->z;
	v2 = v1;
	v2.z = corner2->z;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.x = corner2->x;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.z = corner1->z;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.x = corner1->x;
	draw3dLine(&v1, &v2, color);

// bottom
	v1.x = corner1->x;
	v1.y = corner2->y;
	v1.z = corner1->z;
	v2 = v1;
	v2.z = corner2->z;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.x = corner2->x;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.z = corner1->z;
	draw3dLine(&v1, &v2, color);

	v1 = v2;
	v2.x = corner1->x;
	draw3dLine(&v1, &v2, color);

// up/down lines
	v1.x = corner1->x;
	v1.y = corner1->y;
	v1.z = corner1->z;
	v2 = v1;
	v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

	v1.x = corner1->x;
	v1.y = corner1->y;
	v1.z = corner2->z;
	v2 = v1;
	v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

	v1.x = corner2->x;
	v1.y = corner1->y;
	v1.z = corner2->z;
	v2 = v1;
	v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

	v1.x = corner2->x;
	v1.y = corner1->y;
	v1.z = corner1->z;
	v2 = v1;
	v2.y = corner2->y;
	draw3dLine(&v1, &v2, color);

	return TRUE;
}

/************************************************************************************************
	draw3dLineDebug
*************************************************************************************************/

LOGICAL draw3dLineDebug(const piVECTOR* point1, const piVECTOR* point2, UINT32 Color)
{
	if(_lineDrawing || VehicleObjectAnalysis)
		return draw3dLine(point1, point2, Color);
	else
		return TRUE;
}


/************************************************************************************************
	draw3dGizmoLine
*************************************************************************************************/

LOGICAL draw3dGizmoLine(FLOAT64VECTOR const* inV1, FLOAT64VECTOR const* inV2, piMATRIX const* Mat, const FLOAT64VECTOR* Pos, UINT32 Color)
{
	if (!VehicleObjectAnalysis) return TRUE;

	FLOAT64VECTOR v1, v2;
	piLVERTEX Vertices[2];

	mathVectorMultiply64(&v1,inV1,Mat);
	mathAddVectors(&v1,&v1, Pos);
	v1.z = -v1.z;
	v1.x -= WorldOriginOffset.x; v1.y -= WorldOriginOffset.y; v1.z -= WorldOriginOffset.z;

	mathVectorMultiply64(&v2,inV2,Mat);
	mathAddVectors(&v2, &v2, Pos);
	v2.z = -v2.z;
	v2.x -= WorldOriginOffset.x; v2.y -= WorldOriginOffset.y; v2.z -= WorldOriginOffset.z;

	Vertices[0].x = (FLOAT32)v1.x;
	Vertices[0].y = (FLOAT32)v1.y;
	Vertices[0].z = (FLOAT32)v1.z;
	Vertices[0].color = Color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = (FLOAT32)v2.x;
	Vertices[1].y = (FLOAT32)v2.y;
	Vertices[1].z = (FLOAT32)v2.z;
	Vertices[1].color = Color;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

#if USE_STATESET
#else
	piDisableTexture();
	piDisableMaterial();
 	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
	piRenderScene(piPT_LINELIST, piVT_LVERTEX, Vertices, 2);

	return(TRUE);
}

/************************************************************************************************
	draw3dWorldDebug
*************************************************************************************************/

enum
{
	DRAW3D_WORLD_LINE,
	DRAW3D_WORLD_TEXT,
	DRAW3D_REL_LINE,
};

#define MAX_WORLD_DEBUG_OBJECTS 2048

struct  DebugDrawObject
{
	LOGICAL used;

	int type;
	char name[32];

	char text[32];
	FLOAT64VECTOR v1, v2;

	UINT32 color;
	LOGICAL alwaysOnTop;
	LOGICAL flipZ;
	FLOAT64VECTOR viewRange;

};


static DebugDrawObject debugObjects[MAX_WORLD_DEBUG_OBJECTS];

static void _draw3dWorldLine(DebugDrawObject*);
static void _draw3dWorldText(DebugDrawObject*);
static void _draw3dRelLine(DebugDrawObject*);
static int  _getFreeObject();

/************************************************************************************************
	draw3dDebugLineUpdate
*************************************************************************************************/

void draw3dDebugLineUpdate(LOGICAL drawTopObjects)
{
	if(_lineDrawing || VehicleObjectAnalysis)
	{
		int i;

		for(i = 0; i < MAX_WORLD_DEBUG_OBJECTS; i++)
		{
			if(debugObjects[i].used &&
			  ( (drawTopObjects  &&  debugObjects[i].alwaysOnTop) ||
				(!drawTopObjects && !debugObjects[i].alwaysOnTop)) )
			{
				switch(debugObjects[i].type)
				{
					case DRAW3D_WORLD_LINE:
						_draw3dWorldLine(&debugObjects[i]);
					break;

					case DRAW3D_WORLD_TEXT:
						_draw3dWorldText(&debugObjects[i]);
					break;

					case DRAW3D_REL_LINE:
						_draw3dRelLine(&debugObjects[i]);
					break;
				}
			}
		}
	}
}

/************************************************************************************************
	draw3dSetWorldDebugLine
*************************************************************************************************/

int draw3dSetWorldDebugLine(FLOAT64VECTOR const* v1, FLOAT64VECTOR const* v2, const char* name, UINT32 Color)
{
	DebugDrawObject* obj;
	int objID;

	objID = draw3dFindDebugObject(name);
	if(objID < 0)
		objID = _getFreeObject();
	if(objID < 0) return -1;

	obj = &debugObjects[objID];
	obj->type = DRAW3D_WORLD_LINE;
	strsafecpy(obj->name, name);
	obj->color = Color;
	obj->alwaysOnTop = TRUE;
	obj->flipZ = FALSE;
	obj->v1 = *v1;
	obj->v2 = *v2;

	return objID;
}

/************************************************************************************************
	draw3dSetDebugObjectFlipZ
*************************************************************************************************/

int draw3dSetDebugObjectFlipZ(int objID, LOGICAL flipZ)
{
	if(objID < 0 || objID >= MAX_WORLD_DEBUG_OBJECTS) return -1;

	debugObjects[objID].flipZ = flipZ;

	return objID;
}

/************************************************************************************************
	draw3dSetDebugObjectAlwaysOnTop
*************************************************************************************************/

int draw3dSetDebugObjectAlwaysOnTop(int objID, LOGICAL alwaysOnTop)
{
	if(objID < 0 || objID >= MAX_WORLD_DEBUG_OBJECTS) return -1;

	debugObjects[objID].alwaysOnTop = alwaysOnTop;

	return objID;
}

/************************************************************************************************
	draw3dFindDebugObject
*************************************************************************************************/

int draw3dFindDebugObject(const char* name)
{
	int i;

	if(name[0] == '\0')
		return -1;

	for(i = 0; i < MAX_WORLD_DEBUG_OBJECTS; i++)
	{
		if(debugObjects[i].used && *debugObjects[i].name && !strcmp(name, debugObjects[i].name))
			return i;
	}

	return -1;
}

/************************************************************************************************
	draw3dRemoveAllDebugObjects
*************************************************************************************************/

void draw3dRemoveAllDebugObjects()
{
	int i;

	for(i = 0; i < MAX_WORLD_DEBUG_OBJECTS; i++)
	{
		debugObjects[i].used = FALSE;
	}
}

/************************************************************************************************
	draw3dRemoveDebugObject
*************************************************************************************************/

void draw3dRemoveDebugObject(int i)
{
	if(i >= 0 && i < MAX_WORLD_DEBUG_OBJECTS)
		debugObjects[i].used = FALSE;
}

/************************************************************************************************
	_draw3dWorldLine
*************************************************************************************************/

static void _draw3dWorldLine(DebugDrawObject* obj)
{
	FLOAT64VECTOR v1, v2;
	piLVERTEX  Vertices[2];

	v1 = obj->v1; v2 = obj->v2;

	if(obj->flipZ)
	{
		v1.z = -v1.z; v2.z = -v2.z;
	}

	mathSubVectors(&v1, &v1, &WorldOriginOffset);
	mathSubVectors(&v2, &v2, &WorldOriginOffset);

	Vertices[0].x = (FLOAT32)v1.x;
	Vertices[0].y = (FLOAT32)v1.y;
	Vertices[0].z = (FLOAT32)v1.z;
	Vertices[0].color = obj->color;
	Vertices[0].specular = colorALPHA100;
	Vertices[0].tu = 0.0f;
	Vertices[0].tv = 0.0f;

	Vertices[1].x = (FLOAT32)v2.x;
	Vertices[1].y = (FLOAT32)v2.y;
	Vertices[1].z = (FLOAT32)v2.z;
	Vertices[1].color = obj->color;;
	Vertices[1].specular = colorALPHA100;
	Vertices[1].tu = 0.0f;
	Vertices[1].tv = 0.0f;

#if USE_STATESET
#else
    piDisableTexture();
	piDisableMaterial();
 	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif

	piRenderScene(piPT_LINELIST, piVT_LVERTEX, Vertices, 2);
}

/************************************************************************************************
	_draw3dWorldText
*************************************************************************************************/

static void _draw3dWorldText(DebugDrawObject* obj)
{
}

/************************************************************************************************
	_draw3dRelLine
*************************************************************************************************/

static void _draw3dRelLine(DebugDrawObject* obj)
{
}

/************************************************************************************************
	_getFreeObject
*************************************************************************************************/

static int _getFreeObject()
{
	int i;
	for(i = 0; i < MAX_WORLD_DEBUG_OBJECTS; i++)
	{
		if(!debugObjects[i].used)
		{
			debugObjects[i].used = 1;
			return i;
		}
	}

	return -1;
}

#ifdef BETA_RELEASE

#include <vector>
#include "math/math3d.h"

struct DebugLine
{
	piVECTOR p1;
	piVECTOR p2;
	UINT32 color;
};

std::vector<DebugLine>	debugLines;

void draw3dClearDebugLines()
{
    debugLines.clear();
}

void draw3dAddDebugLine(const piVECTOR& p1, const piVECTOR& p2, UINT32 color)
{
	DebugLine	debugLine;

	debugLine.p1 = p1;
	debugLine.p2 = p2;
	debugLine.color = color;

	debugLines.push_back(debugLine);
}

void draw3dUpdateDebugPositions(const piVECTOR& adjustment)
{
	if(debugLines.size() > 400)
	{
		debugLines.erase(debugLines.begin(), debugLines.begin() + debugLines.size() - 400);
	}

	for(UINT32 i=0; i < debugLines.size(); i++)
	{
		math3dAddVectors(&debugLines[i].p1, &debugLines[i].p1, &adjustment);
		math3dAddVectors(&debugLines[i].p2, &debugLines[i].p2, &adjustment);
	}
}

void draw3dDrawDebug()
{
	piLVERTEX  vertices[2];

	for(UINT32 i=0; i < debugLines.size(); i++)
	{
		vertices[0].x = debugLines[i].p1.x;
		vertices[0].y = debugLines[i].p1.y;
		vertices[0].z = debugLines[i].p1.z;
		vertices[0].color = debugLines[i].color;
		vertices[0].specular = colorALPHA100;
		vertices[0].tu = 0.0f;
		vertices[0].tv = 0.0f;

		vertices[1].x = debugLines[i].p2.x;
		vertices[1].y = debugLines[i].p2.y;
		vertices[1].z = debugLines[i].p2.z;
		vertices[1].color = debugLines[i].color;
		vertices[1].specular = colorALPHA100;
		vertices[1].tu = 0.0f;
		vertices[1].tv = 0.0f;

#if USE_STATESET
#else
		piDisableTexture();
		piDisableMaterial();
 		piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
#endif
		piRenderScene(piPT_LINELIST, piVT_LVERTEX, vertices, 2);
	}
}

void draw3dSetLineDrawing(bool l)
{
	_lineDrawing = l;
}

#endif // #ifdef BETA_RELEASE
