#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obmain.h"
#include "ob/obdlite.h"
#include "ob/obdshad.h"
#include "ob/obcd.h"
#include "math/math3d.h"
#include "ld/ldtex.h"
#include "ld/ldpix.h"
#include "trn/trncd.h"
#include "trn/trnmain.h"
#include "ogl/oglMaterial.h"
#include "scen/ScenarioManager.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

#ifdef USE_SHADOWS

// for extreme debugging purposes
#define SHADOW_BOUNDS_DEBUG
#undef SHADOW_BOUNDS_DEBUG


#define SHADOW_FRAME_X		1
#define SHADOW_FRAME_Y		1


static SINT32		bounds[256][2];
static SINT32		shadow_texture_size, shadow_texture_size1;
static SINT32		shadow_start_x, shadow_finish_x;
static SINT32		shadow_start_y, shadow_finish_y;
static SINT32		shadow_width, shadow_height;
static FLOAT32		sundot;

LOGICAL obdshadInit()
{
    shadow_texture_size = 1 << 6;

	shadow_texture_size1 = shadow_texture_size - 1;

	shadow_start_x = SHADOW_FRAME_X;
	shadow_finish_x = shadow_texture_size1 - SHADOW_FRAME_X;
	shadow_width = shadow_texture_size - 2 * SHADOW_FRAME_X - 1;

	shadow_start_y = SHADOW_FRAME_Y;
	shadow_finish_y = shadow_texture_size1 - SHADOW_FRAME_Y;
	shadow_height = shadow_texture_size - 2 * SHADOW_FRAME_Y - 1;

    return TRUE;
}

void obdshadKill()
{
    Write_Log(LOG_OLD, LOG_INFO, "[obdshadKill]");
}

static void GetStep(float dx, float dy, float* ssx, float* ssy, int* count)
{
float     sx, sy;

	if (fabs(dx) < fabs(dy))
	{
		if (dy < 0.0f)
		{
			sy = -1.0f;
		}
		else
		{
			sy = 1.0f;
		}

		sx = (float)fabs(dx / dy);
		if (dx < 0.0f)
		{
			sx = -sx;
		}

		*count = FloatToInt(dy / sy);
	}
	else
	if (fabs(dy) < fabs(dx))
	{
		if (dx < 0.0f)
		{
			sx = -1.0f;
		}
		else
		{
			sx = 1.0f;
		}

		sy = (float)fabs(dy / dx);
		if (dy < 0.0f)
		{
			sy = -sy;
		}

		*count = FloatToInt(dx / sx);
	}
	else
	{
		if (dx < 0.0f)
		{
			sx = -1.0f;
		}
		else
		{
			sx = 1.0f;
		}

		if (dy < 0.0f)
		{
			sy = -1.0f;
		}
		else
		{
			sy = 1.0f;
		}

		*count = FloatToInt(dy / sy);
	}

	*ssx = sx;
	*ssy = sy;
}

static LOGICAL FillTriangle(piSURFACEDESC* lppisd, piVECTOR* pscrn, UINT32 color)
{
char*		cptr;
short*		sptr;
long*		lptr;
int			i, j, count;
float		bx, by, dx, dy, sx, sy;
piVECTOR	v1, v2, v3;

	v1 = pscrn[0];
	v2 = pscrn[1];
	v3 = pscrn[2];

	for (i=0; i < shadow_texture_size; i++)
	{
		bounds[i][0] = shadow_texture_size;
		bounds[i][1] = -1;
	}

	bx = v1.x;
	by = v1.y;

#ifdef SHADOW_BOUNDS_DEBUG
	if (bx < shadow_start_x)
	{
		bx = (FLOAT32)shadow_start_x;
	}
	else
	if (bx > shadow_finish_x)
	{
		bx = (FLOAT32)shadow_finish_x;
	}

	if (by < shadow_start_y)
	{
		by = (FLOAT32)shadow_start_y;
	}
	else
	if (by > shadow_finish_y)
	{
		by = (FLOAT32)shadow_finish_y;
	}
#endif

	dx = v2.x - bx;
	dy = v2.y - by;
	GetStep(dx, dy, &sx, &sy, &count);
	for (i=0; i < count; i++)
	{
#ifdef SHADOW_BOUNDS_DEBUG
		if (bx < shadow_start_x)
		{
			bx = (FLOAT32)shadow_start_x;
		}
		else
		if (bx > shadow_finish_x)
		{
			bx = (FLOAT32)shadow_finish_x;
		}

		if (by < shadow_start_y)
		{
			by = (FLOAT32)shadow_start_y;
		}
		else
		if (by > shadow_finish_y)
		{
			by = (FLOAT32)shadow_finish_y;
		}
#endif

		if (FloatToInt(bx) < bounds[FloatToInt(by)][0])
		{
			bounds[FloatToInt(by)][0] = FloatToInt(bx);
		}

		if (FloatToInt(bx) > bounds[FloatToInt(by)][1])
		{
			bounds[FloatToInt(by)][1] = FloatToInt(bx);
		}

		bx += sx;
		by += sy;
	}

	bx = v2.x;
	by = v2.y;

#ifdef SHADOW_BOUNDS_DEBUG
	if (bx < shadow_start_x)
	{
		bx = (FLOAT32)shadow_start_x;
	}
	else
	if (bx > shadow_finish_x)
	{
		bx = (FLOAT32)shadow_finish_x;
	}

	if (by < shadow_start_y)
	{
		by = (FLOAT32)shadow_start_y;
	}
	else
	if (by > shadow_finish_y)
	{
		by = (FLOAT32)shadow_finish_y;
	}
#endif

	dx = v3.x - bx;
	dy = v3.y - by;
	GetStep(dx, dy, &sx, &sy, &count);
	for (i=0; i < count; i++)
	{
#ifdef SHADOW_BOUNDS_DEBUG
		if (bx < shadow_start_x)
		{
			bx = (FLOAT32)shadow_start_x;
		}
		else
		if (bx > shadow_finish_x)
		{
			bx = (FLOAT32)shadow_finish_x;
		}

		if (by < shadow_start_y)
		{
			by = (FLOAT32)shadow_start_y;
		}
		else
		if (by > shadow_finish_y)
		{
			by = (FLOAT32)shadow_finish_y;
		}
#endif

		if (FloatToInt(bx) < bounds[FloatToInt(by)][0])
		{
			bounds[FloatToInt(by)][0] = FloatToInt(bx);
		}

		if (FloatToInt(bx) > bounds[FloatToInt(by)][1])
		{
			bounds[FloatToInt(by)][1] = FloatToInt(bx);
		}

		bx += sx;
		by += sy;
	}

	bx = v3.x;
	by = v3.y;

#ifdef SHADOW_BOUNDS_DEBUG
	if (bx < shadow_start_x)
	{
		bx = (FLOAT32)shadow_start_x;
	}
	else
	if (bx > shadow_finish_x)
	{
		bx = (FLOAT32)shadow_finish_x;
	}

	if (by < shadow_start_y)
	{
		by = (FLOAT32)shadow_start_y;
	}
	else
	if (by > shadow_finish_y)
	{
		by = (FLOAT32)shadow_finish_y;
	}
#endif

	dx = v1.x - bx;
	dy = v1.y - by;
	GetStep(dx, dy, &sx, &sy, &count);
	for (i=0; i < count; i++)
	{
#ifdef SHADOW_BOUNDS_DEBUG
		if (bx < shadow_start_x)
		{
			bx = (FLOAT32)shadow_start_x;
		}
		else
		if (bx > shadow_finish_x)
		{
			bx = (FLOAT32)shadow_finish_x;
		}

		if (by < shadow_start_y)
		{
			by = (FLOAT32)shadow_start_y;
		}
		else
		if (by > shadow_finish_y)
		{
			by = (FLOAT32)shadow_finish_y;
		}
#endif

		if (FloatToInt(bx) < bounds[FloatToInt(by)][0])
		{
			bounds[FloatToInt(by)][0] = FloatToInt(bx);
		}

		if (FloatToInt(bx) > bounds[FloatToInt(by)][1])
		{
			bounds[FloatToInt(by)][1] = FloatToInt(bx);
		}

		bx += sx;
		by += sy;
	}

#ifndef SHADOW_BOUNDS_DEBUG
	for (i=shadow_start_y; i <= shadow_finish_y; i++)
	{
		if (bounds[i][0] < shadow_start_x)
		{
			bounds[i][0] = shadow_start_x;
		}
		else
		if (bounds[i][0] > shadow_finish_x)
		{
			bounds[i][0] = shadow_finish_x;
		}

		if (bounds[i][1] <= shadow_start_x)
		{
			bounds[i][1] = shadow_start_x;
		}
		else
		if (bounds[i][1] > shadow_finish_x)
		{
			bounds[i][1] = shadow_finish_x;
		}
	}
#endif

	if (lppisd->PixelFormat.dwRGBBitCount <= 8)
	{
		cptr = (char*)lppisd->Surface;
		for (i=shadow_start_y; i <= shadow_finish_y; i++)
		{
			for (j=bounds[i][0]; j <= bounds[i][1]; j++)
			{
				cptr[j] = (char)color;
			}

			cptr += lppisd->Width;
		}
	}
	else
	if (lppisd->PixelFormat.dwRGBBitCount <= 16)
	{
		sptr = (short*)lppisd->Surface;
		for (i=shadow_start_y; i <= shadow_finish_y; i++)
		{
			for (j=bounds[i][0]; j <= bounds[i][1]; j++)
			{
				sptr[j] = (short)color;
			}

			sptr += lppisd->Width;
		}
	}
	else
	if (lppisd->PixelFormat.dwRGBBitCount <= 32)
	{
		lptr = (long*)lppisd->Surface;
		for (i=shadow_start_y; i < shadow_finish_y; i++)
		{
			for (j=bounds[i][0]; j <= bounds[i][1]; j++)
			{
				lptr[j] = (long)color;
			}

			lptr += lppisd->Width;
		}
	}

	return(TRUE);
}

static LOGICAL ComputeShadowOBox(OBJECT* lpmain, OBJECT* lpobj)
{
SINT32		i;
OBJECT*	lpatt;
piVECTOR	pos;
piVECTOR*	obox;

	obox	= lpobj->obox;

	while (lpmain)
	{
		if (lpmain->sibling)
		{
			if (!ComputeShadowOBox(lpmain->sibling, lpobj))
			{
				return(FALSE);
			}
		}

		if (lpmain->flags & OBJECT_NOTOBJECT)
		{
			lpmain = lpmain->child;
			continue;
		}

		lpatt = lpmain;
		while (lpatt)
		{
			piVectorConstPtr ipos = lpatt->ImmutablePositions();

			for (i=0; i < lpatt->numverts; i++, ++ipos)
			{
				math3dMultiplyRotation(&pos, ipos.ptr, &lpatt->mat);

				if (obox[0].x > pos.x)
				{
					obox[0].x = pos.x;
				}

				if (obox[1].x < pos.x)
				{
					obox[1].x = pos.x;
				}

				if (obox[0].y > pos.y)
				{
					obox[0].y = pos.y;
				}

				if (obox[1].y < pos.y)
				{
					obox[1].y = pos.y;
				}

				if (obox[0].z > pos.z)
				{
					obox[0].z = pos.z;
				}

				if (obox[1].z < pos.z)
				{
					obox[1].z = pos.z;
				}
			}

			lpatt = lpatt->attached;
		}

		lpmain = lpmain->child;
	}

	return(TRUE);
}

LOGICAL obdshadCreate(OBJECT* lptop, OBJECT* lpmain)
{
int			texid;
OBJECT*	lpobj;

#if USE_STATESET
    return TRUE;
#endif

    if(gCustomTerrain)
        return TRUE;

	if (!obmainCreateObject(&lpobj, lpmain->name, 4, 2))
	{
		return(FALSE);
	}

	texid = piCreateRawTexture(-1,
                                0,
                                shadow_texture_size,
                                shadow_texture_size,
                                TexBpp,
                                1,
                                CRT_FLAG_WRITABLE);

	if (piIsTexture(texid, 0))
	{
		if (texid != piCreateRawTexture(texid, 0, shadow_texture_size, shadow_texture_size, TexBpp, 1, CRT_FLAG_WRITABLE))
		{
			return(FALSE);
		}

		if (!ldtexFillTexture(texid, 0, colorBLACK))
		{
			return(FALSE);
		}
	}

	strcat(lpobj->name, "_s");
	lpobj->flags = OBJECT_VISIBLE;
	lpobj->flags |= OBJECT_LITE;
	lpobj->flags |= OBJECT_TRIANGLELIST;
	lpobj->flags |= OBJECT_INDEXED;
	lpobj->flags |= OBJECT_DYNASHADOW;
	lpobj->flags |= OBJECT_CLIPPED;
//	lpobj->flags |= OBJECT_NOFILTER;
//	lpobj->flags |= OBJECT_USEMATRIX;
//	lpobj->flags |= OBJECT_NOBACKFACE;	// Test purposes

	lpobj->flag2 |= OBJECT_HASALPHA;

	lpobj->layer = lpmain->layer + 1;
	lpobj->texid = texid;

	lpobj->minlod = 0.0f;
	lpobj->maxlod = 200.0f * ScaleFactor;
	lpobj->maxlod *= lpobj->maxlod;

	lpobj->obox[0].x =  FLT_MAX;
	lpobj->obox[1].x = -FLT_MAX;
	lpobj->obox[0].y =  FLT_MAX;
	lpobj->obox[1].y = -FLT_MAX;
	lpobj->obox[0].z =  FLT_MAX;
	lpobj->obox[1].z = -FLT_MAX;
	if (!ComputeShadowOBox(lpmain->child, lpobj))
	{
		return(FALSE);
	}

	lpobj->bbox[0] = lpobj->obox[0];
	lpobj->bbox[1] = lpobj->obox[1];

	lpmain->shadow = lpobj;

	// Insure shadow object is hidden
	if (!obmainClearFlagsInThisTree(lpmain, OBJECT_VISIBLE))
	{
		return(FALSE);
	}

	obmainMoveToNotObjectTree(lpmain);

#if USE_STATESET
    lpobj->material = new RenderStateSet;
    lpobj->material->SetRenderState(new Material(lpobj->matid));
    lpobj->material->SetRenderState(new TextureUnit(0, texid));
#else
    lpobj->material = new oglMaterial;
    lpobj->material->SetMaterial(lpobj->matid);
    lpobj->material->AddTexture(0, texid);
#endif

	return(TRUE);
}

static LOGICAL IterateShadow(OBJECT* lpmain, OBJECT* lpshad, LOGICAL calclv, piVECTOR* lp, piVECTOR* lv, UINT32 color, piSURFACEDESC* lppisd)
{
LOGICAL			retval=TRUE, usemat=FALSE;
int				i, numtris;
int				v1, v2, v3;
UINT32			alpha;
float			d;
float			xmn, xmx;
float			ymn, ymx;
float			zmn, zmx;
float			dx, dy, dz;
OBJECT* lpatt, *lpobj;
piVECTOR		vmn, vmx, sv[3];
piVECTOR		psl[3];
piVECTOR const*		psnorms;
piINDICES*		ptris;
piMATRIX*		lpmat;

	vmn = lpshad->obox[0];
	vmx = lpshad->obox[1];
	xmn = vmn.x;
	xmx = vmx.x;
	ymn = vmn.y;
	ymx = vmx.y;
	zmn = vmn.z;
	zmx = vmx.z;

	dx = xmx - xmn;
	dy = ymx - ymn;
	dz = zmx - zmn;

	lpobj = lpmain;
	while (lpobj != nullptr)
	{
		if (lpobj->sibling != nullptr)
		{
			if (!IterateShadow(lpobj->sibling, lpshad, calclv, lp, lv, color, lppisd))
			{
				return(FALSE);
			}
		}

		if (lpobj->flags & OBJECT_NOTOBJECT)
		{
			lpobj = lpobj->child;
			continue;
		}

		lpatt = lpobj;
		while (lpatt != nullptr)
		{
			piVectorConstPtr ipos = lpatt->ImmutablePositions();
			piVertexColorConstPtr ic = lpatt->ImmutableColors();
			numtris		= lpatt->numtris;
			ptris		= lpatt->ptris;
			psnorms		= lpatt->psnorms;

			if(!psnorms)
				break;

			if (lpatt->flags & OBJECT_USEMATRIX)
			{
				usemat = TRUE;
				lpmat = &lpatt->mat;
			}
			else
			{
				usemat = FALSE;
			}

			for (i=0; i < numtris; i++, psnorms++)
			{
				v1 = ptris[i].v1;
				v2 = ptris[i].v2;
				v3 = ptris[i].v3;

				if (calclv)
				{
					math3dSubtractVectors(lv, lp, &ipos[v1]);
					math3dFastNormalize(lv);
				}

				d = -math3dDotProduct(lv, psnorms);
				if (d <= 0)
				{
					continue;
				}

				alpha = ic[v1].color & colorALPHA;
				if (alpha == 0)
				{
					continue;
				}

				if (usemat)
				{
					math3dMultiplyRotation(&psl[0], &ipos[v1], lpmat);
					math3dMultiplyRotation(&psl[1], &ipos[v2], lpmat);
					math3dMultiplyRotation(&psl[2], &ipos[v3], lpmat);
				}
				else
				{
					psl[0] = ipos[v1];
					psl[1] = ipos[v2];
					psl[2] = ipos[v3];
				}

				sv[0].x = (float)FloatToInt(shadow_start_x + ((psl[0].x - xmn) / (dx) * shadow_width));
				sv[0].y = (float)FloatToInt(shadow_start_y + ((psl[0].y - ymn) / (dy) * shadow_height));

				sv[1].x = (float)FloatToInt(shadow_start_x + ((psl[1].x - xmn) / (dx) * shadow_width));
				sv[1].y = (float)FloatToInt(shadow_start_y + ((psl[1].y - ymn) / (dy) * shadow_height));

				sv[2].x = (float)FloatToInt(shadow_start_x + ((psl[2].x - xmn) / (dx) * shadow_width));
				sv[2].y = (float)FloatToInt(shadow_start_y + ((psl[2].y - ymn) / (dy) * shadow_height));

				if (!FillTriangle(lppisd, sv, color))
				{
					retval = FALSE;
					break;
				}
			}

			lpatt = lpatt->attached;
		}

		lpobj = lpobj->child;
		//    break;
	}

	return(TRUE);
}

LOGICAL obdshadProject(OBJECT* lpmain)
{
LOGICAL			retval=TRUE, calclv;//, usemat=FALSE;
int				shadid;
UINT32			color;
OBJECT*		lpshad;
piVECTOR		lp, lv;
piSURFACEDESC	pisd;
LIGHT*			lplite;

	if (shadow_texture_size == 0)
	{
		// shadows are off
		return(TRUE);
	}

	if (lpmain == nullptr)
	{
		return(FALSE);
	}

	lplite = obdliteFindDynaLight("sun");
	if (lplite == nullptr)
	{
		// No sun, no light, no error
		return(TRUE);
	}

	// no origin
	if (lplite->light.dltType == piLIGHT_DIRECTIONAL)
	{
		lv.x = lplite->light.dvDirection.x;
		lv.y = lplite->light.dvDirection.y;
		lv.z = lplite->light.dvDirection.z;
		calclv = FALSE;
	}
	else
	{
		lp.x = lplite->light.dvPosition.x;
		lp.y = lplite->light.dvPosition.y;
		lp.z = lplite->light.dvPosition.z;
		calclv = TRUE;
	}

	lpshad = lpmain->shadow;
	shadid = lpshad->texid;

	if (!piIsTexture(shadid, 0))
	{
		return(FALSE);
	}

	if (!ldtexOpenWork(shadid, 0, &pisd))
	{
		return(FALSE);
	}

    if (!ldtexFillSurface(&pisd, colorBLACK))
	{
		return(FALSE);
	}

	color = piColorToDescColor(&pisd, colorBLACKALPHA);

	if (!IterateShadow(lpmain->child, lpshad, calclv, &lp, &lv, color, &pisd))
	{
		retval = FALSE;
	}

	if (!ldtexCloseWork(shadid, 0))
	{
		return(FALSE);
	}

	return(retval);
}


LOGICAL obdshadPosition(OBJECT* lpmain, OBJECT* lpobj)
{
SINT32			i;
UINT32			color;
FLOAT32			distance;
piVECTOR		oldpos, newpos, pos[4];
piVECTOR*		ob;
piVECTOR		pp, op;
OBJECT*		lpshad;
piINDICES*		ptris;
piMATRIX		mat;

	if (shadow_texture_size == 0)
	{
		// shadows are off
		return(TRUE);
	}

	lpshad		= lpobj->shadow;
/*
	while (lpmain->flags & OBJECT_NOTOBJECT)
	{
		lpmain = lpmain->child;
	}
*/
	lpshad->flags &= ~OBJECT_SKIPDRAWING;
	lpshad->mat	= lpmain->mat;

	// Check object distance from player and skip creation/draw if too far
	pp.x = -playpos.x;
	pp.y = -playpos.y;
	pp.z = -playpos.z;//0.0f

	op = lpmain->pos;
	math3dVectorMultiply(&op, &op, &lpmain->mat);
	//op.z = 0.0f;

	math3dSubtractVectors(&op, &pp, &op);
	distance = math3dDotProduct(&op, &op);
	if (distance > lpshad->maxlod)
	{
		lpshad->flags |= OBJECT_SKIPDRAWING;
		return(FALSE);
	}

	// Object is in range, so create shadow
	// For Granny object shadows
	if (0)
	//if (lpmain->flag2 & OBJECT_ANIMATION)
	{
		ob = lpmain->child->bbox;
		ob[0].z = 0;
		ob[1].z = 0;
		//ob = lpshad->obox;

		lpshad->mat = lpmain->child->mat;
	}
	else
	{
		ob = lpshad->obox;
	}

	piVectorPtr spos = lpshad->StaticPositions();
	piVertexColorPtr sc = lpshad->StaticColors();
	piTexCoordPtr stex = lpshad->StaticTexCoords();

	if(spos.ptr == nullptr) return FALSE;

	ptris		= lpshad->ptris;

	ptris->v1 = 0;
	ptris->v2 = 1;
	ptris->v3 = 2;

	ptris += 1;
	ptris->v1 = 0;
	ptris->v2 = 2;
	ptris->v3 = 3;

	mat = IdentityMatrix;
	mat._41 = lpshad->mat._41;
	mat._42 = lpshad->mat._42;
	mat._43 = lpshad->mat._43;

	pos[0].x = ob[0].x;
	pos[0].y = ob[0].y;
	pos[0].z = ob[1].z;
	math3dVectorMultiply(&pos[0], &pos[0], &mat);

	pos[1].x = ob[0].x;
	pos[1].y = ob[1].y;
	pos[1].z = ob[1].z;
	math3dVectorMultiply(&pos[1], &pos[1], &mat);

	pos[2].x = ob[1].x;
	pos[2].y = ob[1].y;
	pos[2].z = ob[1].z;
	math3dVectorMultiply(&pos[2], &pos[2], &mat);

	pos[3].x = ob[1].x;
	pos[3].y = ob[0].y;
	pos[3].z = ob[1].z;
	math3dVectorMultiply(&pos[3], &pos[3], &mat);

	lpshad->pos.x = (pos[0].x + pos[1].x + pos[2].x + pos[3].x) / 4.0f;
	lpshad->pos.y = (pos[0].y + pos[1].y + pos[2].y + pos[3].y) / 4.0f;
	lpshad->pos.z = (pos[0].z + pos[1].z + pos[2].z + pos[3].z) / 4.0f;

	color = colorBLACK;
	for (i=0; i < 4; i++, ++stex, ++spos, ++sc)
	{
		switch (i)
		{
		case 0:
			stex->u = (FLOAT32)shadow_start_x / (FLOAT32)shadow_texture_size;
			stex->v = (FLOAT32)shadow_start_y / (FLOAT32)shadow_texture_size;
			break;
		case 1:
			stex->u = (FLOAT32)shadow_start_x / (FLOAT32)shadow_texture_size;
			stex->v = (FLOAT32)shadow_finish_y / (FLOAT32)shadow_texture_size;
			break;
		case 2:
			stex->u = (FLOAT32)shadow_finish_x / (FLOAT32)shadow_texture_size;
			stex->v = (FLOAT32)shadow_finish_y / (FLOAT32)shadow_texture_size;
			break;
		case 3:
			stex->u = (FLOAT32)shadow_finish_x / (FLOAT32)shadow_texture_size;
			stex->v = (FLOAT32)shadow_start_x / (FLOAT32)shadow_texture_size;
			break;
		}

		oldpos.x = pos[i].x;
		oldpos.y = pos[i].y;
//		oldpos.z = pos[i].z - 5000.0f * ScaleFactor;
		oldpos.z = pos[i].z - 2.0f * ScaleFactor;
		newpos.x = pos[i].x;
		newpos.y = pos[i].y;
		newpos.z = pos[i].z + 5000.0f * ScaleFactor;

        trncdIgnoreTreeCollide(true);

		if (!trncdCollideDownMediumList(&oldpos, &newpos))
		//if (!trncdObjectToTerrain(nullptr, &oldpos, &newpos))
		{
			lpshad->flags |= OBJECT_SKIPDRAWING;
			return(FALSE);
		}

        trncdIgnoreTreeCollide(false);

		spos->x = pos[i].x;
		spos->y = pos[i].y;
		spos->z = newpos.z - 0.1f * ScaleFactor;

        // Ease z-fighting. 0.015 arbitrary.
        spos->z -= 0.015f;

		math3dSubtractVectors(&op, &pp, &newpos);
		distance = math3dDotProduct(&op, &op);
		if (distance > lpshad->maxlod)
		{
			lpshad->flags |= OBJECT_SKIPDRAWING;
			return(FALSE);
		}

		color = piRGBA_SETALPHA(color, (UINT32)((((lpshad->maxlod - distance) / lpshad->maxlod) * 0.25f) * sundot));
		sc->color = color;
		sc->specular = colorALPHA;
	}

	return(TRUE);
}


LOGICAL obdshadUpdate()
{
    // This routine if totally screwed up. Should be completely removed.

    SINT32		shadcnt;
    OBJECT* lpobj, *lpgrp, *lpshad;
    LIGHT*		sun;
    piVECTOR	up;

#ifdef EVENT_LOG_TIMES
    static UINT64 count = 0;
    static int number = 0;

    UINT64 beginhere = GetMicroSeconds();
#endif

    // This shadow routine is a framerate killer.
    if(gCustomTerrain)
    {
        return true;
    }

    if(ShadowSizeSelect == 0)
	{
		// shadows are off
		return(TRUE);
	}

	sun = obdliteFindDynaLight("sun");
	if (sun == nullptr)
	{
		return(TRUE);
	}

	up.x = 0.0f;
	up.y = 0.0f;
	up.z = -1.0f;
	sundot = -math3dDotProduct(&sun->light.dvDirection,&up);
	if (sundot <= 0.17364817766693034885171662676931f)	// <= 10 degrees, no shadow
	{
		return(TRUE);
	}

	// Shadow decreases intensity at lower sun angles
	sundot *= 255.0f;

	shadcnt = 0;
	lpobj = obmainGetFirstTopmostObject();
	if (lpobj == nullptr)
	{
		// No objects, no error
		return(TRUE);
	}

    for(; lpobj !=  nullptr; lpobj = obmainGetNextTopmostObject(lpobj))
    {
		if ((lpobj->flag2 & OBJECT_IS_PARTOF_VEHICLE) &&
			(!(lpobj->flags & OBJECT_TERRAIN)) &&
			(!(lpobj->flag2 & OBJECT_TERRAIN_OBJECT)) &&
			!(lpobj->flag2 & OBJECT_HIDDEN))
		{
			lpgrp = obmainFindChildFromObject(lpobj, "shadow");

            if(lpgrp && lpgrp->shadow)
			{
				shadcnt += 1;

				lpshad = lpgrp->shadow;
				lpshad->obox[0].x =  FLT_MAX;
				lpshad->obox[1].x = -FLT_MAX;
				lpshad->obox[0].y =  FLT_MAX;
				lpshad->obox[1].y = -FLT_MAX;
				lpshad->obox[0].z =  FLT_MAX;
				lpshad->obox[1].z = -FLT_MAX;

				if (!ComputeShadowOBox(lpgrp->child, lpshad))
				{
					Write_Debug("Could not update obox of shadow object");
					return(FALSE);
				}

				lpshad->bbox[0] = lpshad->obox[0];
				lpshad->bbox[1] = lpshad->obox[1];

				if (!obdshadPosition(lpobj, lpgrp))
				{
					// Shadow is off
					continue;
				}

				if (!obdshadProject(lpgrp))
				{
					Write_Debug("Could not update shadow object");
					return(FALSE);
				}
			}
		}
    }

#ifdef EVENT_LOG_TIMES
    count += GetMicroSeconds() - beginhere;
    if ((number += shadcnt) >= 1000)
    {
        Write_Debug("Dynamic shadow time: %lu us", count);
        number = 0;
        count = 0;
    }
#endif

	return(TRUE);
}

#endif // USE_SHADOWS
