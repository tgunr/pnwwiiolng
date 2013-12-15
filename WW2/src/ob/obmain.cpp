#include <syscfg.h> // Path is project specific.
#include <set>
#include <ctype.h>
#include <math.h>
#include <deque>

#include "pi/pi.h"
#include "ld/ldmdl.h"
#include "ld/ldflt.h"
#include "include/vars.h"
#include "list/list.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obdlite.h"
#include "ob/obdraw.h"
#include "ob/obslite.h"
#include "ob/obplyr.h"
#include "ob/obwld.h"
#include "ob/obpool.h"
#include "trn/trnmain.h" // for world origin offset
#include "ld/ldpix.h"
#include "fx/FXInterface.h"
#include "fx/fxMgr.h"
#include "math/math3d.h"
#include "gr2/gr2Anim.h"
#include "physics/physicsMain.h"
#include "ogl/oglMesh.h"
#include "ob/obMgr.h"
#include "ogl/oglMaterial.h"
#if USE_STATESET
#include "ogl/States.h"
#endif

#if NEW_OBJECT_MEM
#define MAX_BASE_OBJECTS    40960
#endif

#include "spt/sptmain.h"

#if platOS == OS_MACOS
#include <pthread.h>
#endif

typedef UNORDERED_SET<OBJECT*> ObjectSet;

#if !NEW_OBJECT_MEM
static ObjectSet objectmap;
#endif

#if NEW_OBJECT_MEM
static OBJECT freehead;
static OBJECT freetail;
static std::deque<OBJECT*> freechunks;
void obmainAllocateObjects(int size);
#endif

static ObjectSet updatecallbacks;

int NumObjUsed = 0;
static OBJECT		objused;
static OBJECT		topmostlist;

static LOGICAL _SnapSubObjectTreeToXYGrid(OBJECT * Object,FLOAT32 GridSize,UINT32 UseObjectMatrix);
static LOGICAL _SetObjectSubTreeMatrix(OBJECT * Obj, piMATRIX * Mat);

static OBJECT* _newObject(UINT32 obtype);
void obmainFreeObjectMemory(OBJECT*  lpobj);

#ifdef MEMORY_DEBUG
#include <algorithm>
static std::map<OBJECT*, std::pair<std::string, int> > memoryDebug;
#endif

#ifdef ANDREWTEST
#undef ANDREWTEST
#endif

#ifdef ANDREWTEST
static std::set<OBJECT*> objectset;
static int dumpper = true;
#endif

#if NEW_OBJECT_MEM
static OBJECT* PopFree()
{
    if(freehead.next == &freetail)
    {
        obmainAllocateObjects(4096);
    }

    OBJECT* o = freehead.next;
    freehead.next = o->next;
    o->next->prev = &freehead;

    memset(o, 0, sizeof(OBJECT));

    o->valid = true;

    return o;
}

static void PushFree(OBJECT* o)
{
    OBJECT* tailprev = freetail.prev;

    // Push to back
    o->next = &freetail;
    o->prev = tailprev;

    freetail.prev = o;
    tailprev->next = o;

    o->valid = false;
}
#endif

static LOGICAL listAddTopMostOBJECTAfter(OBJECT* objectlist, OBJECT* after);

OBJECT* obmainGetNewObjectA()
{
	OBJECT* lpobj = _newObject(0);

	if (lpobj != NULL)
	{
		listAddItemAfter((LISTHEADER*)&objused, (LISTHEADER*)lpobj);
	}else
	{
		Write_Debug("[obmainGetNewObject] No more storage available for objects!");
		piExitProgram(__FILE__, __LINE__, "No more storage available for objects");
	}
	return lpobj;
}

OBJECT* obmainGetNewNotObjectA(UINT32 obtype)
{
	OBJECT* lpobj = _newObject(obtype);

	if (lpobj != NULL)
	{
		listAddItemBefore((LISTHEADER*)&objused, (LISTHEADER*)lpobj);
	}else
	{
		Write_Debug("[obmainGetNewNotObject] No more storage available for objects!");
		piExitProgram(__FILE__, __LINE__, "No more storage available for objects");
	}

	return(lpobj);

}

OBJECT* obmainGetFreeObjectA()
{
    OBJECT* o = _newObject(0);

    if(o)
    {
		o->matid = -1;
		o->texid = -1;
		o->texname = "";
		o->matname = "";
        o->mat = IdentityMatrix;
        o->comment = NULL;
    }

    return o;
}

LOGICAL obmainFreeObjectA(OBJECT* lpobj)
{
	if (lpobj == NULL)
		return FALSE;

#if !NEW_OBJECT_MEM
	ObjectSet::iterator it = objectmap.find(lpobj);
	if(it != objectmap.end())
		objectmap.erase(it);
    else
        return FALSE;
#else
    if(!lpobj->valid)
        return FALSE;
#endif

	if(lpobj->flag2 & OBJECT_SPEEDTREE || lpobj->flag3 & OBJECT_TREE_CLUSTER)
    {
        sptFreeTreeByObject(lpobj);
    }

    // Remove bullet hits, etc
    fxMgrDetach(lpobj);

  /* bogus check
	if (NumObjUsed <= 0)
		Write_Debug("Attempt to free object when none are used!");
	else
		NumObjUsed -= 1;
  */

	if(lpobj->collider)
		physicsFreeObjectCollider(lpobj);

	listDeleteOBJECT(lpobj);
	obmainFreeObjectMemory(lpobj); // decrements NumObjUsed

	return TRUE;
}

#ifdef MEMORY_DEBUG
OBJECT* obmainGetNewObjectDebug(char* from, int line)
{
    OBJECT* o = obmainGetNewObjectA();
    memoryDebug[o] = std::make_pair(std::string(from) + "_obmainGetNewObject", line);

    return o;
}

OBJECT* obmainGetNewNotObjectDebug(unsigned int type, char* from, int line)
{
    OBJECT* o = obmainGetNewNotObjectA(type);
    memoryDebug[o] = std::make_pair(std::string(from) + "_obmainGetNewNotObject", line);

    return o;
}

OBJECT* obmainGetFreeObjectDebug(char* from, int line)
{
    OBJECT* o = obmainGetFreeObjectA();
    memoryDebug[o] = std::make_pair(std::string(from) + "_obmainGetFreeObject", line);

    return o;
}

LOGICAL obmainFreeObjectDebug(OBJECT* o, char* from, int line)
{
    memoryDebug.erase(o);
    return obmainFreeObjectA(o);
}
#endif

LOGICAL obmainMoveToNotObjectTree(OBJECT* lpobj)
{
#if defined(BETA_RELEASE) && (platOS == OS_MACOS)
// doesn't work anymore since goddamn shawnce changed the application loop
//	if(!pthread_main_np())
//		piExitProgram(__FILE__, __LINE__, "Adding object outside main thread!");
#endif

	if (lpobj == NULL)
	{
		return(FALSE);
	}

	listDeleteOBJECT(lpobj);

	if (lpobj->flag2 & OBJECT_TOPMOST)
	{
		listAddTopMostOBJECTAfter(&topmostlist,lpobj);
	}
	else
	{
		listAddItemBefore((LISTHEADER*)&objused, (LISTHEADER*)lpobj);
	}

	return(TRUE);
}

LOGICAL obmainMoveToObjectTree(OBJECT* lpobj)
{
#if defined(BETA_RELEASE) && (platOS == OS_MACOS)
//	if(!pthread_main_np())
//		piExitProgram(__FILE__, __LINE__, "Adding object outside main thread!");
#endif

	if (lpobj == NULL)
	{
		return(FALSE);
	}

	listDeleteOBJECT(lpobj);

	if (lpobj->flag2 & OBJECT_TOPMOST)
	{
		listAddTopMostOBJECTAfter(&topmostlist,lpobj);
	}
	else
	{
		listAddItemAfter((LISTHEADER*)&objused, (LISTHEADER*)lpobj);
	}

	return(TRUE);
}

LOGICAL obmainAttachObject(OBJECT* lpmain, OBJECT* lpobj)
{
float			minrad, maxrad;
piVECTOR	loc;
piVECTOR	obox[2];

	if (lpmain == NULL ||
		lpobj == NULL)
	{
		return(FALSE);
	}

	lpobj->flags |= OBJECT_ATTACHED;
	lpobj->attached = lpmain->attached;
	lpmain->attached = (OBJECT*)lpobj;

	if (!ldmdlGetObjectBounds(lpmain, &loc, obox, &minrad, &maxrad))
	{
		return(FALSE);
	}

	lpmain->loc = loc;
	lpmain->obox[0] = obox[0];
	lpmain->obox[1] = obox[1];

	return(TRUE);
}

LOGICAL obmainAddChildObject(OBJECT* lpmain, OBJECT* lpobj)
{
	if (lpmain == NULL ||
		lpobj == NULL)
	{
		return(FALSE);
	}

	lpobj->child = lpmain->child;
	lpmain->child = (OBJECT*)lpobj;

	return(TRUE);
}

LOGICAL obmainSetObject(OBJECT* lpnew, OBJECT* lpold)
{
	if (lpnew == NULL ||
		lpold == NULL)
	{
		return(FALSE);
	}

	OBJECT* prev = (OBJECT*)lpnew->prev;
	OBJECT* next = (OBJECT*)lpnew->next;
	*lpnew = *lpold;
	lpnew->prev = (OBJECT*)prev;
	lpnew->next = (OBJECT*)next;

	return(TRUE);
}

void obmainFreeObjectMemory(OBJECT*  lpobj)
{
#ifdef ANDREWTEST
    if ( dumpper && NumObjUsed > 100000 )
    {
        int numverts = 0;
        int numtris = 0;
        int poolcount = 0;

        std::map<const char*,int> models;

        std::set<OBJECT*>::iterator it = objectset.begin();
        while (it!= objectset.end())
        {
            if ((*it)->modelInstance ) //&& (*it)->freepsnorms)
            {
                ModelInstance* mi = (*it)->modelInstance;
                std::map<const char*,int>::iterator iter = models.find(mi->GetModelName());
                if ( iter != models.end() )
                {
                    (*iter).second++;
                }
                else
                {
                    models.insert( std::pair<const char*,int>(mi->GetModelName(),1) );
                }
            }

            numverts += (*it)->numverts;
            numtris += (*it)->numtris;

            it++;
        }

        printf("Count: %d numverts: %d numtris: %d \n",
               NumObjUsed, numverts, numtris
               );

        std::map<const char*,int>::iterator iter = models.begin();
        while ( iter != models.end() )
        {
            printf( "%s %d\n", (*iter).first, (*iter).second );
            iter++;
        }

        dumpper = 0;
    }
    objectset.erase(lpobj);
#endif

	if(!(lpobj->flags & OBJECT_NOTOBJECT))
	{
        if(lpobj->killcallback)
        {
            lpobj->killcallback(lpobj);
            lpobj->killcallback = NULL;
        }

        // Special flag for memory leak issues
		if(lpobj->freepsnorms)
		{
			memFree(lpobj->psnorms);
			lpobj->psnorms = NULL;
            lpobj->freepsnorms = false;
		}

        // Special flag for memory leak issues
        if(lpobj->freepscdn)
		{
			memFree(lpobj->pscdn);
			lpobj->pscdn = NULL;
            lpobj->freepscdn = false;
		}

        // Normal free for psnorms
	    if(lpobj->psnorms && lpobj->psnorms != lpobj->psnorig)
		{
			memFree(lpobj->psnorms);
			lpobj->psnorms = NULL;
		}

        // Normal free for pscdn
		if(lpobj->pscdn && lpobj->pscdn != lpobj->pscdon)
		{
			memFree(lpobj->pscdn);
			lpobj->pscdn = NULL;
		}

		if(!(lpobj->flag2 & OBJECT_POOL))
		{
			if(lpobj->psnorig) 		{ memFree(lpobj->psnorig); 		lpobj->psnorig = NULL; }
			if(lpobj->pscdon) 		{ memFree(lpobj->pscdon); 		lpobj->pscdon = NULL; }
			if(lpobj->ptris) 		{ memFree(lpobj->ptris); 		lpobj->ptris = NULL; }
			if(lpobj->puverts)		{ memFree(lpobj->puverts); 		lpobj->puverts = NULL; }
		}

		if (lpobj->pverts)
        {
            memFree(lpobj->pverts);
            lpobj->pverts = NULL;
        }

		if (lpobj->ptverts)
        {
            memFree(lpobj->ptverts);
            lpobj->ptverts = NULL;
        }

		// only free material and textures if not an animation
		if (!(lpobj->flag2 & OBJECT_ANIMATION))
		{
			// Don't really free the material; just make it invalid.
			lpobj->matid = -1;
			piFreeMipTextures(lpobj->texid);
			lpobj->texid = -1;
		}
	}
	else
	{
		if (lpobj->dof != NULL)
		{
			if(lpobj->dof->dofflags & DOF_FREE_MEMORY)
			{
				lpobj->dof->dofstatic = NULL; // Pool memory
				memFree(lpobj->dof);
				lpobj->dof = NULL;
			}
		}
	}

#ifdef USE_VERTEX_INDEX_BUFFERS
	if (lpobj->vibkey.vbid > 0) // lpobj->flag2 & OBJECT_USEBUFFERS)
	{
		piReleaseVertexSource(lpobj->vibkey.vbid);
		lpobj->vibkey.vbid = -1;
		if (lpobj->flags & OBJECT_INDEXED)
		{
			piReleaseIndexSource(lpobj->vibkey.ibid);
			lpobj->vibkey.ibid = -1;
		}
	}
#endif
	lpobj->flag2 &= ~OBJECT_USEBUFFERS;

	if (!(lpobj->flag2 & OBJECT_POOL))
	{
        lpobj->comment = NULL;
	}

	if ((!(lpobj->flags & OBJECT_NOTOBJECT) || ((lpobj->flags & OBJECT_NOTOBJECT) == OBJECT_GROUP)) && lpobj->shadow != NULL)
	{
		if (lpobj->shadow->flags & OBJECT_DYNASHADOW)
		{
			obmainFreeObject(lpobj->shadow);
			lpobj->shadow = NULL;
		}
	}

	if (lpobj->lpspare != NULL)
	{
		if (lpobj->flag2 & OBJECT_ANIMATION)
		{
			Write_Debug("%%%%%%%%%%%%%%%%%Freeing allocated lpspare!");
			memFree(lpobj->lpspare);

			lpobj->lpspare = NULL;
		}
	}

    if(lpobj->updatecallback)
    {
	    ObjectSet::iterator it = updatecallbacks.find(lpobj);

        if(it != updatecallbacks.end())
		    updatecallbacks.erase(it);

        lpobj->updatecallback = NULL;
    }

    lpobj->modelNode = NULL;
    lpobj->drawcallback = NULL;
    lpobj->cullcallback = NULL;
	lpobj->modelInstance = NULL;
    lpobj->collider = NULL;

	lpobj->flags = 0;
	lpobj->flag2 = 0;

	if (lpobj->animModel != NULL)
	{
		gr2AnimKillObject(lpobj);
	}

    if(lpobj->meshContext)
    {
	    delete lpobj->meshContext;
        lpobj->meshContext = NULL;
    }

    if(lpobj->material)
    {
        if(!lpobj->material->IsPool())
        {
            lpobj->material->unref();
        }

        // Do not delete. This is pointing to the pool material
        lpobj->material = NULL;
    }

#if NEW_OBJECT_MEM
    PushFree(lpobj);
#else

    NumObjUsed--;

	memFree(lpobj);
#endif
}

OBJECT* obmainFindNamedObject(const char* name)
{
OBJECT*    lpobj;

	lpobj = (OBJECT*)topmostlist.next;
	while (lpobj != NULL)
	{
		// search only the topmost objects in the list
		if (!(lpobj->flags & OBJECT_ATTACHED))
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->next;
	}


	lpobj = (OBJECT*)objused.next;
	while (lpobj != NULL)
	{
		// search only the topmost objects in the list
		if (!(lpobj->flags & OBJECT_ATTACHED))
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->next;
	}

	lpobj = (OBJECT*)objused.prev;
	while (lpobj != NULL)
	{
		// search only the topmost objects in the list
		if (!(lpobj->flags & OBJECT_ATTACHED))
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->prev;
	}



	return(NULL);
}

OBJECT* obmainFindChildObject(const char* group, const char* name)
{
	OBJECT* lpgrp = obmainFindNamedObject(group);
	if (lpgrp == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpgrp->child;
	while (lpobj != NULL)
	{
		if (lpobj->name[0] == name[0])
		{
			if (strcmp(lpobj->name, name) == 0)
			{
				return(lpobj);
			}
		}

		lpobj = lpobj->child;
	}

	return(NULL);
}

OBJECT* obmainFindChildFromObject(OBJECT* lpgrp, const char* name)
{
	if (lpgrp == NULL)
	{
		return(NULL);
	}

	OBJECT* lpobj = lpgrp;
	while (lpobj != NULL)
	{
		if (lpobj->name[0] == name[0])
		{
			if (strcmp(lpobj->name, name) == 0)
			{
				return(lpobj);
			}
		}

		if (lpobj->child != NULL)
		{
			OBJECT* lpfnd = obmainFindChildFromObject(lpobj->child, name);
			if (lpfnd != NULL)
			{
				return(lpfnd);
			}
		}

		lpobj = lpobj->sibling;
	}

	return(NULL);
}

// Does do siblings at top level!
LOGICAL obmainSetFlagsInObjectTree(OBJECT* lpmain, UINT32 flags)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlagsInObjectTree(lpobj->child, flags))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flags |= flags;
			lpatt = lpatt->attached;
		}

		lpobj->flags |= flags;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does do siblings at top level!
LOGICAL obmainClearFlagsInObjectTree(OBJECT* lpmain, UINT32 flags)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlagsInObjectTree(lpobj->child, flags))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flags &= ~flags;
			lpatt = lpatt->attached;
		}

		lpobj->flags &= ~flags;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainSetFlagsInThisTree(OBJECT* lpmain, UINT32 flags)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flags |= flags;
		lpatt = lpatt->attached;
	}

	lpobj->flags |= flags;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlagsInObjectTree(lpobj->child, flags))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flags |= flags;
			lpatt = lpatt->attached;
		}

		lpobj->flags |= flags;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainClearFlagsInThisTree(OBJECT* lpmain, UINT32 flags)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flags &= ~flags;
		lpatt = lpatt->attached;
	}

	lpobj->flags &= ~flags;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlagsInObjectTree(lpobj->child, flags))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flags &= ~flags;
			lpatt = lpatt->attached;
		}

		lpobj->flags &= ~flags;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does do siblings at top level!
LOGICAL obmainSetFlag2InObjectTree(OBJECT* lpmain, UINT32 flag2)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlag2InObjectTree(lpobj->child, flag2))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag2 |= flag2;
			lpatt = lpatt->attached;
		}

		lpobj->flag2 |= flag2;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does do siblings at top level!
LOGICAL obmainClearFlag2InObjectTree(OBJECT* lpmain, UINT32 flag2)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlag2InObjectTree(lpobj->child, flag2))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag2 &= ~flag2;
			lpatt = lpatt->attached;
		}

		lpobj->flag2 &= ~flag2;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainSetFlag2InThisTree(OBJECT* lpmain, UINT32 flag2)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flag2 |= flag2;
		lpatt = lpatt->attached;
	}

	lpobj->flag2 |= flag2;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlag2InObjectTree(lpobj->child, flag2))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag2 |= flag2;
			lpatt = lpatt->attached;
		}

		lpobj->flag2 |= flag2;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainClearFlag2InThisTree(OBJECT* lpmain, UINT32 flag2)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flag2 &= ~flag2;
		lpatt = lpatt->attached;
	}

	lpobj->flag2 &= ~flag2;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlag2InObjectTree(lpobj->child, flag2))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag2 &= ~flag2;
			lpatt = lpatt->attached;
		}

		lpobj->flag2 &= ~flag2;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Flag 3
// Does do siblings at top level!
LOGICAL obmainSetFlag3InObjectTree(OBJECT* lpmain, UINT32 flag3)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlag3InObjectTree(lpobj->child, flag3))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag3 |= flag3;
			lpatt = lpatt->attached;
		}

		lpobj->flag3 |= flag3;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does do siblings at top level!
LOGICAL obmainClearFlag3InObjectTree(OBJECT* lpmain, UINT32 flag3)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlag3InObjectTree(lpobj->child, flag3))
			{
				return(FALSE);
			}
		}

		OBJECT* lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag3 &= ~flag3;
			lpatt = lpatt->attached;
		}

		lpobj->flag3 &= ~flag3;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainSetFlag3InThisTree(OBJECT* lpmain, UINT32 flag3)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flag3 |= flag3;
		lpatt = lpatt->attached;
	}

	lpobj->flag3 |= flag3;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainSetFlag3InObjectTree(lpobj->child, flag3))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag3 |= flag3;
			lpatt = lpatt->attached;
		}

		lpobj->flag3 |= flag3;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

// Does NOT do siblings at top level!
LOGICAL obmainClearFlag3InThisTree(OBJECT* lpmain, UINT32 flag3)
{
	if (lpmain == NULL)
	{
		return(FALSE);
	}

	OBJECT* lpobj = lpmain;
	OBJECT* lpatt = lpobj->attached;
	while (lpatt)
	{
		lpatt->flag3 &= ~flag3;
		lpatt = lpatt->attached;
	}

	lpobj->flag3 &= ~flag3;
	lpobj = lpobj->child;

	while (lpobj != NULL)
	{
		if (lpobj->child != NULL)
		{
			if (!obmainClearFlag3InObjectTree(lpobj->child, flag3))
			{
				return(FALSE);
			}
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt->flag3 &= ~flag3;
			lpatt = lpatt->attached;
		}

		lpobj->flag3 &= ~flag3;
		lpobj = lpobj->sibling;
	}

	return(TRUE);
}

OBJECT* obmainFindGroupObject(const char* name)
{
OBJECT*    lpobj;

	lpobj = (OBJECT*)topmostlist.next;
	while (lpobj != NULL)
	{
		// search only the group objects in the list
		if (lpobj->flags & OBJECT_GROUP)
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->next;
	}

	lpobj = (OBJECT*)objused.prev;
	while (lpobj != NULL)
	{
		// search only the group objects in the list
		if (lpobj->flags & OBJECT_GROUP)
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->prev;
	}

	lpobj = (OBJECT*)objused.next;
	while (lpobj != NULL)
	{
		// search only the group objects in the list
		if (lpobj->flags & OBJECT_GROUP)
		{
			if (lpobj->name[0] == name[0])
			{
				if (strcmp(lpobj->name, name) == 0)
				{
					return(lpobj);
				}
			}
		}

		lpobj = (OBJECT*)lpobj->next;
	}

	return(NULL);
}

OBJECT* obmainGetFirstTopmostObject(void)
{
	return (OBJECT*)topmostlist.next;
}

OBJECT* obmainGetNextTopmostObject(OBJECT* lpobj)
{
	return lpobj->next;
}

OBJECT* obmainFindTopmostObject(const char* name)
{
OBJECT*    lpobj;

	lpobj = (OBJECT*)topmostlist.next;
	while (lpobj != NULL)
	{
		// search only the topmost objects in the list

        assert( lpobj->flag2 & OBJECT_TOPMOST);

		//if (lpobj->flag2 & OBJECT_TOPMOST)
		{

			if (strcasecmp(lpobj->name, name) == 0)
			{
				//return(lpobj);
                break;
			}
		}

		lpobj = (OBJECT*)lpobj->next;
	}

	return(lpobj);
}

OBJECT* obmainGetFirstSubobject(OBJECT* lpmain)
{
OBJECT*	lpobj;

	if (lpmain == NULL)
	{
		return(NULL);
	}

	if (lpmain->flags & OBJECT_GROUP)
	{
		lpobj = lpmain->child;
		while (lpobj)
		{
			if (lpobj->flags & OBJECT_GROUP)
			{
				break;
			}

			lpobj = lpobj->child;
		}

		return(lpobj);
	}

	return(lpmain);
}

OBJECT* obmainGetNextSubobject(OBJECT* lpobj)
{
	if (lpobj == NULL)
	{
		return(NULL);
	}

	return(lpobj->sibling);
}

LOGICAL obmainFindCrackedComment(SINT8** retptr, SINT8* comment, SINT8* cmp)
{
SINT8*	pre, *ptr;

	if (comment == NULL)
	{
		return(FALSE);
	}

	ptr = comment;
	while (*ptr)
	{
		while (iscntrl(*ptr))
		{
			ptr += 1;
		}

		pre = ptr;
		while (!iscntrl(*ptr))
		{
			ptr += 1;
		}

		if (strncmp(pre, cmp, ptr - pre) == 0)
		{
			*retptr = pre;
			return(TRUE);
		}
	}

	return(FALSE);
}

LOGICAL obmainFindCrackedCommentFirstPart(SINT8** retptr, SINT8* comment, SINT8* cmp)
{
	SINT8*	pre, *ptr;
	int len;
	if (comment == NULL)
	{
		return(FALSE);
	}

	len = strlen(cmp);
	ptr = comment;
	while (*ptr)
	{
		while (iscntrl(*ptr))
		{
			ptr += 1;
		}

		pre = ptr;
		while (!iscntrl(*ptr))
		{
			ptr += 1;
		}

		if (strncmp(pre, cmp, len) == 0)
		{
			*retptr = pre;
			return(TRUE);
		}
	}

	return(FALSE);
}
LOGICAL obmainFindCommentName(SINT8** retptr, SINT8* comment, SINT8* cmp)
{
SINT8*	pre, *ptr;

	if (comment == NULL)
	{
		return(FALSE);
	}

	ptr = comment;
	while (*ptr)
	{
		while (isspace(*ptr) && *ptr)
		{
			ptr += 1;
		}

		pre = ptr;
		while (!isspace(*ptr) && *ptr)
		{
			ptr += 1;
		}

		if (strncmp(pre, cmp, ptr - pre) == 0)
		{
			*retptr = ptr;
			return(TRUE);
		}

		pre = strstr(ptr, "\r\n");
		if (pre != NULL)
		{
			ptr = pre += 2;
		}
		else
		{
			break;
		}
	}

	return(FALSE);
}

LOGICAL obmainFindCommentFirstPartName(SINT8** retptr, SINT8* comment, SINT8* cmp)
{
	SINT8*	pre, *ptr;

	if (comment == NULL)
	{
		return(FALSE);
	}

	int n = strlen(cmp);

	ptr = comment;
	while (*ptr)
	{
		while (isspace(*ptr) && *ptr)
		{
			ptr += 1;
		}

		pre = ptr;
		while (!isspace(*ptr) && *ptr)
		{
			ptr += 1;
		}

		if (strncmp(pre, cmp, n) == 0)
		{
			*retptr = pre;
			return(TRUE);
		}

		pre = strstr(ptr, "\r\n");
		if (pre != NULL)
		{
			ptr = pre += 2;
		}
		else
		{
			break;
		}
	}

	return(FALSE);
}

LOGICAL obmainFindComment(OBJECT** retobj, OBJECT* lpmain, char* comment)
{
SINT8*		ptr;
OBJECT*    lpobj;

	if (lpmain == NULL)
	{
		return(FALSE);
	}

	lpobj = lpmain;
	while (lpobj)
	{
		if (lpobj->child)
		{
			if (obmainFindComment(retobj, lpobj->child, comment))
			{
				return(TRUE);
			}
		}

		if (obmainFindCrackedComment(&ptr, lpobj->comment, comment))
		{
			*retobj = lpobj;
			return(TRUE);
		}

		lpobj = lpobj->sibling;
	}

	return(FALSE);
}

LOGICAL obmainFindCommentCallback(OBJECT* lpmain, char* comment, UINT32 user, PROCESSCALLBACK callback)
{
LOGICAL		retval=FALSE;
SINT8*		ptr;
OBJECT*    lpobj;

	if (lpmain == NULL)
	{
		return(FALSE);
	}

	lpobj = lpmain;
	while (lpobj)
	{
		if (lpobj->child)
		{
			if (obmainFindCommentCallback(lpobj->child, comment, user, callback))
			{
				retval = TRUE;
			}
		}

		if (obmainFindCrackedComment(&ptr, lpobj->comment, comment))
		{
			callback(lpobj, user);
			retval = TRUE;
		}

		lpobj = lpobj->sibling;
	}

	return(retval);
}

LOGICAL obmainFindCommentFirstPartCallback(OBJECT* lpmain, char* comment, UINT32 user, PROCESSCALLBACK callback)
{
LOGICAL		retval=FALSE;
SINT8*		ptr;
OBJECT*    lpobj;

	if (lpmain == NULL)
	{
		return(FALSE);
	}

	lpobj = lpmain;
	while (lpobj)
	{
		if (lpobj->child)
		{
			if (obmainFindCommentFirstPartCallback(lpobj->child, comment, user, callback))
			{
				retval = TRUE;
			}
		}

		if (obmainFindCrackedCommentFirstPart(&ptr, lpobj->comment, comment))
		{
			callback(lpobj, user);
			retval = TRUE;
		}

		lpobj = lpobj->sibling;
	}

	return(retval);
}

LOGICAL obmainFindObjectLOD(OBJECT* lpmain, int lod, PROCESSCALLBACK callback)
{
SINT8		lodname[32];

	if (lpmain == NULL)
	{
		return(FALSE);
	}

	sprintf(lodname, "lod%d", lod);

	if (!obmainFindCommentCallback(lpmain, lodname, lod, callback))
	{
		return(FALSE);
	}

	return(TRUE);
}

LOGICAL obmainFindObjectDamage(OBJECT* lpmain, int dmg, PROCESSCALLBACK callback)
{
SINT8		dmgname[32];

	if (lpmain == NULL)
	{
		return(FALSE);
	}

	sprintf(dmgname, "damage %d", dmg);

	if (!obmainFindCommentCallback(lpmain, dmgname, dmg, callback))
	{
		return(FALSE);
	}

	return(TRUE);
}

void obmainFindCallback(struct OBJECT* lpmain, FINDCALLBACK callback, void* userdata)
{
	OBJECT* lpobj = NULL;

	if (lpmain == NULL)
		return;

	lpobj = lpmain;

	while (lpobj)
	{
		if (lpobj->child)
			obmainFindCallback(lpobj->child, callback, userdata);

		callback(lpobj, userdata);

		lpobj = lpobj->sibling;
	}
}


LOGICAL obmainCreateObjectFull(OBJECT** newobj, char* name, int numverts, int numtris)
{
	OBJECT*	lpobj;

	lpobj = obmainGetNewObject();
	if (lpobj == NULL)
	{
		return(FALSE);
	}

#ifdef SPATIAL_DB
    lpobj->sdbInfo = NULL;
#endif
	strcpy(lpobj->name, name);
	lpobj->flags = 0;

// Terrain loader changes the numverts on us so we use maxnumverts
	lpobj->numverts = numverts;
	lpobj->numtris =  numtris;
	// only allocate this stuff if we have non zero verts & tris
	if (numverts && numtris)
	{
		lpobj->puverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
		lpobj->ptris		= (piINDICES*)memAlloc(numtris * sizeof(piINDICES));
		lpobj->psnorig		= (piVECTOR*)memAlloc(numtris * sizeof(piVECTOR));
		lpobj->psnorms		= (piVECTOR*)memAlloc(numtris * sizeof(piVECTOR));
		lpobj->pscdon		= (piVECTOR*)memAlloc(numtris * 3 * sizeof(piVECTOR));
		lpobj->pscdn		= (piVECTOR*)memAlloc(numtris * 3 * sizeof(piVECTOR));


		if (lpobj->ptris == NULL ||
			lpobj->psnorms == NULL ||
			lpobj->psnorig == NULL ||
			lpobj->pscdon == NULL ||
			lpobj->pscdn == NULL)
		{
			obmainFreeObject(lpobj);
			return(FALSE);
		}

		//if (lpobj->flags & OBJECT_TESSELATION)
		{
			lpobj->ptverts		= (piCVERTEX*)memAlloc(numtris * 3 * sizeof(piCVERTEX));

            if (lpobj->ptverts == NULL)
			{
				obmainFreeObject(lpobj);
				return(FALSE);
			}
		}
	}
	else
	{
		lpobj->ptris = NULL;
		lpobj->psnorms = NULL;
		lpobj->psnorig = NULL;
		lpobj->pscdon = NULL;
		lpobj->pscdn = NULL;
		lpobj->ptverts = NULL;
	}

	lpobj->mat = IdentityMatrix;

	*newobj = lpobj;

	return(TRUE);
}

LOGICAL obmainCreateObject(OBJECT** newobj, char* name, int numverts, int numtris)
{
	return obmainCreateObjectFull(newobj, name, numverts, numtris);
}

OBJECT* obmainCopyObjectFull(OBJECT* lpcopy)
{
	int		nv, nt;
	OBJECT*	lpobj;

	if ((lpcopy->flags & (OBJECT_NOTOBJECT | OBJECT_USERDRAW)) || (lpcopy->flag2 & OBJECT_TOPMOST))
	{
		lpobj = obmainGetNewNotObject(lpcopy->flags);
	}
	else
	{
		lpobj = obmainGetNewObject();
	}

	if (lpobj == NULL)
	{
		return(NULL);
	}

	memcpy(&lpobj->name, &lpcopy->name, sizeof(OBJECT) - 5 * sizeof(OBJECT*));
	lpobj->flags |= OBJECT_COPY;
	lpobj->flag2 &= ~OBJECT_POOL;

	if (lpcopy->comment != NULL)
	{
		lpobj->comment = (SCHAR*)memAlloc(strlen(lpcopy->comment) + 1);
		strcpy(lpobj->comment, lpcopy->comment);
	}

	if (lpobj->flags & OBJECT_NOTOBJECT)
	{
		return(lpobj);
	}

	do
	{
		nv = lpcopy->numverts;

		if(lpcopy->puverts)
		{
			lpobj->puverts = (piUNIFIED_VERTEX*)memAlloc(nv * sizeof(piUNIFIED_VERTEX));
			memcpy(lpobj->puverts, lpcopy->puverts, nv * sizeof(piUNIFIED_VERTEX));
		}

		if(lpcopy->pverts)
		{
			lpobj->pverts = (piUNIFIED_VERTEX*)memAlloc(nv * sizeof(piUNIFIED_VERTEX));
			memcpy(lpobj->pverts, lpcopy->pverts, nv * sizeof(piUNIFIED_VERTEX));
		}

		if (lpcopy->flags & piPT_TRIANGLELIST || lpcopy->flags & piPT_TRIANGLEFAN)
		{
			nt = lpcopy->numtris;
			lpobj->ptris = (piINDICES*)memAlloc(nt * sizeof(piINDICES));
			if(lpcopy->psnorig)	lpobj->psnorig = (piVECTOR*)memAlloc(nt * sizeof(piVECTOR));
			if(lpcopy->pscdon)	lpobj->pscdon	= (piVECTOR*)memAlloc(nt * 3 * sizeof(piVECTOR));

			if(lpcopy->psnorms != lpcopy->psnorig)
				lpobj->psnorms = (piVECTOR*)memAlloc(nt * sizeof(piVECTOR));
			else
				lpobj->psnorms = lpobj->psnorig;

			if(lpcopy->pscdn != lpcopy->pscdon)
				lpobj->pscdn	= (piVECTOR*)memAlloc(nt * 3 * sizeof(piVECTOR));
			else
				lpobj->pscdn = lpobj->pscdon;

			memcpy(lpobj->ptris, lpcopy->ptris, nt * sizeof(piINDICES));
			if(lpcopy->psnorig)
			{
				memcpy(lpobj->psnorig, lpcopy->psnorig, nt * sizeof(piVECTOR));
				if(lpobj->psnorig != lpobj->psnorms)
					memcpy(lpobj->psnorms, lpcopy->psnorms, nt * sizeof(piVECTOR));
			}
			if(lpcopy->pscdon)
			{
				memcpy(lpobj->pscdon, lpcopy->pscdon, nt * 3 * sizeof(piVECTOR));
				if(lpobj->pscdon != lpobj->pscdn)
					memcpy(lpobj->pscdn, lpcopy->pscdn, nt * 3 * sizeof(piVECTOR));
			}
		}

		//if (lpcopy->flags & OBJECT_TESSELATION)
		{
			nt = lpcopy->numtris;
			nt *= 3;
			lpobj->ptverts = (piCVERTEX*)memAlloc(nt * sizeof(piCVERTEX));
			if (lpobj->ptverts == NULL)
			{
				break;
			}

			if (lpcopy->ptverts != NULL)
				memcpy(lpobj->ptverts, lpcopy->ptverts, nt * sizeof(piLVERTEX));
    	}

		piAddTexRef(lpobj->texid);

		return(lpobj);
	}
	while (FALSE);

	obmainFreeObject(lpobj);

	return(NULL);
}

OBJECT* obmainCopyObject(OBJECT* lpcopy)
{
	return obmainCopyObjectFull(lpcopy);
}

OBJECT* obmainCopyNamedObject(const char* name)
{
	OBJECT* lpobj = obmainFindNamedObject(name);
	if (lpobj == NULL)
	{
		return(NULL);
	}

	return(obmainCopyObject(lpobj));
}

int obmainGetObjects(LISTHEADER** lplist)
{
	if (lplist == NULL)
	{
		return(-1);
	}

	*lplist = (LISTHEADER*)&objused;

	return(NumObjUsed);
}

#if NEW_OBJECT_MEM
void obmainAllocateObjects(int size)
{
    OBJECT* o = (OBJECT*)memAlloc(sizeof(OBJECT) * size);
	freechunks.push_back(o);

    for(int i = 0; i < size; i++)
    {
        PushFree(&o[i]);
    }
}
#endif

LOGICAL obmainInit(void)
{
	NumObjUsed = 0;

#if NEW_OBJECT_MEM
	freehead.next = &freetail;
	freetail.prev = &freehead;

    obmainAllocateObjects(MAX_BASE_OBJECTS);
#endif

    if (!obdrawInit())
	{
		return(FALSE);
	}

	if (!obdliteInit())
	{
		return(FALSE);
	}

	if (!obpoolInit("blitzWor.csv"))
	{
		return(FALSE);
	}

	return(TRUE);
}


extern void ldfltKill() ;	// It's only used here, so why pollute namespaces with it?

void obmainKill(void)
{
	OBJECT*	lpobj, *prev, *next;

	obpoolKill();
	obdliteKill();
	obdrawKill();
    obpoolFreeObjectFinish();
    ldfltKill();

	Write_Debug("Objects still allocated: %d", NumObjUsed);

	for(lpobj=objused.next; lpobj != NULL; lpobj=next)
	{
		next = lpobj->next;
		obmainFreeObject((OBJECT*)lpobj);
	}

	for (lpobj=objused.prev; lpobj != NULL; lpobj=prev)
	{
		prev = lpobj->prev;
		obmainFreeObject((OBJECT*)lpobj);
	}

    OBJECT* o = topmostlist.next;
    OBJECT* free;

    while(o != NULL)
    {
        free = o;
        o = o->next;
        obmainFreeObject(free);
    }

#ifdef MEMORY_DEBUG
    static std::map<OBJECT*, std::pair<std::string, int> >::iterator itr;

    if(memoryDebug.empty())
    {
        Write_Debug("[obmainKill] NO MEMORY LEAKS DETECTED");
    }

    itr = memoryDebug.begin();
    for(; itr != memoryDebug.end(); ++itr)
    {
        Write_Debug("[obmainKill] MEMORY NOT FREED: file = %s line = %d", itr->second.first.c_str(), itr->second.second);
    }
#endif

#if NEW_OBJECT_MEM
	while(!freechunks.empty())
	{
		OBJECT* o = freechunks.front();
		freechunks.pop_front();

		memFree(o);
	}
#endif

    obmgrKill();

    Write_Log(LOG_OLD, LOG_INFO, "[obmainKill]");
}

LOGICAL obmainUpdate(void)
{
	if (!obwldUpdate())
		return FALSE;

    // So we don't have to loop entire object list
    ObjectSet::iterator itr = updatecallbacks.begin();
    OBJECT* o;

    std::vector<OBJECT*> freelist;

    for(; itr != updatecallbacks.end(); ++itr)
    {
        o = *itr;

        if(!obmainValidateObject(o))
            freelist.push_back(o);
        else if(o->updatecallback)
            o->updatecallback(o);
        else
            freelist.push_back(o);
    }

    for(unsigned int i = 0; i < freelist.size(); i++)
    {
        itr = updatecallbacks.find(freelist[i]);
        updatecallbacks.erase(itr);
    }

	return TRUE;
}

LOGICAL obmainSnapObjectTreeToXYGrid(OBJECT * Object,FLOAT32 GridSize,UINT32 UseObjectMatrix)
{
OBJECT * ObjAtt;

	if (Object == NULL)
	{
		return (FALSE);
	}

	ObjAtt = Object;

	do
	{
		obmainSnapObjectToXYGrid(ObjAtt,GridSize,UseObjectMatrix);

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != NULL);

	if (Object->child != NULL)
	{
		_SnapSubObjectTreeToXYGrid(Object->child,GridSize,UseObjectMatrix);
	}

	return (TRUE);
}

static LOGICAL _SnapSubObjectTreeToXYGrid(OBJECT * Object,FLOAT32 GridSize,UINT32 UseObjectMatrix)
{
OBJECT * ObjAtt,*ObjSib;

	if (Object == NULL)
	{
		return (FALSE);
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			obmainSnapObjectToXYGrid(ObjAtt,GridSize,UseObjectMatrix);

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != NULL);

		if (ObjSib->child != NULL)
		{
			_SnapSubObjectTreeToXYGrid(ObjSib->child,GridSize,UseObjectMatrix);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return (TRUE);
}


LOGICAL obmainSnapObjectToXYGrid(OBJECT * Object, FLOAT32 GridSize, UINT32 UseObjectMatrix)
{
UINT32 i;
FLOAT64 TempNum;
piVECTOR TempVec;
piMATRIX InvMat;
FLOAT64 OneOverGridSize;
FLOAT64VECTOR WorldOriginOffset;

	if (Object == NULL)
	{
		return (FALSE);
	}

	if (Object->flags & OBJECT_NOTOBJECT)
	{
		return (TRUE);
	}

	if (GridSize <= 0.0)
	{
		return (FALSE);
	}

	if (UseObjectMatrix == 1)
	{
		math3dInvertMatrix(&InvMat,&Object->mat);
	}

	trnGetWorldOriginOffset(&WorldOriginOffset);

	OneOverGridSize = 1.0/GridSize;

	piVectorPtr mpos = Object->MutablePositions();

	for (i = 0; i < Object->numverts; i++, ++mpos)
	{
		TempVec = *mpos;
		if (UseObjectMatrix == 1)
		{
			math3dVectorMultiply(&TempVec,&TempVec,&Object->mat);
		}

		TempNum = TempVec.x;

		TempNum += WorldOriginOffset.x;
		TempNum *= OneOverGridSize;
		TempNum = floor(TempNum + 0.5);
		TempNum *= GridSize;
		TempNum -= WorldOriginOffset.x;

		TempVec.x = (FLOAT32)TempNum;

		TempNum = TempVec.y;

		TempNum += WorldOriginOffset.y;
		TempNum *= OneOverGridSize;
		TempNum = floor(TempNum + 0.5);
		TempNum *= GridSize;
		TempNum -= WorldOriginOffset.y;

		TempVec.y = (FLOAT32)TempNum;

		if (UseObjectMatrix == 1)
		{
			math3dVectorMultiply(&TempVec,&TempVec,&InvMat);
		}

		mpos->x = TempVec.x;
		mpos->y = TempVec.y;
		mpos->z = TempVec.z;
	}

	return (TRUE);
}

// Get the overall bounding box
LOGICAL obmainGetBoundingBoxMax(FLOAT32* pfLeft,
		  				   	    FLOAT32* pfTop,
							    FLOAT32* pfRight,
							    FLOAT32* pfBottom,
								FLOAT32* pfMaxZ,
								FLOAT32* pfMinZ,
							    OBJECT* pObject)
{
	OBJECT* pObjSib = NULL;
	OBJECT* pObjAtt = NULL;

	if(pObject == NULL)
	{
		return FALSE;
	}

	if(pfLeft == NULL || pfTop == NULL ||
	   pfRight == NULL || pfBottom == NULL ||
	   pfMaxZ == NULL)
	{
		return FALSE;
	}

	pObjSib = pObject;
	pObjAtt = pObject;

	while(pObjAtt != NULL)
	{
		if(!(pObjAtt->flags & OBJECT_NOTOBJECT) &&
            (pObjAtt->flags & OBJECT_VISIBLE) &&
           !(pObjAtt->flags & OBJECT_SKIPDRAWING) &&
		   !(pObjAtt->flag2 & OBJECT_HIDDEN))
		{
			if(pObjAtt->bbox[0].x + pObjAtt->pos.x < *pfLeft)
			{
				*pfLeft = pObjAtt->bbox[0].x + pObjAtt->pos.x;
			}

			if(pObjAtt->bbox[0].y + pObjAtt->pos.y < *pfTop)
			{
				*pfTop = pObjAtt->bbox[0].y + pObjAtt->pos.y;
			}

			if(pObjAtt->bbox[0].z + pObjAtt->pos.z < *pfMinZ)
			{
				*pfMinZ = pObjAtt->bbox[0].z + pObjAtt->pos.z;
			}

			if(pObjAtt->bbox[1].x + pObjAtt->pos.x > *pfRight)
			{
				*pfRight = pObjAtt->bbox[1].x + pObjAtt->pos.x;
			}

			if(pObjAtt->bbox[1].y + pObjAtt->pos.y > *pfBottom)
			{
				*pfBottom = pObjAtt->bbox[1].y + pObjAtt->pos.y;
			}

			if(pObjAtt->bbox[1].z + pObjAtt->pos.z > *pfMaxZ)
			{
				*pfMaxZ = pObjAtt->bbox[1].z + pObjAtt->pos.z;
			}
		}

		pObjAtt = pObjAtt->attached;
	}

	if(pObjSib->child != NULL)
	{
		if(!obmainGetBoundingBoxMaxSiblings(pfLeft,
			   								pfTop,
											pfRight,
											pfBottom,
											pfMaxZ,
											pfMinZ,
											pObjSib->child))
		{
			return FALSE;
		}
	}

	return TRUE;
}

// Get the overall bounding box
LOGICAL obmainGetBoundingBoxMaxSiblings(FLOAT32* pfLeft,
									    FLOAT32* pfTop,
									    FLOAT32* pfRight,
									    FLOAT32* pfBottom,
										FLOAT32* pfMaxZ,
										FLOAT32* pfMinZ,
									    OBJECT* pObject)
{
	OBJECT* pObjSib = NULL;
	OBJECT* pObjAtt = NULL;

	if(pfLeft == NULL || pfTop == NULL ||
	   pfRight == NULL || pfBottom == NULL ||
	   pfMaxZ == NULL)
	{
		return FALSE;
	}

	if(pObject == NULL)
	{
		return FALSE;
	}

	pObjSib = pObject;
	pObjAtt = pObject;

	while(pObjSib != NULL)
	{
		while(pObjAtt != NULL)
		{
		    if(!(pObjAtt->flags & OBJECT_NOTOBJECT) &&
                (pObjAtt->flags & OBJECT_VISIBLE) &&
               !(pObjAtt->flags & OBJECT_SKIPDRAWING) &&
			   !(pObjAtt->flag2 & OBJECT_HIDDEN))
            {
				if(pObjAtt->bbox[0].x + pObjAtt->pos.x < *pfLeft)
				{
					*pfLeft = pObjAtt->bbox[0].x + pObjAtt->pos.x;
				}

				if(pObjAtt->bbox[0].y + pObjAtt->pos.y < *pfTop)
				{
					*pfTop = pObjAtt->bbox[0].y + pObjAtt->pos.y;
				}

				if(pObjAtt->bbox[0].z + pObjAtt->pos.z < *pfMinZ)
				{
					*pfMinZ = pObjAtt->bbox[0].z + pObjAtt->pos.z;
				}

				if(pObjAtt->bbox[1].x + pObjAtt->pos.x > *pfRight)
				{
					*pfRight = pObjAtt->bbox[1].x + pObjAtt->pos.x;
				}

				if(pObjAtt->bbox[1].y + pObjAtt->pos.y > *pfBottom)
				{
					*pfBottom = pObjAtt->bbox[1].y + pObjAtt->pos.y;
				}

				if(pObjAtt->bbox[1].z + pObjAtt->pos.z > *pfMaxZ)
				{
					*pfMaxZ = pObjAtt->bbox[1].z + pObjAtt->pos.z;
				}
			}

			pObjAtt = pObjAtt->attached;
		}

		if(pObjSib->child != NULL)
		{
			if(!obmainGetBoundingBoxMaxSiblings(pfLeft,
												pfTop,
												pfRight,
												pfBottom,
												pfMaxZ,
												pfMinZ,
												pObjSib->child))
			{
				return FALSE;
			}
		}

		pObjSib = pObjSib->sibling;
	}

	return TRUE;
}

UINT32 obmainRemoveSiblings(OBJECT* pObject)
{
	OBJECT* pObjSib;
	OBJECT* pObjAtt;

	if(pObject == NULL)
	{
		return 0;
	}

	pObjSib = pObject;

	do
	{
		pObjAtt = pObjSib;

		do
		{
			listDeleteOBJECT(pObjAtt);

            if(pObjAtt)
            {
                pObjAtt->next = NULL;
                pObjAtt->prev = NULL;
            }

			pObjAtt = pObjAtt->attached;
		}
		while(pObjAtt != NULL);

		if(pObjSib->child != NULL)
		{
			obmainRemoveSiblings(pObjSib->child);
		}

		pObjSib = pObjSib->sibling;
	}
	while(pObjSib != NULL);

	return 0;
}

//////////////////////////////////////////////////////////////////////
//	obmainDeleteNode
//
//Removes the node from the main list and deletes it
//Emptied groups will be removed
//////////////////////////////////////////////////////////////////////
void obmainDeleteNode(OBJECT* node)
{
	if(!node || node->child) return;

	OBJECT*		crtnode			= node->next;
	OBJECT*		sibling			= node->sibling;
	OBJECT*		attached		= node->attached;
	// Whoever implemented the list, loved to make it confusing
	OBJECT*		previous		= node->next;
	OBJECT*		next			= node->prev;
	LOGICAL		empty_parent	= FALSE;


	if(previous->attached == node)
	{
		if(next == attached)	previous->attached = next;
		else	previous->attached = NULL;
	}
	else if(attached)
	{
		attached->flags &= ~OBJECT_ATTACHED;
		attached->sibling = sibling;				// Attached object takes over the sibling pointer
	}

	if(previous->child == node)
	{
		if(attached) previous->child = attached;
		else previous->child = sibling;

		if(!previous->child) empty_parent = TRUE;
	}

	// Handle the possible sibling ptr pointing to the object that is removed
	while(crtnode)
	{
		if(crtnode->sibling == node)
		{
			if(attached) crtnode->sibling = attached;
			else crtnode->sibling = sibling;
			break;
		}
		crtnode = crtnode->next;
	}

	listDeleteOBJECT(node);
	obmainFreeObjectMemory(node);

	if(empty_parent) obmainDeleteNode(previous);
}


//////////////////////////////////////////////////////////////////////
//	obmainDeleteNodeTree
//
//Removes the object tree from the main list and deletes it
//////////////////////////////////////////////////////////////////////
void obmainDeleteNodeTree(OBJECT* node)
{
	if(!node) return;

	OBJECT*		attached = node->attached;
	OBJECT*		child = node->child;
	OBJECT*		sibling = node->sibling;

#if !NEW_OBJECT_MEM
	ObjectSet::iterator it = objectmap.find(node);
	if(it != objectmap.end())
		objectmap.erase(it);
    else
        return;
#else
    if(!node->valid)
        return;
#endif

    listDeleteOBJECT(node);
	obmainFreeObjectMemory(node);

	if(child) obmainDeleteNodeTree(child);
	if(attached) obmainDeleteNodeTree(attached);
	if(sibling) obmainDeleteNodeTree(sibling);
}


UINT32 obmainRemoveObjectTree(OBJECT* pObject)
{
	if(pObject == NULL)
	{
		return 0;
	}

	OBJECT* pObjAtt = pObject;

	do
	{
		listDeleteOBJECT(pObjAtt);

/*		ogs: listDeleteItem already does this
		if(pObjAtt)
        {
            pObjAtt->next = NULL;
            pObjAtt->prev = NULL;
        }
*/

		pObjAtt = pObjAtt->attached;
	}
	while(pObjAtt != NULL);

	if(pObject->child != NULL)
	{
		obmainRemoveSiblings(pObject->child);
	}

	return 0;
}

static LOGICAL _SetObjectSubTreeMatrix(OBJECT * Obj, piMATRIX * Mat)
{
	if (Obj == NULL)
	{
		return (TRUE);
	}

	if (Mat == NULL)
	{
		return (TRUE);
	}

	OBJECT * ObjAtt;
	OBJECT * ObjSib;

	ObjSib = Obj;

	do
	{
		ObjAtt = ObjSib;

		do
		{

			if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
			{
				ObjAtt->mat = *Mat;
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != NULL);

		if (ObjSib->child != NULL)
		{
			_SetObjectSubTreeMatrix(ObjSib->child,Mat);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return (TRUE);
}


// sets the object tree to this matrix
LOGICAL obmainSetObjectTreeMatrix(OBJECT * Obj,piMATRIX * Mat)
{
OBJECT * ObjAtt;

	if (Obj == NULL)
	{
		return (TRUE);
	}

	if (Mat == NULL)
	{
		return (TRUE);
	}

	ObjAtt = Obj;

	do
	{
		if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt->mat = *Mat;
		}

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != NULL);

	if (Obj->child != NULL)
	{
		_SetObjectSubTreeMatrix(Obj->child,Mat);
	}

	return (TRUE);
}

OBJECT* _newObject(UINT32 obtype)
{
	OBJECT* lpobj  = NULL;

	obtype &= OBJECT_NOTOBJECT;

	switch(obtype)
	{
#if NEW_OBJECT_MEM
		case OBJECT_DOF:
		case OBJECT_LOD:
		case OBJECT_GROUP:
		case 0:
            {
                lpobj = PopFree();

                break;
            }
#else
		case OBJECT_DOF:
			lpobj = (OBJECT*)memAlloc(sizeof(DOFOBJECT));
		break;

		case OBJECT_LOD:
			lpobj = (OBJECT*)memAlloc(sizeof(LODOBJECT));
		break;

		case OBJECT_GROUP:
			lpobj = (OBJECT*)memAlloc(sizeof(GROUPOBJECT));
		break;

		case 0:
			lpobj = (OBJECT*)memAlloc(sizeof(OBJECT));
		break;
#endif
		default:
			piExitProgram(__FILE__, __LINE__, "Unexpected object flags");
		break;
	}

	if(lpobj)
	{
		lpobj->matid = -1;
		lpobj->texid = -1;
		lpobj->texname = "";
		lpobj->matname = "";
        lpobj->mat = IdentityMatrix;
        lpobj->comment = NULL;
		lpobj->collider = NULL;

        NumObjUsed++;

#ifdef ANDREWTEST
    objectset.insert( lpobj );
#endif

#if !NEW_OBJECT_MEM
        objectmap.insert(lpobj);
#endif
    }

	return lpobj;
}

const piUNIFIED_VERTEX* OBJECT::RenderData()
{
	if(pverts)
		return pverts;
    else if(puverts)
        return puverts;
    else
        return NULL;
}

/************************************************************************************************
	Render
*************************************************************************************************/

piVectorConstPtr OBJECT::RenderPositions() const
{
	if(pverts)
		return piVectorConstPtr((piVECTOR*)&pverts->x, sizeof(piUNIFIED_VERTEX));
	else if(puverts)
		return piVectorConstPtr((piVECTOR*)&puverts->x, sizeof(piUNIFIED_VERTEX));
	else
		return piVectorConstPtr(0,0);
}

piVectorConstPtr OBJECT::RenderNormals() const
{
	if(pverts)
		return piVectorConstPtr((piVECTOR*)&pverts->nx, sizeof(piUNIFIED_VERTEX));
	else if(puverts)
		return piVectorConstPtr((piVECTOR*)&puverts->nx, sizeof(piUNIFIED_VERTEX));
	else
		return piVectorConstPtr(0,0);
}

piVertexColorConstPtr OBJECT::RenderColors() const
{
	if(pverts && (flags & OBJECT_DYNALITE))
		return piVertexColorConstPtr((piVERTEX_COLOR*)&pverts->color, sizeof(piUNIFIED_VERTEX));
	else if(puverts)
		return piVertexColorConstPtr((piVERTEX_COLOR*)&puverts->color, sizeof(piUNIFIED_VERTEX));
	else
		return piVertexColorConstPtr(0,0);
}

piTexCoordConstPtr OBJECT::RenderTexCoords() const
{
	if(pverts)
		return piTexCoordConstPtr((piUV*)&pverts->tu, sizeof(piUNIFIED_VERTEX));
	else if(puverts)
		return piTexCoordConstPtr((piUV*)&puverts->tu, sizeof(piUNIFIED_VERTEX));
	else
		return piTexCoordConstPtr(0,0);
}

/************************************************************************************************
	Immutable
*************************************************************************************************/

piVectorConstPtr OBJECT::ImmutablePositions() const
{
	if(puverts)
		return piVectorConstPtr((piVECTOR*)&puverts->x, sizeof(piUNIFIED_VERTEX));
	else
		return piVectorConstPtr(0,0);
}

piVectorConstPtr OBJECT::ImmutableNormals() const
{
	if(puverts)
		return piVectorConstPtr((piVECTOR*)&puverts->nx, sizeof(piUNIFIED_VERTEX));
	else
		return piVectorConstPtr(0,0);
}

piVertexColorConstPtr OBJECT::ImmutableColors() const
{
	if(puverts)
		return piVertexColorConstPtr((piVERTEX_COLOR*)&puverts->color, sizeof(piUNIFIED_VERTEX));
	else
		return piVertexColorConstPtr(0,0);
}

piTexCoordConstPtr OBJECT::ImmutableTexCoords() const
{
	if(puverts)
		return piTexCoordConstPtr((piUV*)&puverts->tu, sizeof(piUNIFIED_VERTEX));
	else
		return piTexCoordConstPtr(0,0);
}

/************************************************************************************************
	Mutable
*************************************************************************************************/

#ifdef ANDREWTEST

size_t MutablePositions_size = 0;   size_t MutablePositions_count = 0;
size_t MutableNormals_size = 0;     size_t MutableNormals_count = 0;
size_t MutableColors_size = 0;      size_t MutableColors_count = 0;
size_t MutableTexCoords_size = 0;   size_t MutableTexCoords_count = 0;
size_t MutableCollisionNormals_size = 0;   size_t MutableCollisionNormals_count = 0;
size_t MutableSurfaceNormals_size = 0;   size_t MutableSurfaceNormals_count = 0;

#endif

piVectorPtr OBJECT::MutablePositions()
{
	if(!pverts && numverts)
	{
        if(meshContext)
        {
            // Can't VBO mutable data
            delete meshContext;
            meshContext = NULL;
        }

		pverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
		memcpy(pverts, puverts, numverts * sizeof(piUNIFIED_VERTEX));

#ifdef ANDREWTEST
		MutablePositions_size += numverts * sizeof(piUNIFIED_VERTEX);
		MutablePositions_count ++;
#endif
	}

	return piVectorPtr((piVECTOR*)&pverts->x, sizeof(piUNIFIED_VERTEX));
}

piVectorPtr OBJECT::MutableNormals()
{
    if(!pverts && numverts)
	{
        if(meshContext)
        {
            // Can't VBO mutable data
            delete meshContext;
            meshContext = NULL;
        }

        pverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
		memcpy(pverts, puverts, numverts * sizeof(piUNIFIED_VERTEX));
#ifdef ANDREWTEST
		MutableNormals_size += numverts * sizeof(piUNIFIED_VERTEX);
		MutableNormals_count ++;
#endif
	}

	return piVectorPtr((piVECTOR*)&pverts->nx, sizeof(piUNIFIED_VERTEX));
}

piVertexColorPtr OBJECT::MutableColors()
{
    if(!pverts && numverts)
	{
        if(meshContext)
        {
            // Can't VBO mutable data
            delete meshContext;
            meshContext = NULL;
        }

		pverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
		memcpy(pverts, puverts, numverts * sizeof(piUNIFIED_VERTEX));
#ifdef ANDREWTEST
		MutableColors_size += numverts * sizeof(piUNIFIED_VERTEX);
		MutableColors_count ++;
#endif
	}

	return piVertexColorPtr((piVERTEX_COLOR*)&pverts->color, sizeof(piUNIFIED_VERTEX));
}

piTexCoordPtr OBJECT::MutableTexCoords()
{
    if(!pverts && numverts)
	{
        if(meshContext)
        {
            // Can't VBO mutable data
            delete meshContext;
            meshContext = NULL;
        }

        pverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
		memcpy(pverts, puverts, numverts * sizeof(piUNIFIED_VERTEX));
#ifdef ANDREWTEST
		MutableTexCoords_size += numverts * sizeof(piUNIFIED_VERTEX);
		MutableTexCoords_count ++;
#endif
	}

	return piTexCoordPtr((piUV*)&pverts->tu, sizeof(piUNIFIED_VERTEX));
}

/************************************************************************************************
	Static
*************************************************************************************************/

piVectorPtr OBJECT::StaticPositions()
{
	return piVectorPtr((piVECTOR*)&puverts->x, sizeof(piUNIFIED_VERTEX));
}

piVectorPtr OBJECT::StaticNormals()
{
	return piVectorPtr((piVECTOR*)&puverts->nx, sizeof(piUNIFIED_VERTEX));
}

piVertexColorPtr OBJECT::StaticColors()
{
	return piVertexColorPtr((piVERTEX_COLOR*)&puverts->color, sizeof(piUNIFIED_VERTEX));
}

piTexCoordPtr OBJECT::StaticTexCoords()
{
	return piTexCoordPtr((piUV*)&puverts->tu, sizeof(piUNIFIED_VERTEX));
}

/************************************************************************************************
	StridedRenderData
*************************************************************************************************/

void OBJECT::StridedRenderData(piSTRIDEDDATA* strideData) const
{
	piStridedVertexConstPtr sv;
	StridedRenderVertices(&sv);

	strideData->position.pdata  = (void*)sv.pos.ptr;
	strideData->position.stride = sv.pos.stride;

	strideData->normal.pdata  = (void*)sv.norm.ptr;
	strideData->normal.stride = sv.norm.stride;

	strideData->diffuse.pdata  = (void*)&sv.color->color;
	strideData->diffuse.stride = sv.color.stride;
	strideData->specular.pdata = (void*)&sv.color->specular;
	strideData->specular.stride= sv.color.stride;

	strideData->textureCoords[0].pdata  = (void*)sv.tex.ptr;
	strideData->textureCoords[0].stride = sv.tex.stride;

	strideData->indices.pdata = ptris;
	strideData->indices.stride = sizeof(UINT16);
}

/************************************************************************************************
	StridedRenderVertices
*************************************************************************************************/

void OBJECT::StridedRenderVertices(piStridedVertexConstPtr* sv) const
{
	sv->pos 	= RenderPositions();
	sv->norm 	= RenderNormals();
	sv->color 	= RenderColors();
	sv->tex 	= RenderTexCoords();
}

/************************************************************************************************
	SyncMutableData
*************************************************************************************************/

void OBJECT::SyncMutableData()
{
	if(!puverts || !pverts) return;

	memcpy(pverts, puverts, numverts * sizeof(piUNIFIED_VERTEX));
}

/************************************************************************************************
	CreateStaticData
*************************************************************************************************/

void OBJECT::CreateStaticData()
{
	if(flag2 & OBJECT_POOL) return;

	if(!puverts)
		puverts = (piUNIFIED_VERTEX*)memAlloc(numverts * sizeof(piUNIFIED_VERTEX));
}

/************************************************************************************************
	SyncStaticData
*************************************************************************************************/

void OBJECT::SyncStaticData()
{
// nop
}

/************************************************************************************************
	obmainCreateMutableSurfaceNormals
*************************************************************************************************/

void obmainCreateMutableSurfaceNormals(OBJECT* lpobj)
{
    if(!lpobj->freepsnorms && lpobj->psnorms == lpobj->psnorig)
	{
#ifdef ANDREWTEST
		MutableSurfaceNormals_size += lpobj->numtris * 3 * sizeof(piVECTOR);
		MutableSurfaceNormals_count ++;
#endif

		lpobj->psnorms = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
		if(!lpobj->psnorig)	lpobj->psnorig = lpobj->psnorms;
		else	memcpy(lpobj->psnorms, lpobj->psnorig, lpobj->numtris * sizeof(piVECTOR));

        lpobj->freepsnorms = true;
	}
}

/************************************************************************************************
	obmainCreateMutableCollisionNormals
*************************************************************************************************/

void obmainCreateMutableCollisionNormals(OBJECT* lpobj)
{
    if(!lpobj->freepscdn && lpobj->pscdn == lpobj->pscdon)
	{
		lpobj->pscdn = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
		if( !lpobj->pscdon )
        {
            lpobj->pscdon = lpobj->pscdn;
        }
		else
        {
            if (lpobj->pscdn!=NULL )
            {
                memcpy(lpobj->pscdn, lpobj->pscdon, lpobj->numtris * 3 * sizeof(piVECTOR));
            }
            else
            {
                printf("bad pscdn\n");
                assert(lpobj->pscdn!=NULL);
            }
        }

        lpobj->freepscdn = true;
	}
}

/************************************************************************************************
	obmainValidateObject
*************************************************************************************************/

LOGICAL obmainValidateObject(OBJECT* lpobj)
{
    if(!lpobj)
        return FALSE;

#if NEW_OBJECT_MEM
    return lpobj->valid;
#else
    // disable to see if we've fixed root cause
    LOGICAL	isValid = objectmap.find(lpobj) != objectmap.end();

#if(0)
    if(!isValid)
	{
		piExitProgram(__FILE__, __LINE__, "Invalid object found!");
	}
#endif

    return (isValid);
#endif // NEW OBJECT_MEM
}

LOGICAL obmainGetScreenBBOX(OBJECT* lpobj, float& x, float& y, float &z, float& w, float& h)
{
	if(!lpobj) return 0;

	FLOAT32 points[24]; // 8 points * 3 coords (x,y,z)

	FLOAT32 x1, x2, y1, y2;
	x1 = x2 = y1 = y2 = 0;

	FLOAT64 sx, sy, sz;
	sx = sy = sz = 0;

	// get our bbox min and max
	piVECTOR bl = lpobj->bbox[1];
	piVECTOR tr = lpobj->bbox[0];

	// temp vectors
	piVECTOR final;

	// get them on the same vertical plane as us
	bl.z *= -1;
	tr.z *= -1;

	// x				y					z
	points[0] = bl.x;	points[1] = bl.y;	points[2] = bl.z;	// point 1 bottom back left
	points[3] = tr.x;	points[4] = bl.y;	points[5] = bl.z;	// point 2 bottom back right
	points[6] = tr.x;	points[7] = tr.y;	points[8] = bl.z;	// point 3 bottom front right
	points[9] = bl.x;	points[10] = tr.y;	points[11] = bl.z;	// point 4 bottom front left
	points[12] = bl.x;	points[13] = bl.y;	points[14] = tr.z;	// point 5 top back left
	points[15] = tr.x;	points[16] = bl.y;	points[17] = tr.z;	// point 6 top back right
	points[18] = tr.x;	points[19] = tr.y;	points[20] = tr.z;	// point 7 top front right
	points[21] = bl.x;	points[22] = tr.y;	points[23] = tr.z;	// point 8 top front left

	// reset the world!
	piSetWorldMatrix(&IdentityMatrix);

	for(int i = 0; i < 24; i += 3)
	{
		final.x = points[i];
		final.y = points[i+1];
		final.z = points[i+2];

		// rotate the point
		math3dVectorMultiply(&final, &final, &lpobj->mat);

		if(piGetWorldToScreen(final.x, final.y, final.z, sx, sy, sz))
		{
			FLOAT32 fx, fy;

			fx = (FLOAT32)sx;
			fy = (FLOAT32)sy;

			if(!i)
			{
				x1 = x2 = fx;
				y1 = y2 = fy;
			}
			else
			{
				if(fx < x1) x1 = fx;
				else if(fx > x2) x2 = fx;
				if(fy < y1) y1 = fy;
				else if(fy > y2) y2 = fy;
			}
		}
	}

	w = x2 - x1;
	h = y2 - y1;
	x = x1;
	y = y1;
	z = (FLOAT32)sz;

	return 1;
}

void obmainAddForUpdateCallback(OBJECT* o)
{
    // So we don't have to loop through entire object list for an object update
    updatecallbacks.insert(o);
}

LOGICAL listDeleteOBJECT(OBJECT* obj)
{
    if ( obj->flag2 & OBJECT_TOPMOST )
    {
    }
    return listDeleteItem((LISTHEADER*)obj);
}

static LOGICAL listAddTopMostOBJECTAfter(OBJECT* objectlist, OBJECT* after)
{
    assert( after->flag2 & OBJECT_TOPMOST );

    return listAddItemAfter((LISTHEADER*)objectlist, (LISTHEADER*)after);
}


