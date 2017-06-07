#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "list/list.h"
#include "fio/fio.h"
#include "gr2/gr2Anim.h"
#include "ld/ldflt.h"
#include "ld/ldmdl.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "ob/obwld.h"
#include "ld/ldpix.h"
#include "ld/ldbmp.h"
#include "rn/rnmain.h"
#include "fx/FXInterface.h"
#include "trn/trngrass.h"
#include "spt/sptmain.h"
#include <assert.h>
#include "ob/obMgr.h"
#include "ogl/oglMesh.h"

#if platOS == OS_WIN32
#include "wwii/wwiiClientMessages.h"
#endif

/************************************************************************************************
	obpool
*************************************************************************************************/

#define DEF_TIME_STOP_LIMIT 2000
#define MDL_MAX_PATH 260

#define	FILETYPE_TEXTURE	1
#define	FILETYPE_MDL		2

#define MODELTYPE_NONSPEC	0
#define MODELTYPE_TERRAIN	1
#define MODELTYPE_COCKPIT	2
#define MODELTYPE_VEHICLE	3
#define MODELTYPE_UI		4

#define PLAYNET_COMPANY		('P' + ('N' << 8))		// short for Playnet
#define PLAYNET_OBJECTFILE	('O' + ('B' << 8))		// short for Object (File Type)

// NOTE: Be sure and change a version element every time you change the object file data or structure!!
#define OBJECTFILE_MAJOR	3
#define OBJECTFILE_MINOR	18
#define OBJECTFILE_VERSION	((OBJECTFILE_MAJOR << 8) + OBJECTFILE_MINOR)	// version; MSB is major, LSB is minor

#define OBJDATA(x) 	((POOLOBJDATA*)((UCHAR*)x + x->objdata))
#define PUVERTS(x) 	((piUNIFIED_VERTEX*)((UCHAR*)x + OBJDATA(x)->puverts))
#define PTRIS(x) 	((piINDICES*)((UCHAR*)x + OBJDATA(x)->ptris))
#define PSNORMS(x) 	((piVECTOR*)((UCHAR*)x + OBJDATA(x)->psnorms))
#define PSCDN(x) 	((piVECTOR*)((UCHAR*)x + OBJDATA(x)->pscdn))
#define DOFPTR(x)	((DOF*)((UCHAR*)x + x->dof))

#define SIZEOF_POOLOBJ_DISK 	76
#define SIZEOF_OBJDATA_DISK 	184
#define SIZEOF_DOF_DISK 		396
#define SIZEOF_POOLOBJ 	sizeof(POOLOBJ)
#define SIZEOF_OBJDATA 	sizeof(POOLOBJDATA)
#define SIZEOF_DOF		sizeof(DOF)

#define SERIALIZED 0

#if SERIALIZED
static LISTHEADER objectList;
static SINT32 objectnumber = 0;
#endif


struct POOL_OBJECT_TO_DIRECTORY
{
	POOL_OBJECT_TO_DIRECTORY* prev;
	POOL_OBJECT_TO_DIRECTORY* next;

	SINT8	LocalName[32];
	SINT8	Directory[128];

};

// Callback definitions for scanning object types.
using SCANOBJECT = LOGICAL(*)(OBJECT* lpobj, POOLOBJHDR** lphdr);
using SCANPOOL = LOGICAL(*)(POOLOBJHDR* lphdr, OBJECT** lpobj);

using SCANPOOLTOFILE = LOGICAL(*)(POOLOBJHDR* lphdr, int hfile, UINT32* pool);
using SCANFILETOPOOL = LOGICAL(*)(POOLOBJHDR* lphdr, int hfile, UINT32* pool);

//static SINT8		pathname[256];
static SINT32		numobjects;
static SINT32		numobjused;
static POOLOBJHDR*	objects;
static POOLOBJHDR	objused;
static POOLOBJHDR	objfree;
//static MATERIAL*	matlist;
//static SINT32		nummats;
//static UINT32		miscount;

static POOL_OBJECT_TO_DIRECTORY PoolObjectDirectoryHead;

static POOLOBJHDR* ScanObjectCallback(OBJECT* lpgrp, POOLOBJHDR* lpparent, SCANOBJECT callfunc);

static void SavePool(int hfile, POOLOBJ*);
static void FreeQueuedObjects();

static std::deque<OBJECT*> FreeObjectQue;

static LOGICAL CopyObjectToPool(OBJECT* lpobj, POOLOBJHDR** lpret);
#if defined(OPENGL3D) && platOS == OS_WIN32
void obpoolColorSwap(POOLOBJ* lppool);
#endif

/************************************************************************************************
	obpoolInit
*************************************************************************************************/
// Allocate the pool and initialize
// the pool free list.
LOGICAL obpoolInit(const char* name)
{
logTimerBeginMacro();
	if(SIZEOF_POOLOBJ != SIZEOF_POOLOBJ_DISK || SIZEOF_OBJDATA != SIZEOF_OBJDATA_DISK || SIZEOF_DOF != SIZEOF_DOF_DISK)
	{ // object format sanity check
		piExitProgram(_FL, "incorrect pool object size!");
		return FALSE;
	}

	numobjused = 0;
	numobjects = MINPOOLOBJS;
	objects = (POOLOBJHDR*)memAlloc(sizeof(POOLOBJHDR) * numobjects);

	objfree.prev = objfree.next = (POOLOBJHDR*)objects;
	if (!listInitList((LISTHEADER*)&objfree, MINPOOLOBJS, sizeof(POOLOBJHDR)))
	{
		return(FALSE);
	}

logTimerBeginExtMacro(obwldInit);
	if (!obwldInit(name))
	{
		return(FALSE);
	}
logTimerEndExtMacro(obwldInit);

logTimerEndMacro();

	return(TRUE);
}

/************************************************************************************************
	obpoolKill
*************************************************************************************************/
// Free all pool objects and free the list.
void obpoolKill()
{
	POOLOBJHDR*	lpobj, *next;

	POOL_OBJECT_TO_DIRECTORY * TransTablePtr, *nextPtr;

	if (PoolObjectDirectoryHead.next != nullptr)
	{
		for (TransTablePtr = PoolObjectDirectoryHead.next;
			TransTablePtr != nullptr;
			TransTablePtr = nextPtr)
		{
			nextPtr = TransTablePtr->next;
			memFree(TransTablePtr);
		}
	}

	obwldKill();

	Write_Debug("Pool objects still allocated: %d", numobjused);

	for (lpobj=objused.next ; lpobj != nullptr ; lpobj=next)
	{
		next = lpobj->next;
		obpoolFreeObject((POOLOBJHDR*)lpobj);
	}

	numobjused = 0;
	numobjects = 0;

	if (objects != nullptr)
	{
		memFree(objects);
		objects = nullptr;
	}

   // FreeQueuedObjects();

    Write_Log(LOG_OLD, LOG_INFO, "[obpoolKill]");
}



/************************************************************************************************
	obpoolObjectToPool
*************************************************************************************************/
// Copy a named (usually group) 3d object tree to the pool.
POOLOBJHDR* obpoolObjectToPool(OBJECT* lpgrp, FLOAT32 localscale, SINT32 reduce)
{

	POOLOBJHDR*	lphdr = ScanObjectCallback(lpgrp, nullptr, CopyObjectToPool);;

	assert(lpgrp);
	Write_Debug("Object %s was copied to pool", lpgrp->name);

	return lphdr;
}

//////////////////////////////////////////////////////////////////////
//	FreeObjectTree
//////////////////////////////////////////////////////////////////////
static void FreeObjectTree(OBJECT* o)
{
    if(o)
    {
        FreeObjectTree(o->child);
        FreeObjectTree(o->sibling);
        FreeObjectTree(o->attached);

	    if(o->flag2 & OBJECT_SPEEDTREE || o->flag3 & OBJECT_TREE_CLUSTER)
        {
            sptFreeTreeByObject(o);
        }

        if(o->flag2 & OBJECT_IS_GRASS)
        {
            trngrassFree(o);
        }

        listDeleteOBJECT(o);
        obmainFreeObject(o);
    }

	return;
}

/************************************************************************************************
	FreeQueuedObjects
*************************************************************************************************/
static void FreeQueuedObjects()
{
    while(!FreeObjectQue.empty())
    {
        OBJECT* o = FreeObjectQue.front();
        FreeObjectQue.pop_front();

        if(!obmainValidateObject(o))
            continue;

        if(o->modelInstance)
        {
            delete o->modelInstance;
            o->modelInstance = nullptr;
        }

        listDeleteOBJECT(o);

        FreeObjectTree(o);
    }
}

/************************************************************************************************
	obpoolUpdate
*************************************************************************************************/
void obpoolUpdate()
{
    FreeQueuedObjects();
}

/************************************************************************************************
	obpoolFreeObjectFinish
*************************************************************************************************/

void obpoolFreeObjectFinish()
{
    FreeQueuedObjects();
#if SERIALIZED
    while(objectList.next != nullptr)
    {
        FreeObjectSerialized();
    }
#endif
}

/************************************************************************************************
	obpoolFreeObjectCallback
*************************************************************************************************/

LOGICAL obpoolFreeObjectCallback(OBJECT* lpgrp)
{
	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

    FreeObjectQue.push_back(lpgrp);

    return TRUE;
}

/************************************************************************************************
	obpoolGetNewObject
*************************************************************************************************/
// Get a new pool object
POOLOBJHDR* obpoolGetNewObject()
{
POOLOBJHDR*    lphdr;

	lphdr = (POOLOBJHDR*)listGetItem((LISTHEADER*)&objfree);
	if (lphdr == nullptr)
	{
		Write_Debug("[obpoolGetNewObject] No more storage available for objects!");
		return(nullptr);
	}

	numobjused += 1;
	memset(lphdr, 0, sizeof(POOLOBJHDR));
	listAddItem((LISTHEADER*)&objused, (LISTHEADER*)lphdr);

	lphdr->matid = -1;
	lphdr->texid = -1;
	lphdr->gfxmodes = 0;

	return(lphdr);
}

/************************************************************************************************
	obpoolFindNamedObject
*************************************************************************************************/
// Find the pool object by name.
POOLOBJHDR* obpoolFindNamedObject(char* name)
{
POOLOBJHDR*	lphdr;
POOLOBJ*		lppool;

	lphdr = (POOLOBJHDR*)objused.next;
	while (lphdr != nullptr)
	{
		lppool = lphdr->pool;
		// search only the topmost objects in the list
		if (!(lppool->flags & OBJECT_ATTACHED))
		{
			if (lppool->name[0] == name[0])
			{
				if (lstrcmp(lppool->name, name) == 0)
				{
					return(lphdr);
				}
			}
		}

		lphdr = (POOLOBJHDR*)lphdr->next;
	}

	return(nullptr);
}


/************************************************************************************************
	obpoolCopyObject
*************************************************************************************************/
// Copy a pool object to a new pool object.
POOLOBJHDR* obpoolCopyObject(POOLOBJ* lpcopy)
{
POOLOBJHDR*	lphdr;

	lphdr = obpoolGetNewObject();
	if (lphdr == nullptr)
	{
		return(nullptr);
	}

	lphdr->pool =(POOLOBJ*)memAlloc(lpcopy->size);
	if(lphdr->pool != nullptr)
	{
		memcpy(lphdr->pool, lpcopy, lpcopy->size);
	}
	return(lphdr);
}

/************************************************************************************************
	obpoolCopyNamedObject
*************************************************************************************************/
// Copy a named pool object to a new pool object.
POOLOBJHDR* obpoolCopyNamedObject(char* name)
{
POOLOBJHDR*	lphdr;

	lphdr = obpoolFindNamedObject(name);
	if (lphdr == nullptr)
	{
		return(nullptr);
	}

	return(obpoolCopyObject(lphdr->pool));
}

/************************************************************************************************
	obpoolGetObjects
*************************************************************************************************/
// Get the pool object list head and the
// number of used objects in the pool.
SINT32 obpoolGetObjects(POOLOBJ* *lpobj)
{
	if (lpobj == nullptr)
	{
		return(-1);
	}

	*lpobj = (POOLOBJ*)objused.next;

	return(numobjused);
}

/************************************************************************************************
	obpoolGetObjectSize
*************************************************************************************************/
// Get the size of a 3d object as would be
// required to copy it to the pool.
LOGICAL obpoolGetObjectSize(int* objsize, OBJECT* lpobj)
{
int		os;

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	os = SIZEOF_POOLOBJ;

	if (!(lpobj->flags & OBJECT_NOTOBJECT))
	{
		os += SIZEOF_OBJDATA;

		// puverts
		os += lpobj->numverts * sizeof(piUNIFIED_VERTEX);

		if(lpobj->flag2 & OBJECT_NORMALMAP)
		{
			// tanent data
			os += lpobj->numverts * sizeof(piQUAT);
		}

		// ptris
		os += lpobj->numtris * sizeof(piINDICES);

		if(lpobj->psnorms)
		{
			// psnorms
			os += lpobj->numtris * sizeof(piVECTOR);
		}

		if(lpobj->pscdon)
		{
			// pscdon
			os += lpobj->numtris * 3 * sizeof(piVECTOR);
		}

		if(lpobj->meshContext)	os += lpobj->meshContext->GetPoolSize();
		//if(lpobj->textureContext)	os += lpobj->textureContext->GetPoolSize();
		//if(lpobj->shaderContext)	os += lpobj->shaderContext->GetPoolSize();
	}

	if (lpobj->flags & OBJECT_DOF)
	{
		os += SIZEOF_DOF;
	}

	if (lpobj->comment != nullptr)
	{
		// Add the strlen + null byte 4 byte aligned (+3&~3)
		os += (strlen(lpobj->comment) + 1 + 3) & ~3;
	}

	if (lpobj->lpspare != nullptr)
	{
		os += *(SINT32*)lpobj->lpspare;
	}

	*objsize = os;

	return(TRUE);
}

/************************************************************************************************
	obpoolFreeObject
*************************************************************************************************/
// Free a pool object, remove it from
// the used pool tree and place it in
// the free pool tree.
LOGICAL obpoolFreeObject(POOLOBJHDR* lphdr)
{
POOLOBJ*	lppool;

	if (lphdr == nullptr)
	{
		return(FALSE);
	}

	listDeleteItem((LISTHEADER*)lphdr);

	lppool = lphdr->pool;

	if (lppool == nullptr)
	{
		return(FALSE);
	}

	memFree(lppool);

	lphdr->texid = -1;
	lphdr->matid = -1;
	lphdr->gfxmodes = 0;

	listAddItem((LISTHEADER*)&objfree, (LISTHEADER*)lphdr);
	numobjused -= 1;

	return(TRUE);
}


/************************************************************************************************
	obpoolPoolOffsetsCallback
*************************************************************************************************/
// Routine to scan a 3d pool object, completely.
// Each component pool object has it's relative offsets updated.
// Objects are joined together like the original pool objects.
LOGICAL obpoolPoolOffsetsCallback(POOLOBJHDR* lpgrp, POOLOBJHDR* lpparent, UINT32* pooloff)
{
	POOLOBJHDR*	lpobj;
	POOLOBJHDR*	lpatt;
	POOLOBJHDR*	lplast;
	UINT32			offset;

	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

	offset = lpgrp->pool->size;
	if (lpparent != nullptr)
	{
		lpparent->pool->sibling = lpparent->pool->size;
	}

	lpparent = lpgrp;

	lpobj = lpgrp;
	while (lpobj != nullptr)
	{
		if (lpobj->sibling != nullptr)
		{
			if (!obpoolPoolOffsetsCallback(lpobj->sibling, lpobj, &offset))
			{
				return(FALSE);
			}
		}

		if (lpobj->attached != nullptr)
		{
			lpobj->pool->attached = offset;
			lplast = lpobj;
			lpatt = lpobj->attached;
			while (lpatt != nullptr)
			{
				if (lplast->pool->attached == 0)
				{
					lplast->pool->attached = lplast->pool->size;
				}

				offset += lplast->pool->size;
				lplast = lpatt;
				lpatt = lpatt->attached;
			}
		}

		lplast = lpobj->child;
		if (lplast != nullptr)
		{
			lpobj->pool->child = offset;
			offset += lpobj->pool->size;
			lpparent = lpobj;
		}

		lpobj = lplast;
	}

	*pooloff += offset;

	return(TRUE);
}

/************************************************************************************************
	obpoolSavePoolCallback
*************************************************************************************************/
// Routine to scan a 3d pool object, completely.
// Each component pool object has it's relative offsets updated.
// Objects are joined together like the original pool objects.
LOGICAL obpoolSavePoolCallback(POOLOBJHDR* lpgrp, int hfile)
{
	POOLOBJHDR*	lpobj;
	POOLOBJHDR*	lpatt;
	POOLOBJHDR*	lplast;

	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

	SavePool(hfile, lpgrp->pool);

	lpobj = lpgrp;
	while (lpobj != nullptr)
	{
		if (lpobj->sibling != nullptr)
		{
			if (!obpoolSavePoolCallback(lpobj->sibling, hfile))
			{
				return(FALSE);
			}
		}

		if (lpobj->attached != nullptr)
		{
			lplast = lpobj;
			lpatt = lpobj->attached;
			while (lpatt != nullptr)
			{
				SavePool(hfile, lpatt->pool);
				lplast = lpatt;
				lpatt = lpatt->attached;
			}
		}

		lplast = lpobj->child;
		if (lplast != nullptr)
		{
			SavePool(hfile, lplast->pool);
		}

		lpobj = lplast;
	}

	return(TRUE);
}

/************************************************************************************************
	ScanObjectCallback
*************************************************************************************************/
// Routine to scan a 3d object, completely.
// Each component object is converted to a pool object.
// Pool objects are joined together like the original objects.
static POOLOBJHDR* ScanObjectCallback(OBJECT* lpgrp, POOLOBJHDR* lpparent, SCANOBJECT callfunc)
{
	POOLOBJHDR* lproot;
	POOLOBJHDR*	lphdr;
	POOLOBJHDR*	lplast;
	POOLOBJHDR*	topmost;
	assert(lpgrp);

	callfunc(lpgrp, &lproot);
	if (lpparent != nullptr)
	{
		lpparent->sibling = lproot;
	}

	topmost = lproot;
	lpparent = lproot;

	OBJECT* lpobj = lpgrp;
	while (lpobj != nullptr)
	{
		if (lpobj->sibling != nullptr)
		{
			if(!ScanObjectCallback(lpobj->sibling, lpparent, callfunc))
			{
				return(nullptr);
			}
		}

		lplast = lproot;
		if (lpobj->attached != nullptr)
		{
			OBJECT* lpatt = lpobj->attached;
			while (lpatt != nullptr)
			{
				callfunc(lpatt, &lphdr);
				lplast->attached = lphdr;
				lplast = lphdr;
				lpatt = lpatt->attached;
			}
		}

		lpobj = lpobj->child;
		if (lpobj != nullptr)
		{
			callfunc(lpobj, &lproot);
			lpparent->child = lproot;
			lpparent = lproot;
		}
	}

	return(topmost);
}


/************************************************************************************************
	SavePool
*************************************************************************************************/

static void SavePool(int hfile, POOLOBJ* lppool)
{
	fioWrite(hfile, lppool, lppool->size);
}


/************************************************************************************************
	CopyObjectToPool
*************************************************************************************************/
// Copies a 3d object to a new pool object.
// Only used fields are copied.
// NOTE: All pointers become offsets in pool.
static LOGICAL CopyObjectToPool(OBJECT* lpobj, POOLOBJHDR** lpret)
{
	SINT32			objsize, elemsize;
	UINT32			offset;
	UINT8*			ptr;
	POOLOBJHDR*	lphdr;
	POOLOBJ*		lppool;

	objsize = 0;
	if (!obpoolGetObjectSize((int*)&objsize, lpobj))
	{
		return(FALSE);
	}

	lphdr = obpoolGetNewObject();
	if (lphdr == nullptr)
	{
		return(FALSE);
	}

	lppool = (POOLOBJ*)memAlloc(objsize);
	lphdr->pool = lppool;

	memcpy(lppool->name, lpobj->name, 32);
	lppool->flags	= lpobj->flags;
	lppool->flag2	= lpobj->flag2;
    lppool->flag3   = lpobj->flag3;

	lppool->flag2	|= OBJECT_POOL;

	offset = SIZEOF_POOLOBJ;
	ptr = (UINT8*)lppool;
	ptr += offset;

	if (!(lpobj->flags & OBJECT_NOTOBJECT))
	{
		POOLOBJDATA* objdata = (POOLOBJDATA*)ptr;
		lppool->objdata = offset;

		objdata->feature= lpobj->feature;
		objdata->layer	= lpobj->layer;
		objdata->level	= lpobj->level;
		objdata->loc	= lpobj->loc;
		objdata->pos	= lpobj->pos;

		if (lpobj->matid != -1)
		{
			memset(objdata->matname, 0, 32);

            MATERIAL* materials = nullptr;
            piGetMaterials(&materials);

            int id = oglFindMaterialName(lpobj->matname);

            // Duplicate materials are by values. Names may be different, so make
            // sure the name of the material is the name in the data structure.
            if(id != -1)
            {
                strncpy(objdata->matname, materials[id].name, 31);
            }
            else
            {
			    strncpy(objdata->matname, lpobj->matname,31);
            }
		}

		if (lpobj->texid != -1)
		{
			memset(objdata->texname, 0, 32);
			strncpy(objdata->texname, lpobj->texname,31);
		}

		objdata->minrad	= lpobj->minrad;
		objdata->maxrad	= lpobj->maxrad;
		objdata->minlod	= lpobj->minlod;
		objdata->maxlod	= lpobj->maxlod;
		memcpy(objdata->obox, lpobj->obox, 2 * sizeof(piVECTOR));
		memcpy(objdata->bbox, lpobj->bbox, 2 * sizeof(piVECTOR));

		elemsize = SIZEOF_OBJDATA;
		offset += elemsize;
		ptr += elemsize;

		if (lpobj->numverts != 0)
		{
			objdata->numverts = lpobj->numverts;

			// puverts
			elemsize = lpobj->numverts * sizeof(piUNIFIED_VERTEX);
			memcpy(ptr, lpobj->puverts, elemsize);
			objdata->puverts = offset;
			offset += elemsize;
			ptr += elemsize;

#if(0)
			if(lpobj->flag2 & OBJECT_NORMALMAP)
			{
				piQUAT* tangentData = ldmdlTangentArray3(lpobj);

				elemsize = lpobj->numverts * sizeof(piQUAT);
				memcpy(ptr, tangentData, elemsize);
				objdata->tangentData = offset;
				offset += elemsize;
				ptr += elemsize;

				memFree(tangentData);
			}
#endif
		}

		if (lpobj->numtris != 0)
		{
			objdata->numtris = lpobj->numtris;

			// ptris
			elemsize = lpobj->numtris * sizeof(piINDICES);
			memcpy(ptr, lpobj->ptris, elemsize);
			objdata->ptris = offset;
			offset += elemsize;
			ptr += elemsize;

			// psnorms
			if(lpobj->psnorms)
			{
				elemsize = lpobj->numtris * sizeof(piVECTOR);
				memcpy(ptr, lpobj->psnorms, elemsize);
				objdata->psnorms = offset;
				offset += elemsize;
				ptr += elemsize;
			}

			if(lpobj->pscdon)
			{
				// pscdon
				elemsize = lpobj->numtris * 3 * sizeof(piVECTOR);
				memcpy(ptr, lpobj->pscdon, elemsize);
				objdata->pscdn = offset;
				offset += elemsize;
				ptr += elemsize;
			}
		}

#if defined(OPENGL3D) && platOS == OS_WIN32
		obpoolColorSwap(lppool);
#endif

		if(lpobj->numverts && !(lpobj->flag2 & OBJECT_HIDDEN))
		{
			assert(lpobj->meshContext);
			elemsize = lpobj->meshContext->GetPoolSize();
			lpobj->meshContext->CopyToPool(ptr);
			offset += elemsize;
			ptr += elemsize;
		}
	}

	if (lpobj->flags & OBJECT_DOF)
	{
		elemsize = sizeof(OBJECTSTATICDOF);
		memcpy(ptr, lpobj->dof->dofstatic, elemsize);
		lppool->dof = offset;
		offset += elemsize;
		ptr += elemsize;

		elemsize = sizeof(OBJECTDOF) - sizeof(OBJECTSTATICDOF*);
		memcpy(ptr, &lpobj->dof->dofvarvalue, elemsize);
		offset += elemsize;
		ptr += elemsize;
	}

	if (lpobj->comment != nullptr)
	{
		elemsize = (strlen(lpobj->comment) + 1 + 3) & ~3;	// fast 4 byte align (+3&~3)
        memset(ptr, 0, elemsize);
        strlcpy(reinterpret_cast<char *>(ptr), lpobj->comment, elemsize);
		lppool->comment = offset;
		offset += elemsize;
		ptr += elemsize;
	}

	if (lpobj->lpspare != nullptr)
	{
		elemsize = *(SINT32*)lpobj->lpspare;
		if (elemsize != 0)
		{
			memcpy(ptr, lpobj->lpspare, elemsize);
			lppool->spare = offset;
			offset += elemsize;
			ptr += elemsize;
		}
	}

	lppool->size = offset;
	*lpret = lphdr;

	return(TRUE);
}


#if defined(OPENGL3D) && platOS == OS_WIN32
void obpoolColorSwap(POOLOBJ* lppool)
{
	UINT32		numverts;
	POOLOBJDATA*	objdata;
	piUNIFIED_VERTEX*	puverts;

	if (!(lppool->flags & OBJECT_NOTOBJECT))
	{
		objdata  = OBJDATA(lppool);
		numverts = objdata->numverts;
		if (numverts != 0)
		{
			UINT32	i;

			puverts = PUVERTS(lppool);

			// convert from ABGR to OpenGL format ARGB
			for(i = 0; i < numverts; i++, puverts++)
			{
				puverts->color = piRGBA_MAKE(piRGBA_GETBLUE(puverts->color), piRGBA_GETGREEN(puverts->color), piRGBA_GETRED(puverts->color), piRGBA_GETALPHA(puverts->color));
			}
		}
	}
}
#endif
