#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"
#include <math.h>
#include "ogl/oglvars.h"
#include "mem/mem.h"
#include "include/vars.h"
#include "list/list.h"
#include "ld/ldpix.h"
#include "ob/obmain.h"
#include "ob/obcd.h"
#include "ob/obdraw.h"
#include "ob/obdlite.h"
#include "ob/obcs.h"
#include "trn/trnmain.h"
#include "trn/trntod.h"
#include "trn/trnvars.h"
#include "trn/trnwtr.h"
#include "trn/trnsky.h"
#include "draw/draw3d.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "rn/rnvis.h"
#include "shader/glslNormal.h"
#include "shader/glslSpecular.h"
#include "wwii/wwiiCovert.h"
#include "ld/ldmdl.h"
#include "rn/rnmain.h"
#include "ogl/oglMesh.h"
#include "ob/obMgr.h"
#include "ogl/oglGlobalStates.h"
#include "trn/trncd.h"
#include "ogl/ogl.h"
#include "fx/fxMgr.h"
#include "fx/fxPcl.h"
#include "weap/weapammo.h"
#include "fx/TransformEffect.h"
#include "trn/LensFlare.h"
#include "pers/persmain.h"
#include "shader/glslWater.h"
#include "ogl/States.h"
#include "ogl/oglMaterial.h"
#include "math/mathnum.h"
#include "fx/postRender.h"
#include "osgImport/TextureRectangle.h"
#include "scen/ScenarioManager.h"

#define MAX_DRAW_PRIMITIVES  1000

#define INLINE inline

#include <vector>

#include "spt/sptmain.h"
#include "math/Matrixf.h"
#include "osgImport/FrameBufferObject.h"
#include "osgImport/Texture2D.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/ShadowMap.h"
#include "ld/ldtexture.h"
#include "trn/speedgrass.h"
#include "trn/trngrass.h"
#include "gr2/gr2Anim.h"
#include "fx/fxLgt.h"

#include "sdm/StratDB.h"

static ref_ptr<FrameBufferObject> _refractionFBO;
static ref_ptr<Texture2D> _refractionRTT;

static ref_ptr<FrameBufferObject> _reflectionFBO;
static ref_ptr<Texture2D> _reflectionRTT;

static ref_ptr<FrameBufferObject> _surfaceFBO;
static ref_ptr<Texture2D> _surfaceRTT;

static ref_ptr<FrameBufferObject> _sceneFBO;
static ref_ptr<FrameBufferObject> _resolveSceneFBO;

static ref_ptr<Texture> _sceneRTT;
static ref_ptr<Texture> _sceneDEPTH;

#if USE_STATESET
static ref_ptr<RenderStateSet> depthpass;
#endif

#define OBDRAW_EVENT_LOG
#undef OBDRAW_EVENT_LOG

#define MULTITEXTURE_HEIGHT		255.0f
#define SURFACE_FBO_SIZE        512

static UINT32					MultiTextureTransparentColor;
static MULTITEXTURE*			MultiTextureInfo;
static SINT32					NumMultiTextures;

static Covert<LOGICAL> g_drawObjectTriangles(FALSE);
static Covert<LOGICAL> g_drawObjectNormals(FALSE);
static Covert<LOGICAL> _drawCollisionNormals(FALSE);
static Covert<LOGICAL> _drawboundingbox(FALSE);
static Covert<LOGICAL> _drawTObox(FALSE);
static Covert<bool> _drawHeightMap(false);
static Covert<LOGICAL> _drawCollisionObjects(FALSE);

static Covert<bool> _unsorteddraw(true);
static Covert<bool> _sorteddraw(true);
static Covert<bool> _cockpitsorteddraw(true);
static Covert<bool> _cockpitunsorteddraw = true;
static Covert<bool> _multitexturedraw = true;

static USERDRAWFUNC _UnsortedExternalDrawCallback;
static USERDRAWFUNC _SortedExternalDrawCallback;
static USERDRAWFUNC _UnsortedInternalDrawCallback;
static USERDRAWFUNC _SortedInternalDrawCallback;

extern void trncdDrawShortTOBoundingBox();
extern void trncdDrawShortCollisionObjects();

static LOGICAL _drawDeprecatedPath();

static unsigned int _renderpass;
static bool multisample = false;

static SINT32 interiorMaterial = -1;
static SINT32 transglas = -1; // transglas
static SINT32 propclear = -1; // prop clear
static SINT32 propclear2 = -1; // prop clear
static SINT32 perspex = -1;   // perspex
static SINT32 blackmatte = -1;

extern SINT32 DefaultMaterialId;

static inline void _drawMultiTexture();
static inline void _drawSorted();
static inline void _drawUnsorted();
static void RenderOpaqueBin();
static void RenderTransparentBin();
static void RenderMultiTexture();

static inline int GetType(int flags)
{
	if (flags & OBJECT_LINESTRIP)
	{
		return(piPT_LINESTRIP);
	}
	else
	if (flags & OBJECT_LINECLOSED)
	{
		return(piPT_LINESTRIP);
	}
	else
	if (flags & OBJECT_TRIANGLEFAN)
	{
		if (flags & OBJECT_INDEXED)
		{
			return(piPT_TRIANGLELIST);
		}
		else
		{
			return(piPT_TRIANGLEFAN);
		}
	}
	else
	if (flags & OBJECT_TRIANGLELIST)
	{
		return(piPT_TRIANGLELIST);
	}

	return(0);
}

void obdrawObjectBoundingBox(OBJECT* o, UINT32 color)
{
    // Face 1
    piVECTOR a1 = {o->bbox[0].x, o->bbox[0].y, o->bbox[0].z};
    piVECTOR a2 = {o->bbox[0].x, o->bbox[1].y, o->bbox[0].z};
    piVECTOR a3 = {o->bbox[1].x, o->bbox[1].y, o->bbox[0].z};
    piVECTOR a4 = {o->bbox[1].x, o->bbox[0].y, o->bbox[0].z};

    // Face 2
    piVECTOR b1 = {o->bbox[1].x, o->bbox[1].y, o->bbox[1].z};
    piVECTOR b2 = {o->bbox[0].x, o->bbox[1].y, o->bbox[1].z};
    piVECTOR b3 = {o->bbox[0].x, o->bbox[0].y, o->bbox[1].z};
    piVECTOR b4 = {o->bbox[1].x, o->bbox[0].y, o->bbox[1].z};

    // Move points to object space position
    mathAddVectors(&a1, &a1, &o->pos);
    mathAddVectors(&a2, &a2, &o->pos);
    mathAddVectors(&a3, &a3, &o->pos);
    mathAddVectors(&a4, &a4, &o->pos);

    mathAddVectors(&b1, &b1, &o->pos);
    mathAddVectors(&b2, &b2, &o->pos);
    mathAddVectors(&b3, &b3, &o->pos);
    mathAddVectors(&b4, &b4, &o->pos);

    // Rotate / Translate to world
    math3dVectorMultiply(&a1, &a1, &o->mat);
    math3dVectorMultiply(&a2, &a2, &o->mat);
    math3dVectorMultiply(&a3, &a3, &o->mat);
    math3dVectorMultiply(&a4, &a4, &o->mat);

    math3dVectorMultiply(&b1, &b1, &o->mat);
    math3dVectorMultiply(&b2, &b2, &o->mat);
    math3dVectorMultiply(&b3, &b3, &o->mat);
    math3dVectorMultiply(&b4, &b4, &o->mat);

    // Box is already positioned in the world.
    piSetWorldMatrix(&IdentityMatrix);

    draw3dLine(&a1, &a2, color);
    draw3dLine(&a2, &a3, color);
    draw3dLine(&a3, &a4, color);
    draw3dLine(&a4, &a1, color);

    draw3dLine(&b1, &b2, color);
    draw3dLine(&b2, &b3, color);
    draw3dLine(&b3, &b4, color);
    draw3dLine(&b4, &b1, color);

    draw3dLine(&a1, &b3, color);
    draw3dLine(&a2, &b2, color);
    draw3dLine(&a3, &b1, color);
    draw3dLine(&a4, &b4, color);
}

void obdrawWireframeObject(OBJECT* lpobj, UINT32 color)
{
int   i, numtris;
piINDICES*	pt;

	if (lpobj == nullptr)
	{
		return;
	}

	numtris = lpobj->numtris;
	piVectorConstPtr rpos = lpobj->RenderPositions();
	pt = lpobj->ptris;

	if (rpos.ptr == nullptr || pt == nullptr) return;

	for (i=0; i < numtris; i++)
	{
		draw3dLine(&rpos[pt[i].v1], &rpos[pt[i].v2], color);
		draw3dLine(&rpos[pt[i].v2], &rpos[pt[i].v3], color);
		draw3dLine(&rpos[pt[i].v3], &rpos[pt[i].v1], color);
	}
}

static LOGICAL DrawShadow(OBJECT* lpobj)
{
LOGICAL		retval=TRUE;
int			texid, layer;

	if (lpobj->numverts == 0 ||
		lpobj->flags & OBJECT_SKIPDRAWING)
	{
		return(TRUE);
	}

	texid = lpobj->texid;
	if (!bTexturesDisabled)
	{
		piSetActiveTexture(texid, 0);
	}
	else
	{
		piDisableTexture();
	}

	layer = lpobj->layer + 12;
	if (layer > 16)
	{
		layer = 16;
	}

	piSetRenderState(piRENDERSTATE_ZBIAS, layer);

	if (lpobj->flags & OBJECT_NOFILTER)
	{
		piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
		piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);
	}

	if (lpobj->flags & OBJECT_INDEXED)
	{
		piSTRIDEDDATA sd;
		lpobj->StridedRenderData(&sd);

		if (!piRenderIndexStridedScene(piPT_TRIANGLELIST, piVT_VERTEX,
								&sd, lpobj->numverts,
								(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
		{
			WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (shadow)", lpobj->name);
			retval = FALSE;
		}
	}
	else
	{
		piSTRIDEDDATA sd;
		lpobj->StridedRenderData(&sd);

		if (!piRenderStridedScene((piPRIMITIVETYPE)piPT_TRIANGLELIST, piVT_VERTEX,
									&sd, lpobj->numverts))
		{
			WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (shadow)", lpobj->name);
			retval = FALSE;
		}
	}


	if (lpobj->flags & OBJECT_NOFILTER)
	{
		piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
		piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);
	}


	return(retval);
}

#if(0)
static LOGICAL DrawEnvMap(int type, OBJECT* lpobj)
{
#if(0)
UINT32       flags;
piLVERTEX* pverts;

	flags = lpobj->flags;
	pverts = lpobj->pmapverts;
	if (flags & OBJECT_INDEXED)
	{
		if (!piRenderIndexScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
								pverts, lpobj->numverts,
								(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
		{
			WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (%s lite)", lpobj->name,
						flags & OBJECT_DYNALITE ? "dyna" : "stat");
			return(FALSE);
		}
	}
	else
	{
		if (!piRenderScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
							pverts, lpobj->numverts))
		{
			WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (%s lite)", lpobj->name,
						flags & OBJECT_DYNALITE ? "dyna" : "stat");
			return(FALSE);
		}
	}
#endif
	return(TRUE);
}
#endif

static void DrawObjectInstanced(const OBJECT* o, int units = 0, int primcount = 1)
{
    oglUpdateRenderIndexStats(GetType(o->flags), o->numverts * primcount, o->numtris * 3 * primcount);

	if(o->meshContext && (o->flag2 & OBJECT_USEBUFFERS))
	{
		o->meshContext->DrawInstanced(primcount);
        return;
	}

    if(o->modelInstance && o->modelInstance->GetModel()->vbo)
    {
        o->modelInstance->GetModel()->vbo->DrawInstancedPrimitive(primcount, 0, o->modelNode->vboid);
        return;
    }

    piSTRIDEDDATA sd;
    o->StridedRenderData(&sd);

    ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
    ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, sd.normal.stride, sd.normal.pdata);

    if(units >= 0)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(units >= 1)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(units >= 2)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(sd.indices.pdata)
        ogl::DrawElementsInstanced(GetType(o->flags), o->numtris * 3, GL_UNSIGNED_SHORT, sd.indices.pdata, primcount);
    else
        ogl::DrawArraysInstanced(GetType(o->flags), 0, o->numverts, primcount);
}

static void DrawObject(const OBJECT* o, int units = 0)
{
    oglUpdateRenderIndexStats(GetType(o->flags), o->numverts, o->numtris * 3);

	if(o->meshContext && (o->flag2 & OBJECT_USEBUFFERS))
	{
		o->meshContext->Draw();
        return;
	}

    if(o->modelInstance && o->modelInstance->GetModel()->vbo)
    {
        o->modelInstance->GetModel()->vbo->DrawPrimitive(0, o->modelNode->vboid);
        return;
    }

    piSTRIDEDDATA sd;
    o->StridedRenderData(&sd);

    ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
    ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, sd.normal.stride, sd.normal.pdata);

    if(units >= 0)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(units >= 1)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(units >= 2)
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);

    if(sd.indices.pdata)
        ogl::DrawElements(GetType(o->flags), o->numtris * 3, GL_UNSIGNED_SHORT, sd.indices.pdata);
    else
        ogl::DrawArrays(GetType(o->flags), 0, o->numverts);
}

static inline void _drawVertices(int type, const OBJECT* o)
{
	if(o->meshContext && (o->flag2 & OBJECT_USEBUFFERS))
	{
        oglUpdateRenderIndexStats(oglPT_TRIANGLELIST, o->numverts, o->numtris * 3);
		o->meshContext->Draw();
	}
    else if(o->modelInstance && o->modelInstance->GetModel()->vbo)
    {
        oglUpdateRenderIndexStats(oglPT_TRIANGLELIST, o->numverts, o->numtris * 3);
        o->modelInstance->GetModel()->vbo->DrawPrimitive(0, o->modelNode->vboid);
    }
    else if(o->flags & OBJECT_INDEXED)
    {
        piSTRIDEDDATA sd;
        o->StridedRenderData(&sd);

        piRenderIndexStridedScene((piPRIMITIVETYPE)type,
                                   piVT_VERTEX,
                                   &sd,
                                   o->numverts,
                                   (UINT16*)o->ptris,
                                   o->numtris * 3);
    }
    else
    {
        piSTRIDEDDATA sd;
        o->StridedRenderData(&sd);

        piRenderStridedScene((piPRIMITIVETYPE)type,
                            piVT_VERTEX,
                            &sd,
                            o->numverts);
    }

#ifdef BETA_RELEASE
	//oglSetWorldMatrix(&o->mat);		// Hack until new collision code is used
    if(g_drawObjectTriangles)
    {
        piVectorConstPtr p = o->RenderPositions();

        for(SINT32 i = 0; i < o->numtris; i++)
        {
            draw3dLine(&p[o->ptris[i].v1], &p[o->ptris[i].v2], colorYELLOWALPHA);
            draw3dLine(&p[o->ptris[i].v2], &p[o->ptris[i].v3], colorYELLOWALPHA);
            draw3dLine(&p[o->ptris[i].v3], &p[o->ptris[i].v1], colorYELLOWALPHA);
        }
    }

    if(g_drawObjectNormals)
    {
        piVectorConstPtr normals = o->RenderNormals();
        piVectorConstPtr v = o->RenderPositions();

        piVECTOR pos;

        for(SINT32 i = 0; i < o->numverts; i++)
        {
            pos.x = v[i].x + normals[i].x * 0.5f;
            pos.y = v[i].y + normals[i].y * 0.5f;
            pos.z = v[i].z + normals[i].z * 0.5f;

            draw3dLine(&v[i], &pos, colorREDALPHA);
        }
    }

    if(_drawCollisionNormals && o->pscdn)
    {
        piINDICES* indices = o->ptris;

        piVECTOR p1, p2;

        for(SINT32 i = 0; i < o->numtris; i++)
        {
            p1.x = o->puverts[indices[i].v1].x;
            p1.y = o->puverts[indices[i].v1].y;
            p1.z = o->puverts[indices[i].v1].z;

            p2.x = p1.x + o->pscdn[indices[i].v1].x * 0.5f;
            p2.y = p1.y + o->pscdn[indices[i].v1].y * 0.5f;
            p2.z = p1.z + o->pscdn[indices[i].v1].z * 0.5f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1.x = o->puverts[indices[i].v2].x;
            p1.y = o->puverts[indices[i].v2].y;
            p1.z = o->puverts[indices[i].v2].z;

            p2.x = p1.x + o->pscdn[indices[i].v2].x * 0.5f;
            p2.y = p1.y + o->pscdn[indices[i].v2].y * 0.5f;
            p2.z = p1.z + o->pscdn[indices[i].v2].z * 0.5f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1.x = o->puverts[indices[i].v3].x;
            p1.y = o->puverts[indices[i].v3].y;
            p1.z = o->puverts[indices[i].v3].z;

            p2.x = p1.x + o->pscdn[indices[i].v3].x * 0.5f;
            p2.y = p1.y + o->pscdn[indices[i].v3].y * 0.5f;
            p2.z = p1.z + o->pscdn[indices[i].v3].z * 0.5f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);
        }
    }

    if(_drawboundingbox && !(o->flags & OBJECT_DYNASHADOW))
    {
        // Face 1
        piVECTOR a1 = {o->bbox[0].x, o->bbox[0].y, o->bbox[0].z};
        piVECTOR a2 = {o->bbox[0].x, o->bbox[1].y, o->bbox[0].z};
        piVECTOR a3 = {o->bbox[1].x, o->bbox[1].y, o->bbox[0].z};
        piVECTOR a4 = {o->bbox[1].x, o->bbox[0].y, o->bbox[0].z};

        // Face 2
        piVECTOR b1 = {o->bbox[1].x, o->bbox[1].y, o->bbox[1].z};
        piVECTOR b2 = {o->bbox[0].x, o->bbox[1].y, o->bbox[1].z};
        piVECTOR b3 = {o->bbox[0].x, o->bbox[0].y, o->bbox[1].z};
        piVECTOR b4 = {o->bbox[1].x, o->bbox[0].y, o->bbox[1].z};

        // Move points to object space position
        mathAddVectors(&a1, &a1, &o->pos);
        mathAddVectors(&a2, &a2, &o->pos);
        mathAddVectors(&a3, &a3, &o->pos);
        mathAddVectors(&a4, &a4, &o->pos);

        mathAddVectors(&b1, &b1, &o->pos);
        mathAddVectors(&b2, &b2, &o->pos);
        mathAddVectors(&b3, &b3, &o->pos);
        mathAddVectors(&b4, &b4, &o->pos);

        // Rotate / Translate to world
        math3dVectorMultiply(&a1, &a1, &o->mat);
        math3dVectorMultiply(&a2, &a2, &o->mat);
        math3dVectorMultiply(&a3, &a3, &o->mat);
        math3dVectorMultiply(&a4, &a4, &o->mat);

        math3dVectorMultiply(&b1, &b1, &o->mat);
        math3dVectorMultiply(&b2, &b2, &o->mat);
        math3dVectorMultiply(&b3, &b3, &o->mat);
        math3dVectorMultiply(&b4, &b4, &o->mat);

        // Box is already positioned in the world.
        piSetWorldMatrix(&IdentityMatrix);

        draw3dLine(&a1, &a2, colorWHITEALPHA);
        draw3dLine(&a2, &a3, colorWHITEALPHA);
        draw3dLine(&a3, &a4, colorWHITEALPHA);
        draw3dLine(&a4, &a1, colorWHITEALPHA);

        draw3dLine(&b1, &b2, colorWHITEALPHA);
        draw3dLine(&b2, &b3, colorWHITEALPHA);
        draw3dLine(&b3, &b4, colorWHITEALPHA);
        draw3dLine(&b4, &b1, colorWHITEALPHA);

        draw3dLine(&a1, &b3, colorBLUEALPHA);
        draw3dLine(&a2, &b2, colorBLUEALPHA);
        draw3dLine(&a3, &b1, colorBLUEALPHA);
        draw3dLine(&a4, &b4, colorBLUEALPHA);
    }

#endif
}

static LOGICAL DrawVertices(int type, OBJECT* lpobj)
{
	if (lpobj->flags & OBJECT_TERRAIN)
	{
		if (lpobj->flags & OBJECT_INDEXED)
		{
			if (!piRenderIndexScene((piPRIMITIVETYPE)type, piVT_CVERTEX,
									lpobj->ptverts, lpobj->numtverts,
									(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (no lite)", lpobj->name);

                 return(FALSE);
			}
		}
		else
		{
			if (!piRenderScene((piPRIMITIVETYPE)type, piVT_CVERTEX,
								lpobj->ptverts, lpobj->numtverts))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (no lite)", lpobj->name);

                return(FALSE);
			}
		}
	}
	else
	{
		if(lpobj->meshContext && lpobj->meshContext->IsBuffered())
		{
            lpobj->meshContext->Draw();
		}
		else
		if (lpobj->flags & OBJECT_INDEXED)
		{
			piSTRIDEDDATA sd;
			lpobj->StridedRenderData(&sd);

			if (!piRenderIndexStridedScene((piPRIMITIVETYPE)type, piVT_VERTEX,
									&sd, lpobj->numverts,
									(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (no lite)", lpobj->name);

                return(FALSE);
			}
		}
		else
		{
			piSTRIDEDDATA sd;
			lpobj->StridedRenderData(&sd);

			if (!piRenderStridedScene((piPRIMITIVETYPE)type, piVT_VERTEX,
										&sd, lpobj->numverts))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (no lite)", lpobj->name);

                return(FALSE);
			}
		}
	}

	return(TRUE);
}

#if(0)
static LOGICAL DrawLitVertices(int type, OBJECT* lpobj)
{
int				numvis;
UINT32			flags;

	flags = lpobj->flags;
	if (flags & OBJECT_TERRAIN)
	{
		if (flags & OBJECT_INDEXED)
		{
			if (!piRenderIndexScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
									lpobj->ptverts, lpobj->numtverts,
									(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (lite)", lpobj->name);
				return(FALSE);
			}
		}
		else
		{
			if (!piRenderScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
								lpobj->ptverts, lpobj->numtverts))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (lite)", lpobj->name);
				return(FALSE);
			}
		}
	}
	else
	{
		numvis = lpobj->numverts;

		if (flags & OBJECT_INDEXED)
		{
			piSTRIDEDDATA sd;
			lpobj->StridedRenderData(&sd);

			if (!piRenderIndexStridedScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
									&sd, lpobj->numverts,
									(UINT16*)lpobj->ptris,  lpobj->numtris * 3))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderIndexScene failed at entity %s (%s lite)", lpobj->name,
							flags & OBJECT_DYNALITE ? "dyna" : "stat");
				return(FALSE);
			}
		}
		else
		{
			piSTRIDEDDATA sd;
			lpobj->StridedRenderData(&sd);

			if (!piRenderStridedScene((piPRIMITIVETYPE)type, piVT_LVERTEX,
								&sd, numvis))
			{
				WriteLog(LOG_RENDER, LOG_DEBUG, "piRenderScene failed at entity %s (%s lite)", lpobj->name,
							flags & OBJECT_DYNALITE ? "dyna" : "stat");
				return(FALSE);
			}
		}
	}

	return(TRUE);
}
#endif

static LOGICAL InitMultiTexture()
{
SINT32		i;

	if (MultiTextureInfo != nullptr)
	{
		WriteLog(LOG_RENDER, LOG_DEBUG, "MultiTextureInfo structure pointer has already been initialized");
		return(FALSE);
	}

	MultiTextureInfo = (MULTITEXTURE*)memAlloc(MAX_MULTITEXTURES * sizeof(MULTITEXTURE));
	if (MultiTextureInfo == nullptr)
	{
		return(FALSE);
	}

	for (i=0; i < MAX_MULTITEXTURES; i++)
	{
		MultiTextureInfo[i].MultiTextureId = -1;
		MultiTextureInfo[i].MultiMaterialId = -1;
		MultiTextureInfo[i].MultiTextureMatrix = IdentityMatrix;
	}

	MultiTextureTransparentColor = colorMAGENTA;

	NumMultiTextures = 0;

	return(TRUE);
}

static void KillMultiTexture()
{
SINT32		i;

	if (MultiTextureInfo == nullptr)
	{
		return;
	}

	for (i=0; i < MAX_MULTITEXTURES; i++)
	{
		piFreeMipTextures(MultiTextureInfo[i].MultiTextureId);
		piFreeMaterial(MultiTextureInfo[i].MultiMaterialId);
	}

	if (MultiTextureInfo != nullptr)
	{
		memFree(MultiTextureInfo);
		MultiTextureInfo = nullptr;
	}
}

LOGICAL obdrawSetMultiTexture(const char* BaseTexture, const char* MultiTexture, SINT32 TextureOpcode, FLOAT32 MaxDistance, FLOAT32 TextureMultiplierX, FLOAT32 TextureMultiplierY)
{
SINT32		i, j, btid, mtid;

	if (MultiTextureInfo == nullptr)
	{
		WriteLog(LOG_RENDER, LOG_DEBUG, "MultiTextureInfo structure pointer has not been initialized");
		return(FALSE);
	}

	if (NumMultiTextures >= MAX_MULTITEXTURES)
	{
		WriteLog(LOG_RENDER, LOG_DEBUG, "Too many multitexture pairs have been specified");
		return(FALSE);
	}

#ifdef EVENT_LOG_EXTREME
	WriteLog(LOG_RENDER, LOG_DEBUG, "MultiTexture %s - %s", BaseTexture, MultiTexture);
#endif

    std::string str;

	if (MultiTextureEnable)
	{
		btid = ldpixLoadImageFileToSurface(BaseTexture, MultiTextureTransparentColor, LDPIX_MIPMAP);
		if (!piIsTexture(btid, 0))
		{
			return(FALSE);
		}

 		mtid = ldpixLoadImageFileToSurface(MultiTexture, MultiTextureTransparentColor, LDPIX_MIPMAP);
		if (!piIsTexture(mtid, 0))
		{
			return(FALSE);
		}

        str = MultiTexture;

        int pos = str.rfind('.');

        if(pos != std::string::npos)
        {
            str.erase(pos, str.length()-pos);
        }

        str += "_normal";

        int id = ldpixLoadImageFileToSurface(str.c_str(), MultiTextureTransparentColor, LDPIX_MIPMAP);

        if(!piIsTexture(id, 0))
        {
            str = MultiTexture;

            if(pos != std::string::npos)
            {
                str.erase(pos, str.length()-pos);
            }

            str += "_nrm";

            id = ldpixLoadImageFileToSurface(str.c_str(), MultiTextureTransparentColor, LDPIX_MIPMAP);
        }

        if(piIsTexture(id, 0))
        {
            piGetTextureData(mtid)->normalMap = id;
        }

        str = MultiTexture;

        if(pos != std::string::npos)
        {
            str.erase(pos, str.length()-pos);
        }

        str += "_spec";

        id = ldpixLoadImageFileToSurface(str.c_str(), MultiTextureTransparentColor, LDPIX_MIPMAP);

        if(piIsTexture(id, 0))
        {
            piGetTextureData(mtid)->specularMap = id;
        }
	}
	else
	{
		btid = mtid = -1;
	}

	for (i=0; i < NumMultiTextures; i++)
	{
		if (btid == MultiTextureInfo[i].BaseTextureId &&
			mtid == MultiTextureInfo[i].MultiTextureId)
		{
 			return(TRUE);
		}
	}

	for (i=0; i < NumMultiTextures; i++)
	{
		if (btid < MultiTextureInfo[i].BaseTextureId)
		{
			break;
		}
	}

	if (i == NumMultiTextures)
	{
		strcpy(MultiTextureInfo[NumMultiTextures].BaseTextureName, BaseTexture);
		MultiTextureInfo[NumMultiTextures].BaseTextureId = btid;
		strcpy(MultiTextureInfo[NumMultiTextures].MultiTextureName, MultiTexture);
		MultiTextureInfo[NumMultiTextures].MultiTextureId = mtid;

		MultiTextureInfo[NumMultiTextures].MultiTextureOpcode = TextureOpcode;

		MultiTextureInfo[NumMultiTextures].MultiTextureMatrix._11 = TextureMultiplierX;
		MultiTextureInfo[NumMultiTextures].MultiTextureMatrix._22 = TextureMultiplierY;

		MultiTextureInfo[NumMultiTextures].MultiMaxDistance = MaxDistance;

        TEXTURE* lpTex = piGetTextureData(btid);

        if(lpTex)
        {
            lpTex->multiTextureInfoID = NumMultiTextures + 1;
        }

		NumMultiTextures += 1;
	}
	else
	{
        TEXTURE* lpTex = nullptr;

		for (j=NumMultiTextures; j > i; j--)
		{
			MultiTextureInfo[j] = MultiTextureInfo[j-1];

            lpTex = piGetTextureData(MultiTextureInfo[j].BaseTextureId);

            if(lpTex)
            {
                lpTex->multiTextureInfoID = j + 1;
            }
		}

		NumMultiTextures += 1;

		strcpy(MultiTextureInfo[i].BaseTextureName, BaseTexture);
		MultiTextureInfo[i].BaseTextureId = btid;
		strcpy(MultiTextureInfo[i].MultiTextureName, MultiTexture);
		MultiTextureInfo[i].MultiTextureId = mtid;

        lpTex = piGetTextureData(btid);

        if(lpTex)
        {
            lpTex->multiTextureInfoID = i + 1;
        }

		MultiTextureInfo[i].MultiTextureOpcode = TextureOpcode;

		MultiTextureInfo[i].MultiTextureMatrix._11 = TextureMultiplierX;
		MultiTextureInfo[i].MultiTextureMatrix._22 = TextureMultiplierY;

		MultiTextureInfo[i].MultiMaxDistance = MaxDistance;
	}

	return(TRUE);
}

LOGICAL obdrawFindMultiTextureByName(SINT8* BaseTexture, SINT32* MTOffset)
{
SINT32		i;

	if (MultiTextureInfo == nullptr)
	{
		return(FALSE);
	}

	for (i=0; i < NumMultiTextures; i++)
	{
		if (BaseTexture[0] == MultiTextureInfo[i].MultiTextureName[0])
		{
			if (strcmp(BaseTexture, MultiTextureInfo[i].MultiTextureName) == 0)
			{
				*MTOffset = i;
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

LOGICAL obdrawFindMultiTextureById(SINT32 BaseId, SINT32* MTOffset)
{
    if(MultiTextureInfo == nullptr)
        return FALSE;

    TEXTURE* lpTex = piGetTextureData(BaseId);

    if(lpTex == nullptr)
        return FALSE;

    SINT32 i = lpTex->multiTextureInfoID - 1;

    if(i < 0 || i >= NumMultiTextures)
        return FALSE;

    *MTOffset = i;

    return TRUE;
}

SINT32 obdrawGetMultiTextureOpcode(SINT32 BaseId, SINT32* MTOffset)
{
    if(MultiTextureInfo == nullptr)
        return MT_NORMAL;

    TEXTURE* lpTex = piGetTextureData(BaseId);

    if(lpTex == nullptr)
        return MT_NORMAL;

    SINT32 i = lpTex->multiTextureInfoID - 1;

    if(i < 0 || i >= NumMultiTextures)
        return MT_NORMAL;

    if(MTOffset)
    {
        *MTOffset = i;
    }

    return MultiTextureInfo[i].MultiTextureOpcode;
}

LOGICAL obdrawGetMultiTextureInfo(MULTITEXTURE** retMultiTextureInfo, SINT32* retNumMultiTextures)
{
	if (MultiTextureInfo == nullptr)
	{
		return(FALSE);
	}

	*retMultiTextureInfo = MultiTextureInfo;
	*retNumMultiTextures = NumMultiTextures;

	return(TRUE);
}

LOGICAL obdrawApplyMultiTexture(OBJECT* lpobj)
{
LOGICAL			retval=TRUE;
SINT32			texid, offset, type;
FLOAT32			height;
piVECTOR		v;

	if (!MultiTextureEnable || bTexturesDisabled)
	{
		return TRUE;
	}

	texid = lpobj->texid;
	while (obdrawFindMultiTextureById(texid, &offset))
	{
		v.x = lpobj->mat._41 + playpos.x;
		v.y = lpobj->mat._42 + playpos.y;
		v.z = lpobj->mat._43 + playpos.z;
		height = math3dDotProduct(&v, &v);
		height = sqrt(height) + lpobj->pos.z + lpobj->bbox[0].z;
		//height = (lpobj->mat._31 * lpobj->mat._41 + lpobj->mat._32 * lpobj->mat._42 + lpobj->mat._33 * lpobj->mat._43) + playpos.z + lpobj->pos.z + lpobj->bbox[0].z;
		if (height >= MultiTextureInfo[offset].MultiMaxDistance)
		{
			break;
		}

		if (height < 0.0f)
		{
			height = 0.0f;
		}

		if (height >= (MultiTextureInfo[offset].MultiMaxDistance - 255.0f))
		{
			height -= MultiTextureInfo[offset].MultiMaxDistance - 255.0f;
			if (height > 255.0f)
			{
				height = 255.0f;
			}
		}
		else
		{
			height = 0.0f;
		}

		piSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);

		texid = MultiTextureInfo[offset].MultiTextureId;
		if (!bTexturesDisabled)
		{
			piSetActiveTexture(texid, 0);
		}

		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
		piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_LESSEQUAL);
		piSetRenderState(piRENDERSTATE_SRCBLEND, piBLEND_SRCALPHA);
		piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_INVSRCALPHA);
		piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, TRUE);
		piSetRenderState(piRENDERSTATE_ALPHAREF, (UINT32)height);

		piSetRenderStageState(0, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);
		piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_LINEAR);

		type = GetType(lpobj->flags);

#ifndef USE_3DAPI_LIGHTING
		if (lpobj->flags & OBJECT_LITE)
		{
			if (!DrawLitVertices(type, lpobj))
			{
				retval = FALSE;
			}
		}
		else
#endif
		{
			if (!DrawVertices(type, lpobj))
			{
				retval = FALSE;
			}
		}

		piSetTextureMatrix(&IdentityMatrix);

		piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
		piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
		piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
		piSetRenderState(piRENDERSTATE_DESTBLEND, SrcBlend);
		piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
		piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
		piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

		texid = MultiTextureInfo[offset].MultiTextureId;
	}

	return(retval);
}

// For non replacement tiles
LOGICAL obdrawApplyMultiTextureStage2(OBJECT* lpobj)
{
SINT32				texid, offset, stage, combine;
piTexCoordConstPtr	ptu;

	if (!MultiTextureEnable || bTexturesDisabled)
	{
		return TRUE;
	}

    if(!lpobj) return TRUE;

	ptu = lpobj->ImmutableTexCoords();

	texid = lpobj->texid;
	piSetActiveTexture(texid, 0);

	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);
	piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);
	piSetRenderStageState(0, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(0, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(0, piTSS_SOURCE0_ALPHA, piTSS_TEXTURE);
	piSetRenderStageState(0, piTSS_OPERAND0_ALPHA, piTSS_SRC_ALPHA);

	stage = 1;
	while (stage < gMaxTextureUnits && obdrawFindMultiTextureById(texid, &offset))
	{
		switch ((SINT32)MultiTextureInfo[offset].MultiMaxDistance)
		{
		case 0:
			combine = piTSS_REPLACE;
			break;
		case 1:
			combine = piTSS_MODULATE;
			break;
		case 2:
			combine = piTSS_ADD;
			break;
		case 3:
			combine = piTSS_ADD_SIGNED;
			break;
		case 4:
			combine = piTSS_INTERPOLATE;
			break;
		case 5:
			combine = piTSS_SUBTRACT;
			break;
		default:
			combine = piTSS_MODULATE;
			break;
		}

		piSetRenderStageState(stage-1, piTSS_COMBINE_RGB, combine);
		piSetRenderStageState(stage-1, piTSS_COMBINE_ALPHA, combine);

		oglSetActiveTextureStage(stage);

		piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

		piSetActiveTexture(MultiTextureInfo[offset].MultiTextureId, stage);
		piSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);

	    if(!(lpobj->meshContext && (lpobj->flag2 & OBJECT_USEBUFFERS)))
		{
			if (lpobj->flags & OBJECT_TERRAIN)
			{
				ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &lpobj->ptverts->tu);
			}
			else
			{
				ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &ptu->u);
			}
		}

		piSetRenderStageState(stage, piTSS_COMBINE_RGB, combine);
		piSetRenderStageState(stage, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
		piSetRenderStageState(stage, piTSS_SOURCE1_RGB, piTSS_PREVIOUS);
		piSetRenderStageState(stage, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
		piSetRenderStageState(stage, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

		piSetRenderStageState(stage, piTSS_COMBINE_ALPHA, combine);
		piSetRenderStageState(stage, piTSS_SOURCE0_ALPHA, piTSS_TEXTURE);
		piSetRenderStageState(stage, piTSS_SOURCE1_ALPHA, piTSS_PREVIOUS);
		piSetRenderStageState(stage, piTSS_OPERAND0_ALPHA, piTSS_SRC_ALPHA);
		piSetRenderStageState(stage, piTSS_OPERAND1_ALPHA, piTSS_SRC_ALPHA);

		stage += 1;
		texid = MultiTextureInfo[offset].MultiTextureId;
	}

	if (stage < gMaxTextureUnits)
	{
		oglSetActiveTextureStage(stage);
		piDisableTexture();
	}

	oglSetActiveTextureStage(0);
	piEnableTexture();

	piSetTextureMatrix(&IdentityMatrix);//MultiTextureInfo[offset].MultiTextureMatrix);

	return(TRUE);
}

#if USE_STATESET
LOGICAL obdrawApplyMultiTextureStage(OBJECT* lpobj)
{
    lpobj->material->Apply();
	return(TRUE);
}
#else
// For replacement tiles
LOGICAL obdrawApplyMultiTextureStage(OBJECT* lpobj)
{
SINT32				texid, offset, stage, combine;
piTexCoordConstPtr	ptu;

	if (!MultiTextureEnable || bTexturesDisabled)
	{
		return TRUE;
	}

    if(!lpobj) return TRUE;

	ptu = lpobj->ImmutableTexCoords();

	texid = lpobj->texid;

    lpobj->material->ApplyTexture(0);

	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);

	piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);
	piSetRenderStageState(0, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(0, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(0, piTSS_SOURCE0_ALPHA, piTSS_TEXTURE);
	piSetRenderStageState(0, piTSS_OPERAND0_ALPHA, piTSS_SRC_ALPHA);

    TextureState::SetTextureUnit(2);
    oglSetTextureMatrix(&IdentityMatrix);

	stage = 1;

	while (stage < gMaxTextureUnits && obdrawFindMultiTextureById(texid, &offset))
	{
		switch ((SINT32)MultiTextureInfo[offset].MultiMaxDistance)
		{
		case 0:
			combine = piTSS_REPLACE;
			break;
		case 1:
			combine = piTSS_MODULATE;
			break;
		case 2:
			combine = piTSS_ADD;
			break;
		case 3:
			combine = piTSS_ADD_SIGNED;
			break;
		case 4:
			combine = piTSS_INTERPOLATE;
			break;
		case 5:
			combine = piTSS_SUBTRACT;
			break;
		default:
			combine = piTSS_MODULATE;
			break;
		}

		piSetRenderStageState(stage-1, piTSS_COMBINE_RGB, combine);
		piSetRenderStageState(stage-1, piTSS_COMBINE_ALPHA, combine);

        oglSetActiveTextureStage(stage);

        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

        piSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);

		piSetRenderStageState(stage, piTSS_COMBINE_RGB, combine);
		piSetRenderStageState(stage, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
		piSetRenderStageState(stage, piTSS_SOURCE1_RGB, piTSS_PREVIOUS);
		piSetRenderStageState(stage, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
		piSetRenderStageState(stage, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

		piSetRenderStageState(stage, piTSS_COMBINE_ALPHA, combine);
		piSetRenderStageState(stage, piTSS_SOURCE0_ALPHA, piTSS_TEXTURE);
		piSetRenderStageState(stage, piTSS_SOURCE1_ALPHA, piTSS_PREVIOUS);
		piSetRenderStageState(stage, piTSS_OPERAND0_ALPHA, piTSS_SRC_ALPHA);
		piSetRenderStageState(stage, piTSS_OPERAND1_ALPHA, piTSS_SRC_ALPHA);

		stage += 1;
		texid = MultiTextureInfo[offset].MultiTextureId;
	}

	return(TRUE);
}
#endif

static void matrixInvert(piMATRIX* in, piMATRIX* out)
{
  // Transpose rotation
  out->_11 = in->_11;  out->_12 = in->_21;  out->_13 = in->_31;
  out->_21 = in->_12;  out->_22 = in->_22;  out->_23 = in->_32;
  out->_31 = in->_13;  out->_32 = in->_23;  out->_33 = in->_33;

  // Clear shearing terms
  out->_14 = 0.0f; out->_24 = 0.0f; out->_34 = 0.0f; out->_44 = 1.0f;

  // Translation is minus the dot of translation and rotations
  out->_41 = -(in->_41 * in->_11) - (in->_42 * in->_12) - (in->_43 * in->_13);
  out->_42 = -(in->_41 * in->_21) - (in->_42 * in->_22) - (in->_43 * in->_23);
  out->_43 = -(in->_41 * in->_31) - (in->_42 * in->_32) - (in->_43 * in->_33);
}

LOGICAL obdrawUpdateDOT3ColorPoint(OBJECT* lpobj)
{
	int			i;			// loop index
	LIGHT*		lplight;	// light pointer
	piVECTOR	lpES;		// light position in Eye space
	piVECTOR	lpOS;		// light position in Object space
	piVECTOR	lv;			// vertex to light vector
	piVECTOR	lvTS;		// light vector in Tangent space

	piVectorPtr			pverts;
	piVertexColorPtr	pcolors;

	piMATRIX	modelViewMatrix;
	piMATRIX	modelViewMatrixInverse;

	static piMATRIX tangentMatrix = {	{1.0, 0.0, 0.0, 0.0,
										0.0,-1.0, 0.0, 0.0,
										0.0, 0.0, 1.0, 0.0,
 										0.0, 0.0, 0.0, 1.0}};

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	pverts = lpobj->MutablePositions();
	pcolors = lpobj->MutableColors();

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&modelViewMatrix);
	matrixInvert(&modelViewMatrix, &modelViewMatrixInverse);

	// get light's position in eye space
	lplight = trnskyGetSunSource();
	lpES = lplight->light.dvPosition;

	// convert from eye space to object space
	math3dMultiplyRotation(&lpOS, &lpES, &modelViewMatrixInverse);

	for (i=0; i < lpobj->numverts; i++)
	{
		lv.x = lpOS.x - pverts[i].x;
		lv.y = lpOS.y - pverts[i].y;
		lv.z = lpOS.z - pverts[i].z;
		math3dNormalize(&lv);

		// rotate into tangent space
		math3dMultiplyRotation(&lvTS, &lv, &tangentMatrix);

		pcolors[i].color = piRGBA((lvTS.x * 0.5f) + 0.5f, (lvTS.y * 0.5f) + 0.5f, (lvTS.z * 0.5f) + 0.5f, 1.0f);
	}

	return(TRUE);
}

LOGICAL obdrawUpdateDOT3ColorDirectional(OBJECT* lpobj)
{
	SINT32		i;
	LIGHT*		lplight;	// light pointer
	piVECTOR	lpES;		// light position in Eye space
	piVECTOR	lpOS;		// light position in Object space
	piVECTOR	lvTS;		// light vector in Tangent space
	UINT32		color;		// color is actually light vector

	piVertexColorPtr	pcolors;

	piMATRIX	modelViewMatrix;
	piMATRIX	modelViewMatrixInverse;

	static piMATRIX tangentMatrix = {	{1.0, 0.0, 0.0, 0.0,
										0.0,-1.0, 0.0, 0.0,
										0.0, 0.0, 1.0, 0.0,
 										0.0, 0.0, 0.0, 1.0}};

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&modelViewMatrix);
	matrixInvert(&modelViewMatrix, &modelViewMatrixInverse);

	// get light's position in eye space
	lplight = trnskyGetSunSource();
	lpES.x = -lplight->light.dvDirection.x;
	lpES.y = -lplight->light.dvDirection.y;
	lpES.z = -lplight->light.dvDirection.z;

	// convert from eye space to object space
	math3dMultiplyRotation(&lpOS, &lpES, &modelViewMatrixInverse);

	// rotate into tangent space
	math3dMultiplyRotation(&lvTS, &lpOS, &tangentMatrix);

	color = piRGBA((lvTS.x * 0.5f) + 0.5f, (lvTS.y * 0.5f) + 0.5f, (lvTS.z * 0.5f) + 0.5f, 1.0f);

	pcolors = lpobj->MutableColors();

	for (i=0; i < lpobj->numverts; i++)
	{
		pcolors[i].color = color;
	}

	return(TRUE);
}

LOGICAL obdrawApplyBumpmapTextureStage(OBJECT* lpobj)
{
	SINT32				texid, offset, stage;
	piTexCoordConstPtr	ptu;
	piCOLORVALUE		ambient;

    return TRUE;
	if (!MultiTextureEnable || bTexturesDisabled)
	{
		return TRUE;
	}

	ptu = lpobj->ImmutableTexCoords();

	texid = lpobj->texid;
	if (!obdrawFindMultiTextureById(texid, &offset))
	{
		return(FALSE);
	}

	// The ambient material takes on a very special roll when we use bump mapping
	// In order to get the lighting to look natural the ambient color is set to
	// represent a normal vector halved and normalized to between 0.0 and 1.0
	// r = x, g = y, b = z
	ambient.r = ambient.g = 0.5f;
	ambient.a = ambient.b = 1.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, (GLfloat*)&ambient);

	piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);

	// Update the color elements with a direction vector
	obdrawUpdateDOT3ColorDirectional(lpobj);

	stage = 0;
	oglSetActiveTextureStage(stage);
	piSetActiveTexture(MultiTextureInfo[offset].MultiTextureId, stage);

	piSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);
	piSetRenderStageState(stage, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);

	piSetRenderStageState(stage, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);

	piSetRenderStageState(stage, piTSS_COMBINE_RGB, piTSS_DOT3_RGB);
	piSetRenderStageState(stage, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(stage, piTSS_SOURCE1_RGB, piTSS_PRIMARY_COLOR);
	piSetRenderStageState(stage, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(stage, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

	stage += 1;

	if (lpobj->flags & OBJECT_TERRAIN)
	{
		ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &lpobj->ptverts->tu);
	}
	else
	{
		ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &ptu->u);
	}

	oglSetActiveTextureStage(stage);
	piSetActiveTexture(texid, stage);

	piSetTextureMatrix(&IdentityMatrix);//MultiTextureInfo[offset].MultiTextureMatrix);
	piSetRenderStageState(stage, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);

	piSetRenderStageState(stage, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);

	piSetRenderStageState(stage, piTSS_COMBINE_RGB, piTSS_MODULATE);
	piSetRenderStageState(stage, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(stage, piTSS_SOURCE1_RGB, piTSS_PREVIOUS);
	piSetRenderStageState(stage, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(stage, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

	stage += 1;
	texid = MultiTextureInfo[offset].MultiTextureId;
	while (stage < gMaxTextureUnits && obdrawFindMultiTextureById(texid, &offset))
	{
		oglSetActiveTextureStage(stage);

		piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

		piSetRenderStageState(stage, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);

		piSetActiveTexture(MultiTextureInfo[offset].MultiTextureId, stage);
		piSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);

		if (lpobj->flags & OBJECT_TERRAIN)
		{
			ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &lpobj->ptverts->tu);
		}
		else
		{
			ogl::SetTextureUnitCoordPointer(GL_TEXTURE0+stage, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &ptu->u);
		}

		piSetRenderStageState(stage, piTSS_COMBINE_RGB, piTSS_MODULATE);
		piSetRenderStageState(stage, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
		piSetRenderStageState(stage, piTSS_SOURCE1_RGB, piTSS_PREVIOUS);
		piSetRenderStageState(stage, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
		piSetRenderStageState(stage, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

		stage += 1;
		texid = MultiTextureInfo[offset].MultiTextureId;
	}

	if (stage < gMaxTextureUnits)
	{
		oglSetActiveTextureStage(stage);
		piDisableTexture();
	}

	return(TRUE);
}

LOGICAL obdrawReleaseMultiTextureStage(OBJECT* lpobj)
{
		oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);

    for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
		oglSetActiveTextureStage(i);
		oglSetTextureMatrix(&IdentityMatrix);
		oglDisableTexture();
	}

    oglSetActiveTextureStage(0);

    piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_MODULATE);
	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);

	return(TRUE);
}

LOGICAL obdrawReleaseBumpmapTextureStage(OBJECT* lpobj)
{
SINT32			texid, offset;

	if (!MultiTextureEnable || bTexturesDisabled)
	{
		return TRUE;
	}

	texid = lpobj->texid;
	if (!obdrawFindMultiTextureById(texid, &offset))
	{
		return(FALSE);
	}

	piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

	piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_MODULATE);

	oglSetActiveTextureStage(0);
	piDisableTexture();
	piSetTextureMatrix(&IdentityMatrix);


	oglSetActiveTextureStage(1);
	piDisableTexture();
	piSetTextureMatrix(&IdentityMatrix);

	return(TRUE);
}

Texture2D* obdrawGetWaterReflectionTexture()
{
    return _reflectionRTT.get();
}

Texture2D* obdrawGetSurfaceTexture()
{
    return _surfaceRTT.get();
}

Texture2D* obdrawGetWaterRefractionTexture()
{
    return _refractionRTT.get();
}

static bool _initSurfaceFBO()
{
    _surfaceRTT = new Texture2D;

    _surfaceRTT->setSourceFormat(GL_RGBA);
    _surfaceRTT->setSourceType(GL_UNSIGNED_BYTE);
    _surfaceRTT->setInternalFormat(GL_RGBA8);
    _surfaceRTT->setTextureWidth(SURFACE_FBO_SIZE);
    _surfaceRTT->setTextureHeight(SURFACE_FBO_SIZE);

    _surfaceRTT->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _surfaceRTT->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
    _surfaceRTT->setWrap(Texture::WRAP_S, Texture::CLAMP);
    _surfaceRTT->setWrap(Texture::WRAP_T, Texture::CLAMP);

    _surfaceFBO = new FrameBufferObject;
    ref_ptr<FrameBufferAttachment> fba = new FrameBufferAttachment(_surfaceRTT.get());
    _surfaceFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba.get());

    RenderBuffer* rb = new RenderBuffer(SURFACE_FBO_SIZE, SURFACE_FBO_SIZE, GL_DEPTH_COMPONENT24);
    ref_ptr<FrameBufferAttachment> fba2 = new FrameBufferAttachment(rb);
    _surfaceFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2.get());

    return true;
}

static bool _initWaterReflectionFBO()
{
    _reflectionRTT = new Texture2D;

    _reflectionRTT->setSourceFormat(GL_RGBA);
    _reflectionRTT->setSourceType(GL_UNSIGNED_BYTE);
    _reflectionRTT->setInternalFormat(GL_RGBA8);
    _reflectionRTT->setTextureWidth(256);
    _reflectionRTT->setTextureHeight(256);

    _reflectionRTT->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _reflectionRTT->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
    _reflectionRTT->setWrap(Texture::WRAP_S, Texture::CLAMP);
    _reflectionRTT->setWrap(Texture::WRAP_T, Texture::CLAMP);

    _reflectionFBO = new FrameBufferObject;
    ref_ptr<FrameBufferAttachment> fba = new FrameBufferAttachment(_reflectionRTT.get());
    _reflectionFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba.get());

    ref_ptr<RenderBuffer> rb = new RenderBuffer(256, 256, GL_DEPTH_COMPONENT24);
    ref_ptr<FrameBufferAttachment> fba2 = new FrameBufferAttachment(rb.get());
    _reflectionFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2.get());

    return true;
}

#if(0)
static bool _initWaterRefractionFBO()
{
    _refractionRTT = new Texture2D;

    _refractionRTT->setSourceFormat(GL_RGBA);
    _refractionRTT->setSourceType(GL_UNSIGNED_BYTE);
    _refractionRTT->setInternalFormat(GL_RGBA8);
    _refractionRTT->setTextureWidth(256);
    _refractionRTT->setTextureHeight(256);

    _refractionRTT->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _refractionRTT->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
    _refractionRTT->setWrap(Texture::WRAP_S, Texture::CLAMP);
    _refractionRTT->setWrap(Texture::WRAP_T, Texture::CLAMP);

    _refractionFBO = new FrameBufferObject;
    FrameBufferAttachment* fba = new FrameBufferAttachment(_refractionRTT.get());
    _refractionFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba);

    RenderBuffer* rb = new RenderBuffer(256, 256, GL_DEPTH_COMPONENT24);
    FrameBufferAttachment* fba2 = new FrameBufferAttachment(rb);
    _refractionFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2);

    return true;
}
#endif

Texture* obdrawGetSceneTexture()
{
    return _sceneRTT.get();
}

Texture* obdrawGetSceneDepthTexture()
{
    return _sceneDEPTH.get();
}

static bool _initSceneFBO()
{
    _sceneFBO = new FrameBufferObject;

    if(oglCanUseTexture2D(WantedWidth, WantedHeight))
    {
        Texture2D* tex = new Texture2D;

        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_BYTE);
        tex->setInternalFormat(GL_RGBA);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setTextureHeight(WantedHeight);
        tex->setTextureWidth(WantedWidth);

        _sceneRTT = tex;

        tex = new Texture2D;

        tex->setTextureSize(WantedWidth, WantedHeight);
        tex->setInternalFormat(GL_DEPTH_COMPONENT32F);
        tex->setSourceFormat(GL_DEPTH_COMPONENT);
        tex->setSourceType(GL_FLOAT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_BORDER);
        tex->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_BORDER);

        _sceneDEPTH = tex;

        if(!multisample)
        {
            FrameBufferAttachment* fba = new FrameBufferAttachment(dynamic_cast<Texture2D*>(_sceneRTT.get()));
            _sceneFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba);

            FrameBufferAttachment* fba2 = new FrameBufferAttachment(dynamic_cast<Texture2D*>(_sceneDEPTH.get()));
            _sceneFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2);
        }
        else
        {
            _resolveSceneFBO = new FrameBufferObject;
            FrameBufferAttachment* att1 = new FrameBufferAttachment(dynamic_cast<Texture2D*>(_sceneRTT.get()));
            _resolveSceneFBO->setAttachment(FrameBufferObject::COLOR_BUFFER, *att1);

            FrameBufferAttachment* att2 = new FrameBufferAttachment(dynamic_cast<Texture2D*>(_sceneDEPTH.get()));
            _resolveSceneFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *att2);
        }
    }
    // Default to texture rectangle (non-normalized)
    else if(GLEW_ARB_texture_rectangle)
    {
        TextureRectangle* tex = new TextureRectangle;

        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_BYTE);
        tex->setInternalFormat(GL_RGBA8);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setTextureHeight(WantedHeight);
        tex->setTextureWidth(WantedWidth);

        _sceneRTT = tex;

        tex = new TextureRectangle;

        tex->setTextureSize(WantedWidth, WantedHeight);
        tex->setInternalFormat(GL_DEPTH_COMPONENT32F);
        tex->setSourceFormat(GL_DEPTH_COMPONENT);
        tex->setSourceType(GL_FLOAT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_BORDER);
        tex->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_BORDER);

        _sceneDEPTH = tex;

        if(!multisample)
        {
             FrameBufferAttachment* fba = new FrameBufferAttachment(dynamic_cast<TextureRectangle*>(_sceneRTT.get()));
            _sceneFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba);

            FrameBufferAttachment* fba2 = new FrameBufferAttachment(dynamic_cast<TextureRectangle*>(_sceneDEPTH.get()));
            _sceneFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2);
        }
        else
        {
            _resolveSceneFBO = new FrameBufferObject;
            FrameBufferAttachment* att1= new FrameBufferAttachment(dynamic_cast<TextureRectangle*>(_sceneRTT.get()));
            _resolveSceneFBO->setAttachment(FrameBufferObject::COLOR_BUFFER, *att1);

            FrameBufferAttachment* att2 = new FrameBufferAttachment(dynamic_cast<TextureRectangle*>(_sceneDEPTH.get()));
            _resolveSceneFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *att2);
        }
    }

    if(multisample)
    {
        int samples = 16;

        RenderBuffer* rb1 = new RenderBuffer(WantedWidth, WantedHeight, GL_RGBA, samples);
        FrameBufferAttachment* fba1 = new FrameBufferAttachment(rb1);
        _sceneFBO->setAttachment(FrameBufferObject::COLOR_BUFFER, *fba1);

        RenderBuffer* rb2 = new RenderBuffer(WantedWidth, WantedHeight, GL_DEPTH_COMPONENT32F, samples);
        FrameBufferAttachment* fba2 = new FrameBufferAttachment(rb2);
        _sceneFBO->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba2);
    }

    _sceneFBO->apply();
    oglClearTargets(oglCLEAR_VIEWPORT | oglCLEAR_ZBUFFER);
    _sceneFBO->UnApply();

    if(_resolveSceneFBO.valid())
    {
        _resolveSceneFBO->apply();
        oglClearTargets(oglCLEAR_VIEWPORT);
        _resolveSceneFBO->UnApply();
    }

    return true;
}

LOGICAL obdrawInit()
{
    if (!obcsInit())
	{
		return(FALSE);
	}

	if (!InitMultiTexture())
	{
		return(FALSE);
	}

    multisample = GLEW_EXT_framebuffer_multisample && GLEW_EXT_framebuffer_blit;
    WriteLog(LOG_RENDER, LOG_DEBUG, "Multisample FBO %s ", multisample ? "ENABLED" : "DISABLED");

    obdrawSetRenderPass(RENDER_SCENE);

	_UnsortedExternalDrawCallback = nullptr;
	_SortedExternalDrawCallback = nullptr;
	_UnsortedInternalDrawCallback = nullptr;
	_SortedInternalDrawCallback = nullptr;

    _initWaterReflectionFBO();
    _initSurfaceFBO();

#if USE_STATESET
    _initSceneFBO();

    depthpass = new RenderStateSet;
    depthpass->SetRenderState(new ColorMask(false, false, false, false));
    depthpass->SetRenderState(new Program("opaquedepth"));
    depthpass->SetRenderState(new ShadeModel(GL_FLAT));
    depthpass->SetMode(GL_LIGHTING, 0);
    depthpass->SetMode(GL_FOG, 0);
#else
    if(!ogl::UseDeprecatedPath())
        _initSceneFBO();
#endif

    return(TRUE);
}

void obdrawKill()
{
    if(_surfaceFBO.valid())
    {
        while(_surfaceFBO->referenceCount() > 1)
            _surfaceFBO->unref();

        _surfaceFBO = nullptr;
    }

    if(_refractionFBO.valid())
    {
        while(_refractionFBO->referenceCount() > 1)
            _refractionFBO->unref();

        _refractionFBO = nullptr;
    }

    if(_reflectionFBO.valid())
    {
        while(_reflectionFBO->referenceCount() > 1)
            _reflectionFBO->unref();

        _reflectionFBO = nullptr;
    }

	if (_reflectionRTT.valid())
	{
		while (_reflectionRTT->referenceCount() > 1)
			_reflectionRTT->unref();
	}

	if (_surfaceRTT.valid())
	{
		while (_surfaceRTT->referenceCount() > 1)
			_surfaceRTT->unref();
	}

    _surfaceRTT = nullptr;
    _reflectionRTT = nullptr;

    KillMultiTexture();
	obcsKill();
    ShadowMap::GetInstance().Kill();
    FrameBufferKill();
}

#if USE_STATESET
static void RenderSurfaceStuff()
{
    static ref_ptr<ClearNode> clear;

    if(!clear.valid())
    {
        clear = new ClearNode(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, true, Vec4f(0, 0, 0, 1));
    }

    _surfaceFBO->apply();
    oglSetViewport(0, 0, SURFACE_FBO_SIZE, SURFACE_FBO_SIZE);

    clear->apply();

    if(ogl::UseDeprecatedPath())
        DrawSurfaceFxDeprecated();
    else
        DrawSurfaceFx();

    oglSetViewport(0, 0, piGetWidth(), piGetHeight());
    _surfaceFBO->UnApply();
}
#else
static void RenderSurfaceStuff()
{
    oglSetActiveTextureStage(0);

    oglSetViewport(0, 0, SURFACE_FBO_SIZE, SURFACE_FBO_SIZE);
    _surfaceFBO->apply();

    oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, piRGBA(0, 0, 0, 1));
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);

    piClearTargets(piCLEAR_ZBUFFER | piCLEAR_VIEWPORT);

    if(ogl::UseDeprecatedPath())
        DrawSurfaceFxDeprecated();
    else
        DrawSurfaceFx();

    _surfaceFBO->UnApply();
    oglSetViewport(0, 0, piGetWidth(), piGetHeight());

    oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, FogColor);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
}
#endif

static void RenderWaterReflectionsToFBO()
{
    // Setup the render texture
    // Viewport has to be same size
    oglSetViewport(0, 0, 256, 256);
    _reflectionFBO->apply();

    piClearTargets(piCLEAR_ZBUFFER | piCLEAR_VIEWPORT);

    oglSetRenderState(oglRENDERSTATE_CULLMODE, GL_CW);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);

    // Make sure no one changes this during the render process
    oglLockState(oglRENDERSTATE_CULLMODE, true);

    obdrawSetRenderPass(RENDER_REFLECTION);

    piMATRIX oldview = *oglGetViewMatrix();
    oglSetViewMatrix(oglGetWaterReflectionCamera());

    logTimerCallVoid(trnskyDraw);
    logTimerCallVoid(RenderOpaqueBin);
    logTimerCallVoid(RenderMultiTexture);
    logTimerCallVoid(sptDraw);
    logTimerCallVoid(glslNormalDrawBin);
    logTimerCallVoid(glslSpecularDrawBin);
    //logTimerCallVoid(gr2DrawTrooperBin);

    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    logTimerCallVoid(RenderTransparentBin);

    oglSetViewMatrix(&oldview);

    obdrawSetRenderPass(RENDER_SCENE);

    // Done here. Reset
    oglLockState(piRENDERSTATE_CULLMODE, false);

    oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);

    _reflectionFBO->UnApply();
    oglSetViewport(0, 0, piGetWidth(), piGetHeight());
}

#if USE_STATESET
static void RenderWaterReflectionsToFBODeprecated()
{
    static ref_ptr<ClearNode> clear;
    static ref_ptr<RenderStateSet> stateset;

    if(!clear.valid())
    {
        clear = new ClearNode(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT, true, Vec4f(0, 0, 0, 1));
        stateset = new RenderStateSet;

        stateset->SetRenderState(new FrontFace(GL_CW));
        stateset->SetRenderState(new ShadeModel(GL_FLAT));
        stateset->SetRenderState(_reflectionFBO.get());
    }

    // Any water drawn?
    if(glslWaterCanReflect())
    {
        stateset->Apply();

        // Setup the render texture
        // Viewport has to be same size
        oglSetViewport(0, 0, 256, 256);
        clear->apply();

        bool normalhistory = g_wantNormalMaps;
        bool specularhistory = WantSpecularMaps;

        g_wantNormalMaps = false;
        WantSpecularMaps = false;

        // Keep states locked until finished
        States::LockState(RS_FRAMEBUFFEROBJECT, true);
        States::LockState(RS_SHADEMODEL, true);
        States::LockState(RS_FRONTFACE, true);

        obdrawSetRenderPass(RENDER_REFLECTION);

        piMATRIX oldview = *oglGetViewMatrix();
        oglSetViewMatrix(oglGetWaterReflectionCamera());

        logTimerCallVoid(trnskyDrawDeprecated);
        logTimerCallVoid(_drawUnsorted);
        logTimerCallVoid(_drawMultiTexture);
        logTimerCallVoid(sptDisplayDeprecated);

        logTimerCallVoid(_drawSorted);

        States::LockState(RS_FRAMEBUFFEROBJECT, false);
        States::LockState(RS_SHADEMODEL, false);
        States::LockState(RS_FRONTFACE, false);

        oglSetViewMatrix(&oldview);

        g_wantNormalMaps = normalhistory;
        WantSpecularMaps = specularhistory;

        obdrawSetRenderPass(RENDER_SCENE);

        oglSetViewport(0, 0, piGetWidth(), piGetHeight());
    }
}
#else
static void RenderWaterReflectionsToFBODeprecated()
{
    // Setup the render texture
    // Viewport has to be same size
    oglSetViewport(0, 0, 256, 256);
    _reflectionFBO->apply();

    piClearTargets(piCLEAR_ZBUFFER | piCLEAR_VIEWPORT);

    // Any water drawn?
    if(!glslWaterCanReflect())
    {
        _reflectionFBO->UnApply();
        oglSetViewport(0, 0, piGetWidth(), piGetHeight());
        return;
    }

    oglSetRenderState(oglRENDERSTATE_CULLMODE, GL_CW);

    // Make sure no one changes this during the render process
    oglLockState(oglRENDERSTATE_CULLMODE, true);

    bool normalhistory = g_wantNormalMaps;
    g_wantNormalMaps = false;

    bool specularhistory = WantSpecularMaps;
    WantSpecularMaps = false;

    obdrawSetRenderPass(RENDER_REFLECTION);

    piMATRIX oldview = *oglGetViewMatrix();
    oglSetViewMatrix(oglGetWaterReflectionCamera());

    trnskyDrawDeprecated();

    oglSetRenderState(oglRENDERSTATE_SHADEMODE, GL_FLAT);
    oglLockState(oglRENDERSTATE_SHADEMODE, true);

    logTimerCallVoid(_drawUnsorted);
    logTimerCallVoid(_drawMultiTexture);
    logTimerCallVoid(sptDisplayDeprecated);
    logTimerCallVoid(_drawSorted);

    obdrawSetRenderPass(RENDER_SCENE);

    // Done here. Reset
    oglLockState(piRENDERSTATE_CULLMODE, false);
    oglLockState(oglRENDERSTATE_SHADEMODE, false);

    oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, ShadeMode);

    g_wantNormalMaps = normalhistory;
    WantSpecularMaps = specularhistory;

    oglSetViewMatrix(&oldview);

    _reflectionFBO->UnApply();
    oglSetViewport(0, 0, piGetWidth(), piGetHeight());
}
#endif

void _renderWaterRefractions()
{
#if(0)
    extern void obcsCullForWaterRefraction();

    // Setup the render texture
    // Viewport has to be same size
    oglSetViewport(0, 0, 256, 256);
    _refractionFBO->apply();

    piClearTargets(piCLEAR_ZBUFFER | piCLEAR_VIEWPORT);

    bool normalhistory = g_wantNormalMaps;
    g_wantNormalMaps = false;

    obdrawSetRenderPass(RENDER_REFRACTION);

    // Draw objects to render texture
    obdrawDrawWaterRefraction();

    obdrawSetRenderPass(RENDER_SCENE);

    g_wantNormalMaps = normalhistory;

    glBindFramebufferEXT(FrameBufferObject::READ_DRAW_FRAMEBUFFER, 0);
    oglSetViewport(0, 0, piGetWidth(), piGetHeight());
#endif
}

static void ApplyMultiTexture(OBJECT* o)
{
	int stage = 1;
    int texid = o->texid;
    int offset = 0;

	while(stage < gMaxTextureUnits && obdrawFindMultiTextureById(texid, &offset))
	{
		oglSetActiveTextureStage(stage);
        oglSetTextureMatrix(&MultiTextureInfo[offset].MultiTextureMatrix);

		stage++;
		texid = MultiTextureInfo[offset].MultiTextureId;
	}

	oglSetActiveTextureStage(0);
	oglSetTextureMatrix(&IdentityMatrix);
}

/*------------------------------------------------------------------------
Function: AddDrawInstance
Description:
-------------------------------------------------------------------------*/
static void AddDrawInstance(const OBJECT* o, bool push = false)
{
    logTimerBeginMacro();

	static std::vector<Vec4f> data;
    static ref_ptr<oglTextureObject> tbo;

    if(!tbo.valid())
    {
        tbo = oglTextureObject::GetTextureObject("dynamicglobal");
    }

    static int primcount = 0;

    if(o && o->material)
    {
        if(push)
        {
            // Time to push objects to card.
            if(primcount)
            {
                tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0], 0);

                o->material->Apply();

                if(primcount > 1)
                    obdrawDrawObjectInstanced(o, 0, primcount);
                else
                    obdrawDrawObject(o);
            }

            primcount = 0;

			data.clear();
        }
        else
        {
            Quat q = Matrixf(o->mat).getRotate();

            Vec4f p(o->mat._41, o->mat._42, o->mat._43, o->level * colorONEOVER255);

            data.push_back(q.asVec4f());
            data.push_back(p);

            primcount++;

            if(primcount >= MAX_DRAW_PRIMITIVES)
            {
                tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0], 0);

                o->material->Apply();

				obdrawDrawObjectInstanced(o, 0, primcount);

                primcount = 0;

				data.clear();
            }
        }
    }

    logTimerEndMacro();
}

#if USE_STATESET
/*------------------------------------------------------------------------
Function: RenderMultiTexture
Description:
-------------------------------------------------------------------------*/
static void RenderMultiTexture()
{
#ifdef BETA_RELEASE
    if(!_multitexturedraw)
    {
        return;
    }
#endif

    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    std::vector<OBJECT*> bin;
    obcsGetMultiTextureBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    const RenderStateSet* old = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material)
        {
            o->material->Apply();
        }

        oglSetWorldMatrix(&o->mat);
        old = o->material;

        DrawObject(o, 3);

        last = o;
    }
}
#else
/*------------------------------------------------------------------------
Function: RenderMultiTexture
Description:
-------------------------------------------------------------------------*/
static void RenderMultiTexture()
{
#ifdef BETA_RELEASE
    if(!_multitexturedraw)
    {
        return;
    }
#endif

    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    std::vector<OBJECT*> bin;
    obcsGetMultiTextureBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    const oglMaterial* old = nullptr;

#if DEBUG_WW2
    int count = 0;
    int same = 0;
#endif

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material)
        {
            ApplyMultiTexture(o);
            o->material->Apply();
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        oglSetWorldMatrix(&o->mat);
        old = o->material;

        DrawObject(o, 3);

        last = o;

#if DEBUG_WW2
        count++;
#endif
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: RenderOpaqueBin
Description:
-------------------------------------------------------------------------*/
static void RenderOpaqueBin()
{
#ifdef BETA_RELEASE
    if(!_unsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    const RenderStateSet* old = nullptr;

    const unsigned int size = bin.size();

    oglSetWorldMatrix(&IdentityMatrix);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material || (last && last->pverts))
        {
            if(last)
            {
                AddDrawInstance(last, true);
            }
        }

        old = o->material;
        AddDrawInstance(o);
        last = o;
    }

    AddDrawInstance(last, true);
}
#else
/*------------------------------------------------------------------------
Function: RenderOpaqueBin
Description:
-------------------------------------------------------------------------*/
static void RenderOpaqueBin()
{
#ifdef BETA_RELEASE
    if(!_unsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

#if DEBUG_WW2
    int count = 0;
    int same = 0;
#endif

    const oglMaterial* old = nullptr;

    const unsigned int size = bin.size();

    oglSetWorldMatrix(&IdentityMatrix);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

#if DEBUG_WW2
        count++;
#endif

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material || (last && last->pverts))
        {
            if(last)
            {
                AddDrawInstance(last, true);
#if DEBUG_WW2
            same = 0;
#endif
            }
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->material;
        AddDrawInstance(o);
        last = o;
    }

    AddDrawInstance(last, true);
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: RenderTransparentBin
Description:
-------------------------------------------------------------------------*/
static void RenderTransparentBin()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentBin(bin);

    OBJECT* o = nullptr;

    const RenderStateSet* old = nullptr;
    OBJECT* last = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            if(DrawFxSystem(o->distsqr))
            {
                old = nullptr;
            }

            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(DrawFxSystem(o->distsqr))
        {
            old = nullptr;
        }

        if(o->material && old != o->material)
        {
            o->material->Apply();
        }

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }

    DrawFxSystem(-1);
}
#else
/*------------------------------------------------------------------------
Function: RenderTransparentBin
Description:
-------------------------------------------------------------------------*/
static void RenderTransparentBin()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentBin(bin);

    OBJECT* o = nullptr;

    //fxMgrDraw(true);

#ifdef DEBUG_WW2
    int count = 0;
    int same = 0;
#endif

    const oglMaterial* old = nullptr;
    OBJECT* last = nullptr;

    const unsigned int size = bin.size();
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

#if DEBUG_WW2
        count++;
#endif

        if(o->drawcallback)
        {
            if(DrawFxSystem(o->distsqr))
            {
                old = nullptr;
            }

            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(DrawFxSystem(o->distsqr))
        {
            old = nullptr;
        }

        if(o->material && old != o->material)
        {
            o->material->Apply();
#if DEBUG_WW2
            same = 0;
#endif
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }

    DrawFxSystem(-1);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: RenderOpaqueCockpitBin
Description:
-------------------------------------------------------------------------*/
static void RenderOpaqueCockpitBin()
{
#ifdef BETA_RELEASE
    if(!_cockpitunsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueCockpitBin(bin);

    const RenderStateSet* old = nullptr;
    OBJECT* last = nullptr;
    OBJECT* o;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material || (last && last->pverts))
        {
            o->material->Apply();
        }

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }
}
#else
/*------------------------------------------------------------------------
Function: RenderOpaqueCockpitBin
Description:
-------------------------------------------------------------------------*/
static void RenderOpaqueCockpitBin()
{
#ifdef BETA_RELEASE
    if(!_cockpitunsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueCockpitBin(bin);

#if DEBUG_WW2
    int same = 0;
#endif

    const oglMaterial* old = nullptr;
    OBJECT* last = nullptr;
    OBJECT* o;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material || (last && last->pverts))
        {
            o->material->Apply();
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: RenderTransparentCockpitBin
Description:
-------------------------------------------------------------------------*/
static void RenderTransparentCockpitBin()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentCockpitBin(bin);

    OBJECT* o = nullptr;

    const RenderStateSet* old = nullptr;
    OBJECT* last = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material)
        {
            o->material->Apply();
        }

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }
}
#else
/*------------------------------------------------------------------------
Function: RenderTransparentCockpitBin
Description:
-------------------------------------------------------------------------*/
static void RenderTransparentCockpitBin()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentCockpitBin(bin);

    OBJECT* o = nullptr;

#ifdef DEBUG_WW2
    int same = 0;
#endif

    const oglMaterial* old = nullptr;
    OBJECT* last = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(old != o->material)
        {
            o->material->Apply();
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->material;
        oglSetWorldMatrix(&o->mat);
        DrawObject(o);

        last = o;
    }
}
#endif

/*------------------------------------------------------------------------
Function: RenderDepth
Description:
-------------------------------------------------------------------------*/
static void RenderDepth()
{
#if(0)
    logTimerBeginMacro();

    oglSetRenderState(oglRENDERSTATE_COLORMASK, 0);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 1);

    oglLockState(oglRENDERSTATE_COLORMASK, 1);
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 1);

    logTimerCallVoid(trnskyPreDrawFlare);
    logTimerCallVoid(trnskyReadControlPixels);

    logTimerCallVoid(trnmainDraw);
    logTimerCallVoid(RenderOpaqueBin);
    logTimerCallVoid(RenderMultiTexture);
    logTimerCallVoid(sptDraw);
    logTimerCallVoid(speedgrassDraw);
    logTimerCallVoid(glslNormalDrawBin);
    logTimerCallVoid(glslSpecularDrawBin);
    logTimerCallVoid(glslWaterDrawBin);
    logTimerCallVoid(gr2AnimDrawCorpses);
    logTimerCallVoid(gr2DrawTrooperBin);

    oglLockState(oglRENDERSTATE_COLORMASK, 0);
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 0);

    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));

    logTimerEndMacro();
#endif
}

/*------------------------------------------------------------------------
Function: Render
Description: Non-deprecated render path
-------------------------------------------------------------------------*/
static void Render()
{
#if(0)
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 1);
#endif
    if(g_postRenderFilter && _sceneFBO.valid())
    {
        _sceneFBO->apply();
        oglClearTargets(piCLEAR_ZBUFFER);
    }

    logTimerCallVoid(trnskyDraw);
    logTimerCallVoid(trnskyPreDrawFlare);
    logTimerCallVoid(trnskyReadControlPixels);

    logTimerCallVoid(fxLgtSetLights);

    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 1);

    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 1);
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 1);

    logTimerCallVoid(trnmainDraw);
    logTimerCallVoid(RenderOpaqueBin);
    logTimerCallVoid(RenderMultiTexture);
    logTimerCallVoid(sptDraw);
    logTimerCallVoid(speedgrassDraw);
    logTimerCallVoid(glslNormalDrawBin);
    logTimerCallVoid(glslSpecularDrawBin);
    logTimerCallVoid(glslWaterDrawBin);
    logTimerCallVoid(gr2AnimDrawCorpses);
    //logTimerCallVoid(gr2DrawTrooperBin);

    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 1);

    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 1);

    logTimerCallVoid(vehIconDraw);
    logTimerCallVoid(RenderTransparentBin);

    logTimerCallVoid(mlDrawProps);

    logTimerCallVoid(trnskyAdjustBrightness);

    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 1);

    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);

    oglSetFieldOfView(FieldOfView, 0.02f, 500.0f);
    oglClearTargets(piCLEAR_ZBUFFER);

    logTimerCallVoid(RenderOpaqueCockpitBin);
    //logTimerCallVoid(gr2DrawTrooperBin);

    oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

    logTimerCallVoid(RenderTransparentCockpitBin);
    logTimerCallVoid(persDrawGunsight);

    if(g_postRenderFilter && _sceneFBO.valid())
        _sceneFBO->UnApply();

    logTimerCallVoid(trnskyDrawFlare);

#if(0)
    oglLockState(oglRENDERSTATE_ZWRITEENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
#endif
}

/*------------------------------------------------------------------------
Function: obdrawObjects
Description:
-------------------------------------------------------------------------*/
void obdrawObjects()
{
    LISTHEADER* list = nullptr;

    if(interiorMaterial == -1)
        interiorMaterial = piFindMaterialName("interior");

    if(transglas == -1)
        transglas = piFindMaterialName("transglas");

    if(propclear == -1)
        propclear = piFindMaterialName("prop clear");

    if(perspex == -1)
        perspex = piFindMaterialName("perspex");

    if(blackmatte == -1)
        blackmatte = piFindMaterialName("blackmatte");

    static int numobjects = 0;

	numobjects = obmainGetObjects(&list);
	OBJECT* o = (OBJECT*)list->next;

    obcsResetSortedCockpitList();
    obcsResetUnsortedCockpitList();

    // Must be first...
    numobjects = obcsCullAndSortObjects(numobjects, o);
    logTimerCallVoid(sptCull);

    if(g_wantWaterReflections && gWaterShadersEnabled)
    {
        RenderWaterReflectionsToFBO();
    }

    logTimerCallVoid(CullFxSystem);
    logTimerCallVoid(speedgrassCull);
    logTimerCallVoid(RenderSurfaceStuff);

    glslWaterReset();

    logTimerCallVoid(RenderDepth);
    logTimerCallVoid(Render);
}

void obdrawCull()
{
    LISTHEADER* list;

	int numobjects = obmainGetObjects(&list);
	OBJECT* o = (OBJECT*)list->next;

    // reset the lists
    obcsResetSortedList();
    obcsResetUnsortedList();
    obcsResetMultiTexture();
    obcsResetSortedCockpitList();
    obcsResetUnsortedCockpitList();

    if(gCustomTerrain)
        ShadowMap::GetInstance().setupCull();

    // Must be first...
    numobjects = obcsCullAndSortObjects(numobjects, o);

    if(gCustomTerrain)
        logTimerCallVoid(sptCull);

    CullFxSystem();
    speedgrassCull();
}

#if USE_STATESET
LOGICAL obdrawObjectsDeprecated()
{
    if(interiorMaterial == -1)
        interiorMaterial = piFindMaterialName("interior");

    if(transglas == -1)
        transglas = piFindMaterialName("transglas");

    if(propclear == -1)
        propclear = piFindMaterialName("prop clear");

    if(propclear2 == -1)
        propclear2 = piFindMaterialName("prop clear "); //<-- space at end

    if(perspex == -1)
        perspex = piFindMaterialName("perspex");

    if(blackmatte == -1)
        blackmatte = piFindMaterialName("blackmatte");

    if(g_wantWaterReflections && gWaterShadersEnabled)
    {
        RenderWaterReflectionsToFBODeprecated();
    }

    if(gWaterShadersEnabled)
        glslWaterDrawBinDeprecated(true);

    RenderSurfaceStuff();
    glslWaterReset();
    obdliteDynaLiteUpdate();
    _drawDeprecatedPath();

	return TRUE;
}
#else
LOGICAL obdrawObjectsDeprecated()
{
    LOGICAL retval = TRUE;

    if(interiorMaterial == -1)
        interiorMaterial = piFindMaterialName("interior");

    if(transglas == -1)
        transglas = piFindMaterialName("transglas");

    if(propclear == -1)
        propclear = piFindMaterialName("prop clear");

    if(propclear2 == -1)
        propclear2 = piFindMaterialName("prop clear "); //<-- space at end

    if(perspex == -1)
        perspex = piFindMaterialName("perspex");

    if(blackmatte == -1)
        blackmatte = piFindMaterialName("blackmatte");

    if(g_wantWaterReflections && gWaterShadersEnabled)
    {
        RenderWaterReflectionsToFBODeprecated();
    }

    RenderSurfaceStuff();

    glslWaterReset();

    if (!obdliteDynaLiteUpdate())
    {
        retval = FALSE;
    }

    _drawDeprecatedPath();

	return(retval);
}
#endif

static inline void _setStates(OBJECT* o)
{
    if(
#ifndef USE_3DAPI_LIGHTING
        o->flag2 & OBJECT_ANIMATION ||
#endif
        !(o->flags & OBJECT_LITE) || !(o->flags & OBJECT_DYNALITE))
    {
        piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    }

    if(o->flags & OBJECT_NOBACKFACE)
    {
        piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);
    }

    if(o->flag2 & OBJECT_ZDISABLE)
    {
        piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_ALWAYS);
    }

    if(o->flags & OBJECT_ZWRITEOFF || o->flags & OBJECT_DYNASHADOW)
    {
        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
    }

    if(o->flags & OBJECT_NOFILTER)
    {
        piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
        piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);
        piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);

        if(MultiTextureEnable)
        {
            piSetRenderStageState(1, piTSS_MINFILTER, piTFN_POINT);
            piSetRenderStageState(1, piTSS_MAGFILTER, piTFG_POINT);
            piSetRenderStageState(1, piTSS_MIPFILTER, piTFP_NONE);
        }
    }

    if(o->flag2 & OBJECT_BLENDADDITIVE)
    {
        piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
    }

    if(o->flag2 & OBJECT_NOFOG)
	{
        piSetRenderState(piRENDERSTATE_FOGENABLE, FALSE);
	}

#if(0)
    switch(o->gfxmodes & GRAPHICS_TAM_MASK)
    {
    case GRAPHICS_TAM_TUV_WRAP:
        // Default value, so no need to set
        break;
    case GRAPHICS_TAM_TUV_CLAMP:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_CLAMP);
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_CLAMP);
        break;
    case GRAPHICS_TAM_TUV_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_MIRROR);
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_MIRROR);
        break;
    case GRAPHICS_TAM_TU_WRAP:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_WRAP);
        break;
    case GRAPHICS_TAM_TU_CLAMP:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_CLAMP);
        break;
    case GRAPHICS_TAM_TU_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_MIRROR);
        break;
    case GRAPHICS_TAM_TV_WRAP:
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_WRAP);
        break;
    case GRAPHICS_TAM_TV_CLAMP:
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_CLAMP);
        break;
    case GRAPHICS_TAM_TV_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_MIRROR);
        break;
    }
#endif
}

static inline void _unsetStates(OBJECT* o)
{
    if(!o)
        return;

#if(0)
    // Graphics modes reset to default
    switch (o->gfxmodes & GRAPHICS_TAM_MASK)
    {
    case GRAPHICS_TAM_TUV_WRAP:
        // Default value so no need to set
        break;
    case GRAPHICS_TAM_TUV_CLAMP:
    case GRAPHICS_TAM_TUV_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_WRAP);
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_WRAP);
        break;
    case GRAPHICS_TAM_TU_WRAP:
    case GRAPHICS_TAM_TU_CLAMP:
    case GRAPHICS_TAM_TU_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_WRAP);
        break;
    case GRAPHICS_TAM_TV_WRAP:
    case GRAPHICS_TAM_TV_CLAMP:
    case GRAPHICS_TAM_TV_MIRROR:
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_WRAP);
        break;
    }
#endif

    if(o->flags & OBJECT_ZWRITEOFF || o->flags & OBJECT_DYNASHADOW)
    {
        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
    }

    if(o->flag2 & OBJECT_ZDISABLE)
    {
        piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
    }

    if(o->flag2 & OBJECT_NOFOG)
    {
        piSetRenderState(piRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);
    }

    if(o->flag2 & OBJECT_BLENDADDITIVE)
    {
        piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
    }

    if(o->flags & OBJECT_NOFILTER)
    {
        piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
        piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
        piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

        if(MultiTextureEnable)
        {
            piSetRenderStageState(1, piTSS_MIPFILTER, TextureMipFilter);
            piSetRenderStageState(1, piTSS_MINFILTER, TextureMinFilter);
            piSetRenderStageState(1, piTSS_MAGFILTER, TextureMagFilter);
        }
	}

    if(
#ifndef USE_3DAPI_LIGHTING
        o->flag2 & OBJECT_ANIMATION ||
#endif
        !(o->flags & OBJECT_LITE) || !(o->flags & OBJECT_DYNALITE))
    {
        piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    }

    if(o->flags & OBJECT_NOBACKFACE)
    {
        piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
    }
}

#if USE_STATESET
inline void _drawObject(OBJECT* o)
{
    static OBJECT* oldObject = nullptr;
    static RenderStateSet* oldmaterial = nullptr;

    if(!o)
    {
        oldmaterial = nullptr;
        oldObject = nullptr;
        return;
    }

    if(o->drawcallback)
    {
        oldObject = o;

        bool success = o->drawcallback(o);

        if(success)
        {
            oldmaterial = o->material;

#ifdef BETA_RELEASE
            if(_drawboundingbox)
                obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
            return;
        }
        // else fall through and draw normally
    }

    if(!o->material)
        return;

    SINT32 matid = o->matid;
    Material* material = dynamic_cast<Material*>(o->material->GetRenderState(RS_MATERIAL));

    material->SetMaterialID(matid);

    if(o->flag3 & OBJECT_IS_INTERIOR)
    {
        if(matid == transglas ||
           matid == perspex ||
           matid == propclear ||
           matid == propclear2 ||
           matid == blackmatte)
        {
            // Do nothing
        }
        else if(!(o->flags & OBJECT_DYNASHADOW))
        {
            material->SetMaterialID(interiorMaterial);
        }
    }

    // Guess we can't do anything consistent.
    if(o->flags & OBJECT_DYNASHADOW && !(o->flag2 & OBJECT_ANIMATION))
    {
        oglSetWorldMatrix(&IdentityMatrix);
    }
    else
    {
	    oglSetWorldMatrix(&o->mat);
    }

    if(oldmaterial != o->material)
    {
        oldmaterial = o->material;
        o->material->Apply();
    }

    _drawVertices(GetType(o->flags), o);
}
#else
inline void _drawObject(OBJECT* o)
{
    static OBJECT* oldObject = nullptr;
    static oglMaterial* oldmaterial = nullptr;

    if(!o)
    {
        oldmaterial = nullptr;
        oldObject = nullptr;
        return;
    }

    if(o->drawcallback)
    {
        oldObject = o;

        bool success = o->drawcallback(o);

        if(success)
        {
            oldmaterial = o->material;

#ifdef BETA_RELEASE
            if(_drawboundingbox)
                obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
            return;
        }
        // else fall through and draw normally
    }

    if(!o->material)
        return;

    SINT32 matid = o->matid;
    o->material->SetMaterial(matid);

    if(o->flag3 & OBJECT_IS_INTERIOR)
    {
        if(matid == transglas ||
           matid == perspex ||
           matid == propclear ||
           matid == propclear2 ||
           matid == blackmatte)
        {
            // Do nothing
        }
        else if(!(o->flags & OBJECT_DYNASHADOW))
        {
            o->material->SetMaterial(interiorMaterial);
        }
    }

    if(o->material != oldmaterial || o->flags & OBJECT_DYNASHADOW)
    {
        // Only unset states if material has changed
        _unsetStates(oldObject);

        _setStates(o);

        o->material->Apply();

        oldObject = o;
    }

    // Guess we can't do anything consistent.
    if(o->flags & OBJECT_DYNASHADOW && !(o->flag2 & OBJECT_ANIMATION))
    {
        oglSetWorldMatrix(&IdentityMatrix);
    }
    else
    {
	    oglSetWorldMatrix(&o->mat);
    }

     _drawVertices(GetType(o->flags), o);

    if(o->flags & OBJECT_DYNASHADOW)
    {
        _unsetStates(o);
    }

    oldmaterial = o->material;
}
#endif

#if USE_STATESET
static inline void _drawUnsorted()
{
#ifdef BETA_RELEASE
    if(!_unsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;
    const RenderStateSet* old = nullptr;

    int same = 0;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o) continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(!o->material)
            continue;

        if(old != o->material)
        {
            int matid = o->matid;

            Material* m = dynamic_cast<Material*>(o->material->GetRenderState(RS_MATERIAL));

            m->SetMaterialID(matid);

            if(o->flag3 & OBJECT_IS_INTERIOR)
            {
                if(matid == transglas ||
                   matid == perspex ||
                   matid == propclear ||
                   matid == propclear2 ||
                   matid == blackmatte)
                {
                    // Do nothing
                }
                else if(!(o->flags & OBJECT_DYNASHADOW))
                {
                    m->SetMaterialID(interiorMaterial);
                }
            }

            old = o->material;
            o->material->Apply();
        }
        else
        {
            same++;
        }

        oglSetWorldMatrix(&o->mat);

        _drawVertices(GetType(o->flags), o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}

static inline void _drawUnsortedDepth()
{
#ifdef BETA_RELEASE
    if(!_unsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    if(bin.empty())
        return;

    depthpass->Apply();

    const unsigned int size = bin.size();
    OBJECT* o;

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(o)
        {
            if(o->drawcallback)
            {
                if(o->modelInstance && !strncmp(o->modelInstance->GetModelName(), "tracker", 7))
                    continue;
            }

            oglSetWorldMatrix(&o->mat);
            _drawVertices(GetType(o->flags), o);
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
static inline void _drawUnsorted()
{
#ifdef BETA_RELEASE
    if(!_unsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    if(bin.empty())
        return;

    // Reset change detection
    _drawObject(nullptr);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;
    SINT32 oldLevel = 0;

    const oglMaterial* old = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o) continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                old = o->material;

                continue;
            }
            // else fall through and draw normally
        }

        if(!o->material)
            continue;

        if(old != o->material)
        {
            // Only set alpha refs if changed.
            if(o->level == 0 && oldLevel > 0)
            {
                oldLevel = 0;

                // Reset
                piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
            }

            // Only set alpha refs if changed.
            if(o->level > 0 && o->level != oldLevel)
            {
                oldLevel = o->level;

                piSetRenderState(piRENDERSTATE_ALPHAREF, o->level);
            }

            _unsetStates(last);
            _setStates(o);

            int matid = o->matid;

            o->material->SetMaterial(matid);

            if(o->flag3 & OBJECT_IS_INTERIOR)
            {
                if(matid == transglas ||
                   matid == perspex ||
                   matid == propclear ||
                   matid == propclear2 ||
                   matid == blackmatte)
                {
                    // Do nothing
                }
                else if(!(o->flags & OBJECT_DYNASHADOW))
                {
                    o->material->SetMaterial(interiorMaterial);
                }
            }

            o->material->Apply();
        }

        oglSetWorldMatrix(&o->mat);
        old = o->material;

        _drawVertices(GetType(o->flags), o);

        last = o;
    }

    _unsetStates(last);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

    piSetWorldMatrix(&IdentityMatrix);
}
#endif

#if USE_STATESET
static inline void _drawUnsortedCockpitDepth()
{
#ifdef BETA_RELEASE
    if(!_cockpitunsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueCockpitBin(bin);
    _drawObject(nullptr);

    OBJECT* o = nullptr;

    const unsigned int size = bin.size();

    depthpass->Apply();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        oglSetWorldMatrix(&o->mat);
        _drawVertices(GetType(o->flags), o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}

static inline void _drawUnsortedCockpit()
{
#ifdef BETA_RELEASE
    if(!_cockpitunsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueCockpitBin(bin);
    _drawObject(nullptr);

    OBJECT* o = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        _drawObject(o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
static inline void _drawUnsortedCockpit()
{
#ifdef BETA_RELEASE
    if(!_cockpitunsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueCockpitBin(bin);

    // Reset change detection
    _drawObject(nullptr);

    piMATRIX wm = *piGetWorldMatrix();
    piMATRIX vm = *piGetViewMatrix();

    piSetViewMatrix(piGetCockpitViewMatrix());

    OBJECT* o = nullptr;
    SINT32 oldLevel = 0;

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

#if DEBUG_WW2
    int count = 0;
#endif

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        // Only set alpha refs if changed.
        if(o->level == 0 && oldLevel > 0)
        {
            oldLevel = 0;

            // Reset
            piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
        }

        // Only set alpha refs if changed.
        if(o->level > 0 && o->level != oldLevel)
        {
            oldLevel = o->level;

            piSetRenderState(piRENDERSTATE_ALPHAREF, o->level);
        }

         _drawObject(o);

#if DEBUG_WW2
        count++;
#endif
    }

    _unsetStates(o);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

    piSetViewMatrix(&vm);
    piSetWorldMatrix(&wm);
}
#endif

#if USE_STATESET
static inline void _drawSorted()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentBin(bin);

    OBJECT* o = nullptr;

    fxMgrDraw(true);

    _drawObject(nullptr);

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        // Draw all particles greater than this distance first.
        if(o->distsqr > 0 && !(o->flags & OBJECT_DYNASHADOW))
        {
            if(DrawFxSystemDeprecated(o->distsqr))
                _drawObject(nullptr);
        }

        _drawObject(o);
    }

    oglSetWorldMatrix(&IdentityMatrix);

    DrawFxSystemDeprecated(-1);
}
#else
static inline void _drawSorted()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

#if(1)
    extern void obcsGetTransparentBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentBin(bin);
#else
     SORTHEADER* ul = obcsGetSortedList();
#endif

    // Reset change detection
    _drawObject(nullptr);

    OBJECT* o = nullptr;

    fxMgrDraw(true);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

    // This is wrong. Transparent objects should not be written to the z buffer. But to
    // get those FLTs fixed with the correct flags is not worth having to put up with
    // all the f'ing whining.
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, 1);

#ifdef DEBUG_WW2
    int count = 0;
#endif

#if(1)
    const unsigned int size = bin.size();

    TextureState::Start();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];
#else
    for(ul; ul != nullptr; ul = obcsGetNextOnList(ul))
    {
        o = ul->part;
#endif
        if(!o)
            continue;

        // Draw all particles greater than this distance first.
        if(o->distsqr > 0 && !(o->flags & OBJECT_DYNASHADOW))
        {
            if(DrawFxSystemDeprecated(o->distsqr))
            {
                // All because we can't render sorted objects correctly...with zwrite off!
                // Reset states otherwise clouds are fubar'd
                _drawObject(nullptr);
            }
        }

        _drawObject(o);

#ifdef DEBUG_WW2
        count++;
#endif
    }

    DrawFxSystemDeprecated(-1);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);

   _unsetStates(o);
    piSetWorldMatrix(&IdentityMatrix);
    TextureState::End();
}
#endif

#if USE_STATESET
inline void _drawSortedCockpit()
{
#ifdef BETA_RELEASE
    if(!_cockpitsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentCockpitBin(bin);

    // Reset change detection
    _drawObject(nullptr);

    OBJECT* o = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        _drawObject(o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
inline void _drawSortedCockpit()
{
#ifdef BETA_RELEASE
    if(!_cockpitsorteddraw)
    {
        return;
    }
#endif

    extern void obcsGetTransparentCockpitBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetTransparentCockpitBin(bin);

    // Reset change detection
    _drawObject(nullptr);

    piMATRIX vm = *piGetViewMatrix();

    piSetViewMatrix(piGetCockpitViewMatrix());

    OBJECT* o = nullptr;

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

    // This is wrong. Transparent objects should not be written to the z buffer. But to
    // get those FLTs fixed with the correct flags is not worth having to put up with
    // all the f'ing whining.
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, 1);

#if DEBUG_WW2
    int count = 0;
#endif

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        _drawObject(o);

#if DEBUG_WW2
        count++;
#endif
    }

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);

    _unsetStates(o);

    piSetViewMatrix(&vm);
    piSetWorldMatrix(&IdentityMatrix);
}
#endif

inline void DrawShadowCockpitReceivers()
{
    if(gCustomTerrain)
    {
        return;
    }

	if(!(ShadowMapFlags & SHADOW_COCKPITS))
	{
		return;
	}

    oglDisableTexturesAboveAndIncluding(0);
    ogl::Disable();

    bool oldnormal = g_wantNormalMaps;
    g_wantNormalMaps = false;

    oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_DST_COLOR);//GL_ZERO);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ZERO);//GL_SRC_COLOR);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);
    oglSetRenderState(oglRENDERSTATE_ZFUNC, GL_EQUAL);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);

#if(0)
    // Don't need to render past 200 meters
    piMATRIX proj = *oglGetProjectionMatrix();
    proj._33 = (200.0f + 0.25f) / (200.0f - 0.25f);
    proj._43 = -2 * 200.0f * 0.25f / (200.0f - 0.25f);

    oglPushMatrix(PROJECTION_MATRIX);
    oglSetProjectionMatrix(&proj);
#endif

    obdrawSetRenderPass(RENDER_FINAL_SHADOW);

    ShadowMap::GetInstance().ApplyShadowMaps();

    OBJECT* o;

    SORTHEADER* ul = obcsGetUnsortedCockpitList();

    for(; ul != nullptr; ul = obcsGetPrevOnList(ul))
    {
        o = ul->part;

        if(!o) continue;

        if(o->flag3 & OBJECT_IS_INTERIOR)
            continue;

        if(piIsMaterialInterior(o->matid))
            continue;

        if(ShadowMap::GetInstance().SetShadowStuff(o))
        {
            if(o->drawcallback)
            {
                o->drawcallback(o);
                continue;
            }

            if(o->meshContext)
            {
                UINT32 old = o->meshContext->GetVertexType();
                UINT32 type = ogl::VertexAttributes::POSITION;

                o->meshContext->SetVertexType(type);
                _drawVertices(GetType(o->flags), o);

                o->meshContext->SetVertexType(old);
            }
            else
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                ogl::SetVertexPointer(3, GL_FLOAT, sd.position.stride, sd.position.pdata);
                ogl::DrawElements(GL_TRIANGLES, o->numtris * 3, GL_UNSIGNED_SHORT, (UINT16*)o->ptris);
            }
        }
    }

    ShadowMap::GetInstance().UnApplyShadowMaps();

    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ZFUNC, ZFunction);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    obdrawSetRenderPass(RENDER_SCENE);

#if(0)
    oglPopMatrix(PROJECTION_MATRIX);
#endif

    g_wantNormalMaps = oldnormal;
}

inline static bool OkToShadow(const OBJECT* o)
{
    // We are inside a building
    if(o->flag3 & OBJECT_IS_INTERIOR)
        return false;

    if(piIsMaterialInterior(o->matid))
        return false;

    if(o->distsqr >= 800.0f)
        return false;

	if(o->flag2 & (OBJECT_ISFPS | OBJECT_ANIMATION))
	{
		if(!(ShadowMapFlags & SHADOW_VEHICLES))
			return false;
	}

	if(o->modelInstance)
	{
		switch(o->modelInstance->GetModelType())
		{
		case ModelTypes::VEHICLE:
			{
				if(!(ShadowMapFlags & SHADOW_VEHICLES))
					return false;

				break;
			}
		case ModelTypes::AUTOGUNNER:
		case ModelTypes::BRIDGE:
		case ModelTypes::CAPTURE_PLACE:
		case ModelTypes::CONTROL_POINT:
		case ModelTypes::FACTORY:
		case ModelTypes::SPAWN:
		case ModelTypes::BUILDING:
			{
				if(!(ShadowMapFlags & SHADOW_OBJECTS))
				{
					return false;
				}
			}
		}
	}

	return true;
}

inline void DrawShadowReceivers()
{
    if(gCustomTerrain)
    {
        return;
    }

    oglDisableTexturesAboveAndIncluding(0);
    ogl::Disable();

    bool oldnormal = g_wantNormalMaps;
    g_wantNormalMaps = false;

    bool oldspecular = WantSpecularMaps;
    WantSpecularMaps = false;

    oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_DST_COLOR);//GL_ZERO);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ZERO);//GL_SRC_COLOR);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);
    oglSetRenderState(oglRENDERSTATE_ZFUNC, GL_EQUAL);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_ALPHAFUNC, GL_GEQUAL);
    oglSetRenderState(oglRENDERSTATE_ALPHAREF, (unsigned int)(0.99f * 255.0f));

#if(0)
    // Don't need to render past 200 meters
    piMATRIX proj = *oglGetProjectionMatrix();
    proj._33 = (200.0f + 0.25f) / (200.0f - 0.25f);
    proj._43 = -2 * 200.0f * 0.25f / (200.0f - 0.25f);

    oglPushMatrix(PROJECTION_MATRIX);
    oglSetProjectionMatrix(&proj);
#endif

    obdrawSetRenderPass(RENDER_FINAL_SHADOW);

    ShadowMap::GetInstance().ApplyShadowMaps();

    OBJECT* o;

    // Non replacement tile terrain (Have to split up the mesh into octets for proper shadow map choice)
    std::map<int, std::vector<piVECTOR> > octets;
    std::map<int, OBJECT*> objects;

    // Octes is a mapping of each culled octet (vertices of octet)
    // Objects is the object associated with that octet for world matrix stuff
    trnmainGetShadowReceivers(octets, objects);

    std::map<int, std::vector<piVECTOR> >::iterator itr;
    itr = octets.begin();

    piVECTOR* ptr;

    OBJECT* old = nullptr;
    piVECTOR p;

    for(; itr != octets.end(); ++itr)
    {
        // No vertices in this octet
        if(!itr->second.empty())
        {
            unsigned int size = itr->second.size();
            ptr = &itr->second.front();

            // Find object used for this octet world positioning
            std::map<int, OBJECT*>::iterator itr2 = objects.find(itr->first);

            if(itr2 == objects.end())
                continue;

            // ptr[2] is the center point of this octet
            math3dVectorMultiply(&p, &ptr[2], &itr2->second->mat);

            // Assign shadow map if necessary
            if(ShadowMap::GetInstance().SetShadowStuff(p))
            {
                // Only set world if changed
                if(itr2->second != old)
                {
                    old = itr2->second;
                    oglSetWorldMatrix(&old->mat);
                }

                ogl::SetVertexPointer(3, GL_FLOAT, 0, &ptr[0]);
                ogl::DrawArrays(GL_TRIANGLES, 0, size);
            }
        }
    }

    extern void obcsGetOpaqueBin(std::vector<OBJECT*>& bin);
    std::vector<OBJECT*> bin;

    obcsGetOpaqueBin(bin);

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

		if(!OkToShadow(o))
		{
			continue;
		}

        if(ShadowMap::GetInstance().SetShadowStuff(o))
        {
            // Be careful here. These draw callbacks can reset texture (enable/disable) coords
            // and change states which will screw up shadows. Check for
            // obdrawGetRenderType() == RENDER_FINAL_SHADOW to handle these cases.
            if(o->drawcallback)
            {
                o->drawcallback(o);
                continue;
            }

            if(o->meshContext)
            {
                UINT32 old = o->meshContext->GetVertexType();
                UINT32 type = ogl::VertexAttributes::POSITION;

                o->meshContext->SetVertexType(type);
                _drawVertices(GetType(o->flags), o);

                o->meshContext->SetVertexType(old);
            }
            else
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                ogl::SetVertexPointer(3, GL_FLOAT, sd.position.stride, sd.position.pdata);
                ogl::DrawElements(GL_TRIANGLES, o->numtris * 3, GL_UNSIGNED_SHORT, (UINT16*)o->ptris);
            }
        }
    }

    // Replacement terrain
    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    obcsGetMultiTextureBin(bin);

    for(unsigned int i = 0; i < bin.size(); i++)
    {
        o = bin[i];

        if(!o)
            continue;

		if(!OkToShadow(o))
		{
			continue;
		}

        if(ShadowMap::GetInstance().SetShadowStuff(o))
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            if(o->meshContext)
            {
                UINT32 old = o->meshContext->GetVertexType();
                UINT32 type = ogl::VertexAttributes::POSITION;

                o->meshContext->SetVertexType(type);
                _drawVertices(GetType(o->flags), o);

                o->meshContext->SetVertexType(old);
            }
            else
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                ogl::SetVertexPointer(3, GL_FLOAT, sd.position.stride, sd.position.pdata);
                ogl::DrawElements(GL_TRIANGLES, o->numtris * 3, GL_UNSIGNED_SHORT, (UINT16*)o->ptris);
            }
        }
    }

	if(ShadowMapFlags & SHADOW_CORPSES)
	{
		gr2AnimDrawCorpses();
	}

    ShadowMap::GetInstance().UnApplyShadowMaps();

#if(0)
    // Shadow transparent objects
    ul = obcsGetSortedList();

    unsigned int continueflag3 = (OBJECT_IS_INTERIOR | OBJECT_CLOUDS | OBJECT_RAINCLOUDS);

    for(ul; ul != nullptr; ul = obcsGetNextOnList(ul))
    {
        o = ul->part;

        if(!o) continue;

        if(o->flag3 & continueflag3)
            continue;

        if(o->distsqr >= 1600.0f)
            continue;

        if(ShadowMap::GetInstance().SetShadowStuff(o))
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            ogl::SetVertexPointer(3, GL_FLOAT, sd.position.stride, sd.position.pdata);
            ogl::DrawElements(GL_TRIANGLES, o->numtris * 3, GL_UNSIGNED_SHORT, (UINT16*)o->ptris);
        }
    }
#endif

    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ZFUNC, ZFunction);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHAFUNC, piALPHAFUNC_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);


    obdrawSetRenderPass(RENDER_SCENE);

#if(0)
    oglPopMatrix(PROJECTION_MATRIX);
#endif

    g_wantNormalMaps = oldnormal;
    WantSpecularMaps = oldspecular;
}

#if USE_STATESET
inline void _drawMultiTextureDepth()
{
#ifdef BETA_RELEASE
    if(!_multitexturedraw)
    {
        return;
    }
#endif

    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    std::vector<OBJECT*> bin;
    obcsGetMultiTextureBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;

    depthpass->Apply();

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        oglSetWorldMatrix(&o->mat);
        _drawVertices(GetType(o->flags), o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}

inline void _drawMultiTexture()
{
#ifdef BETA_RELEASE
    if(!_multitexturedraw)
    {
        return;
    }
#endif

    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    std::vector<OBJECT*> bin;
    obcsGetMultiTextureBin(bin);

    if(bin.empty())
        return;

    OBJECT* o = nullptr;

    const RenderStateSet* old = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(!o->material)
            continue;

        if(old != o->material || o->pverts)
        {
            if(!(o->meshContext && (o->flag2 & OBJECT_USEBUFFERS)))
            {
                int id = o->texid;
                int offset = 0;

                for(int unit = 1; unit < gMaxTextureUnits; unit++)
                {
                    if(!obdrawFindMultiTextureById(id, &offset))
                        break;

                    if(o->flags & OBJECT_TERRAIN)
				        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0 + unit, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &o->ptverts->tu);
                    else
                    {
                        piTexCoordConstPtr uv = o->ImmutableTexCoords();
				        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0 + unit, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &uv->u);
                    }

                    id = MultiTextureInfo[offset].MultiTextureId;
                }
            }

            if(old != o->material)
            {
                o->material->Apply();
            }
        }

        oglSetWorldMatrix(&o->mat);
        old = o->material;

        _drawVertices(GetType(o->flags), o);
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
inline void _drawMultiTexture()
{
#ifdef BETA_RELEASE
    if(!_multitexturedraw)
    {
        return;
    }
#endif

    extern void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin);

    std::vector<OBJECT*> bin;
    obcsGetMultiTextureBin(bin);

    if(bin.empty())
        return;

    // Reset change detection
    _drawObject(nullptr);

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    const oglMaterial* old = nullptr;

    const unsigned int size = bin.size();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = bin[i];

        if(!o)
            continue;

        if(o->drawcallback)
        {
            bool success = o->drawcallback(o);

            if(success)
            {
                old = o->material;

#ifdef BETA_RELEASE
                if(_drawboundingbox)
                    obdrawObjectBoundingBox(o, colorBLUEALPHA);
#endif
                continue;
            }
            // else fall through and draw normally
        }

        if(!o->material)
            continue;

        if(old != o->material || o->pverts)
        {
            obdrawApplyMultiTextureStage(o);

            _unsetStates(last);
            _setStates(o);

            if(!(o->meshContext && (o->flag2 & OBJECT_USEBUFFERS)))
            {
                // Has at least 3 textures
                for(int unit = 1; unit < 3; unit++)
                {
                    if(o->flags & OBJECT_TERRAIN)
				        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0 + unit, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &o->ptverts->tu);
                    else
                    {
                        piTexCoordConstPtr uv = o->ImmutableTexCoords();
				        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0 + unit, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), &uv->u);
                    }
                }
            }
            o->material->Apply();
        }

        oglSetWorldMatrix(&o->mat);
        old = o->material;

        _drawVertices(GetType(o->flags), o);

        last = o;
    }

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

    obdrawReleaseMultiTextureStage(last);

    _unsetStates(last);
    piSetWorldMatrix(&IdentityMatrix);
}
#endif

void obdrawDrawWaterRefraction()
{
#ifdef BETA_RELEASE
    if(!_sorteddraw)
    {
        return;
    }
#endif

    SORTHEADER* ul = obcsGetUnsortedList();

    // Reset change detection
    _drawObject(nullptr);
    oglSetWorldMatrix(&IdentityMatrix);

    OBJECT* o = nullptr;

    oglPushRenderState(oglRENDERSTATE_ALPHABLENDENABLE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);

    for(; ul != nullptr; ul = obcsGetNextOnList(ul))
    {
        o = ul->part;

        if(!o) continue;

        _drawObject(o);
    }

    oglPopRenderState(oglRENDERSTATE_ALPHABLENDENABLE);

   _unsetStates(o);
   oglSetWorldMatrix(&IdentityMatrix);
}

#if defined(BETA_RELEASE)
static void drawHeightMap()
{
    OBJECT* const o = trndtl[12].terrainorig;

    oglSetWorldMatrix(&o->mat);

    piVectorConstPtr p = o->RenderPositions();

    for(int ii = 0; ii < o->numtris; ii++)
    {
        draw3dLine(&p[o->ptris[ii].v1], &p[o->ptris[ii].v2], colorBLUEALPHA);
        draw3dLine(&p[o->ptris[ii].v2], &p[o->ptris[ii].v3], colorBLUEALPHA);
        draw3dLine(&p[o->ptris[ii].v3], &p[o->ptris[ii].v1], colorBLUEALPHA);
    }
}
#endif

#if USE_STATESET
void obdrawDrawShadowCasters()
{
    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        logTimerCallVoid(_drawUnsorted);
    }
}
#else
void obdrawDrawShadowCasters()
{
    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        oglDisableTexturesAboveAndIncluding(0);

        oglLockTextureState(true);
        oglLockMaterialState(1);

        logTimerCallVoid(_drawUnsorted);

        oglLockTextureState(false);
        oglLockMaterialState(0);
    }
}
#endif

#include "trn/trncloud.h"
#include "gr2/gr2Anim.h"
#include "trn/speedgrass.h"

#if USE_STATESET
LOGICAL _drawDeprecatedPath()
{
    logTimerBeginMacro();

    static ref_ptr<ClearNode> clear;

    if(!clear.valid())
    {
        clear = new ClearNode(GL_DEPTH_BUFFER_BIT, true, Vec4f(0, 0, 0, 1));
    }

    if(g_postRenderFilter && _sceneFBO.valid())
    {
        _sceneFBO->apply();
        clear->apply();
    }

    glslWaterClearBin();
    glslNormalClearBin();
    glslSpecularClearBin();

    trnmainDraw();
    _drawUnsorted();
    glslNormalDrawBinDeprecated();
    glslSpecularDrawBinDeprecated();
    _drawMultiTexture();
   /*
    gr2AnimDrawCorpses();
    */
    glslWaterDrawBinDeprecated();
    trnskyDrawDeprecated();

    sptDisplayDeprecated();
    speedgrassDraw();

    depthpass->Apply();
    _drawUnsortedCockpit();

    if(g_postRenderFilter && _sceneFBO.valid())
    {
        if(multisample)
        {
            _sceneFBO->Blit(_resolveSceneFBO.get(),
                            0,
                            0,
                            WantedWidth,
                            WantedHeight,
                            0,
                            0,
                            WantedWidth,
                            WantedHeight,
                            GL_DEPTH_BUFFER_BIT,
                            GL_NEAREST);
        }
    }

    _drawSorted();

    trnskyAdjustBrightness();

    clear->apply();

    oglSetFieldOfView(FieldOfView, 0.02f, 500.0f);
    _drawUnsortedCockpit();
    _drawSortedCockpit();
    oglSetFieldOfView(FieldOfView, rnmainGetZBufferNearVal(), piLongLikeFloat(FogEnd));

    if(g_postRenderFilter && _sceneFBO.valid())
    {
        if(multisample)
        {
            _sceneFBO->Blit(_resolveSceneFBO.get(),
                            0,
                            0,
                            WantedWidth,
                            WantedHeight,
                            0,
                            0,
                            WantedWidth,
                            WantedHeight,
                            GL_COLOR_BUFFER_BIT,
                            GL_NEAREST);
        }

        _sceneFBO->UnApply();
    }

    trnskyDrawFlare();

    logTimerEndMacro();

    return TRUE;
}
#else
LOGICAL _drawDeprecatedPath()
{
    logTimerBeginMacro();

    logTimerCallVoid(_drawUnsorted);
    logTimerCallVoid(_drawMultiTexture);

    if(gCustomTerrain)
    {
        // For water
        PostRenderFilter::GetInstance().copyscene();
    }

    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

    logTimerCallVoid(glslNormalDrawBinDeprecated);
    logTimerCallVoid(glslSpecularDrawBinDeprecated);
    logTimerCallVoid(glslWaterDrawBinDeprecated);

    // Infantry 'hulks'
    logTimerCallVoid(gr2AnimDrawCorpses);
    //logTimerCallVoid(gr2DrawTrooperBin);

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        logTimerCallVoid(DrawShadowReceivers);
    }

    logTimerCallVoid(sptDisplayDeprecated);
    logTimerCallVoid(speedgrassDraw);

    logTimerCallVoid(vehIconDrawDeprecated);
    logTimerCallVoid(_drawSorted);

#ifdef BETA_RELEASE
    if(_drawHeightMap)
    {
        drawHeightMap();
    }

    if(_drawTObox)
    {
        trncdDrawShortTOBoundingBox();
    }

    if(_drawCollisionObjects)
    {
        trncdDrawShortCollisionObjects();
    }
#endif

#ifdef USE_LENS_FLARE
    if(g_postRenderFilter &&
       g_lensFlare.isVisible() &&
       g_lensFlare.GetSunTexture())
    {
        // Fixes bleedthrough in cockpits...
        // Depth only.
        oglSetRenderState(oglRENDERSTATE_COLORMASK, 0);

        _drawUnsortedCockpit();

        oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));
		piSetFieldOfView(FieldOfView, 0.2f, piLongLikeFloat(FogEnd));
    }

    trnskyAdjustBrightness();
    trnskyDrawFlare();
#endif

    mlDrawProps();

	// this trick works for regular z buffers as well
    piSetFieldOfView(FieldOfView, 0.02f, 500.0f);

    piClearTargets(piCLEAR_ZBUFFER);

	if (_UnsortedInternalDrawCallback != nullptr)
	{
		if (_UnsortedInternalDrawCallback(TRUE) == FALSE)
		{
			return FALSE;
		}
	}

    logTimerCallVoid(_drawUnsortedCockpit);
    //logTimerCallVoid(gr2DrawTrooperBin);

    if (_UnsortedInternalDrawCallback != nullptr)
	{
		if (_UnsortedInternalDrawCallback(FALSE) == FALSE)
		{
			return FALSE;
		}
	}

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        logTimerCallVoid(DrawShadowCockpitReceivers);
    }

	if (_SortedInternalDrawCallback != nullptr)
	{
		if (_SortedInternalDrawCallback(TRUE) == FALSE)
		{
			return FALSE;
		}
	}

    logTimerCallVoid(_drawSortedCockpit);

    if (_SortedInternalDrawCallback != nullptr)
	{
		if (_SortedInternalDrawCallback(FALSE) == FALSE)
		{
			return FALSE;
		}
	}

    logTimerEndMacro();

    return(1);
}
#endif

LOGICAL obdrawDrawSingleObject(OBJECT* lpobj)
{
LOGICAL			retval=TRUE;
SINT32			texid, type, textype;
piMATRIX		mat,mat1;

	// find drawing type
	type = GetType(lpobj->flags);
	if (type == 0)
	{
		return(TRUE);
	}

	if (
#ifndef USE_3DAPI_LIGHTING
		lpobj->flag2 & OBJECT_ANIMATION ||
#endif
		!(lpobj->flags & OBJECT_LITE) ||
		!(lpobj->flags & OBJECT_DYNALITE))
	{
		piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
	}

	// don't cull the backface of polygons
	if (lpobj->flags & OBJECT_NOBACKFACE)
	{
        piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);
	}
	else
	{
        piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
	}

	// Z buffer work
	if (lpobj->flag2 & OBJECT_ZDISABLE)
	{
		piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_ALWAYS);
	}

	if (lpobj->flags & OBJECT_ZWRITEOFF)
	{
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
	}

    lpobj->material->Apply();

	mat1 = *piGetViewMatrix();
	// some objects use a matrix for placement in the world
	if (lpobj->flags & OBJECT_USEMATRIX)
	{
		mat = *piGetWorldMatrix();

		if (lpobj->flag2 & OBJECT_ISCOCKPIT && bZEnable != piZB_USEW)
		{
			mat1 = *piGetViewMatrix();

			piSetViewMatrix(piGetCockpitViewMatrix());
//			piSetWorldMatrix(&lpobj->matint);
			piSetWorldMatrix(&lpobj->mat);
		}
		else
		{
			piSetWorldMatrix(&lpobj->mat);
		}
	}

	if (lpobj->level != 0)
	{
#ifdef OBDRAW_EVENT_LOG
		if (DrawingSortedObjects)
		{
			WriteLog(LOG_RENDER, LOG_DEBUG, "ALPHAREF objects should be in an unsorted list");
		}
#endif

		piSetRenderState(piRENDERSTATE_ALPHAREF, lpobj->level);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);

        // Blend one, zero is the same as glDisable(GL_BLEND)
		//piSetRenderState(piRENDERSTATE_SRCBLEND, piBLEND_ONE);
		//piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ZERO);
	}

	// some objects don't want filtering
	if (lpobj->flags & OBJECT_NOFILTER)
	{
		piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
		piSetRenderStageState(0, piTSS_MAGFILTER, piTFG_POINT);
		piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);
		if (MultiTextureEnable)
		{
			piSetRenderStageState(1, piTSS_MINFILTER, piTFN_POINT);
			piSetRenderStageState(1, piTSS_MAGFILTER, piTFG_POINT);
			piSetRenderStageState(1, piTSS_MIPFILTER, piTFP_NONE);
		}
	}

	if (lpobj->flag2 & OBJECT_BLENDADDITIVE)
	{
		piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
	}

	if (lpobj->flag2 & OBJECT_ISCOCKPIT)
	{
		piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);
		if (MultiTextureEnable)
		{
			piSetRenderStageState(1, piTSS_MIPFILTER, piTFP_NONE);
		}
	}

	if (lpobj->flag2 & OBJECT_NOFOG)
	{
		piSetRenderState(piRENDERSTATE_FOGENABLE, FALSE);
	}

	textype = obdrawGetMultiTextureOpcode(lpobj->texid, nullptr);
	if (textype == MT_SINGLEPASS)
	{
		// Single-pass
		obdrawApplyMultiTextureStage(lpobj);
	}

	// shadow object
	if (lpobj->flags & OBJECT_DYNASHADOW)
	{
		if (!DrawShadow(lpobj))
		{
			retval = FALSE;
		}
	}
	else
#ifndef USE_3DAPI_LIGHTING
	if (lpobj->flags & OBJECT_LITE)
	{
		if (!DrawLitVertices(type, lpobj))
		{
			retval = FALSE;
		}
	}
	else
#endif
	{
		if (!DrawVertices(type, lpobj))
		{
			retval = FALSE;
		}
	}

	// Apply any required multitexturing
	if (textype == MT_SINGLEPASS)
	{
		// Single-pass
		obdrawReleaseMultiTextureStage(lpobj);
	}
	else
	if (textype == MT_BUMPMAP)
	{
		// Bump map
//		obdrawApplyBumpmapTextureStage(lpobj);

		if (!DrawVertices(type, lpobj))
		{
			retval = FALSE;
		}

		// Bump map
//		obdrawReleaseBumpmapTextureStage(lpobj);
	}
	else
	if (textype == MT_MULTIPASS)
	{
		// Multi-pass
		obdrawApplyMultiTexture(lpobj);
	}

	if (trnwtrIsWaterTexture(lpobj->texid))
	{
		trnwtrPreDraw(lpobj);

		if (!DrawVertices(type, lpobj))
		{
			retval = FALSE;
		}

		trnwtrPostDraw();
	}

	if (lpobj->flag2 & OBJECT_USESDECAL &&
		lpobj->shadow != nullptr &&
		!(lpobj->flags & OBJECT_DYNASHADOW))
	{
		// texture selection and mipmapping
		texid = lpobj->shadow->texid;
		if (!bTexturesDisabled)
		{
			piSetActiveTexture(texid, 0);
		}
		else
		{
			piDisableTexture();
		}

#ifndef USE_3DAPI_LIGHTING
		if (lpobj->flags & OBJECT_LITE)
		{
			if (!DrawLitVertices(type, lpobj))
			{
				retval = FALSE;
			}
		}
		else
#endif
		{
			if (!DrawVertices(type, lpobj))
			{
				retval = FALSE;
			}
		}
	}

	// Z buffer work for effects
	if (lpobj->flags & OBJECT_ZWRITEOFF)
	{
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
	}

	if (lpobj->flag2 & OBJECT_ZDISABLE)
	{
		piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
	}

    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

	if (lpobj->flag2 & OBJECT_NOFOG)
	{
		piSetRenderState(piRENDERSTATE_FOGENABLE, bFogEnabled);
	}

	if (lpobj->flag2 & OBJECT_ISCOCKPIT)
	{
		piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
		if (MultiTextureEnable)
		{
			piSetRenderStageState(1, piTSS_MIPFILTER, TextureMipFilter);
		}
	}

	if (lpobj->flag2 & OBJECT_BLENDADDITIVE)
	{
		piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
	}

	// some objects don't want filtering
	if (lpobj->flags & OBJECT_NOFILTER)
	{
		piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
		piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
		piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);
		if (MultiTextureEnable)
		{
			piSetRenderStageState(1, piTSS_MIPFILTER, TextureMipFilter);
			piSetRenderStageState(1, piTSS_MINFILTER, TextureMinFilter);
			piSetRenderStageState(1, piTSS_MAGFILTER, TextureMagFilter);
		}
	}

	if (lpobj->level != 0)
	{
		piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
		//piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
		//piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
	}

	if (
#ifndef USE_3DAPI_LIGHTING
		lpobj->flag2 & OBJECT_ANIMATION ||
#endif
		!(lpobj->flags & OBJECT_LITE) ||
		!(lpobj->flags & OBJECT_DYNALITE))
	{
		piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
	}

	// outline all triangles
	if (0)
	{
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
		obdrawWireframeObject(lpobj, colorREDALPHA);
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
	}

	// indicate center of object
	if (0)
	{
	piVECTOR	s, e;

		s = e = lpobj->pos;
		s.x -= 1.0f * ScaleFactor;
		e.x += 1.0f * ScaleFactor;
		draw3dLine(&s, &e, colorBLUEALPHA);

		s = e = lpobj->pos;
		s.y -= 1.0f * ScaleFactor;
		e.y += 1.0f * ScaleFactor;
		draw3dLine(&s, &e, colorBLUEALPHA);

		s = e = lpobj->pos;
		s.z -= 1.0f * ScaleFactor;
		e.z += 1.0f * ScaleFactor;
		draw3dLine(&s, &e, colorBLUEALPHA);
	}

	// some objects use a matrix for placement in the world
	if (lpobj->flags & OBJECT_USEMATRIX)
	{
		if (lpobj->flag2 & OBJECT_ISCOCKPIT && bZEnable != piZB_USEW)
		{
			piSetViewMatrix(&mat1);
		}

		piSetWorldMatrix(&mat);
	}

	if (retval == FALSE)
	{
		return(FALSE);
	}

	return(TRUE);
}

void obdrawDrawBoundBox(LOGICAL draw)
{
    _drawboundingbox = draw;
}

void obdrawDrawCollisionObjects(LOGICAL draw)
{
    _drawCollisionObjects = draw;
}

void obdrawDrawTOBoundBox(LOGICAL draw)
{
    _drawTObox = draw;
}

void obdrawDrawHeightMap(bool draw)
{
    _drawHeightMap = draw;
}

void obdrawObjectTriangles(LOGICAL draw)
{
    g_drawObjectTriangles = draw;
}

void obdrawObjectNormals(LOGICAL draw)
{
    g_drawObjectNormals = draw;
}

void obdrawCollisionNormals(LOGICAL draw)
{
    _drawCollisionNormals = draw;
}

void obdrawToggleUnsortedDraw(bool draw)
{
    _unsorteddraw = draw;
}

void obdrawToggleSortedDraw(bool draw)
{
    _sorteddraw = draw;
}

void obdrawToggleMultitextureDraw(bool draw)
{
    _multitexturedraw = draw;
}

void obdrawToggleSortedCockpitDraw(bool draw)
{
    _cockpitsorteddraw = draw;
}

void obdrawToggleUnsortedCockpitDraw(bool draw)
{
    _cockpitunsorteddraw = draw;
}

LOGICAL obdrawProcessLODCallback(OBJECT* lplod, UINT32 user,piVECTOR* InPos,
								 LOGICAL ReLight,FLOAT32 Radius,LOGICAL * LODChanged)
{
FLOAT32		distsqr;
FLOAT32		maxlod;
piVECTOR	pp, pos;
//piMATRIX	mat;
OBJECT * 	lplodtopobject;
FLOAT32		ratio;

	if (LODChanged != nullptr)
	{
		*LODChanged = FALSE;
	}

	pp.x = -playpos.x;
	pp.y = -playpos.y;
	pp.z = -playpos.z;

	lplodtopobject = lplod;

	while (lplod->flags & OBJECT_NOTOBJECT)
	{
		lplod = lplod->child;
		if (lplod == nullptr)
		{
			return(FALSE);
		}
	}

//	pos = lplod->pos;
/*
	if (lplodtopobject->flags & OBJECT_USEMATRIX)
	{
		// Rotate/Translate position to object space
		math3dInvertMatrix(&mat, &lplodtopobject->mat);
		math3dVectorMultiply(&pp, &pp, &mat);
	}
	else
	{
		if (lplod->flags & OBJECT_USEMATRIX)
		{
			// Rotate/Translate position to object space
			math3dInvertMatrix(&mat, &lplod->mat);
			math3dVectorMultiply(&pp, &pp, &mat);
		}
	}
*/
/*
	if (lplodtopobject->flags & OBJECT_USEMATRIX)
	{
		math3dVectorMultiply(&pos,&pos,&lplodtopobject->mat);
	}
	else
	{
		if (lplod->flags & OBJECT_USEMATRIX)
		{
			math3dVectorMultiply(&pos,&pos,&lplod->mat);
		}
	}
*/
	pos = *InPos;

	math3dSubtractVectors(&pp, &pp, &pos);

	// z axis only applies at 1/2 value, so that planes in the air don't lose
	// complete vis on the object beyond horizon distance in the z axis
//	pp.z *= 0.5;

	// factor in field of view, assuming that 90 degree FOV results in normal LOD
	// cos of 45 degrees = 0.0707106781
//	ratio =  (0.5f / 0.707106781f) / (FLOAT32)cos(FieldOfView * PI / 180.0 * 0.5);
	if (LODFieldOfView == TRUE)
	{
		if (rnvisVectorFOVCheck(&pos,Radius) == TRUE)
		{
			ratio = FieldOfView / 90.0f;
			math3dScaleVector(&pp,&pp,ratio);
		}
	}
	else
	{
		ratio = FieldOfView / 90.0f;
		math3dScaleVector(&pp,&pp,ratio);
	}

	// we're disabling this for the time being due to ground objects
	// popping in/out
//	math3dScaleVector(&pp, &pp, InvFrameFactor);

#ifndef VEHICLE_OBJECT_ANALYSIS // for artists to test LOD's in-game with external dropped camera
	if (lplod->flag2 & OBJECT_USERVEHICLE)
	{
		distsqr = 1.0f;
	}
	else
#endif
	{
		distsqr = math3dDotProduct(&pp, &pp);
	}

	maxlod = lplod->maxlod;
/*
    FLOAT32 horizon = 0.0f;
    trnmainGetHorizon(&horizon);

	if (maxlod > horizon * horizon)
	{
		maxlod = piLongLikeFloat(FogEnd) + 800.0f;
		maxlod *= maxlod;
	}
*/
	if (distsqr >= lplod->minlod &&
		distsqr < maxlod)
	{
		if (lplodtopobject->flags & OBJECT_SKIPDRAWING)
		{
			// enable this LOD
			if (!obmainClearFlagsInThisTree(lplodtopobject, OBJECT_SKIPDRAWING))
			{
				return(FALSE);
			}

#ifndef USE_3DAPI_LIGHTING
			if (ReLight == TRUE)
			{
				obdliteLightTree(lplodtopobject);
			}
#endif

			obAdjustObjectTreeVisListStatus(lplodtopobject,0);

			if (LODChanged != nullptr)
			{
				*LODChanged = TRUE;
			}
		}
	}
	else
	{
		if (!(lplodtopobject->flags & OBJECT_SKIPDRAWING))
		{
			// disable this LOD
			if (!obmainSetFlagsInThisTree(lplodtopobject, OBJECT_SKIPDRAWING))
			{
				return(FALSE);
			}

			obAdjustObjectTreeVisListStatus(lplodtopobject,0);

			if (LODChanged != nullptr)
			{
				*LODChanged = TRUE;
			}
		}
	}

	return(TRUE);
}

LOGICAL obAdjustObjectTreeVisListStatus(OBJECT * Object, UINT32 IncludeSibs)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;

	if (Object == nullptr)
	{
		return(TRUE);
	}

	if(Object->next == nullptr && Object->prev == nullptr) return TRUE;

	ObjSib = Object;

	do
	{
		if (ObjSib->flags & OBJECT_NOTOBJECT)
		{
			obmainMoveToNotObjectTree(ObjSib); // just to be sure
		}
		else
		{
			if ((!(ObjSib->flags & OBJECT_VISIBLE) ||
				   ObjSib->flags & OBJECT_SKIPDRAWING ||
				   ObjSib->flag2 & OBJECT_HIDDEN))
			{
				ObjAtt = ObjSib;
				do
				{
					obmainMoveToNotObjectTree(ObjAtt);

					ObjAtt = ObjAtt->attached;
				}
				while (ObjAtt != nullptr);
			}
			else
			{
				ObjAtt = ObjSib;
				do
				{
					obmainMoveToObjectTree(ObjAtt);

					ObjAtt = ObjAtt->attached;
				}
				while (ObjAtt != nullptr);
			}
		}


		if (ObjSib->child != nullptr)
		{
			obAdjustObjectTreeVisListStatus(ObjSib->child,1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && IncludeSibs != 0);

	return(TRUE);
}


// Time for sun to travel in the sky 1 degree
#define UPDATE_TOD_DELTA	(4 * 60)

LOGICAL obdrawUpdateTerrainObjectLighting(LOGICAL Override)
{
#if defined(WTF_DOES_THIS_DO)
//LOGICAL			retval=TRUE;
int				numobjects;
OBJECT*		lpobj;
LISTHEADER*	lplist;
SORTHEADER*	lpsort;
UINT32			tod;

static UINT32	lasttod;


	trnTODGetTimeOfDay(&tod);

	if (Override == FALSE &&
		TerrainLightingUpdate == FALSE)
	{
		if (abs((int)(tod - lasttod)) < UPDATE_TOD_DELTA)
		{
			return(TRUE);
		}
	}

	lasttod = tod;

	obcsResetUnsortedList();

	numobjects = obmainGetObjects(&lplist);
	if (numobjects <= 0)
	{
		return(TRUE);
	}

	lpobj = (OBJECT*)lplist->next;
	for (; lpobj; lpobj=lpobj->next)
	{
		// only operate on selected objects
		if (!(lpobj->flag2 & OBJECT_TERRAIN_OBJECT))
		{
			continue;
		}

        if(lpobj->flag2 & OBJECT_SPEEDTREE)
        {
            continue;
        }

		lpsort = obcsGetNextSortItem();
		if (lpsort == nullptr)
		{
			break;
		}

		lpsort->part = lpobj;
		lpsort->distsqr = lpobj->distsqr;
		lpsort->layer = lpobj->layer;

		obcsAddToUnsortedList(lpsort);
	}

#ifndef USE_3DAPI_LIGHTING
	if (!obdliteCalculate((SORTHEADER*)obcsGetUnsortedList(), TRUE))
	{
		return(FALSE);
	}
#endif
#endif

	return(TRUE);
}


LOGICAL obdrawStraightAhead(piVECTOR* pos, piVECTOR* rot, float dist, UINT32 color)
{
LOGICAL      retval=TRUE;
piVECTOR  norm;

	norm.x = -(pos->x - playdir.x * dist);
	norm.y = -(pos->y - playdir.y * dist);
	norm.z = -(pos->z - playdir.z * dist);
	if (!draw3dPoint(&norm, color))
	{
		retval = FALSE;
	}

	return(retval);
}

UINT32 obdrawDrawTreeSiblings(OBJECT* pObject)
{
	OBJECT* pObjSib;
	OBJECT* pObjAtt;

	if(pObject == nullptr)
	{
		return 0;
	}

	pObjSib = pObject;

	do
	{
		pObjAtt = pObjSib;

		if(!(pObjAtt->flags & OBJECT_NOTOBJECT))
		{
			do
			{
				if (pObjAtt->flags & OBJECT_VISIBLE &&
					!(pObjAtt->flags & OBJECT_SKIPDRAWING) &&
					!(pObjAtt->flag2 & OBJECT_HIDDEN))
				{
                    _drawObject(pObjAtt);
				}

				pObjAtt = pObjAtt->attached;
			}
			while(pObjAtt != nullptr);
		}

		if(pObjSib->child != nullptr)
		{
			obdrawDrawTreeSiblings(pObjSib->child);
		}

		pObjSib = pObjSib->sibling;
	}
	while(pObjSib != nullptr);

	return 0;
}

UINT32 obdrawDrawTree(OBJECT* pObject)
{
	OBJECT* pObjSib;
	OBJECT* pObjAtt;

	if(pObject == nullptr)
	{
		return 0;
	}

	pObjSib = pObject;
	pObjAtt = pObjSib;

		if(!(pObjAtt->flags & OBJECT_NOTOBJECT))
		{
			do
			{
				if	(pObjAtt->flags & OBJECT_VISIBLE &&
					!(pObjAtt->flags & OBJECT_SKIPDRAWING) &&
					!(pObjAtt->flag2 & OBJECT_HIDDEN))
				{
                    _drawObject(pObjAtt);
				}

				pObjAtt = pObjAtt->attached;
			}
			while(pObjAtt != nullptr);
		}

		if(pObjSib->child != nullptr)
		{
			obdrawDrawTreeSiblings(pObjSib->child);
		}

		pObjSib = pObjSib->sibling;

	return 0;
}

LOGICAL obdrawInstallDrawCallback(USERDRAWFUNC FuncPtr,UINT32 FuncNum)
{
	if (FuncPtr == nullptr)
	{
		return FALSE;
	}

	if (FuncNum == OBDRAW_UNSORTED_EXTERNAL_DRAW)
	{
		_UnsortedExternalDrawCallback = FuncPtr;
	}

	if (FuncNum == OBDRAW_SORTED_EXTERNAL_DRAW)
	{
		_SortedExternalDrawCallback = FuncPtr;
	}

	if (FuncNum == OBDRAW_UNSORTED_INTERNAL_DRAW)
	{
		_UnsortedInternalDrawCallback = FuncPtr;
	}

	if (FuncNum == OBDRAW_SORTED_INTERNAL_DRAW)
	{
		_SortedInternalDrawCallback = FuncPtr;
	}

	return TRUE;
}

void obdrawDrawUnsorted()
{
    _drawUnsorted();
}

void obdrawDrawSorted()
{
    _drawSorted();
}

void obdrawDrawMultitextured()
{
    _drawMultiTexture();
}

void obdrawSetRenderPass(unsigned int type)
{
    _renderpass = type;
}

bool obdrawIsRenderPass(unsigned int pass)
{
    return pass & _renderpass;
}

void obdrawDrawUnsortedExternalCallback(LOGICAL isbeforestage)
{
    if(_UnsortedExternalDrawCallback)
    {
        _UnsortedExternalDrawCallback(isbeforestage);
    }
}

void obdrawDrawSortedExternalCallback(LOGICAL isbeforestage)
{
    if(_SortedExternalDrawCallback)
    {
        _SortedExternalDrawCallback(isbeforestage);
    }
}

void obdrawDrawUnsortedInternalCallback(LOGICAL isbeforestage)
{
    if(_UnsortedInternalDrawCallback)
    {
        _UnsortedInternalDrawCallback(isbeforestage);
    }
}

void obdrawDrawSortedInternalCallback(LOGICAL isbeforestage)
{
    if(_SortedInternalDrawCallback)
    {
        _SortedInternalDrawCallback(isbeforestage);
    }
}

void obdrawDrawSortedCockpit()
{
    _drawSortedCockpit();
}

void obdrawDrawUnsortedCockpit()
{
    _drawUnsortedCockpit();
}

bool obdrawDrawBuildingShadowCasters(int sx,
                                     int sy,
                                     int c,
                                     int r,
                                     const piVECTOR& offset)
{
    bool success = false;

    StratDB::StratObjList list;

    list = StratDB::BuildingsInOctet(sx, sy, c, r);

    if(list.empty())
        return false;

    StratDB::StratObjList::iterator itr = list.begin();

    for(; itr != list.end(); ++itr)
    {
        StratDB::StratObject* so = *itr;

        if(so->StratType() != SDM_BUILDING_GENERIC)
            continue;

        char buff[128];
        strprintf(buff, "%s_%x_%x", so->Filename(), (UINT16)so->PosX(), (UINT16)so->PosY());

        OBJECT* o = obmainFindTopmostObject(buff);

        if(!o)
            continue;

        OBJECT* shadow = nullptr;

        if(!o->shadow)
        {
            obmainFindComment(&shadow, o, "shadow");

            if(!shadow)
                continue;

            o->shadow = shadow;
        }
        else
        {
            shadow = o->shadow;
        }

#if(0)
        obmainClearFlag2InThisTree(shadow, OBJECT_HIDDEN);
#endif

        // NOTE: This is a hack to make building shadows simulate following the sun...
        // somewhat. I don't like this. It would be much preferable to use deferred
        // shadow maps....which hopefully can be implemented in the near future.
        o = shadow->child;

        oglDisableTexture();
        oglDisableMaterial();

        static LIGHT* light = 0;

        if(!light)
        {
            light = obdliteFindDynaLight("sun");
        }

        if(!light)
            return false;

        const piLIGHT& lt = light->light;

        if(!(light->flags & LITE_ON))
        {
            return false;
        }

        oglSetWorldMatrix(&o->mat);

        float scalex, scaley;

        // Modifying objects in object space
        piVECTOR objspace;
        math3dMultiplyRotation(&objspace, &offset, &o->matint);

        // Scale based on capped sun movement (offset is capped direction)
        scalex = -objspace.x;
        scaley = -objspace.y;

        std::map<float, piVECTOR*> dis;

        while(o)
        {
            if(o->numverts != 4)
            {
                o = o->sibling;
                continue;
            }

            piVECTOR lightdir;
            lightdir = lt.dvDirection;
            math3dScaleVector(&lightdir, &lt.dvDirection, -SDM_OCTET_SIZE_2);

            piVECTOR pos;

            math3dVectorMultiply(&pos, &o->pos, &o->mat);

            // Translate light to object pos
            mathAddVectors(&lightdir, &lightdir, &pos);

            oglSetWorldMatrix(&o->mat);

            piVectorPtr sp = o->StaticPositions();
            piVectorPtr mp = o->MutablePositions();

            piVECTOR wp;

            // Find closest vertex to the sun (in 2d)
            for(int i = 0; i < o->numverts; i++)
            {
                // Put in world
                math3dVectorMultiply(&wp, &sp[i], &o->mat);

                float x1 = lightdir.x - wp.x;
                float y1 = lightdir.y - wp.y;

                dis[x1 * x1 + y1 * y1] = &sp[i];
            }

            // Map sorts for us.
            std::map<float, piVECTOR*>::iterator itr = dis.begin();

            // First node is closest node.
            piVECTOR* p1 = itr->second;

            for(int i = 0; i < o->numverts; i++)
            {
                // Reset
                mp[i].x = sp[i].x;
                mp[i].y = sp[i].y;

                // Skip closest vertice to the sun
                if(&sp[i] == p1)
                {
                    continue;
                }

                // Adjust x and y directions
                if(sp[i].y != p1->y)
                {
                    mp[i].y = scaley + sp[i].y;
                }

                if(sp[i].x != p1->x)
                {
                    mp[i].x = scalex + sp[i].x;
                }
            }

            dis.clear();

            _drawVertices(GetType(o->flags), o);
            o = o->sibling;
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);

    return success;
}

void obdrawDrawAdvertisements()
{
#ifdef USE_GAME_ADVERTISING
    _drawAdvertisements();
#endif
}

void obdrawDrawMesh(OBJECT* o)
{
    if(ogl::UseDeprecatedPath())
        _drawVertices(GetType(o->flags), o);
    else
        DrawObject(o);
}

void obdrawDrawObject(const OBJECT* o, int units)
{
    if(ogl::UseDeprecatedPath())
        _drawObject((OBJECT*)o);
    else
        DrawObject(o, units);
}

void obdrawDrawObjectInstanced(const OBJECT* o, int units, int primcount)
{
    if(!ogl::UseDeprecatedPath())
        DrawObjectInstanced(o, units, primcount);
}

UINT32 obdrawGetMultiTextureCount(SINT32 texId)
{
	SINT32	offset		= 0;
	SINT32	multiTexId	= texId;

	for(int i=0; i < gMaxTextureUnits; i++)
	{
		if(!obdrawFindMultiTextureById(multiTexId, &offset)) return i;
		multiTexId = MultiTextureInfo[offset].MultiTextureId;
	}
	return gMaxTextureUnits;
}

int obdrawGetMultiTextureID(int offset)
{
    return MultiTextureInfo[offset].MultiTextureId;
}

void obdrawGetMultiTextureMatrix(int offset, Matrixf& matrix)
{
    matrix.set(MultiTextureInfo[offset].MultiTextureMatrix);
}
