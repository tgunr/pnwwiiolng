#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldmdl.h"
#include "ob/obmain.h"
#include "ob/obplyr.h"
#include "math/math3d.h"
#include "ogl/oglMesh.h"
#include "ob/obMgr.h"


LOGICAL obplyrApplyMatrixToObjectTree(OBJECT* lpobj, piMATRIX* mat)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->sibling)
		{
			if (!obplyrApplyMatrixToObjectTree(lpobj->sibling, mat))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			math3dMatrixMultiply(&lpatt->mat, mat, &lpatt->mat);
			lpatt = lpatt->attached;
		}

		math3dMatrixMultiply(&lpobj->mat, mat, &lpobj->mat);
		lpobj = lpobj->child;
	}

	return(TRUE);
}


LOGICAL obplyrTranslateObjectTree(OBJECT* lpobj, const piVECTOR& transVec)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->sibling)
		{
			if (!obplyrTranslateObjectTree(lpobj->sibling, transVec))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			math3dMatrixTranslate(&lpatt->mat, transVec);
			lpatt = lpatt->attached;
		}

		math3dMatrixTranslate(&lpobj->mat, transVec);
		lpobj = lpobj->child;
	}

	return(TRUE);
}

LOGICAL obplyrRotateAndTranslateObjectTree(OBJECT* lpobj, const piMATRIX& rotMatrix, const piVECTOR& transVec)
{
	OBJECT*		lpatt;

	while (lpobj)
	{
		if (lpobj->sibling)
		{
			if (!obplyrRotateAndTranslateObjectTree(lpobj->sibling, rotMatrix, transVec))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			math3dMatrixMultiply(&lpatt->mat, &lpatt->mat, &rotMatrix);
			math3dMatrixTranslate(&lpatt->mat, transVec);

			lpatt = lpatt->attached;
		}

		math3dMatrixMultiply(&lpobj->mat, &lpobj->mat, &rotMatrix);
		math3dMatrixTranslate(&lpobj->mat, transVec);

		lpobj = lpobj->child;
	}

	return(TRUE);
}

void obplyrApplyProjectionAndHeight(OBJECT* o, const piMATRIX& mat)
{
	o->mat = mat;

	if(o->attached)	obplyrApplyProjectionAndHeight(o->attached, mat);
	if(o->child)	obplyrApplyProjectionAndHeight(o->child, mat);
	if(o->sibling)	obplyrApplyProjectionAndHeight(o->sibling, mat);
}


void obplyrApplyBaseProjection(OBJECT* o, const piMATRIX& projectionMatrix, FLOAT32 minTerrainHeight, FLOAT32 maxTerrainHeight)
{
	o->mat._43 += minTerrainHeight;

	if(o->comment && strstr(o->comment, "base"))
	{
		if(o->numverts)
		{
			piVectorPtr				mutableVerts = o->MutablePositions();

			for(int i=0; i < o->numverts; i++, ++mutableVerts)
			{
				if(mutableVerts->z >= -0.001)		// This should pick the bottom vertices of the base
				{
					math3dVectorMultiply(mutableVerts.ptr, mutableVerts.ptr, &projectionMatrix);
				}
			}

			FLOAT32	oldMinZ = o->bbox[0].z + o->pos.z;
			FLOAT32	minTerrainHeightAbsolute = minTerrainHeight - o->mat._43;
			FLOAT32	maxTerrainHeightAbsolute = maxTerrainHeight - o->mat._43;

			// Get the new center height
			o->pos.z = -o->mat._43 + maxTerrainHeightAbsolute - (maxTerrainHeightAbsolute - minTerrainHeightAbsolute) * 0.5f;

			o->bbox[0].z = oldMinZ - o->pos.z;
			o->bbox[1].z = maxTerrainHeightAbsolute - o->pos.z;

			o->maxrad = o->bbox[0].x * o->bbox[0].x + o->bbox[0].y * o->bbox[0].y + o->bbox[0].z * o->bbox[0].z;
			o->minrad = o->bbox[1].x * o->bbox[1].x + o->bbox[1].y * o->bbox[1].y + o->bbox[1].z * o->bbox[1].z;

			if(o->flags & OBJECT_COLLISION)	ldmdlCalcNormals2(o);		// A projection should be implemented for this too
		}
	}

	if(o->attached)	obplyrApplyBaseProjection(o->attached, projectionMatrix, minTerrainHeight, maxTerrainHeight);
	if(o->child)	obplyrApplyBaseProjection(o->child, projectionMatrix, minTerrainHeight, maxTerrainHeight);
	if(o->sibling)	obplyrApplyBaseProjection(o->sibling, projectionMatrix, minTerrainHeight, maxTerrainHeight);
}


void obplyrApplyProjectionAndHeightToBase(OBJECT* o, const piMATRIX* mat)
{
	static	piMATRIX		inverseMatrix;


	if(o->comment && strstr(o->comment, "base"))
	{
		if(o->meshContext)	o->meshContext->SetModelViewMatrix(mat);
		if(o->numverts)
		{//This (and much more) doesn't have to be done with the new collision code
			piVectorPtr		mutableVerts	= o->MutablePositions();

			math3dInvertMatrix(&inverseMatrix, &o->mat);
			for(int i=0; i < o->numverts; i++, ++mutableVerts)
			{
				math3dVectorMultiply(mutableVerts.ptr, mutableVerts.ptr, mat);
				math3dVectorMultiply(mutableVerts.ptr, mutableVerts.ptr, &inverseMatrix);
			}
		}
	}

	if(o->attached)	obplyrApplyProjectionAndHeightToBase(o->attached, mat);
	if(o->child)	obplyrApplyProjectionAndHeightToBase(o->child, mat);
	if(o->sibling)	obplyrApplyProjectionAndHeightToBase(o->sibling, mat);
}

/*

LOGICAL obplyrApplyMatrixToObjectNormals(OBJECT* lpmain, piMATRIX* mat)
{
int				i;
piVECTOR		n, cn;

	if (lpmain == nullptr)
	{
		return(FALSE);
	}

	if (mat == nullptr)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != nullptr)
	{
		if (lpobj->sibling != nullptr)
		{
			obplyrApplyMatrixToObjectNormals(lpobj->sibling, mat);
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt != nullptr)
		{
			piVectorPtr mnorm = lpatt->MutableNormals();
			pn	= lpatt->psnorms;
			pcn	= lpatt->pscdn;

			for (i=lpatt->numtris; i; i--, pn++, ++mnorm)
			{
				// rotate surface normals
				n.x = mnorm->x * mat->_11 + mnorm->y * mat->_21 + mnorm->z * mat->_31;
				n.y = mnorm->x * mat->_12 + mnorm->y * mat->_22 + mnorm->z * mat->_32;
				n.z = mnorm->x * mat->_13 + mnorm->y * mat->_23 + mnorm->z * mat->_33;
				*mnorm = n;

				// rotate lighting normals
				n.x = pn->x * mat->_11 + pn->y * mat->_21 + pn->z * mat->_31;
				n.y = pn->x * mat->_12 + pn->y * mat->_22 + pn->z * mat->_32;
				n.z = pn->x * mat->_13 + pn->y * mat->_23 + pn->z * mat->_33;
				*pn = n;

				// rotate collision normals
				cn.x = pcn->x * mat->_11 + pcn->y * mat->_21 + pcn->z * mat->_31;
				cn.y = pcn->x * mat->_12 + pcn->y * mat->_22 + pcn->z * mat->_32;
				cn.z = pcn->x * mat->_13 + pcn->y * mat->_23 + pcn->z * mat->_33;
				*pcn++ = cn;

				cn.x = pcn->x * mat->_11 + pcn->y * mat->_21 + pcn->z * mat->_31;
				cn.y = pcn->x * mat->_12 + pcn->y * mat->_22 + pcn->z * mat->_32;
				cn.z = pcn->x * mat->_13 + pcn->y * mat->_23 + pcn->z * mat->_33;
				*pcn++ = cn;

				cn.x = pcn->x * mat->_11 + pcn->y * mat->_21 + pcn->z * mat->_31;
				cn.y = pcn->x * mat->_12 + pcn->y * mat->_22 + pcn->z * mat->_32;
				cn.z = pcn->x * mat->_13 + pcn->y * mat->_23 + pcn->z * mat->_33;
				*pcn++ = cn;
			}

			lpatt = lpatt->attached;
		}

		lpobj = lpobj->child;
	}

	return(TRUE);
}*/

LOGICAL obplyrApplyMatrixToObject(OBJECT* lpobj, piMATRIX* inmat)
{
int				i;
float			minrad, maxrad;
OBJECT*		lpmain;
piVECTOR*		pno;
piVECTOR*		pn;
piVECTOR*		pcon;
piVECTOR*		pcn;
piMATRIX		mat;
piVECTOR		obox[2];

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	if (inmat == nullptr)
	{
		return(FALSE);
	}

	mat = *inmat;


	if (lpobj->flags & OBJECT_USEMATRIX)
	{
		lpmain = lpobj;
		do
		{
			lpobj->mat = mat;
			lpobj = lpobj->attached;
		}
		while (lpobj);
	}
	else
	{
		lpmain = lpobj;
		do
		{
			lpobj->mat = mat;

			lpobj->pos.x = mat._41;
			lpobj->pos.y = mat._42;
			lpobj->pos.z = mat._43;

			piVectorPtr spos  = lpobj->StaticPositions();
			piVectorPtr snorm = lpobj->StaticNormals();
			if(spos.ptr == nullptr) return FALSE;

			for (i=lpobj->numverts; i; i--, ++spos, ++snorm)
			{
				piVECTOR v;
				// rotate and translate all vertices
				v.x = spos->x * mat._11 + spos->y * mat._21 + spos->z * mat._31 + mat._41;
				v.y = spos->x * mat._12 + spos->y * mat._22 + spos->z * mat._32 + mat._42;
				v.z = spos->x * mat._13 + spos->y * mat._23 + spos->z * mat._33 + mat._43;
				*spos = v;

				// rotate vertex lighting normal
				v.x = snorm->x * mat._11 + snorm->y * mat._21 + snorm->z * mat._31;
				v.y = snorm->x * mat._12 + snorm->y * mat._22 + snorm->z * mat._32;
				v.z = snorm->x * mat._13 + snorm->y * mat._23 + snorm->z * mat._33;
				*snorm = v;
			}

			lpobj->SyncStaticData();
			lpobj->SyncMutableData();

			pno   = lpobj->psnorig;
			pn    = lpobj->psnorms;
			pcon  = lpobj->pscdon;
			pcn   = lpobj->pscdn;

			for (i=lpobj->numtris; i; i--, pno++, pn++)
			{
				// rotate surface normal
				pn->x = pno->x * mat._11 + pno->y * mat._21 + pno->z * mat._31;
				pn->y = pno->x * mat._12 + pno->y * mat._22 + pno->z * mat._32;
				pn->z = pno->x * mat._13 + pno->y * mat._23 + pno->z * mat._33;

				// rotate collision normals
				pcn->x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
				pcn->y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
				pcn->z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
				pcn += 1;
				pcon += 1;

				pcn->x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
				pcn->y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
				pcn->z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
				pcn += 1;
				pcon += 1;

				pcn->x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
				pcn->y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
				pcn->z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
				pcn += 1;
				pcon += 1;
			}

			ldmdlGetBounds(lpobj);

			lpobj = lpobj->attached;
		}
		while (lpobj);

		ldmdlGetObjectBounds(lpmain, &lpmain->loc, obox, &minrad, &maxrad);
		ldmdlUpdateLocation(lpmain);
	}


	return(TRUE);
}

// The pos and rot are delta values (change)
// does NOT affect the origin vertices and normals
LOGICAL obplyrUpdateThisObject(OBJECT* lpobj, piVECTOR* pos, piVECTOR* rot)
{
piMATRIX mat;

	mat = IdentityMatrix;
	math3dMatrixRotate(&mat, rot);

	mat._41 = pos->x;
	mat._42 = pos->y;
	mat._43 = pos->z;

	obplyrApplyMatrixToObject(lpobj, &mat);

	return(TRUE);
}

// The rot is a delta value (change)
// it DOES affect the origin vertices and normals
/*LOGICAL obplyrRotateRawObject(OBJECT* lpobj, piVECTOR* rot)
{
int			i;
float		x, y, z, minrad, maxrad;
OBJECT*    lpmain;
piVECTOR	vec, normal;
piVECTOR*  pno, pn, pcon, pcn;
piVERTEX*  pvo, pv;
piLVERTEX* psl, pdl;
piMATRIX	mat;
piVECTOR    loc;
piVECTOR    obox[2];
//BBOX		obox;
#ifdef EVENT_LOG_TIMES
LARGE_INTEGER count1, count2;
static __int64    count=0;
static int        number=0;
#endif

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

#ifdef EVENT_LOG_TIMES
	if (!QueryPerformanceCounter(&count1))
	{
	}
#endif

	lpmain = lpobj;
	do
	{
		mat = IdentityMatrix;
		math3dMatrixRotate(&mat, rot);

		x = lpobj->loc.x;
		y = lpobj->loc.y;
		z = lpobj->loc.z;

		pvo = lpobj->pvorig;
		pv  = lpobj->pverts;
		psl = lpobj->pslverts;
		pdl = lpobj->pdlverts;

		for (i=lpobj->numverts; i; i--, pvo++, pv++, psl++, pdl++)
		{
			// rotate and translate all vertices
			vec.x = (pvo->x - x) * mat._11 + (pvo->y - y) * mat._21 + (pvo->z - z) * mat._31 + x;
			vec.y = (pvo->x - x) * mat._12 + (pvo->y - y) * mat._22 + (pvo->z - z) * mat._32 + y;
			vec.z = (pvo->x - x) * mat._13 + (pvo->y - y) * mat._23 + (pvo->z - z) * mat._33 + z;

			pvo->x = pv->x = psl->x = pdl->x = vec.x;
			pvo->y = pv->y = psl->y = pdl->y = vec.y;
			pvo->z = pv->z = psl->z = pdl->z = vec.z;

			// rotate vertex lighting normal
			normal.x = pvo->nx * mat._11 + pvo->ny * mat._21 + pvo->nz * mat._31;
			normal.y = pvo->nx * mat._12 + pvo->ny * mat._22 + pvo->nz * mat._32;
			normal.z = pvo->nx * mat._13 + pvo->ny * mat._23 + pvo->nz * mat._33;

			pvo->nx = pv->nx = normal.x;
			pvo->ny = pv->ny = normal.y;
			pvo->nz = pv->nz = normal.z;
		}

		pno   = lpobj->psnorig;
		pn    = lpobj->psnorms;
		pcon  = lpobj->pscdon;
		pcn   = lpobj->pscdn;

		for (i=lpobj->numtris; i; i--, pno++, pn++)
		{
			// rotate surface normal
			normal.x = pno->x * mat._11 + pno->y * mat._21 + pno->z * mat._31;
			normal.y = pno->x * mat._12 + pno->y * mat._22 + pno->z * mat._32;
			normal.z = pno->x * mat._13 + pno->y * mat._23 + pno->z * mat._33;

			pno->x = pn->x = normal.x;
			pno->y = pn->y = normal.y;
			pno->z = pn->z = normal.z;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;
		}

		ldmdlGetBounds(lpobj);

		lpobj = (OBJECT*)lpobj->attached;
	}
	while (lpobj);

	ldmdlGetObjectBounds(lpmain, &loc, obox, &minrad, &maxrad);
	//lpmain->loc = loc;
	lpmain->obox[0] = obox[0];
	lpmain->obox[1] = obox[1];

#ifdef EVENT_LOG_TIMES
	if (QueryPerformanceCounter(&count2))
	{
		count += count2.QuadPart - count1.QuadPart;
		if (++number == 4096)
		{
			QueryPerformanceFrequency(&count2);
			WriteDebug("Average rotate raw time: %d us", (DWORD)(count * 1000 / 4096) / (DWORD)(count2.LowPart / 1000));
			number = 0;
			count = 0;
		}
	}
#endif

  return(TRUE);
}*/

// The pos and rot are delta values (change)
// changes the origin vertices and normals
LOGICAL obplyrUpdateObject(OBJECT* lpobj, piVECTOR* pos, piVECTOR* rot)
{
return FALSE;
}
/*int			i;
OBJECT*    lpmain;
piVECTOR	vec, normal;
piVECTOR*  pno, pn, pcon, pcn;
piMATRIX	mat;

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	mat = IdentityMatrix;
	math3dMatrixRotate(&mat, rot);
	mat._41 = pos->x;
	mat._42 = pos->y;
	mat._43 = pos->z;

	lpmain = lpobj;
	do
	{
		lpobj->mat = mat;

		pvo = lpobj->pvorig;
		pv  = lpobj->pverts;
		psl = lpobj->pslverts;
		pdl = lpobj->pdlverts;

		for (i=lpobj->numverts; i; i--, pvo++, pv++, psl++, pdl++)
		{
			// rotate and translate all vertices
			vec.x = pvo->x * mat._11 + pvo->y * mat._21 + pvo->z * mat._31 + mat._41;
			vec.y = pvo->x * mat._12 + pvo->y * mat._22 + pvo->z * mat._32 + mat._42;
			vec.z = pvo->x * mat._13 + pvo->y * mat._23 + pvo->z * mat._33 + mat._43;

			pvo->x = pv->x = psl->x = pdl->x = vec.x;
			pvo->y = pv->y = psl->y = pdl->y = vec.y;
			pvo->z = pv->z = psl->z = pdl->z = vec.z;

			// rotate vertex lighting normal
			normal.x = pvo->nx * mat._11 + pvo->ny * mat._21 + pvo->nz * mat._31;
			normal.y = pvo->nx * mat._12 + pvo->ny * mat._22 + pvo->nz * mat._32;
			normal.z = pvo->nx * mat._13 + pvo->ny * mat._23 + pvo->nz * mat._33;

			pvo->nx = pv->nx = normal.x;
			pvo->ny = pv->ny = normal.y;
			pvo->nz = pv->nz = normal.z;
		}

		pno   = lpobj->psnorig;
		pn    = lpobj->psnorms;
		pcon  = lpobj->pscdon;
		pcn   = lpobj->pscdn;

		for (i=lpobj->numtris; i; i--, pno++, pn++)
		{
			// rotate surface normal
			normal.x = pno->x * mat._11 + pno->y * mat._21 + pno->z * mat._31;
			normal.y = pno->x * mat._12 + pno->y * mat._22 + pno->z * mat._32;
			normal.z = pno->x * mat._13 + pno->y * mat._23 + pno->z * mat._33;

			pno->x = pn->x = normal.x;
			pno->y = pn->y = normal.y;
			pno->z = pn->z = normal.z;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;

			normal.x = pcon->x * mat._11 + pcon->y * mat._21 + pcon->z * mat._31;
			normal.y = pcon->x * mat._12 + pcon->y * mat._22 + pcon->z * mat._32;
			normal.z = pcon->x * mat._13 + pcon->y * mat._23 + pcon->z * mat._33;
			*pcon++ = *pcn++ = normal;
		}

		ldmdlGetBounds(lpobj);

		lpobj = (OBJECT*)lpobj->attached;
	}
	while (lpobj);

	ldmdlGetObjectBounds(lpmain, &lpmain->loc, lpmain->obox, &lpmain->minrad, &lpmain->maxrad);
	ldmdlUpdateLocation(lpmain);

	return(TRUE);
}*/

// The pos and rot are delta values (change)
/*LOGICAL obplyrUpdateNamedObject(char* name, piVECTOR* pos, piVECTOR* rot)
{
piVECTOR	objpos, objrot;
OBJECT*	lpobj;

	lpobj = obmainFindNamedObject(name);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	objpos = *pos;
	objrot = *rot;

	return(obplyrUpdateObject(lpobj, &objpos, &objrot));
}
*/

LOGICAL obplyrOffsetObject(OBJECT* lpmain, piVECTOR* loc)
{
return FALSE;
/*
int         i;
float       ax, ay, az;
piVERTEX*	pvo, pv;
piLVERTEX*	pdl, psl;
OBJECT*	lpobj;

	if (lpmain == nullptr)
	{
		return(FALSE);
	}

	lpobj = lpmain;

	ax = loc->x;
	ay = loc->y;
	az = loc->z;
	do
	{
		lpobj->loc.x += loc->x;
		lpobj->loc.y += loc->y;
		lpobj->loc.z += loc->z;
		lpobj->pos.x += loc->x;
		lpobj->pos.y += loc->y;
		lpobj->pos.z += loc->z;
		pvo = lpobj->pvorig;
		pv = lpobj->pverts;
		psl = lpobj->pslverts;
		pdl = lpobj->pdlverts;
		for (i=0; i < lpobj->numverts; i++)
		{
			pvo[i].x -= ax;
			pvo[i].y -= ay;
			pvo[i].z -= az;

			pv[i].x -= ax;
			pv[i].y -= ay;
			pv[i].z -= az;

			psl[i].x -= ax;
			psl[i].y -= ay;
			psl[i].z -= az;

			pdl[i].x -= ax;
			pdl[i].y -= ay;
			pdl[i].z -= az;
		}

		ldmdlGetBounds(lpobj);

		lpobj = lpobj->attached;
	}
	while (lpobj);

	ldmdlGetObjectBounds(lpmain, &lpmain->loc, lpmain->obox, &lpmain->minrad, &lpmain->maxrad);
	ldmdlUpdateLocation(lpmain);

	return(TRUE);*/
}
