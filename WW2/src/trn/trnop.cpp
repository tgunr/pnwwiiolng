#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obpool.h"
#include "ob/obplyr.h"
#include "ogrp/ogrptype.h"
#include "trn/trnmain.h"
#include "trn/trncd.h"
#include "trn/trnop.h"
#include "trn/trnvars.h"
#include "ld/ldmdl.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "draw/draw3d.h"
#include "rn/rnmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "wwii/wwiiCovert.h"
#include "trn/trngrass.h"
#include "spt/sptmain.h"
#include "math/mathnum.h"
#include "fx/fxUtl.h"
#include <assert.h>
#include "ob/obMgr.h"
#include "list/list.h"
#include "math/matheul.h"
#include "math/Matrixf.h"
#include "math/Matrixd.h"
#include "spt/Tree.h"
#include "scen/ScenarioManager.h"
#include "trn/trnScenario.h"

#ifdef BETA_RELEASE
#include "ui/uiChat.h"
#endif

#define VECTOR_EPSILON	(0.01f * ScaleFactor)	// 100th of a meter

static LOGICAL _UpdateGroupNormals(OBJECT * Object,UINT32 DummyNum);
static LOGICAL _UpdateBaseGroupNormals(OBJECT * Object);

static UINT32 _RecalcBoundsAndNormals(OBJECT * Object);
static UINT32 _ParallelagramizeObjectToTriangle(OBJECT * Object,
												const piVECTOR * PlanePoint,
												const piVECTOR * Normal);
static UINT32 _ParallelagramizeObject(OBJECT * Object,
									  const piVECTOR * PlanePoint,
									  const piVECTOR * Normal);
static UINT32 _ParallelagramizeSubObject(OBJECT * Object,
									  const piVECTOR * PlanePoint,
									  const piVECTOR * Normal);
static LOGICAL _CollideDownSpecial(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly);

static SINT32 _ParseSpeedTreeComment(SCHAR* pComment, SINT32 iTreeValues[]);

static LOGICAL HeightConformLOD(OBJECT* lpobj, UINT32 lod);
static LOGICAL HeightPlaceLOD(OBJECT* lpobj, UINT32 lod);

// This object pointer is used during object placement incase a sub-object
// operation requires knowledge of the topmost parent object information (i.e. name)
static OBJECT*	lpobjTopmost;

static SINT32 _superCell = 0;

#if defined(BETA_RELEASE)
static LOGICAL g_bLoadTrees = TRUE;
#endif


LOGICAL trnopCollideDownWithDTL(piVECTOR* retpos,
                                piVECTOR* pos,
                                SINT32* poly,
                                SINT32 iDTLIndex)
{
LOGICAL		retval=FALSE;
piVECTOR	oldpos, newpos;
COLLISION	collision;
FLOAT64VECTOR WorldOriginOffset;

	oldpos = newpos = *pos;
	oldpos.z -= 20000.0f * ScaleFactor;
	newpos.z += 20000.0f * ScaleFactor;

	memset(&collision, 0, sizeof(collision));

	collision.oldpos.x = oldpos.x;
	collision.oldpos.y = oldpos.y;
	collision.oldpos.z = oldpos.z;
	collision.newpos.x = newpos.x;
	collision.newpos.y = newpos.y;
	collision.newpos.z = newpos.z;

	retval = trncdCheckTerrainDownWithDTL(&collision, &trndtl[iDTLIndex]);

	if (retval)
	{
		trnGetWorldOriginOffset(&WorldOriginOffset);

		retpos->x = (FLOAT32)(collision.colpos.x - WorldOriginOffset.x);
		retpos->y = (FLOAT32)(collision.colpos.y - WorldOriginOffset.y);
		retpos->z = (FLOAT32)(collision.colpos.z - WorldOriginOffset.z);

        if(poly)
        {
		    *poly = collision.colpoly;
        }
	}
	else
	{
//		Write_Debug("The ground could not be located!");
		retpos->x = oldpos.x;
		retpos->y = oldpos.y;
		retpos->z = 0.0f;
	}

	return(retval);
}

LOGICAL trnopCollideDown(piVECTOR* retpos, SINT32* poly, piVECTOR* pos)
{
LOGICAL		retval=FALSE;
piVECTOR	oldpos, newpos;
COLLISION	collision;
FLOAT64VECTOR WorldOriginOffset;

	oldpos = newpos = *pos;
	oldpos.z -= 20000.0f * ScaleFactor;
	newpos.z += 20000.0f * ScaleFactor;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = oldpos.x;
	collision.oldpos.y = oldpos.y;
	collision.oldpos.z = oldpos.z;
	collision.newpos.x = newpos.x;
	collision.newpos.y = newpos.y;
	collision.newpos.z = newpos.z;

	retval = trncdCheckTerrainDown(&collision);
	if (retval)
	{
		trnGetWorldOriginOffset(&WorldOriginOffset);

		retpos->x = (FLOAT32)(collision.colpos.x - WorldOriginOffset.x);
		retpos->y = (FLOAT32)(collision.colpos.y - WorldOriginOffset.y);
		retpos->z = (FLOAT32)(collision.colpos.z - WorldOriginOffset.z);
		*poly = collision.colpoly;
	}
	else
	{
//		Write_Debug("The ground could not be located!");
		retpos->x = oldpos.x;
		retpos->y = oldpos.y;
		retpos->z = 0.0f;
	}

	return(retval);
}

// special collider for octet replacement tiles
static LOGICAL _CollideDownSpecial(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly)
{
LOGICAL		retval=FALSE;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = oldpos->x;
	collision.oldpos.y = oldpos->y;
	collision.oldpos.z = oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;
	retval = trncdCheckTerrainDown(&collision);
	if (retval)
	{
		*retpos = collision.colpos;
		*lpobj = collision.colobj;
		*poly = collision.colpoly;
	}
	else
	{
		////Write_Debug("The ground could not be located!");
	}

	return(retval);
}


LOGICAL trnopCollide(piVECTOR const* oldpos, piVECTOR const* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly)
{
LOGICAL		retval=FALSE;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = oldpos->x;
	collision.oldpos.y = oldpos->y;
	collision.oldpos.z = oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;

	retval = trncdCheckTerrain(&collision);
	if (retval)
	{
		*retpos = collision.colpos;
		*lpobj = collision.colobj;
		*poly = collision.colpoly;
	}
	else
	{
		////Write_Debug("The ground could not be located!");
	}

	return(retval);
}

LOGICAL trnopConformBase(piVECTOR* rot, OBJECT* lpobj, SINT32 poly)
{
piVECTOR	n;

	if (lpobj == NULL)
	{
		return(FALSE);
	}

	n = lpobj->psnorig[poly];

	rot->x = -(float)asin(n.y);
	rot->y = (float)asin(n.x);
	rot->z = 0.0f;

	return(FALSE);
}

#ifdef SAVE
static LOGICAL OnGroundLODImplement(OBJECT* lpobj)
{
SINT32		poly;
FLOAT32		dz;
piVECTOR	pos, cpos;
#if(0)
piMATRIX	mat;
#endif

	// Rotate/Translate position to object space
	math3dVectorMultiply(&pos, &lpobj->pos, &lpobj->mat);

	if (!trnopCollideDown(&cpos, &poly, &pos))
	{
#ifdef TERRAIN_EDITOR
		Write_Debug("OnGroundLODImplement: Could not find the ground for object '%s'", lpobj->name);
#endif
		return(FALSE);
	}

	dz = cpos.z;

#if(0)
	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = dz;
	math3dMatrixMultiply(&lpobj->mat, &mat, &lpobj->mat);
#endif

    // Only multiply what is necessary here, instead of math3dMatrixMultiply.
    // Go from 64 multiplications to 4.
    lpobj->mat._41 = dz * lpobj->mat._31 + lpobj->mat._41;
    lpobj->mat._42 = dz * lpobj->mat._32 + lpobj->mat._42;
    lpobj->mat._43 = dz * lpobj->mat._33 + lpobj->mat._43;
    lpobj->mat._44 = dz * lpobj->mat._34 + lpobj->mat._44;

	return(TRUE);
}

static LOGICAL OnGroundLODInterate(OBJECT* lpobj)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!OnGroundLODInterate(lpobj->child))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			OnGroundLODImplement(lpatt);
			lpatt = lpatt->attached;
		}

		OnGroundLODImplement(lpobj);
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

#endif

static LOGICAL OnGroundLODImplement(OBJECT* lpobj)
{
SINT32		poly;
FLOAT32		dz;
piVECTOR	pos, cpos;
#if(0)
piMATRIX	mat;
#endif


    if(lpobj->flag2 & OBJECT_IS_ROTATED)
    {
	    // Rotate/Translate position to world space
	    math3dVectorMultiply(&pos, &lpobj->pos, &lpobj->mat);
    }
    else
    {
        pos.x = lpobj->pos.x + lpobj->mat._41;
        pos.y = lpobj->pos.y + lpobj->mat._42;
        pos.z = lpobj->pos.z + lpobj->mat._43;
    }

    if(!trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
	{
#ifdef TERRAIN_EDITOR
		Write_Debug("OnGroundLODImplement: Could not find the ground for object '%s'", lpobj->name);
#endif

		return(FALSE);
	}

	dz = cpos.z;

#if(0)
	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = dz;
	math3dMatrixMultiply(&lpobj->mat, &mat, &lpobj->mat);
#endif

    // Only multiply what is necessary here, instead of math3dMatrixMultiply.
    // Go from 64 multiplications to 4.
    lpobj->mat._41 = dz * lpobj->mat._31 + lpobj->mat._41;
    lpobj->mat._42 = dz * lpobj->mat._32 + lpobj->mat._42;
    lpobj->mat._43 = dz * lpobj->mat._33 + lpobj->mat._43;
    lpobj->mat._44 = dz * lpobj->mat._34 + lpobj->mat._44;

	return(TRUE);
}

static LOGICAL OnGroundLODInterate(OBJECT* lpobj)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!OnGroundLODInterate(lpobj->child))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			OnGroundLODImplement(lpatt);
			lpatt = lpatt->attached;
		}

		OnGroundLODImplement(lpobj);
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

static LOGICAL OnGroundLOD(OBJECT* lpobj, UINT32 lod)
{
#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for on ground", lod);
#endif

	if (lpobj->flags & OBJECT_LOD)
	{
		OnGroundLODInterate(lpobj->child);
	}
	else
	{
		OnGroundLODInterate(lpobj);
	}

	return(TRUE);
}

/*
1	On Ground
	Place the object on the ground using the bottom center of the
	object for positioning. No rotation or other manipulation is
	done.
	Use: for placing trees, rocks or other vertical, baseless
	objects.
*/
static LOGICAL PlaceOnGround(OBJECT* lpobj)
{
int			i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place on ground: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, OnGroundLOD))
		{
//			break;
		}
	}

	return TRUE;
}

/*
2	Relative to Ground
	Place the object without manipulating it to lay on the ground.
	Always place the object so that regardless of the terrain height
	the object will appear at its constructed height.
	Use: for "floating" objects
*/
static LOGICAL PlaceRelativeToGround(OBJECT* lpobj)
{
int		i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place relative to ground: %s", lpobj->name);
#endif
	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, HeightPlaceLOD))
		{
//			break;
		}
	}

	return TRUE;
}

/*
3	Absolute
	Place the object at the height specified without regard to
	terrain height.
	Uses: maybe water objects
*/
static LOGICAL PlaceAbsolute(OBJECT* lpobj)
{
#ifdef TERRAIN_EDITOR
	Write_Debug("Place absolute: %s", lpobj->name);
#endif
	return TRUE;
}

static LOGICAL ApplyMatrixToObjectTree(OBJECT* lpobj, piMATRIX* mat)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!ApplyMatrixToObjectTree(lpobj->child, mat))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			math3dMatrixMultiply(&lpatt->mat, mat, &lpatt->mat);
			////picoCreateImmutableBuffer(lpatt->vibkey.unique, lpatt);
			lpatt = lpatt->attached;
		}

		math3dMatrixMultiply(&lpobj->mat, mat, &lpobj->mat);
		////picoCreateImmutableBuffer(lpobj->vibkey.unique, lpobj);
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

/*
Place 2
Relative to ground - Place the object without manipulating it to lay on the ground.
Always place the object so that regardless of the terrain height the object will appear
at its constructed height.  Uses: for "floating" objects
*/
static LOGICAL HeightPlaceLOD(OBJECT* lpobj, UINT32 lod)
{
SINT32		i, num, poly;
FLOAT32		bz1, bz2, dz;
piVECTOR	pos, cpos;
piMATRIX	mat;
OBJECT*	lpbase;


#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for height conformance", lod);
#endif
	if (!obmainFindComment(&lpbase, lpobj->child, "base"))
	{
        if(lpobj->child->flag2 & OBJECT_IS_ROTATED)
        {
		    math3dVectorMultiply(&pos, &lpobj->child->pos, &lpobj->child->mat);
        }
        else
        {
            pos.x = lpobj->child->pos.x + lpobj->child->mat._41;
            pos.y = lpobj->child->pos.y + lpobj->child->mat._42;
            pos.z = lpobj->child->pos.z + lpobj->child->mat._43;
        }

        if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
		{
			dz = cpos.z;
		}

		mat = IdentityMatrix;
		mat._41 = 0.0f;
		mat._42 = 0.0f;
		mat._43 = dz;
		ApplyMatrixToObjectTree(lpobj->child, &mat);

#ifdef TERRAIN_EDITOR
		Write_Debug("HeightPlaceLOD: Could not find 'base' subobject for object '%s'", lpobj->name);
#endif

		return(FALSE);
	}

    if(lpbase->flag2 & OBJECT_IS_ROTATED)
    {
	    math3dVectorMultiply(&pos, &lpbase->pos, &lpbase->mat);
    }
    else
    {
        pos.x = lpbase->pos.x + lpbase->mat._41;
        pos.y = lpbase->pos.y + lpbase->mat._42;
        pos.z = lpbase->pos.z + lpbase->mat._43;
    }

    if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
	{
		dz = cpos.z;
	}

	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = dz;
	obplyrApplyMatrixToObjectTree(lpobj->child, &mat);

	bz1 = +VECTOR_EPSILON;
	bz2 = -VECTOR_EPSILON;
	dz = -FLT_MAX;

	num = lpbase->numverts;
	piVectorPtr objPos = lpbase->MutablePositions();
	lpbase->flag2 &= ~OBJECT_USEBUFFERS; // should just put this in MutablePositions()

	for (i=0; i < num; i++, ++objPos)
	{
		if (bz1 > objPos->z &&
			bz2 < objPos->z)
		{
            if(lpbase->flag2 & OBJECT_IS_ROTATED)
            {
			    // Rotate/Translate position to object space
			    math3dVectorMultiply(&pos, objPos.ptr, &lpbase->mat);
            }
            else
            {
                pos.x = objPos.ptr->x + lpbase->mat._41;
                pos.y = objPos.ptr->y + lpbase->mat._42;
                pos.z = objPos.ptr->z + lpbase->mat._43;
            }

            if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
			{
				dz = pos.z - cpos.z;
				objPos->z -= dz;
			}
			else
			{
#ifdef TERRAIN_EDITOR
				Write_Debug("HeightPlaceLOD: Could not find the ground for 'base' for object '%s'", lpobj->name);
#endif
			}
		}
	}


	return(TRUE);
}

/*
4	Height Conform LOD
*/
static LOGICAL HeightConformLOD(OBJECT* lpobj, UINT32 lod)
{
SINT32		i, num, poly;
FLOAT32		bz1, bz2, dz, heightFromTerrain, lowestVertex;
piVECTOR	pos, cpos;
piMATRIX	mat;
OBJECT*	lpbase, *lpnext;


#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for height conformance", lod);
#endif

	if (!obmainFindComment(&lpbase, lpobj->child, "base"))
	{
        if(lpobj->child->flag2 & OBJECT_IS_ROTATED)
        {
		    math3dVectorMultiply(&pos, &lpobj->child->pos, &lpobj->child->mat);
        }
        else
        {
            pos.x = lpobj->child->pos.x + lpobj->child->mat._41;
            pos.y = lpobj->child->pos.y + lpobj->child->mat._42;
            pos.z = lpobj->child->pos.z + lpobj->child->mat._43;
        }

        if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
		{
			dz = cpos.z;
		}

		mat = IdentityMatrix;
		mat._41 = 0.0f;
		mat._42 = 0.0f;
		mat._43 = dz;
		ApplyMatrixToObjectTree(lpobj->child, &mat);

#ifdef TERRAIN_EDITOR
		Write_Debug("HeightConformLOD: Could not find 'base' subobject for object '%s'", lpobj->name);
#endif

		return(FALSE);
	}

    if(lpbase->flag2 & OBJECT_IS_ROTATED)
    {
	    math3dVectorMultiply(&pos, &lpbase->pos, &lpbase->mat);
    }
    else
    {
        pos.x = lpbase->pos.x + lpbase->mat._41;
        pos.y = lpbase->pos.y + lpbase->mat._42;
        pos.z = lpbase->pos.z + lpbase->mat._43;
    }

    if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
	{
		dz = cpos.z;
	}

	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = dz;

	obplyrApplyMatrixToObjectTree(lpobj->child, &mat);

	lpnext = lpbase;

	while (lpnext)
	{
		num = lpnext->numverts;
		piVectorConstPtr objPos = lpnext->RenderPositions();

		for (i=0; i < num; i++, ++objPos)
		{
            if(lpnext->flag2 & OBJECT_IS_ROTATED)
            {
			    // Rotate/Translate position to object space
			    math3dVectorMultiply(&pos, objPos.ptr, &lpnext->mat);
            }
            else
            {
                pos.x = objPos.ptr->x + lpnext->mat._41;
                pos.y = objPos.ptr->y + lpnext->mat._42;
                pos.z = objPos.ptr->z + lpnext->mat._43;
            }

            if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
			{
				if (dz < cpos.z)
				{
					dz = cpos.z;
				}
			}
		}

		lpnext = lpnext->attached;
	}

	lpnext = lpbase;

	while(lpnext)
	{
		num = lpnext->numverts;
		piVectorConstPtr objPos = lpnext->RenderPositions();

		lowestVertex = -10000;	// very high

		for(i = 0; i < num; i++, ++objPos)
		{
			lowestVertex = lowestVertex < objPos->z ? objPos->z : lowestVertex;
		}

		lpnext = lpnext->attached;
	}

	bz1 = -lowestVertex + VECTOR_EPSILON;
	bz2 = lowestVertex - VECTOR_EPSILON;
	heightFromTerrain = -dz;
	dz  = -FLT_MAX;
	lpnext = lpbase;

	while(lpnext)
	{
		num = lpbase->numverts;
		piVectorPtr objPos = lpbase->MutablePositions();
		lpbase->flag2 &= ~OBJECT_USEBUFFERS;

		for (i=0; i < num; i++, ++objPos)
		{
			if (bz1 > objPos->z &&
				bz2 < objPos->z)
			{
                if(lpbase->flag2 & OBJECT_IS_ROTATED)
                {
				    // Rotate/Translate position to object space
				    math3dVectorMultiply(&pos, objPos.ptr, &lpbase->mat);
                }
                else
                {
                    pos.x = objPos.ptr->x + lpbase->mat._41;
                    pos.y = objPos.ptr->y + lpbase->mat._42;
                    pos.z = objPos.ptr->z + lpbase->mat._43;
                }

                if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
				{
					dz = (cpos.z - pos.z) + lowestVertex;
					objPos->z = dz;
				}
				else
				{
#ifdef TERRAIN_EDITOR
					Write_Debug("HeightConformLOD: Could not find the ground for 'base' for subobject '%s'", lpnext->name);
#endif
				}
			}
		}

		lpnext = lpnext->attached;
	}


	return(TRUE);
}

/*
4	Height Conform
	Place the object at the height specified and then conform the
	"base" object (internal) to the terrain contours. Height is
	relative to terrain height.
	Use: for bridge abutments
*/
static LOGICAL PlaceHeightConform(OBJECT* lpobj)
{
int			i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place height conform: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, HeightConformLOD))
		{
//			break;
		}
	}

	return(TRUE);
}

/*
8	Octet Replace
*/
static LOGICAL OctetReplace(OBJECT* lpobj)
{
OBJECT*		OctetObject;
OBJECT*		TempObjPtr;
SCHAR		s[64];
piVECTOR	pos, cpos, pos1, pos2;
SINT32		i, poly;

	// first conform object to ground

	// determine octet
	TempObjPtr = lpobj;
	while (TempObjPtr->flags & OBJECT_NOTOBJECT)
	{
		TempObjPtr = TempObjPtr->child;

		if (TempObjPtr == NULL)
		{
			return (FALSE);
		}
	}

	math3dVectorMultiply(&pos, &TempObjPtr->pos, &TempObjPtr->mat);

	pos1 = pos2 = pos;
	pos1.z -= 20000.0;
	pos2.z += 20000.0;
	if (_CollideDownSpecial(&pos1,&pos2,&cpos,&OctetObject,&poly))
	{
		poly = poly / 8 * 8; // get 1st triangle of octet

		// ok place objects o1-o7
		for (i=1; i < 8; i++)
		{
			sprintf(s, "o%d", i);
			ogrpFindSubObjectByName(&TempObjPtr, lpobj, s);
			if (TempObjPtr != NULL)
			{
				_ParallelagramizeObject(TempObjPtr,
									 	&OctetObject->ImmutablePositions()[OctetObject->ptris[poly+i].v1],
									  	&OctetObject->psnorms[poly+i]);
#if WANT_NON_REPLACEMENT_TILES
                obmainSnapObjectTreeToXYGrid(TempObjPtr, (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
#endif
            }
		}

		// now place o8
		ogrpFindSubObjectByName(&TempObjPtr, lpobj, "o8");
		if (TempObjPtr != NULL)
		{
			_ParallelagramizeObject(TempObjPtr,
									&OctetObject->ImmutablePositions()[OctetObject->ptris[poly].v1],
									&OctetObject->psnorms[poly]);

#if WANT_NON_REPLACEMENT_TILES
			obmainSnapObjectTreeToXYGrid(TempObjPtr, (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
#endif
        }
	}
	else
	{
		Write_Debug("Could not find the ground in OctetReplace (%s)", lpobj->name);
#ifdef TERRAIN_EDITOR
		{
		SINT8	txt[256];

			sprintf(txt, "No ground, OctetReplace");
			rnmainListText(txt);
		}
#endif
	}

	return (TRUE);
}

static UINT32 _ParallelagramizeObject(OBJECT * Object,
									 const  piVECTOR * PlanePoint,
									  const piVECTOR * Normal)
{
OBJECT * ObjAtt;

	if (Object == NULL)
	{
		return 0;
	}

	if (!(Object->flags & OBJECT_NOTOBJECT))
	{
		ObjAtt = Object;

		do
		{
			_ParallelagramizeObjectToTriangle(ObjAtt,PlanePoint,Normal);
			ObjAtt = ObjAtt->attached;;
		}
		while (ObjAtt != NULL);

	}

	if (Object->child != NULL)
	{
		_ParallelagramizeSubObject(Object->child,PlanePoint,Normal);
	}

	return 0;
}

static UINT32 _ParallelagramizeSubObject(OBJECT * Object,
									  const piVECTOR * PlanePoint,
									  const piVECTOR * Normal)
{
OBJECT * ObjAtt, * ObjSib;

	if (Object == NULL)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				_ParallelagramizeObjectToTriangle(ObjAtt,PlanePoint,Normal);

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != NULL);

		}

		if (ObjSib->child != NULL)
		{
			_ParallelagramizeSubObject(ObjSib->child,PlanePoint,Normal);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return 0;
}


static UINT32 _ParallelagramizeObjectToTriangle(OBJECT * Object,
												const piVECTOR * PlanePoint,
												const piVECTOR * Normal)
{
	UINT32 i;
	piVECTOR TempVec;
	FLOAT64 D;
	FLOAT32 TempNum;
	piMATRIX MatInv;

	if (Object == NULL)
	{
		return 0;
	}

	if (Object->numverts == 0)
	{
		return 0;
	}

	D = (FLOAT64)(Normal->x) * (FLOAT64)(PlanePoint->x) +
		(FLOAT64)(Normal->y) * (FLOAT64)(PlanePoint->y) +
		(FLOAT64)(Normal->z) * (FLOAT64)(PlanePoint->z);
	math3dInvertMatrix(&MatInv,&Object->mat);

	piVectorConstPtr origPos = Object->ImmutablePositions();
	piVectorPtr newPos = Object->MutablePositions();
	Object->flag2 &= ~OBJECT_USEBUFFERS;

	for (i = 0; i < Object->numverts; i++, ++origPos, ++newPos)
	{
		math3dVectorMultiply(&TempVec,origPos.ptr,&Object->mat);
		TempNum = (FLOAT32)(D - (FLOAT64)(Normal->x) * (FLOAT64)(TempVec.x)
							  - (FLOAT64)(Normal->y) * (FLOAT64)(TempVec.y))/
				  Normal->z;
		TempVec.z += TempNum;
		math3dVectorMultiply(newPos.ptr,&TempVec,&MatInv);
	}

	return 0;
}

static LOGICAL BottomConformLOD(OBJECT* lpobj, UINT32 lod)
{
SINT32		i, num, poly;
FLOAT32		bz1, bz2, dz;
piVECTOR	pos, cpos;
piMATRIX	mat;
OBJECT*	lpbase, *lpnext;


#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for bottom conformance", lod);
#endif
	if (!obmainFindComment(&lpbase, lpobj->child, "base"))
	{
		math3dVectorMultiply(&pos, &lpobj->child->pos, &lpobj->child->mat);

        if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
		{
			dz = cpos.z;
		}

		mat = IdentityMatrix;
		mat._41 = 0.0f;
		mat._42 = 0.0f;
		mat._43 = dz;
		ApplyMatrixToObjectTree(lpobj->child, &mat);

#ifdef TERRAIN_EDITOR
		Write_Debug("BottomConformLOD: Could not find 'base' subobject for object '%s'", lpobj->name);
#endif

		return(FALSE);
	}

	math3dVectorMultiply(&pos, &lpbase->pos, &lpbase->mat);

    if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
	{
		dz = cpos.z;
	}

	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = dz;
	obplyrApplyMatrixToObjectTree(lpobj->child, &mat);

	bz1 = +VECTOR_EPSILON;
	bz2 = -VECTOR_EPSILON;
	dz = -FLT_MAX;

	lpnext = lpbase;
	while (lpnext)
	{
		lpnext->flag2 &= ~OBJECT_USEBUFFERS;
		num = lpnext->numverts;
		piVectorConstPtr objPos = lpnext->RenderPositions();
		for (i=0; i < num; i++, ++objPos)
		{
			// Rotate/Translate position to object space
			math3dVectorMultiply(&pos, objPos.ptr, &lpnext->mat);

            if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
			{
				if (dz < (pos.z - cpos.z))
				{
					dz = (pos.z - cpos.z);
				}
			}
		}

		lpnext = lpnext->attached;
	}

#ifdef TERRAIN_EDITOR
	Write_Debug("Elevating subobject by %f meters", dz / ScaleFactor);
#endif

	mat = IdentityMatrix;
	mat._41 = 0.0f;
	mat._42 = 0.0f;
	mat._43 = -dz;
	obplyrApplyMatrixToObjectTree(lpobj->child, &mat);

	lpnext = lpbase;
	while (lpnext)
	{
		num = lpnext->numverts;
		piVectorPtr objPos = lpnext->MutablePositions();
		lpnext->flag2 &= ~OBJECT_USEBUFFERS;

		for (i=0; i < num; i++, ++objPos)
		{
			if (bz1 > objPos->z &&
				bz2 < objPos->z)
			{
				// Rotate/Translate position to object space
				math3dVectorMultiply(&pos, objPos.ptr, &lpnext->mat);

                if(trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell))
				{
					dz = pos.z - cpos.z;
					objPos->z -= dz;
				}
				else
				{
#ifdef TERRAIN_EDITOR
					Write_Debug("BottomConformLOD: Could not find the ground for 'base' for subobject '%s'", lpnext->name);
#endif
				}
			}
		}

		lpnext = lpnext->attached;
	}


	return(TRUE);
}

/*
5	Bottom Conform
	Place the object so that at least one vertex contacts the
	terrain. Then, conform the "base" object (internal) to the
	terrain contours.
	Use: for buildings so that floors, etc. remain flat on any
	surface
*/
static LOGICAL PlaceBottomConform(OBJECT* lpobj)
{
SINT32		i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place bottom conform: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, BottomConformLOD))
		{
//			break;
		}
	}

	return(TRUE);
}



// places an object, conforming to the terrain
// each subobject can be in a different octet, however they must be wholly contained
// by that octet
static LOGICAL ConformRadialLODIterate(OBJECT* lpobj)
{
OBJECT * OctetObject;
OBJECT * o;
piVECTOR	pos, cpos, pos1, pos2;
SINT32		i, poly;
piVECTOR CenterPoint;
piVECTOR TempVec1,TempVec2;
piVECTOR Normal;
piVECTOR PlanePoint;
piMATRIX MatInv = IdentityMatrix;
UINT32 TriNum;
FLOAT64 Angle;
FLOAT64 D;
FLOAT32 TempNum;


	// first conform object to ground

	// determine octet
	o = lpobj;
	while (o->flags & OBJECT_NOTOBJECT)
	{
		o = o->child;

		if (o == NULL)
		{
			return (FALSE);
		}
	}

    if(o->flag2 & OBJECT_IS_ROTATED)
    {
	    math3dVectorMultiply(&pos, &o->pos, &o->mat);
    }
    else
    {
        pos.x = o->pos.x + o->mat._41;
        pos.y = o->pos.y + o->mat._42;
        pos.z = o->pos.z + o->mat._43;
    }

	pos1 = pos2 = pos;
	pos1.z -= 20000.0;
	pos2.z += 20000.0;
	if (_CollideDownSpecial(&pos1,&pos2,&cpos,&OctetObject,&poly))
	{
        if(lpobj->flag2 & OBJECT_IS_ROTATED)
        {
		    math3dInvertMatrix(&MatInv,&lpobj->mat);
        }
        else
        {
            MatInv._41 = -lpobj->mat._41;
            MatInv._42 = -lpobj->mat._42;
            MatInv._43 = -lpobj->mat._43;
        }

		poly = poly / 8 * 8; // get 1st triangle of octet

		// find center point
		CenterPoint = OctetObject->RenderPositions()[OctetObject->ptris[poly].v3];
		math3dVectorMultiply(&CenterPoint,&CenterPoint,&OctetObject->mat);

		// adjust vertecies
		piVectorConstPtr objPos = lpobj->ImmutablePositions();
		piVectorPtr newObjPos = lpobj->MutablePositions();
		lpobj->flag2 &= ~OBJECT_USEBUFFERS;

		for (i=0;i<lpobj->numverts;i++, ++objPos, ++newObjPos)
		{
			// determine which triangle vertex is
			TempVec2 = *objPos;

            if(lpobj->flag2 & OBJECT_IS_ROTATED)
            {
			    math3dVectorMultiply(&TempVec2,&TempVec2,&lpobj->mat);
            }
            else
            {
                TempVec2.x = TempVec2.x + lpobj->mat._41;
                TempVec2.y = TempVec2.y + lpobj->mat._42;
                TempVec2.z = TempVec2.z + lpobj->mat._43;
            }

            mathSubVectors(&TempVec1,&TempVec2,&CenterPoint);
			Angle = atan2((FLOAT64)TempVec1.x,(FLOAT64)TempVec1.y);

			TriNum = (UINT32)-1; // boy we sure like to cast -1 to unsigned values
			if (Angle >= -PI/4.0 && Angle < 0.0)
			{
				TriNum = 2;
			}
			if (Angle >= 0.0 && Angle < PI/4.0)
			{
				TriNum = 3;
			}
			if (Angle >= PI/4.0 && Angle < PI/2.0)
			{
				TriNum = 4;
			}
			if (Angle >= PI/2.0 && Angle < 3.0 * PI / 4.0)
			{
				TriNum = 5;
			}
			if (Angle >= 3.0 * PI / 4.0 && Angle < PI)
			{
				TriNum = 6;
			}
			if (Angle >= -PI && Angle < -3.0 * PI / 4.0)
			{
				TriNum = 7;
			}
			if (Angle >= -3.0 * PI / 4.0f && Angle < -PI / 2.0)
			{
				TriNum = 0;
			}
			if (Angle >= -PI / 2.0 && Angle < -PI/4.0)
			{
				TriNum = 1;
			}

			if (TriNum == (UINT32)-1)
			{
				Write_Debug("Miscollide on place 10 mode");
				TriNum = 0;
			}

			// now conform vertex to that triangle
			Normal = OctetObject->psnorms[poly+TriNum];
			PlanePoint = CenterPoint;

			D = (FLOAT64)(Normal.x) * (FLOAT64)(PlanePoint.x) +
				(FLOAT64)(Normal.y) * (FLOAT64)(PlanePoint.y) +
				(FLOAT64)(Normal.z) * (FLOAT64)(PlanePoint.z);

			TempNum = (FLOAT32)(D - (FLOAT64)(Normal.x) * (FLOAT64)(TempVec2.x)
								  - (FLOAT64)(Normal.y) * (FLOAT64)(TempVec2.y))/
					  Normal.z;
			TempVec2.z += TempNum;
			math3dVectorMultiply(newObjPos.ptr,&TempVec2,&MatInv);

		}
#if WANT_NON_REPLACEMENT_TILES
		obmainSnapObjectTreeToXYGrid(lpobj,(FLOAT32)TERRAIN_SNAPTO_GRIDSIZE,1);
#endif
	}
	else
	{
		Write_Debug("Could not find the ground in ConformRadialLODIterate (%s)", lpobj->name);
#ifdef TERRAIN_EDITOR
		{
		SINT8	txt[256];

			sprintf(txt, "No ground, ConformRadialLODIterate");
			rnmainListText(txt);
		}
#endif
	}

	return (TRUE);
}

static LOGICAL ConformRadialLODSub(OBJECT* lpobj)
{
OBJECT * ObjAtt;
OBJECT * ObjSib;

	if (lpobj == NULL)
	{
		return (TRUE);
	}

	ObjSib = lpobj;
	do
	{

		ObjAtt = ObjSib;

		if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
		do
		{
			ConformRadialLODIterate(ObjAtt);

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != NULL);


		if (ObjSib->child != NULL)
		{
			ConformRadialLODSub(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return (TRUE);
}

static LOGICAL ConformRadialLOD(OBJECT* lpobj, UINT32 lod)
{
OBJECT * ObjAtt;

	if (lpobj == NULL)
	{
		return (TRUE);
	}

	ObjAtt = lpobj;

	if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
	do
	{
		ConformRadialLODIterate(ObjAtt);

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != NULL);

	if (lpobj->child != NULL)
	{
		ConformRadialLODSub(lpobj->child);
	}

	return (TRUE);
}

static LOGICAL DownLODImplement(OBJECT* lpobj)
{
    SINT32		i, num;
    piVECTOR	pos;

	if((lpobj->flags & OBJECT_NOTOBJECT))
    {
        return TRUE;
    }

	lpobj->flag2 &= ~OBJECT_USEBUFFERS;

#ifdef USE_NEW_GRASS
    if(lpobj->flag2 & OBJECT_IS_GRASS)
    {
        trngrassAddGrass(lpobj);
    }
#endif

	num = lpobj->numverts;
	piVectorPtr objPos = lpobj->MutablePositions();
	lpobj->flag2 &= ~OBJECT_USEBUFFERS;

    piVECTOR retpos = {0, 0, 0};
    SINT32 poly = 0;

	for (i=0; i < num; i++, ++objPos)
	{
		pos = *objPos;

        if(lpobj->flag2 & OBJECT_IS_ROTATED)
        {
		    // Rotate/Translate position to object space
		    math3dVectorMultiply(&pos, &pos, &lpobj->mat);
        }
        else
        {
            pos.x = pos.x + lpobj->mat._41;
            pos.y = pos.y + lpobj->mat._42;
            pos.z = pos.z + lpobj->mat._43;
        }

        trnopCollideDownWithDTL(&retpos, &pos, &poly, _superCell);

        objPos->z += retpos.z - VECTOR_EPSILON;// * 25;;
	}

    return(TRUE);
}

static LOGICAL DownLODInterate(OBJECT* lpobj)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!DownLODInterate(lpobj->child))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			DownLODImplement(lpatt);
			lpatt = lpatt->attached;
		}

		DownLODImplement(lpobj);
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

static LOGICAL DownLOD(OBJECT* lpobj, UINT32 lod)
{
#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for down", lod);
#endif

	DownLODInterate(lpobj->child);

	return(TRUE);
}

/*
6	Place Down
	Place an object down on the terrain manipulate vertices height
	so that it is flush with the terrain contour.
	Use: for river objects or objects that lay on the terrain
*/
static LOGICAL PlaceDown(OBJECT* lpobj)
{
SINT32		i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place down: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, DownLOD))
		{
//			break;
		}
	}

	return(TRUE);
}

/*
10	Place Down and Conform Radial
*/
static LOGICAL PlaceConformRadial(OBJECT* lpobj)
{
SINT32		i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place conform radial: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, ConformRadialLOD))
		{
//			break;
		}
	}

	return (TRUE);
}


static LOGICAL RotateLODImplement(OBJECT* lpobj)
{
SINT32		i, num, poly;
piVECTOR	pos, cpos;


	num = lpobj->numverts;
	piVectorPtr objPos = lpobj->MutablePositions();
	lpobj->flag2 &= ~OBJECT_USEBUFFERS;

	for (i=0; i < num; i++, ++objPos)
	{
		pos = *objPos;

        if(lpobj->flag2 & OBJECT_IS_ROTATED)
        {
		    // Rotate/Translate position to object space
		    math3dVectorMultiply(&pos, &pos, &lpobj->mat);
        }
        else
        {
            pos.x = pos.x + lpobj->mat._41;
            pos.y = pos.y + lpobj->mat._42;
            pos.z = pos.z + lpobj->mat._43;
        }

        trnopCollideDownWithDTL(&cpos, &pos, &poly, _superCell);

		objPos->z += cpos.z - VECTOR_EPSILON;// * 10;
	}


	return(TRUE);
}

static LOGICAL RotateLODInterate(OBJECT* lpobj)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!RotateLODInterate(lpobj->child))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			RotateLODImplement(lpatt);
			lpatt = lpatt->attached;
		}

		RotateLODImplement(lpobj);
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

static LOGICAL RotateLOD(OBJECT* lpobj, UINT32 lod)
{
#ifdef TERRAIN_EDITOR
	Write_Debug("Processing LOD %d for rotate", lod);
#endif

	RotateLODInterate(lpobj->child);

	return(TRUE);
}

/*
7	Place and Rotate
	Place an object on the terrain and rotate/transform it
	so that it is flush with the terrain contour.
	Use: for rocks or other objects that be perpendicular to terrain
*/
static LOGICAL PlaceRotate(OBJECT* lpobj)
{
SINT32		i;

#ifdef TERRAIN_EDITOR
	Write_Debug("Place and rotate: %s", lpobj->name);
#endif

	for (i=1; i < 10; i++)
	{
		if (!obmainFindObjectLOD(lpobj->child, i, RotateLOD))
		{
//			break;
		}
	}

	return(TRUE);
}

static LOGICAL ApplyAttitudeToObjectTree(OBJECT* lpobj, piVECTOR* att)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->child)
		{
			if (!ApplyAttitudeToObjectTree(lpobj->child, att))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->att = *att;
			lpatt = lpatt->attached;
		}

		lpobj->att = *att;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}


//////////////////////////////////////////////////////////////////////
//	HandleSpecialSubObjects
//////////////////////////////////////////////////////////////////////
static void HandleSpecialSubObjects(OBJECT* o)
{
	#ifdef USE_NEW_GRASS
    if(o->flag2 & OBJECT_IS_GRASS)
    {
        trngrassAddGrass(o);
    }
	#endif

	if(o->child)	HandleSpecialSubObjects(o->child);
	if(o->attached)	HandleSpecialSubObjects(o->attached);
	if(o->sibling)	HandleSpecialSubObjects(o->sibling);
}


void trnopLoadTrees(LOGICAL bLoad)
{
#if defined(BETA_RELEASE)
    g_bLoadTrees = bLoad;
#endif
}

void trnopSetSC(int sc)
{
    _superCell = sc;
}

struct ParseTrees
{
    int types[32];
    OBJECT* o;
    COLLISION c;

    void apply(OBJECT* o)
    {
        if(o)
        {
            if(o->comment)
            {
                if(!strncmp(o->comment, "treetype", 8))
                {
                    _ParseSpeedTreeComment(o->comment, types);
                }
            }

            apply(o->child);
            apply(o->sibling);

            if(types[0] > 0 && !(o->flags & OBJECT_NOTOBJECT))
            {
                o->trnum = _superCell;

                if(gCustomTerrain)
                {
                    // Disable trees associated with any buildings for custom scenarios
#if(0)
                    piVECTOR pos;
                    math3dVectorMultiply(&pos, &o->pos, &o->mat);

                    unsigned int seed = (SINT32)((SINT64)pos.x * (SINT64)113510315245LL *
                                        (SINT64)pos.y * (SINT64)103515051525LL);

                    int seed2 = (int)seed;

                    // Get index for iTreeValues
                    int i = abs(mathGetRandomNumberFromSeed(&seed2) % types[0]) + 1;

                    // Comment section has tree types as 1 based.
                    int base = types[i] - 1;

                    Vec3d treepos(pos);
                    trnScenarioAddTree(treepos, base);
#endif
                    return;
                }

                if(o->flag3 & OBJECT_TREE_CLUSTER)
                {
                    sptAddCluster(o, types);
                    return;
                }

                // Hide the triangle used to place the tree.
                o->flag2 |= OBJECT_HIDDEN;

                if(o->flag2 & OBJECT_IS_ROTATED)
                {
                    c.newpos.x = o->pos.x * o->mat._11 +
                                 o->pos.y * o->mat._21 +
                                 o->pos.z * o->mat._31 +
                                 o->mat._41;

                    c.newpos.y = o->pos.x * o->mat._12 +
                                 o->pos.y * o->mat._22 +
                                 o->pos.z * o->mat._32 +
                                 o->mat._42;
                }
                else
                {
                    c.newpos.x = o->pos.x + o->mat._41;
                    c.newpos.y = o->pos.y + o->mat._42;
                }

                c.oldpos.x = c.newpos.x;
                c.oldpos.y = c.newpos.y;
                c.oldpos.z = -10000.0f;
                c.newpos.z =  10000.0f;

                if(trncdCheckTerrainDownWithDTL(&c, &trndtl[_superCell]))
                {
                    // Only care about z. Sink all trees 0.5 meters into the ground
                    o->mat._43 = c.colpos.z + 0.5f;

                    UINT32 seed1 = (SINT32)((SINT64)c.newpos.x * (SINT64)113510315245LL *
                                            (SINT64)c.newpos.y * (SINT64)103515051525LL);

                    SINT32 seed2 = (SINT32)seed1;

                    // Get index for iTreeValues
                    SINT32 i = abs(mathGetRandomNumberFromSeed(&seed2) % types[0]) + 1;

                    // Comment section has tree types as 1 based.
                    SINT16 base = types[i] - 1;

#if CHRISTMAS
                    if(base == 2)
                        o->mat._43 -= 0.5f;
#endif

                    if(base >= 0)
                    {
                        FLOAT64VECTOR pos;

                        pos.x = c.newpos.x + WorldOriginOffset.x;
                        pos.y = c.newpos.y + WorldOriginOffset.y;
                        pos.z = o->mat._43;

                        if(!sptIsTreeCollidable(base))
                        {
                            listDeleteOBJECT(o);
                        }

                        sptAddTree(o, base, &trndtl[_superCell], pos);
                    }
                }
            }
        }
    }

    void Apply()
    {
        types[0] = 0;
        memset(&c, 0, sizeof(COLLISION));
        apply(o);
    }
};

SINT32 trnopHandleSpeedTree(OBJECT* group)
{
	// Turn on/off loading for profiling
#if defined(BETA_RELEASE)
	if(!g_bLoadTrees) return false;
#endif

    if(group == NULL)
    {
        return -1;
    }

    OBJECT* o = NULL;
    SINT32 treeValues[256] = {0};
    SINT32 overriddenTrees[256] = {0};
    COLLISION collision;

    LOGICAL overridden = FALSE;

    memset(&collision, 0, sizeof(COLLISION));

    if(trndtl[_superCell].m_trees == NULL)
    {
        return 0;
    }

    static struct FindTree ft;

    ft.tree = NULL;
    ft.Find(group);

    if(!ft.tree)
        return 0;

    static struct ParseTrees parse;
    parse.o = ft.tree->child;
    parse.Apply();

    return 0;
}


LOGICAL trnopOrientSubobjects(OBJECT* lpobj, piVECTOR* pos, bool nonPlanarBase)
{
SINT8*			ptr;
SINT32			place;
piMATRIX		mat;
OBJECT*		lpsub;
FLOAT64VECTOR	WorldOriginOffset;
UINT32			i;
UINT32			toplevel;
LOGICAL			DoGroupNormals;
OBJECT*			TempObj;

    // Set current supercell being loaded.
    _superCell = lpobj->trnum;

	lpobjTopmost = lpobj;

	mat = IdentityMatrix;

	trnGetWorldOriginOffset(&WorldOriginOffset);

    if(lpobj->flag2 & OBJECT_IS_ROTATED)
    {
	    ApplyAttitudeToObjectTree(lpobj, &lpobj->att);
	    math3dMatrixRotate(&mat, &lpobj->att);
    }

	piVECTOR	translation;
	translation.x = (FLOAT32)((FLOAT64)pos->x - WorldOriginOffset.x);
	translation.y = (FLOAT32)((FLOAT64)pos->y - WorldOriginOffset.y);
	translation.z = (FLOAT32)((FLOAT64)pos->z - WorldOriginOffset.z);

	obplyrRotateAndTranslateObjectTree(lpobj, mat, translation);


#ifdef TERRAIN_EDITOR
	Write_Debug("trnopOrientSubobjects for '%s'", lpobj->name);
#endif

	// lpobj should point to a Topmost object
	lpsub = NULL;
	toplevel = 0;
	if (lpobj->comment != NULL)
	{
		if (obmainFindCommentName(&ptr, lpobj->comment, "place"))
		{
			if (atoi(ptr) == 8)
			{
				lpsub = lpobj;
				toplevel = 1;
			}
		}
	}

	if (lpsub == NULL)
	{
		lpsub = obmainGetFirstSubobject(lpobj);
		if (lpsub == NULL)
		{
			lpsub = lpobj;
		}
	}

    while (lpsub)
	{

        if(lpsub->flag2 & OBJECT_SPEEDTREE)
        {
            obmainMoveToNotObjectTree(lpsub);

			if(trnopHandleSpeedTree(lpsub) != 0)
                    return FALSE;

			lpsub = lpsub->sibling;
            continue;
        }

		if (!obmainFindCommentName(&ptr, lpsub->comment, "place"))
		{
			lpsub = lpsub->sibling;
			continue;
		}
		else
		{
			place = atoi(ptr);
		}

#ifdef TERRAIN_EDITOR
		Write_Debug(" subobject '%s', place = %d", lpsub->name, place);
#endif

		switch (place)
		{
			case OBJVAL_ON_GROUND:
			default:
				PlaceOnGround(lpsub);
				break;
			case OBJVAL_RELATIVE_GROUND:
				PlaceRelativeToGround(lpsub);
				break;
			case OBJVAL_ABSOLUTE:
				PlaceAbsolute(lpsub);
				break;
			case OBJVAL_HEIGHT_CONFORM:
				PlaceHeightConform(lpsub);
				break;
			case OBJVAL_BOTTOM_CONFORM:
			// only makes copy of base vertices no need for that fancy shearing stuff as far as i can tell
			// which doesn't appear to work in all cases anyway
				PlaceBottomConform(lpsub);
				break;
			case OBJVAL_PLACE_DOWN:
				PlaceDown(lpsub);
				break;
			case OBJVAL_PLACE_ROTATE:
				PlaceRotate(lpsub);
				break;
			case OBJVAL_OCTET_REPLACE:
				OctetReplace(lpsub);
				break;
			case OBJVAL_OCTET_CONFORMRADIAL:
				PlaceConformRadial(lpsub);
				break;
		}

		DoGroupNormals = TRUE;

		for (TempObj = lpsub;
			TempObj != NULL && TempObj->flags & OBJECT_NOTOBJECT;
			TempObj = TempObj->child)
			;

		// foliage material objects don't need grouped normals
		if (TempObj != NULL)
		{
			if (TempObj->matid == FoliageMatId)
			{
				DoGroupNormals = FALSE;
			}
		}

		if (DoGroupNormals == TRUE)
		{
			switch (place)
			{
			case OBJVAL_ON_GROUND:
			case OBJVAL_RELATIVE_GROUND:
			case OBJVAL_ABSOLUTE:
				break;
			case OBJVAL_HEIGHT_CONFORM:
			case OBJVAL_PLACE_BASE:
			case OBJVAL_BOTTOM_CONFORM:
				_UpdateBaseGroupNormals(lpsub);
				break;
			case OBJVAL_PLACE_DOWN:
			case OBJVAL_PLACE_ROTATE:
			case OBJVAL_OCTET_REPLACE:
			case OBJVAL_OCTET_CONFORMRADIAL:
				for (i=1; i < 10; i++)
				{
					if (!obmainFindObjectLOD(lpsub, i, (PROCESSCALLBACK)_UpdateGroupNormals))
					{
//						break;
					}
				}
				break;
			default:
				break;
			}
		}
		else
		{
			trnopRecalcBoundsAndNormals(lpsub);
		}


		if (toplevel == 1)
		{
			break;
		}

        lpsub = lpsub->sibling;

	}

	//picoCreateVIBObject(lpobj->child);


	return(TRUE);
}

// this function assume positive y to be the front of the object
bool trnopDynamicBase(const FLOAT64VECTOR& worldPos, // this is a reference because we only read it
									 piMATRIX worldAtt, // this is a copy because we re-use it again below
									 FLOAT32 voffset,
									 FLOAT32 hoffset,
									 FLOAT64VECTOR& obPos,
									 piVECTOR& obAtt)
{
	colliderQuery collider = colliderQuery(0, false);
	colliderQuery::CollisionList hits;

	const int rayCount		= 5;

	Vec3d position				= Vec3d(worldPos);
	Matrixf attitude			= Matrixf(worldAtt);
	Vec3d rayPos				= Vec3d(0, 1, 0);
	Vec3d dir					= Vec3d(0, 0, -1.0f); // direction pointer

    collider.addGeomToCollideWith((dGeomID)colliderTerrainSpace());
	collider.setMaxContactsPerHit(1);

	// ray geoms are destroyed when the collider goes out of scope
	dGeomID rays[rayCount];

	for(int i = 0; i < rayCount; i++)
	{
		rays[i] = collider.addGeom(dCreateRay(collider.space(), 4.0f), 0, COLLIDER_HARDTERRAIN_MASK);

		// set the ray pos
		switch(i)
		{
			case 0: rayPos.set(0,0,0);						break; // center
			case 1: rayPos.set(-hoffset,voffset,0);	break; // front left
			case 2: rayPos.set(hoffset,voffset,0);		break; // front right
			case 3: rayPos.set(-hoffset,-voffset,0);	break; // back left
			case 4: rayPos.set(hoffset,-voffset,0);	break; // back right
		}

		// rotate into world orientation
		rayPos = rayPos * attitude;

		// put us into the world
		rayPos += position;

		// put us into the physical world
		rayPos = physicsGameToPhysical(rayPos);

		// update our ray
		dGeomRaySet(rays[i], rayPos.x(), rayPos.y(), position.z() + 2.0f, dir.x(), dir.y(), dir.z());
	}

	if(collider.findCollisions(true))
	{
		collider.sort();

		Vec3d worldNormal;
		FLOAT64 zTotal = 0;
		UINT32 zCount = 0;

		for(int r = 0; r < rayCount; ++r)
		{
			hits.clear();

			if(collider.getCollisions(rays[r], hits, false))
			{
				colliderQuery::Collision* hit = NULL;

				for(unsigned int i = 0; i < hits.size(); ++i)
				{
					colliderQuery::Collision& current = hits[i];

					if(current.normal.z() > 0 && (!hit || current.pos.z() > hit->pos.z()))
						hit = &current;
				}

				if(hit)
				{
					worldNormal = worldNormal + hit->normal;
					zTotal += hit->pos.z();
					++zCount;
				}
			}
		}

		// if we have hits, calculate
		if(zCount)
		{
			// set our position and averaged z
			obPos = worldPos;
			obPos.z = zTotal / zCount;

			worldNormal.normalize();

			// for creating rotation matrices
			Vec3d from;
			Vec3d to;

			// forward
			from.set(0.0f, 1.0f, 0.0);
			to.set(0.0f, 1.0f, 0.0f);

			// orient our forward vector so we can rotate our terrain object around the z
			to = to * attitude;
			to.z() = 0;
			to.normalize();

			// create a orientation around the z
			Matrixd rotate;
			rotate.makeRotate(from, to);

			// now we'll rotate around the x and y based on the world normal
			from.set(0.0f, 0.0f, 1.0f);

			// save our current rotation
			attitude = rotate;

			rotate.makeIdentity();
			rotate.makeRotate(from, worldNormal);

			// add in the new rotation
			attitude = attitude * rotate;

			// put it into a game matrix so we can convert it to an attitude
			attitude.get(worldAtt);

			// rotate us by the average of the hit normals as well as the vehicle's yaw
			// convert it to the expected attitude vector
			mathEul_FromMatrix(&obAtt, &worldAtt, EulOrdXYZr);

			// mirror it as usual...does anyone know why?
			obAtt.x = -obAtt.x;
			obAtt.y = -obAtt.y;

			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Function _ParseSpeedTreeComment
// Description: Comment is in the format:
// "treetype=1,2,3,etc... where 1,2,3 are the tree types and
// corresponds to the tree file in terrain/trees
//////////////////////////////////////////////////////////////////////
SINT32 _ParseSpeedTreeComment(SCHAR* comment, SINT32 treevalues[])
{
    SCHAR* p1 = strstr(comment, "treetype");

    if(!p1) return -1;

    SCHAR szTokens[] = "=,n\0";
    SCHAR begin[128];
    SCHAR* pToken = NULL;
    SINT32 i = 1;

    strcpy(begin, p1);

    SCHAR* p2 = strstr(begin, "collideobject");

    if(p2) *p2 = 0;

    pToken = strtok(begin, szTokens);

    while(pToken != NULL)
    {
        pToken = strtok(NULL, szTokens);

        if(pToken)
        {
            treevalues[i++] = atol(pToken);
        }
    }

    treevalues[0] = i - 1;

    return 0;
}

static LOGICAL _UpdateGroupNormals(OBJECT * Object, UINT32 DummyNum)
{
	if (Object != NULL)
	{
		if (Object->child != NULL)
		{
			trnopRecalcBoundsAndNormals(Object->child);
		}
	}

	return TRUE;
}

static LOGICAL _UpdateBaseNormals(OBJECT * Object, UINT32 DummyNum)
{
OBJECT*	lpobj;

	if (!obmainFindComment(&lpobj, Object, "base"))
	{
		return(FALSE);
	}

	trnopRecalcBoundsAndNormals(Object);
	//_UpdateGroupNormals(lpobj, DummyNum);

	return(TRUE);
}

static LOGICAL _UpdateBaseGroupNormals(OBJECT * Object)
{
SINT32		i;

	if (Object != NULL)
	{
		for (i=1; i < 10; i++)
		{
			if (!obmainFindObjectLOD(Object, i, (PROCESSCALLBACK)_UpdateBaseNormals))
			{
//				break;
			}
		}
	}

	return TRUE;
}

void trnopRecalcBoundsAndNormals(OBJECT* o)
{
	assert(o);

	if(!(o->comment && strstr(o->comment, "speedtree")))
	{
		if(!(o->flags & OBJECT_NOTOBJECT))
		{
			ldmdlGetBounds(o);
			ldmdlCalcNormals2(o);
		}

		if(o->child)	trnopRecalcBoundsAndNormals(o->child);
		if(o->attached)	trnopRecalcBoundsAndNormals(o->attached);
	}
	if(o->sibling)	trnopRecalcBoundsAndNormals(o->sibling);
}
