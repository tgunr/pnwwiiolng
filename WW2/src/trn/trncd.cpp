#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ob/obmain.h"
#include "ob/obcd.h"
#include "ob/obdraw.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "trn/trncd.h"
#include "trn/trnautogun.h"
#include "trn/trnop.h"
#include "draw/draw3d.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "rn/rnmain.h"
#include "pers/persmain.h"
#include "locl/loclMain.h"
#include "ml/mlgauges.h"
#include "ogrp/ogrptype.h"
#include "ml/mlutility.h"
#include "ob/obpool.h"
#include "parse/parseWor.h"
#include "math/mathUnitConversion.h"
#include "sdm/StratDB.h"
#include "ld/ldpix.h"
#include "fx/fxSnd.h"
#include "trn/trnfx.h"
#include "spt/sptmain.h"
#include "ob/obMgr.h"
#include <assert.h>
#include "fio/fio.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "weap/weapList.h"
#include "miss/missionUtils.h"
#include "fx/TransformEffect.h"
#include "fx/fxWeather.h"
#include "strt/strtTypes.h"
#include "wwii/wwiiHash.h"
#include "scen/ScenarioManager.h"
#include "physics/colliderMain.h"

#define TRNCD_USE_ODE
#undef TRNCD_USE_ODE

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

typedef std::map<OBJECT*, trnCDTerrainObject *> mapOBJList;
mapOBJList mapOBJ; // All objects associated with a TO

//#define xANDREWCOLLISIONTESTS

//#ifndef ANDREWCOLLISIONTESTS

static LOGICAL g_bCheckTreeBox;

typedef struct tagTERRAINID
{
    SINT32 x;
    SINT32 y;
    UINT32 stratID;
} TERRAINID;

typedef std::map<TERRAINID, trnCDTerrainObject *> mapTOList;

static mapTOList mapTO; // Non spawn objects
static mapTOList mapSpawnTO; // Spawn objects only

typedef std::set<trnCDTerrainObject*> ODEQueueSet;
static ODEQueueSet odeQueue;

static bool _updateCollideLists = false;


static std::unordered_set<std::string> _buildingsWithSpawnData;

// Mapping of dof names to the weapon specs allowed to spawn there.
typedef std::map<std::string, std::vector<UINT32> > dofToWeapSpecs;
static std::map<std::string, dofToWeapSpecs> _buildingToDofToClass;


static bool operator < (const TERRAINID& l, const TERRAINID& r)
{
    if(l.x != r.x)
    {
        return l.x < r.x;
    }
    else if(l.y != r.y)
    {
        return l.y < r.y;
    }

    return l.stratID < r.stratID;
}


#define _MAX_COUNTRIES 32

#define CD_POLY_FENCE
//#undef CD_POLY_FENCE

StratDB::StratObject* trnCDTerrainObject::StratObj()
{
	FULL_OBJ_ID fid = nullObjID;
	populateFullObjID(&fid, StratSupercellX, StratSupercellY, ObjId);
	return StratDB::FindStratObject(fid);
}

const char* trnCDTerrainObject::ObjName()
{
	if(TerrainMasterObject && TerrainMasterObject->modelInstance)
		return TerrainMasterObject->modelInstance->GetModelName();

	return "";
}

bool trnCDTerrainObject::IsCollidable(UINT32 cflags)
{
	if(!cflags || !physicsColliderSpace)
		return true;

	return (dGeomGetCategoryBits((dGeomID)physicsColliderSpace) & cflags);
}

// terrain object collision detection stuff


typedef struct tagStratMessageQue
{
	struct tagStratMessageQue* prev;
	struct tagStratMessageQue* next;

	SINT32 scx;
	SINT32 scy;
	UINT32 Id;
	SINT32 Country;
	SINT32 VisLevel;

} _StratMessageQue;

// This was decreased from 24000 because
// we load in a lot fewer terrain objects now.
// Saves approx 9 MB of static memory.
#define _MAX_NUM_CDOBJECTS 6000

#define _MAX_STRAT_MESSAGE_QUE 16384

static std::deque<trnCDTerrainObject*> freeToObjects;
static UINT32 _NumTO;

static trnCDTerrainObject _TerrainObjectsHead;

static SINT32 _CountryTexIds[_MAX_COUNTRIES];
static struct OBJECT* _FlagObject;

static _StratMessageQue _StratMessageQueArray[_MAX_STRAT_MESSAGE_QUE];
static _StratMessageQue _StratMsgHead;
static _StratMessageQue _StratMsgFree;
static _StratMessageQue * _StratMsgTail;

#ifdef TRNCD_USE_ODE
static bool _odeCollide(const piVECTOR& pt1, const piVECTOR& pt2, piVECTOR& collidept, float& time, int& tri);
static bool _odeCollide(const piVECTOR& pt1, const piVECTOR& pt2, FASTCOLLIDE** list, unsigned int& NumSlotsLeft);
#endif

static UINT32 _AddToListHead(trnCDTerrainObject* TO);
static UINT32 _RemoveFromList(trnCDTerrainObject* TO);
static UINT32 _AddToFreeList(trnCDTerrainObject* TerrainObject);
#if(0)
static UINT32 _FreeTerrainObject(trnCDTerrainObject** TerrainObject);
#endif

static UINT32 _GetNewCDTerrainObject(trnCDTerrainObject** TerrainObject);
static UINT32 _KillCDTerrainObject(trnCDTerrainObject* TO);
static void _AddTerrainObject(OBJECT* Object, OBJECT* TopObject, SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id, REL_OBJ_ID * ObjId,
								SINT32 DamThreshold);
static UINT32 _RemoveTerrainObject(OBJECT* Object);
static UINT32 _FindLODs(OBJECT*** CurrentLODSlot, SINT32** CurrentLODDamageSlot, UINT32* LODSlotsLeft, OBJECT* Object);
//static void _cdRemoveTerrainObject(OBJECT* Object);
static void _RecurseAndSetBoundingBox(OBJECT* Object, piVECTOR& BBox1, piVECTOR& BBox2, bool& FirstBBox);
//static UINT32 _cdAddTerrainObject(OBJECT* Object, OBJECT* TopObject, SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id, REL_OBJ_ID * ObjId,
//								  SINT32 DamThreshold);
static UINT32 _CollideWithTerrainObject(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft, OBJECT* CurrentObject, OBJECT* TopObject,
										trnCDTerrainObject* TO, piVECTOR const* OldPnt, piVECTOR const* NewPnt, UINT32 NoSiblings,
										UINT32 DontIgnoreBackface, UINT32 Flags = 0);
static UINT32 _CheckObjectTreeForLineCollision(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
												OBJECT* Object, OBJECT* TopObject, OBJECT* LineCollideObject, piMATRIX* InvMat, UINT32 Flags = 0);
static void _AdjustObjectMatrixPosition(OBJECT* Object, const piVECTOR* const Adjustment);
static void _FindDamageCollisionObject(OBJECT** const CollisionObject, OBJECT* Object);
static UINT32 _SetDamageLevel(OBJECT* Object,UINT32 DamageLevel);
static UINT32 _SetTOObjectCountry(OBJECT * Object, SINT32 Country, UINT32 IncludeSibs);
static UINT32 _AddStratMsgToQue(SINT32 scx,SINT32 scy,UINT32 Id,SINT32 Country,SINT32 VisLevel);

static _StratMessageQue * _GetNewStratMsg(void);
static UINT32 _FreeStratMsg(_StratMessageQue * StratMsg);
static UINT32 _RemoveStratMsgFromList(_StratMessageQue * StratMsg);
static UINT32 _AddStratMsgToFreeList(_StratMessageQue * StratMsg);
static UINT32 _AddStratMsgToActiveList(_StratMessageQue * StratMsg);

static SINT32 _GetDamageLevel(SINT32 visLevel);

static SINT32 _NullTexId;

SINT32 trncdReplacementTileTextureID() { return _NullTexId; }

static LOGICAL g_bIgnoreTreeCollide = FALSE;

#ifdef TERRAIN_EDITOR
SINT32 _CollisionTriCnt;
#endif

#ifndef _DEBUG

#define OutlineTriangle(a, b, c)
#define HighliteView(a, b)

#else

static void OutlineTriangle(piVERTEX* pv, piINDICES* pt, DWORD color)
{
	draw3dLine((piVECTOR*)&pv[pt->v1], (piVECTOR*)&pv[pt->v2], color);
	draw3dLine((piVECTOR*)&pv[pt->v2], (piVECTOR*)&pv[pt->v3], color);
	draw3dLine((piVECTOR*)&pv[pt->v3], (piVECTOR*)&pv[pt->v1], color);
}

static void HighliteView(piVECTOR* pv, DWORD color)
{
	draw3dPoint(pv, color);
}

#endif

static _StratMessageQue * _GetNewStratMsg(void)
{
_StratMessageQue * StratMsg;

	if (_StratMsgFree.next == nullptr)
	{
		return nullptr;
	}

	StratMsg = _StratMsgFree.next;

	_RemoveStratMsgFromList(StratMsg);
	_AddStratMsgToActiveList(StratMsg);

	return StratMsg;
}

// puts clutter onto free list
static UINT32 _FreeStratMsg(_StratMessageQue * StratMsg)
{
	_RemoveStratMsgFromList(StratMsg);
	memset(StratMsg,0,sizeof(_StratMessageQue));
	_AddStratMsgToFreeList(StratMsg);

	return 0;
}

static UINT32 _RemoveStratMsgFromList(_StratMessageQue * StratMsg)
{
	if (StratMsg == nullptr)
	{
		return 0;
	}

	if (_StratMsgTail == StratMsg)
	{
		_StratMsgTail = StratMsg->prev;
	}

	if (StratMsg->prev != nullptr)
	{
		StratMsg->prev->next = StratMsg->next;
	}
	if (StratMsg->next != nullptr)
	{
		StratMsg->next->prev = StratMsg->prev;
	}
	StratMsg->next = StratMsg->prev = nullptr;

	return 0;
}

static UINT32 _AddStratMsgToFreeList(_StratMessageQue * StratMsg)
{
	if (StratMsg == nullptr)
	{
		return 0;
	}

	StratMsg->next = _StratMsgFree.next;
	StratMsg->prev = &_StratMsgFree;
	if (_StratMsgFree.next != nullptr)
	{
		_StratMsgFree.next->prev = StratMsg;
	}
	_StratMsgFree.next = StratMsg;

	return 0;
}

static UINT32 _AddStratMsgToActiveList(_StratMessageQue * StratMsg)
{
	if (StratMsg == nullptr)
	{
		return 0;
	}

	StratMsg->next = _StratMsgHead.next;
	StratMsg->prev = &_StratMsgHead;
	if (_StratMsgHead.next != nullptr)
	{
		_StratMsgHead.next->prev = StratMsg;
	}
	_StratMsgHead.next = StratMsg;

	if (_StratMsgTail == &_StratMsgHead ||
		_StratMsgTail == nullptr)
	{
		_StratMsgTail = StratMsg;
	}

	return 0;
}

/*--------------------------------------------------------------------------
Function: _expandClass
--------------------------------------------------------------------------*/
static void _expandClass(const char* classname, std::vector<UINT32>& weaponspecs)
{
    // Get the spec for this class
    WEAPON_SPEC spec = weapFindClass(classname);

    // Get all the weapons that are under this class
    WEAPON_LIST* list = weapFindSpawnList(spec);

    if(!list)
    {
        return;
    }

    WEAPON_LIST::iterator itr = list->begin();

    for(; itr != list->end(); ++itr)
    {
        // Make sure it's a valid weapon
        if(itr->first.wClass > 0)
        {
            weaponspecs.push_back(itr->first.asUINT32());
        }
    }
}

/*--------------------------------------------------------------------------
Function: _classhandler
--------------------------------------------------------------------------*/
static void _classhandler(xmlNode* root, std::vector<UINT32>& weaponspecs)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"class", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                _expandClass((char*)s, weaponspecs);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"weapon", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                WEAPON_SPEC spec = weapFindWeaponByName((const char*)s);

                if(spec.wType > 0)
                {
                    weaponspecs.push_back(spec.asUINT32());
                }

                xmlFree(s);
            }

            continue;
        }
    }
}

/*--------------------------------------------------------------------------
Function: _buildinghandler
--------------------------------------------------------------------------*/
static void _buildinghandler(xmlNode* root, dofToWeapSpecs& dws)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"dof", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                std::vector<UINT32> weaponspecs;

                _classhandler(node->children, weaponspecs);

                // These are the vehicle weapon specs which can spawn at this dof
                dws[(const char*)s] = weaponspecs;

                xmlFree(s);
            }
        }
    }
}

/*--------------------------------------------------------------------------
Function: _spawnhandler
--------------------------------------------------------------------------*/
static void _spawnhandler(xmlNode* root)
{
    xmlNode* node = root;

    for(; node != nullptr; node = node->next)
    {
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"building", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                _buildingsWithSpawnData.insert((const char*)s);

                dofToWeapSpecs dws;

                _buildinghandler(node->children, dws);
                _buildingToDofToClass[(const char*)s] = dws;

                xmlFree(s);
            }
        }
    }
}

/*--------------------------------------------------------------------------
Function: _loadSpawnXML
--------------------------------------------------------------------------*/
static void _loadSpawnXML(const char* file)
{
#if(0)
    return;
#endif

    fioXmlRegisterInputCallbacks();

    xmlDocPtr doc = xmlReadFile(file, nullptr, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            Write_Log(LOG_OLD, LOG_INFO, "*** %s FAILED ***", file);
            return;
        }

        if(!xmlStrcmp((xmlChar*)"SPAWNER", root->name))
        {
            _spawnhandler(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

static void _SetupFlagObject(OBJECT& Object)
{
	// UINT32 flagnum forces overflow on values less than '0'.
	const UINT32 FlagNum = Object.name[4] - '0';
	if (FlagNum < _MAX_COUNTRIES && Object.texid > 0)
	{
		// This is ugly; hack for now
		if(gRegion == 5) // chinese
		{
			// Load new textures
			std::string path = oglGetTextureData(Object.texid)->name;
			path += ".dds";

			const char* regionalPath = fioGetRegionalPath(path.c_str());

			int id = ldpixLoadImageFileToSurface(regionalPath, 0, LDPIX_MIPMAP);

			if(id > 0 && id != Object.texid)
			{
				// If id is valid then free
				piFreeMipTextures(Object.texid);

				// update the obj tex id
				Object.texid = id;
			}
		}

		_CountryTexIds[FlagNum] = Object.texid;
	}
}

static void _ScanAndSetupFlags(OBJECT* Object)
{
	do
	{
		if (!(Object->flags & OBJECT_NOTOBJECT))
		{
			if (strncmp(Object->name,"flag",4) == 0)
			{
				_SetupFlagObject(*Object);
			}
		}

		if (Object->child != nullptr)
		{
			_ScanAndSetupFlags(Object->child);
		}

		Object = Object->sibling;
	}
	while (Object != nullptr);
}

UINT32 trncdInit(void)
{
	trnmainSetObjectCreationCallback(trncdAddTerrainObject);
	trnmainSetObjectDeletionCallback(trncdRemoveTerrainObject);

	if(!obmgrCreateModelInstance("flagtable1"))
	{
		Write_Debug("Unable to load terrain object flag file");

		piExitProgram(__FILE__, __LINE__,"Unable to load terrain object flag file");
	}

	_FlagObject = obmainFindNamedObject("flagtable1");

	if (_FlagObject == nullptr)
	{
		Write_Debug("Terrain flag object copied, but unable to find it by name");

		piExitProgram(__FILE__, __LINE__,"Unable to find terrain flag file by name");
	}

	_ScanAndSetupFlags(_FlagObject);
	obmainSetFlagsInThisTree(_FlagObject,OBJECT_SKIPDRAWING);

	for (int i=0;i<_MAX_STRAT_MESSAGE_QUE;i++)
	{
		_FreeStratMsg(&_StratMessageQueArray[i]);
	}
	_StratMsgTail = &_StratMsgHead;

	_NullTexId = ldpixLoadImageFileToSurface("terrain\\null1.bmp", colorBLACK, LDPIX_NONE);

	if (!piIsTexture(_NullTexId, 0))
	{
	SINT8 s[256];
		sprintf(s,"Unable to load up the null terrain texture map! (terrain\\null1.bmp)");
		Write_Debug(s);
		piExitProgram(__FILE__, __LINE__,s);
	}

    _loadSpawnXML("terrain/xml/spawn.xml");

	return 0;
}

UINT32 trncdKill(void)
{
    if(!mapTO.empty())
    {
        Write_Log(LOG_OLD, LOG_INFO, "[trncdKill] mapTO size %d", mapTO.size());
    }

    if(!mapOBJ.empty())
    {
        Write_Log(LOG_OLD, LOG_INFO, "[trncdKill] mapOBJ size %d", mapOBJ.size());
    }

    piFreeMipTextures(_NullTexId);

    if(_FlagObject)
    {
        obpoolFreeObjectCallback(_FlagObject);
    }

    trnCDTerrainObject*	to = _TerrainObjectsHead.Next;
    trnCDTerrainObject* free = nullptr;

    while(to)
	{
        free = to;
        to = to->Next;

        _KillCDTerrainObject(free);
	}

    _buildingsWithSpawnData.clear();
    _buildingToDofToClass.clear();

    while(!freeToObjects.empty())
    {
        trnCDTerrainObject* to = freeToObjects.front();
        freeToObjects.pop_front();
        memFree(to);
    }

	return 0;
}


static UINT32 _KillCDTerrainObject(trnCDTerrainObject* TO)
{
	if (TO == nullptr)
	{
		return 0;
	}

	if (TO->TrnAutoGun != nullptr)
	{
		trnKillAutogun(&(TO->TrnAutoGun));
		TO->TrnAutoGun = nullptr;
	}

	// make sure that this TO is no longer on any of the collision lists used by the colliders
	mlCheckRemovedObjectOnCollisionList(TO);

	// remove our ODE collider if we have one
	trncdRemoveODEColliderImmediate(*TO, true);

	if(TO->physicsBoxCollider)
	{
		dGeomDestroy((dGeomID)TO->physicsBoxCollider);
		TO->physicsBoxCollider = 0;
	}

#ifdef SPATIAL_DB
    if ( TO->TerrainMasterObject->sdbInfo )
    {
        SDB_RemoveObject( TO->TerrainMasterObject );
    }
#endif
    _RemoveFromList(TO);

	_AddToFreeList(TO);

    mapOBJList::iterator I1 = mapOBJ.find(TO->TerrainObject);

    if(I1 != mapOBJ.end())
    {
        mapOBJ.erase(I1);
    }

	TO->TerrainObject = nullptr;
	TO->InUse = 0;

    TERRAINID ID = {TO->StratSupercellX, TO->StratSupercellY, TO->StratId};

    mapTOList::iterator I2 = mapTO.find(ID);

    if(I2 != mapTO.end())
    {
        mapTO.erase(I2);
    }

    I2 = mapSpawnTO.find(ID);

    if(I2 != mapSpawnTO.end())
    {
        mapSpawnTO.erase(I2);
    }

    // Objects must be remapped, since the are deleted.
    if(TO->spawndata)
    {
        delete TO->spawndata;
        TO->spawndata = nullptr;
    }

	_NumTO--;

	return 0;
}

trnCDTerrainObject* trncdFindTerrainObject(OBJECT* o)
{
    if(!o)
    {
        return nullptr;
    }

    mapOBJList::iterator find = mapOBJ.find(o);

    if(find != mapOBJ.end())
    {
        return find->second;
    }

    return nullptr;
}

static UINT32 _GetNewCDTerrainObject(trnCDTerrainObject** TerrainObject)
{
	if (TerrainObject == nullptr)
	{
		return 1;
	}

    if(freeToObjects.empty())
    {
        *TerrainObject = (trnCDTerrainObject*)memAlloc(sizeof(trnCDTerrainObject));
    }
    else
    {
        *TerrainObject = freeToObjects.front();
        freeToObjects.pop_front();
    }

	memset(*TerrainObject, 0, sizeof(trnCDTerrainObject));

	if(_AddToListHead(*TerrainObject) == 1)
	{
		return 1;
	}

    _NumTO++;

	return 0;
}

#if(0)
static UINT32 _FreeTerrainObject(trnCDTerrainObject** TerrainObject)
{
	return 0;
}
#endif

static UINT32 _AddToFreeList(trnCDTerrainObject* TerrainObject)
{
    if(!TerrainObject)
        return 1;

    freeToObjects.push_back(TerrainObject);

	return 0;
}

static UINT32 _RemoveFromList(trnCDTerrainObject* TO)
{
	if (TO == nullptr)
	{
		return 1;
	}

	if (TO->Next != nullptr)
	{
		TO->Next->Prev = TO->Prev;
	}

	if (TO->Prev != nullptr)
	{
		TO->Prev->Next = TO->Next;
	}

	TO->Prev = TO->Next = nullptr;

	return 0;
}

static UINT32 _AddToListHead(trnCDTerrainObject* TO)
{
	if (TO == nullptr)
	{
		return 1;
	}

	TO->Prev = &_TerrainObjectsHead;
	TO->Next = _TerrainObjectsHead.Next;
	if (_TerrainObjectsHead.Next != nullptr)
	{
		_TerrainObjectsHead.Next->Prev = TO;
	}

	_TerrainObjectsHead.Next = TO;

	return 0;
}

LOGICAL trncdRemoveTerrainObject(OBJECT* Object, SINT32 DummyNum1, SINT32 DummyNum2, UINT32 Id,
								 REL_OBJ_ID * ObjId)
{
#ifdef TERRAIN_EDITOR
	Write_Debug("Removing overall object %s",Object->name);
#endif

	if (Object != nullptr)
	{
		//_cdRemoveTerrainObject(Object->child);

        OBJECT* x = Object->child;
        while(x != nullptr)
        {
            if(x->child != nullptr)
            {
                _RemoveTerrainObject(x);
            }

            x = x->sibling;
        }
	}

	return 0;
}

void trncdRemoveODEColliderImmediate(trnCDTerrainObject& TO, bool release)
{
	odeQueue.erase(&TO);

	if(release && TO.physicsColliderSpace)
	{
		if(TO.CollisionObject)
		{
			physicsRecurseAndFreeObjectCollider(TO.CollisionObject);

			if(TO.CollisionObject != TO.DamageCollisionObject)
				physicsRecurseAndFreeObjectCollider(TO.DamageCollisionObject);
		}

		dSpaceDestroy((dSpaceID)TO.physicsColliderSpace);
		TO.physicsColliderSpace = 0;
	}
}

void trncdRemoveODEColliderQueued(trnCDTerrainObject& TO)
{
	if(TO.physicsColliderSpace)
	{
		odeQueue.insert(&TO);
	}
}

void trncdProcessODEQueue(bool all)
{
	// limit how often ode colliders freed
	if(!odeQueue.empty())
	{
		if(all)
		{
			// clear the queue (like when despawning)
			ODEQueueSet::iterator it = odeQueue.begin();

			for(; it != odeQueue.end(); it++)
			{
				trnCDTerrainObject* TO = *it;

				if(TO->physicsColliderSpace)
				{
					if(TO->CollisionObject)
					{
						physicsRecurseAndFreeObjectCollider(TO->CollisionObject);

						if(TO->CollisionObject != TO->DamageCollisionObject)
							physicsRecurseAndFreeObjectCollider(TO->DamageCollisionObject);
					}

					dSpaceDestroy((dSpaceID)TO->physicsColliderSpace);
					TO->physicsColliderSpace = 0;
				}
			}

			odeQueue.clear();
		}
		else
		{
			//uiChatPrintLine("Processing %d TO objects for ODE colliders!", odeTOList.size());
			ODEQueueSet::iterator it = odeQueue.begin();
			trncdRemoveODEColliderImmediate(*(*it), true);
		}
	}
}

/*
static void _cdRemoveTerrainObject(OBJECT* o)
{
	while(o != nullptr)
	{
        if(o->child != nullptr)
        {
            _RemoveTerrainObject(o);
        }

        o = o->sibling;
    }
}
*/
//
//	adds a newly-created object to the collision detection terrain object list
//
LOGICAL trncdAddTerrainObject(OBJECT* Object,SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id,
							  REL_OBJ_ID * ObjId)
{
    SINT8 Buffer[64];
    SINT8 * ptr;
    BUILDING_PARAMS* Params;
    SINT32 DamThreshold;
#ifdef TERRAIN_EDITOR
    Write_Debug("Adding overall object %s",Object->name);
#endif

	if (Object != nullptr)
	{
		DamThreshold = 0;

		strcpy(Buffer,Object->name);
		ptr = strrchr(Buffer,'_');
		if (ptr != nullptr)
		{
			*ptr = '\0';
			ptr = strrchr(Buffer,'_');
			if (ptr != nullptr)
			{
				*ptr = '\0';

				Params = parseGetBuildingValues(Buffer);

				if (Params != nullptr)
				{
					DamThreshold = Params->damageThreshold;
				}

			}
		}

        if(Object->child != nullptr) // check the children
        {
            OBJECT* Object1 = Object->child;
            do
            {
                if (Object1->child != nullptr)
                {
                    _AddTerrainObject( Object1, Object, SuperCellX, SuperCellY, Id, ObjId, DamThreshold );
                }
                Object1 = Object1->sibling;

            } while (Object1 != nullptr);
        }

		//_cdAddTerrainObject(Object->child,Object,SuperCellX,SuperCellY,Id,ObjId,DamThreshold);
	}
	return 0;
}

/*
static UINT32 _cdAddTerrainObject(OBJECT* Object, OBJECT* TopObject, SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id,REL_OBJ_ID * ObjId,
								  SINT32 DamThreshold)
{
OBJECT*	Object1;

	if (Object == nullptr)
	{
		return 0;
	}

	Object1 = Object;
	do
	{
//		if (Object1->comment != nullptr)
//		{
//			if (strncmp(Object1->comment,"lod1",4) == 0)
//			{
//				_AddTerrainObject(Object1,TopObject);
//			}
//		}
		if (Object1->child != nullptr)
		{
			_AddTerrainObject(Object1,TopObject,SuperCellX,SuperCellY,Id,ObjId,DamThreshold);

		}

//		if (Object1->child != nullptr)
//		{
//			_cdAddTerrainObject(Object1->child,TopObject);
//		}

		Object1 = Object1->sibling;
	} while (Object1 != nullptr);

	return 0;
}
*/

static UINT32 _RemoveTerrainObject(OBJECT* o)
{
    if(!o) return 0;

    mapOBJList::iterator I = mapOBJ.find(o);

    if(I == mapOBJ.end())
    {
#ifdef TERRAIN_EDITOR
		Write_Debug("Object being removed that isn't on the collide list!");
#endif
        return 0;
    }

    trnCDTerrainObject* to;

#ifdef TERRAIN_EDITOR
	Write_Debug("_RemoveTerrainObject: %s", o->name);
#endif

    to = I->second;

    _KillCDTerrainObject(to);

    return 0;
}

// Is this building in spawn.xml?
bool trncdSpawnDataExists(const char* buildingname)
{
    if(!buildingname)
        return false;

    // Find the spawn data associated with this building
    auto itr = _buildingsWithSpawnData.find(buildingname);
    return itr != _buildingsWithSpawnData.cend();
 }

static void _findSpawnData(trnCDTerrainObject* to)
{
    if(!to->TerrainMasterObject)
    {
        return;
    }

    if(_buildingsWithSpawnData.empty())
    {
        return;
    }

    std::string str = to->TerrainMasterObject->name;

    UINT32 pos = str.find_last_of('_');

    if(pos != std::string::npos)
    {
        str.erase(pos);
    }

    pos = str.find_last_of('_');

    if(pos != std::string::npos)
    {
        str.erase(pos);
    }

    // Find the spawn data associated with this building
    auto itr = _buildingsWithSpawnData.find(str);
    if(itr == _buildingsWithSpawnData.cend())
    {
        to->spawndata = nullptr;
    }
    else
    {
        to->spawndata = new SpawnData;
        to->spawndata->buildingname = str;
    }
}

struct ShearObject
{
    OBJECT* o;
    bool sheared;

    void Apply(OBJECT* o)
    {
        if(o)
        {
            Apply(o->child);
            Apply(o->sibling);
            Apply(o->attached);

            if(o->mat._13 != 0.0f || o->mat._23 != 0.0f)
            {
                if(!(o->flags & OBJECT_NOTOBJECT))
                {
                    piVectorPtr pos = o->MutablePositions();

                    for(int i = 0; i < o->numverts; i++)
                    {
                        // Shear z
                        pos[i].z = pos[i].x * o->mat._13 + pos[i].y * o->mat._23 + pos[i].z;
                    }

                    sheared = true;
                    // Don't need this anymore (can't do this for any other placemodes)
                    o->mat._13 = o->mat._23 = 0.0f;
                }
            }
        }
    }

    bool Shear()
    {
        sheared = false;
        Apply(o);

        return sheared;
    }
};

static void _AddTerrainObject(OBJECT* Object, OBJECT* TopObject, SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id,REL_OBJ_ID * ObjId,
								SINT32 DamThreshold)
{
trnCDTerrainObject*	TO = nullptr;
piVECTOR			BBox1, BBox2;
piVECTOR			TempVec;
UINT32				LODSlotsLeft;
OBJECT**				CurrentLODSlot;
OBJECT*				Obj;
SINT32*				CurrentLODDamageSlot;
UINT32				i;
SINT8*				ptr;
_StratMessageQue*	StratMsg;
_StratMessageQue*	PrevStratMsg;

	if (Object == nullptr)
	{
		return;
	}

#ifdef TERRAIN_EDITOR
	Write_Debug("Adding Object %s, top level %s",Object->name,TopObject->name);
#endif

    mapOBJList::iterator I = mapOBJ.find(Object);

    if(I != mapOBJ.end())
    {
        Write_Debug("Attempt to add duplicate object to the terrain object list!");
        return;
    }

	// first grab an object slot
	if (_GetNewCDTerrainObject(&TO) == 1 || TO == nullptr)
	{
		Write_Debug("Unable to get new terrain object");
		return;
	}

	TO->TerrainObject = Object;
	TO->TerrainMasterObject = TopObject;
	if (Object->child != nullptr)
	{
		Object->child->lpspare = (void*)TO;
	}
	TO->InUse = 1;
	TO->StratSupercellX = SuperCellX;
	TO->StratSupercellY = SuperCellY;
	TO->StratId = Id;
	TO->DamThreshold = DamThreshold;
	if (ObjId != nullptr)
	{
		TO->ObjId = *ObjId;
	}

	// find the collision object
	TO->CollisionObject = nullptr;
	trnFindCollisionObject(&TO->CollisionObject,Object->child);
	_FindDamageCollisionObject(&TO->DamageCollisionObject,Object->child);

	if (TO->DamageCollisionObject == nullptr)
	{
		if(!TO->CollisionObject)
		{
#ifdef TERRAIN_EDITOR
			Write_Debug("Cannot find damage collision object for object %s, using entire object tree instead",
							TO->TerrainObject->name);
#endif
		TO->DamageCollisionObject = Object;
		}
		else
		{
			TO->DamageCollisionObject = TO->CollisionObject;
		}
	}


	if (TO->CollisionObject == nullptr)
	{
#ifdef TERRAIN_EDITOR
		Write_Debug("Cannot find collision object for object %s, using entire object tree instead",
						TO->TerrainObject->name);
#endif
		TO->CollisionObject = Object;
	}

    // F-me!!!
    if(TO->CollisionObject->modelInstance)
    {
        if(TO->CollisionObject->modelInstance->IsShearPlacement())
        {
            static struct ShearObject shear;

            shear.o = TO->CollisionObject->child;

            if(shear.Shear())
            {
                trnopRecalcBoundsAndNormals(TO->CollisionObject->child);
            }
        }
    }

    // F-me!!!
    if(TO->DamageCollisionObject &&
       TO->DamageCollisionObject != TO->CollisionObject &&
       TO->DamageCollisionObject->modelInstance &&
       TO->DamageCollisionObject->modelInstance->IsShearPlacement())
    {
        static struct ShearObject shear;

        shear.o = TO->DamageCollisionObject->child;

        if(shear.Shear())
        {
            trnopRecalcBoundsAndNormals(TO->DamageCollisionObject->child);
        }
    }

	// next set the bounding box
	BBox1.x = BBox1.y = BBox1.z = 0;
	BBox2.x = BBox2.y = BBox2.z = 0;

	if (TO->CollisionObject->child != nullptr)
	{
		bool FirstBBox = true;
		_RecurseAndSetBoundingBox(TO->CollisionObject->child,BBox1,BBox2,FirstBBox);
	}

	TO->BBox1 = BBox1;
	TO->BBox2 = BBox2;
	TO->Center.x = (BBox2.x + BBox1.x) / 2;
	TO->Center.y = (BBox2.y + BBox1.y) / 2;
	TO->Center.z = (BBox2.z + BBox1.z) / 2;
	TO->OrigCenter = TO->Center;
	mathSubVectors(&TempVec,&BBox2,&TO->Center);
	TO->Radius = (FLOAT32)mathExactVectorLength(&TempVec);


	if (TO->ObjId.objClass == CLASS_BUILDING &&
		TO->ObjId.objType == BUILDING_AUTOGUNNER)
	{
		trnAddAutoGun(TO);
	}

	// next scan for any/all LODs
	// note: lod's are not cumulative, if an LOD is found, no children
	// are checked
	// also, the toplevel object cannot be an LOD
	LODSlotsLeft = TRN_MAX_LOD;
	CurrentLODSlot = TO->LOD;
	CurrentLODDamageSlot = TO->LODDamageLevel;

	if (TO->TerrainObject->flags & OBJECT_LOD)
	{
		*CurrentLODSlot = TO->TerrainObject;
		if (obmainFindCommentName(&ptr, TO->TerrainObject->comment, "damagelevel"))
		{
			*CurrentLODDamageSlot = atoi(ptr);
		}
		else
		{
			*CurrentLODDamageSlot = 0;
		}
	}
	else
	{
		_FindLODs(&CurrentLODSlot,&CurrentLODDamageSlot,&LODSlotsLeft,TO->TerrainObject->child);
	}

	for (i=0;i<TRN_MAX_LOD;i++)
	{
		if (TO->LOD[i] != nullptr)
		{
			Obj = TO->LOD[i];
			while (Obj->child != nullptr)
			{
				Obj = Obj->child;
			}
			if (Obj->flags & OBJECT_NOTOBJECT)
			{
				Write_Debug("Terrain object % has LOD with no direct child object",
							TO->TerrainMasterObject->name);
			}
			else
			{
                if(Obj->flag2 & OBJECT_AIRRAIDSIREN)
                {
                    TO->LOD[i]->flag2 |= OBJECT_AIRRAIDSIREN;
                }
                else
                {
                  if (TO->ObjId.objClass == CLASS_FACILITY && TO->DamageLevel != FB_CLOSED_VISLEVEL &&
                      (TO->ObjId.objType == FACILITY_ARMYBASE || 
                       TO->ObjId.objType == FACILITY_NAVALBASE || 
                       TO->ObjId.objType == FACILITY_AIRBASE))
                  {
                    TO->LOD[i]->flag2 |= OBJECT_AIRRAIDSIREN;
                  }
                }

				TO->LODSavedMin[i] = Obj->minlod;
				TO->LODSavedMax[i] = Obj->maxlod;
			}
		}
	}

    TransformEffect::Apply(TO);

	// now ensure that damage level zero is displayed until told otherwise
	trnSetDamageLevel(TO,0);

	if(!(TO->TerrainObject->flag3 & OBJECT_HANDLES_COUNTRY))
	{
		_SetTOObjectCountry(TO->TerrainObject, WWII_CHINA, 0); // set to china so people know something is wrong
		TO->Country = WWII_CHINA;
	}

	TO->LevelInited = FALSE;

//	trnSetDamageLevel(TO,1);

    if (Object->flag3 & OBJECT_SPAWN)
	{
        _findSpawnData(TO);

        ogrpParseDOFParameters(Object,(UCHAR*)(&mlDOFVarNameTable[0][0]),ML_DOF_MAXNUM,32);
		obmainClearFlagsInThisTree(Object, OBJECT_VISIBLE);
		// we're a spawn object
		TO->SpawnEntries = 0;

		for (i=0;i<TRN_MAX_SPAWN_LOCATIONS;i++)
		{
			TO->SpawnLocations[i] = nullptr;
			ogrpFindSubObjectByDOFVarAndSubVar(&TO->SpawnLocations[i],
												Object,
												ML_DOF_SPAWNLOCATION,
												i);
			if (TO->SpawnLocations[i] != nullptr)
			{
				TO->SpawnEntries += 1;

                if(TO->spawndata) // Note to self: Clean this up.
                {
                    SpawnData* sd = TO->spawndata;

                    // Get the dof name
                    std::string dofname = TO->SpawnLocations[i]->comment;

                    // Erase past '/n'
                    dofname.erase(dofname.find(13));

                    // Mapping of dof name to all vehicles that can spawn there.
                    std::map<std::string, dofToWeapSpecs>::iterator itr;

                    // Find the container for this building that holds the dofs and the
                    // weapons allowed to spawn there.
                    itr = _buildingToDofToClass.find(sd->buildingname);

                    if(itr == _buildingToDofToClass.end())
                    {
                        continue;
                    }

                    // Find the container that holds all weapons allowed to spawn at this dof
                    std::map<std::string, std::vector<UINT32> >::iterator dofitr;
                    dofitr = itr->second.find(dofname);

                    // Map all the vehicles that can spawn to this object
                    if(dofitr != itr->second.end())
                    {
                        // Iterate through the classes allowed to spawn at this dof
                        for(unsigned int q = 0; q < dofitr->second.size(); q++)
                        {
                            // Weap spec to OBJECT
                            sd->addSpawnObject(dofitr->second[q], TO->SpawnLocations[i]);
                        }
                    }
			    }
            }
		}

        TERRAINID id = {TO->StratSupercellX, TO->StratSupercellY, TO->StratId};

        // These TOs are for spawn objects only.
        mapSpawnTO.insert(std::make_pair(id, TO));
	}
    else
    {
        TERRAINID id = {TO->StratSupercellX, TO->StratSupercellY, TO->StratId};

        // For strat updates
        mapTO.insert(std::make_pair(id, TO));
    }

	// check que for updates to this object
	for (StratMsg = _StratMsgTail;StratMsg != nullptr && StratMsg != &_StratMsgHead;)
	{
		if (StratMsg->scx == SuperCellX &&
			StratMsg->scy == SuperCellY &&
			StratMsg->Id == Id)
		{
			PrevStratMsg = StratMsg->prev;

			trnUpdateObjectStatus(StratMsg->scx,
									StratMsg->scy,
									StratMsg->Id,
									StratMsg->Country,
									StratMsg->VisLevel);

			_FreeStratMsg(StratMsg);

			StratMsg = PrevStratMsg;
		}
		else
		{
			StratMsg = StratMsg->prev;
		}
	}

    // For object deletion
    mapOBJ.insert(std::make_pair(Object, TO));
}

UINT32 trnGetSpawnObject(OBJECT** o,
                         SINT32 SuperCellX,
                         SINT32 SuperCellY,
                         UINT32 Id,
                         UINT32 SpawnNum,
                         WEAPON_SPEC& spec)
{
    trnCDTerrainObject* to = nullptr;

    if(!Id)
    {
        *o = nullptr;
        return 0;
    }

    TERRAINID id = {SuperCellX, SuperCellY, Id};

    mapTOList::iterator I = mapSpawnTO.find(id);

    if(I == mapSpawnTO.end())
    {
        SCHAR s[128];

        sprintf(s, "[trnGetSpawnObject] x=%d y=%d id=%d** FAILED**", SuperCellX, SuperCellY, Id);
        Write_Log(LOG_OLD, LOG_FATAL, s);
        piExitProgram(__FILE__,__LINE__, s);
        return 1;
    }

    to = I->second;

    if(to->SpawnEntries == 0)
    {
        SCHAR s[128];

        sprintf(s, "[trnGetSpawnObject] spawn entries = 0 x=%d y=%d id=%d** FAILED**", SuperCellX, SuperCellY, Id);
        Write_Log(LOG_OLD, LOG_FATAL, s);
        piExitProgram(__FILE__,__LINE__, s);
        return 1;
    }

    SpawnData* sd = to->spawndata;

    bool useold = true;

    if(sd)
    {
        OBJECT* obj = sd->findSpawnObject(spec);

        if(obj != nullptr)
        {
            *o = obj;
            useold = false;
        }
    }

    // Default to old method.
    if(useold)
    {
        if(to->SpawnEntries > 1)
        {
            PNRandom::seedrandom(frametime);
            int idx = PNRandom::random(0, to->SpawnEntries - 1);
            *o = to->SpawnLocations[idx];
        }
        else
            *o = to->SpawnLocations[0];
    }

	return 0;
}

static UINT32 _FindLODs(OBJECT*** CurrentLODSlot, SINT32** CurrentLODDamageSlot, UINT32* LODSlotsLeft, OBJECT* Object)
{
OBJECT*	ObjSib;
SINT8*	ptr;

	if (Object == nullptr)
	{
		return 0;
	}

	if (*LODSlotsLeft <= 0)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (ObjSib->flags & OBJECT_LOD)
		{
            **CurrentLODSlot = ObjSib;
			**CurrentLODDamageSlot = 0;
			if (obmainFindCommentName(&ptr, ObjSib->comment, "damagelevel"))
			{
				**CurrentLODDamageSlot = atoi(ptr);
			}
			else
			{
				**CurrentLODDamageSlot = 0;
			}

			(*CurrentLODSlot)++;
			(*CurrentLODDamageSlot)++;
			(*LODSlotsLeft)--;
		}
		else
		{
			if (ObjSib->child != nullptr)
			{
				_FindLODs(CurrentLODSlot,CurrentLODDamageSlot,LODSlotsLeft,ObjSib->child);
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && *LODSlotsLeft > 0);

	return 0;
}


void trnFindCollisionObject(OBJECT ** const CollisionObject, OBJECT* Object)
{
OBJECT*	ObjSib;
SCHAR*	RetPtr;

	if (Object == nullptr)
	{
		return;
	}

	ObjSib = Object;

	do
	{
		if (ObjSib->comment != nullptr)
		{
			if (obmainFindCommentName(&RetPtr,ObjSib->comment,"collideobject") == TRUE)
			{
				*CollisionObject = ObjSib;
				break;
			}
		}

		if (ObjSib->child != nullptr)
		{
			trnFindCollisionObject(CollisionObject,ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && *CollisionObject == nullptr);

	return;
}

static void _FindDamageCollisionObject(OBJECT** const CollisionObject, OBJECT* Object)
{
OBJECT*	ObjSib;
SCHAR*	RetPtr;

	if (Object == nullptr)
	{
		return;
	}

	ObjSib = Object;

	do
	{
		if (ObjSib->comment != nullptr)
		{
			if (obmainFindCommentName(&RetPtr,ObjSib->comment,"damagecollideobject") == TRUE)
			{
				*CollisionObject = ObjSib;
				break;
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindDamageCollisionObject(CollisionObject,ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && *CollisionObject == nullptr);
}

static void _AdjustObjectMatrixPosition(OBJECT* Object, const piVECTOR* const Adjustment)
{
	while(Object != nullptr)
	{
		if (Object->child != nullptr)
		{
			_AdjustObjectMatrixPosition(Object->child, Adjustment);
		}

		if (!(Object->flags & OBJECT_NOTOBJECT))
		{
			OBJECT* ObjAtt = Object;
			do
			{
				ObjAtt->mat._41 += Adjustment->x;
				ObjAtt->mat._42 += Adjustment->y;
				ObjAtt->mat._43 += Adjustment->z;
				math3dInvertMatrixComplete(ObjAtt->matint, ObjAtt->mat);
				ObjAtt = ObjAtt->attached;
			} while(ObjAtt != nullptr);
		}

		Object = Object->sibling;
	}
}

static void _UpdateObjectInvMatrix(OBJECT* Object)
{
	while(Object != nullptr)
	{
		if (Object->child != nullptr)
			_UpdateObjectInvMatrix(Object->child);

		if (!(Object->flags & OBJECT_NOTOBJECT))
		{
			OBJECT* ObjAtt = Object;

			do
			{
				math3dInvertMatrixComplete(ObjAtt->matint, ObjAtt->mat);
				ObjAtt = ObjAtt->attached;
			}
			while(ObjAtt != nullptr);
		}

		Object = Object->sibling;
	}
}

static void _growBoundingBoxMin(piVECTOR& mins, const piVECTOR& bbox)
{
	if(mins.x > bbox.x)
		mins.x = bbox.x;
	if(mins.y > bbox.y)
		mins.y = bbox.y;
	if(mins.z > bbox.z)
		mins.z = bbox.z;
}

static void _growBoundingBoxMax(piVECTOR& maxs, const piVECTOR& bbox)
{
	if(maxs.x < bbox.x)
		maxs.x = bbox.x;
	if(maxs.y < bbox.y)
		maxs.y = bbox.y;
	if(maxs.z < bbox.z)
		maxs.z = bbox.z;
}

static void _RecurseAndSetBoundingBox(OBJECT* Object, piVECTOR& BBox1, piVECTOR& BBox2, bool& FirstBBox)
{
OBJECT*		Object1, *Object2;
piVECTOR	bbox[2];
piVECTOR	BBox[8];
UINT32		i;

	if (Object == nullptr)
	{
		return;
	}

	Object1 = Object;
	do
	{
		if (!(Object1->flags & OBJECT_NOTOBJECT))
		{
			Object2 = Object1;

			do
			{
				mathAddVectors(&bbox[0],&Object2->bbox[0],&Object2->pos);
				mathAddVectors(&bbox[1],&Object2->bbox[1],&Object2->pos);

				// ok, this bounding box is in object space and we need it in world space
				// generate the 8 points of the bounding box
				BBox[0].x = bbox[0].x;
				BBox[0].y = bbox[0].y;
				BBox[0].z = bbox[0].z;
				BBox[1].x = bbox[0].x;
				BBox[1].y = bbox[1].y;
				BBox[1].z = bbox[0].z;
				BBox[2].x = bbox[1].x;
				BBox[2].y = bbox[1].y;
				BBox[2].z = bbox[0].z;
				BBox[3].x = bbox[1].x;
				BBox[3].y = bbox[0].y;
				BBox[3].z = bbox[0].z;
				BBox[4].x = bbox[0].x;
				BBox[4].y = bbox[0].y;
				BBox[4].z = bbox[1].z;
				BBox[5].x = bbox[0].x;
				BBox[5].y = bbox[1].y;
				BBox[5].z = bbox[1].z;
				BBox[6].x = bbox[1].x;
				BBox[6].y = bbox[1].y;
				BBox[6].z = bbox[1].z;
				BBox[7].x = bbox[1].x;
				BBox[7].y = bbox[0].y;
				BBox[7].z = bbox[1].z;

				// now rotate these 8 points
				for (i=0;i<8;i++)
				{
					math3dVectorMultiply(&BBox[i],&BBox[i],&Object2->mat);
				}

				bbox[0] = BBox[0];
				bbox[1] = BBox[0];

				for (i=1;i<8;i++)
				{
					_growBoundingBoxMin(bbox[0], BBox[i]);
					_growBoundingBoxMax(bbox[1], BBox[i]);
				}
				// check mins
				if (FirstBBox)
				{
					FirstBBox = false;
					BBox1 = bbox[0];
					BBox2 = bbox[1];
				}
				else
				{
					_growBoundingBoxMin(BBox1, bbox[0]);
					// check maxs
					_growBoundingBoxMax(BBox2, bbox[1]);
				}

				Object2 = Object2->attached;
			} while (Object2 != nullptr);

		}


		if (Object1->child != nullptr)
		{
			_RecurseAndSetBoundingBox(Object1->child,BBox1,BBox2,FirstBBox);
		}

		Object1 = Object1->sibling;

	} while (Object1 != nullptr);
}

LOGICAL trncdGetListOfTerrainPolysNearBoundingBox(piVECTOR* BBox, OBJECT** CollideObj, UINT32* CollidePoly,
	  											  UINT32* NumCollisionSlotsLeft, UINT32 Flags)
{
TERRAIN_DETAIL*	td;
OBJECT*			lpobj;
SINT32				l, m;
UINT32				i, j;
piVECTOR			BBox1, BBox2;
piVECTOR			triBBox1, triBBox2;
UINT32				tripos;
UINT32				numtris;
UINT32				numoctets;
piINDICES*			pt;
piVECTOR const*		pv1, *pv2, *pv3;

	BBox1 = BBox[0];
	BBox2 = BBox[1];

	td = trndtl;


	for (l=0; l < NUM_TERRAIN_RECTANGLES && *NumCollisionSlotsLeft > 0; l++, td++)
	{
		lpobj = td->terrain;
		if (lpobj == nullptr)
		{
			continue;
		}

		for (m=0; m < MAX_TERRAIN_TEXTURES && *NumCollisionSlotsLeft > 0; lpobj=td->nonreplacementtiles[m++])
		{
			if (lpobj == nullptr)
			{
				continue;
			}

			if (!(lpobj->flags & OBJECT_VISIBLE))
			{
				continue;
			}

			if (!(lpobj->flags & OBJECT_COLLISION))
			{
				continue;
			}

			if(lpobj->collider && !lpobj->collider->isCollidable(Flags))
			{
				continue;
			}

			// note: this assumes* NO* rotation component of the matrix
			if (lpobj->flags & OBJECT_USEMATRIX)
			{
				BBox1.x = BBox[0].x - lpobj->mat._41;
				BBox1.y = BBox[0].y - lpobj->mat._42;
				BBox1.z = BBox[0].z - lpobj->mat._43;

				BBox2.x = BBox[1].x - lpobj->mat._41;
				BBox2.y = BBox[1].y - lpobj->mat._42;
				BBox2.z = BBox[1].z - lpobj->mat._43;
			}

			// do an initial bounding box bailout for the terrain object
			// assumes the incoming bounding box is smaller than the terrain object bbox
			if (BBox1.x < lpobj->bbox[0].x + lpobj->pos.x &&
				BBox2.x < lpobj->bbox[0].x + lpobj->pos.x)
			{
				continue;
			}
			if (BBox1.x > lpobj->bbox[1].x + lpobj->pos.x &&
				BBox2.x > lpobj->bbox[1].x + lpobj->pos.x)
			{
				continue;
			}
			if (BBox1.y < lpobj->bbox[0].y + lpobj->pos.y &&
				BBox2.y < lpobj->bbox[0].y + lpobj->pos.y)
			{
				continue;
			}
			if (BBox1.y > lpobj->bbox[1].y + lpobj->pos.y &&
				BBox2.y > lpobj->bbox[1].y + lpobj->pos.y)
			{
				continue;
			}
			if (BBox1.z < lpobj->bbox[0].z + lpobj->pos.z &&
				BBox2.z < lpobj->bbox[0].z + lpobj->pos.z)
			{
				continue;
			}
			if (BBox1.z > lpobj->bbox[1].z + lpobj->pos.z &&
				BBox2.z > lpobj->bbox[1].z + lpobj->pos.z)
			{
				continue;
			}

			numtris = lpobj->numtris;
			piVectorConstPtr objPos = lpobj->RenderPositions();
			pt      = lpobj->ptris;
			tripos = 0;

			numoctets = numtris / NUM_TRIANGLES_PER_OCTET;

			for (i=0; i < numoctets && *NumCollisionSlotsLeft > 0; i++)
			{
				pv1 = &objPos[i*NUM_VERTICES_PER_OCTET+1];
				pv2 = &objPos[i*NUM_VERTICES_PER_OCTET+5];

				if (BBox1.x < pv1->x &&
					BBox2.x < pv1->x)
				{
					pt += NUM_TRIANGLES_PER_OCTET;
					tripos += NUM_TRIANGLES_PER_OCTET;
					continue;
				}

				if (BBox1.x > pv2->x &&
					BBox2.x > pv2->x)
				{
					pt += NUM_TRIANGLES_PER_OCTET;
					tripos += NUM_TRIANGLES_PER_OCTET;
					continue;
				}

				if (BBox1.y < pv1->y &&
					BBox2.y < pv1->y)
				{
					pt += NUM_TRIANGLES_PER_OCTET;
					tripos += NUM_TRIANGLES_PER_OCTET;
					continue;
				}

				if (BBox1.y > pv2->y &&
					BBox2.y > pv2->y)
				{
					pt += NUM_TRIANGLES_PER_OCTET;
					tripos += NUM_TRIANGLES_PER_OCTET;
					continue;
				}


				for (j=0; j < NUM_TRIANGLES_PER_OCTET && *NumCollisionSlotsLeft > 0; j++, pt++, /*pn++, pcn+=3,*/ tripos++)
				{
					if (pt->v1 == pt->v2 && pt->v2 == pt->v3)
					{
						continue;
					}

					pv1 = (piVECTOR*)&objPos[pt->v1];
					pv2 = (piVECTOR*)&objPos[pt->v2];
					pv3 = (piVECTOR*)&objPos[pt->v3];

					// generate triangle bounding box
					if (pv2->x < pv1->x)
					{
						triBBox1.x = pv2->x;
						triBBox2.x = pv1->x;
					}
					else
					{
						triBBox1.x = pv1->x;
						triBBox2.x = pv2->x;
					}
					if (pv3->x < triBBox1.x)
					{
						triBBox1.x = pv3->x;
					}
					if (pv3->x > triBBox2.x)
					{
						triBBox2.x = pv3->x;
					}

					if (pv2->y < pv1->y)
					{
						triBBox1.y = pv2->y;
						triBBox2.y = pv1->y;
					}
					else
					{
						triBBox1.y = pv1->y;
						triBBox2.y = pv2->y;
					}
					if (pv3->y < triBBox1.y)
					{
						triBBox1.y = pv3->y;
					}
					if (pv3->y > triBBox2.y)
					{
						triBBox2.y = pv3->y;
					}

					if (pv2->z < pv1->z)
					{
						triBBox1.z = pv2->z;
						triBBox2.z = pv1->z;
					}
					else
					{
						triBBox1.z = pv1->z;
						triBBox2.z = pv2->z;
					}
					if (pv3->z < triBBox1.z)
					{
						triBBox1.z = pv3->z;
					}
					if (pv3->z > triBBox2.z)
					{
						triBBox2.z = pv3->z;
					}

					if ((BBox1.x > triBBox1.x &&
						 BBox1.x < triBBox2.x) ||
						(BBox2.x > triBBox1.x &&
						 BBox2.x < triBBox2.x) ||
						(triBBox1.x > BBox1.x &&
						 triBBox1.x < BBox2.x) ||
						(triBBox2.x > BBox1.x &&
						 triBBox2.x < BBox2.x))
					{
						if ((BBox1.y > triBBox1.y &&
							 BBox1.y < triBBox2.y) ||
							(BBox2.y > triBBox1.y &&
							 BBox2.y < triBBox2.y) ||
							(triBBox1.y > BBox1.y &&
							 triBBox1.y < BBox2.y) ||
							(triBBox2.y > BBox1.y &&
							 triBBox2.y < BBox2.y))
						{
							if ((BBox1.z > triBBox1.z &&
								 BBox1.z < triBBox2.z) ||
								(BBox2.z > triBBox1.z &&
								 BBox2.z < triBBox2.z) ||
								(triBBox1.z > BBox1.z &&
								 triBBox1.z < BBox2.z) ||
								(triBBox2.z > BBox1.z &&
								 triBBox2.z < BBox2.z))
							{
								*CollideObj = lpobj;
								*CollidePoly = tripos;
								CollideObj++;
								CollidePoly++;
								(*NumCollisionSlotsLeft)--;
								continue;
							}
						}
					}
				}

			}
		}
	}

	return 0;
}


//
// Terrain Collision Routines
//

// has a bug near the edges, needs to add an error factor to catch
// collision rays within the margin of error for a FLOAT32
LOGICAL trncdCheckTerrainCollision(OBJECT* lpobj, COLLISION* collision)
{
LOGICAL			retval=FALSE;
int				i, j, numoctets, numtris;
float			d, d1, d2, d3, eps;
piVECTOR		tp, dir;
const piVECTOR*	pv1, *pv2, *pv3;
piVECTOR		m, q;
piVECTOR*		op;
piVECTOR*		np;
piVECTOR		opp, npp;
piVECTOR const*	pn;
piVECTOR const*	pcn;
piINDICES*		pt;
piVECTOR*		lpcrnr;
piMATRIX		InvMat;

// The Terrain Editor uses this routine to find collisions
// in octets that are terrain replacement tiles. The texture
// objects that have the null1.bmp texture use this flag
// (OBJECT_SKIPDRAWING) to keep the texture from drawing
// but we still need collisions on these objects for the
// Terrain Editor. I* think* this was done for a speed
// optimization. If this causes trouble in the Terrain
// Editor, then we may have to duplicate the entire routine
// just to get the functionality we require. Kango
#ifndef TERRAIN_EDITOR
	if (lpobj->flags & OBJECT_SKIPDRAWING)
	{
		return FALSE;
	}
#endif

	if (lpobj->flags & OBJECT_USEMATRIX)
	{
		math3dInvertMatrix(&InvMat,&lpobj->mat);
		math3dVectorMultiply(&opp,&collision->oldpos,&InvMat);
		math3dVectorMultiply(&npp,&collision->newpos,&InvMat);
	}

	op = &opp;
	np = &npp;

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	numtris = lpobj->numtris;
	piVectorConstPtr objPos = lpobj->RenderPositions();
	pt      = lpobj->ptris;
	pn      = lpobj->psnorms;
	pcn     = lpobj->pscdn;
	lpcrnr  = lpobj->bbox;

	eps = 10.0f;
	numoctets = numtris / NUM_TRIANGLES_PER_OCTET;
	for (i=0; i < numoctets && !retval; i++)
	{
		pv1 = &objPos[i*NUM_VERTICES_PER_OCTET+1];
		pv2 = &objPos[i*NUM_VERTICES_PER_OCTET+5];

		if (op->x < pv1->x &&
			np->x < pv1->x)
		{
			pt += NUM_TRIANGLES_PER_OCTET;
			pn += NUM_TRIANGLES_PER_OCTET;
			pcn += 3 * NUM_TRIANGLES_PER_OCTET;
			continue;
		}

		if (op->x > pv2->x &&
			np->x > pv2->x)
		{
			pt += NUM_TRIANGLES_PER_OCTET;
			pn += NUM_TRIANGLES_PER_OCTET;
			pcn += 3 * NUM_TRIANGLES_PER_OCTET;
			continue;
		}

		if (op->y < pv1->y &&
			np->y < pv1->y)
		{
			pt += NUM_TRIANGLES_PER_OCTET;
			pn += NUM_TRIANGLES_PER_OCTET;
			pcn += 3 * NUM_TRIANGLES_PER_OCTET;
			continue;
		}

		if (op->y > pv2->y &&
			np->y > pv2->y)
		{
			pt += NUM_TRIANGLES_PER_OCTET;
			pn += NUM_TRIANGLES_PER_OCTET;
			pcn += 3 * NUM_TRIANGLES_PER_OCTET;
			continue;
		}

		for (j=0; j < NUM_TRIANGLES_PER_OCTET; j++, pt++, pn++, pcn+=3)
		{
			// if the polygon normal is facing away, skip the tests
			d = math3dDotProduct(&dir, pn);
			if (d >= 0.0f)
			{
				continue;
			}

			pv1 = (piVECTOR*)&objPos[pt->v1];
			pv2 = (piVECTOR*)&objPos[pt->v2];
			pv3 = (piVECTOR*)&objPos[pt->v3];


			d1 = math3dDotProduct(pv1, pn);
			d2 = math3dDotProduct(op, pn);
			d3 = math3dDotProduct(np, pn);

			// vector crosses the poly plain, do some work
			if (d2 >= d1 && d3 <= d1)
			{
				// create a vertex where the vector crosses the poly plane
				math3dScaleVector(&m, &dir, 1.0f / -d);
				math3dScaleVector(&q, &m, d2 - d1);
				math3dAddVectors(&tp, op, &q);

// terrain editor uses some very long rays for collision for mouse tracking
// and it creates objects on the fly which don't work too well with this bailout
// since collision speed isn't essential for terrain editor, we don't use this
// early out when in terrain editor mode
#ifndef TERRAIN_EDITOR
				m = q = tp;
				m.x -= eps;
				m.y -= eps;
				m.z -= eps;
				q.x += eps;
				q.y += eps;
				q.z += eps;
				if ((q.x < pv1->x &&
					 q.x < pv2->x &&
					 q.x < pv3->x) ||
					(m.x > pv1->x &&
					 m.x > pv2->x &&
					 m.x > pv3->x))
				{
					continue;
				}

				if ((q.y < pv1->y &&
					 q.y < pv2->y &&
					 q.y < pv3->y) ||
					(m.y > pv1->y &&
					 m.y > pv2->y &&
					 m.y > pv3->y))
				{
					continue;
				}

				if ((q.z < pv1->z &&
					 q.z < pv2->z &&
					 q.z < pv3->z) ||
					(m.z > pv1->z &&
					 m.z > pv2->z &&
					 m.z > pv3->z))
				{
					continue;
				}
#endif

#ifdef CD_POLY_FENCE
				// polygon fence algorithm
				// used to determine whether the point is within the polygon bounds
				{
				float       da, db;

					da = math3dDotProduct(&pcn[0], pv1);
					db = math3dDotProduct(&pcn[0], &tp);
					if (db > da)
					{
						continue;
					}

					da = math3dDotProduct(&pcn[1], pv2);
					db = math3dDotProduct(&pcn[1], &tp);
					if (db > da)
					{
						continue;
					}

					da = math3dDotProduct(&pcn[2], pv3);
					db = math3dDotProduct(&pcn[2], &tp);
					if (db > da)
					{
						continue;
					}
				}
#else
				// sum of the angles algorithm
				// used to determine whether the point is within the polygon bounds
				{
					piVECTOR		w1, w2, w3;

					math3dSubtractVectors(&w1, pv1, &tp);
					math3dSubtractVectors(&w2, pv2, &tp);
					math3dSubtractVectors(&w3, pv3, &tp);

					math3dNormalize(&w1);
					math3dNormalize(&w2);
					math3dNormalize(&w3);

					d1 = math3dDotProduct(&w1, &w2);
					d2 = math3dDotProduct(&w1, &w3);
					d3 = math3dDotProduct(&w2, &w3);
				}

				if (d1 + d2 + d3 <= -1.0f)
#endif
				{
					collision->colobj = lpobj;
					collision->colpoly = i * NUM_TRIANGLES_PER_OCTET + j;
					collision->colpos = tp;
					collision->colnormal = *pn;
					collision->colangle = d;
					retval = TRUE;
					break;
				}
			}
		}
	}

	return(retval);
}

// has a bug near the edges, needs to add an error factor to catch
// collision rays within the margin of error for a FLOAT32
//
//	this bug is probably fixed, haven't had time to determine if so
//
// assumes several things
// it assumes terrainorig arranged in the following way:
// octets arranged from left to right, then from top to bottom
// each octet comprised of 10 vertecies, 8 triangles
LOGICAL trncdCheckTerrainDown(COLLISION* collision)
{
LOGICAL        retval=FALSE;
TERRAIN_DETAIL*	td;
OBJECT* ColObject;
piVECTOR Pos;
FLOAT32 Width;
FLOAT32 Height;
SINT32 x,y;
UINT32 i;
SINT32 PolyNum;
SINT32 VertNum;
piVECTOR const*  pn;
piVECTOR  pv1;
FLOAT32 d,d1,d2;
piVECTOR m,q,tp;
piVECTOR dir;
FLOAT32 eps;
piMATRIX InvMat;
UINT32 Found;
piVECTOR CenterPos;
piVECTOR Vec1,Vec2;
UINT32 TriNum;

	dir.z = 1.0f;
	dir.x = 0.0f;
	dir.y = 0.0f;
	eps = 10.0f;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		td = &(trndtl[i]);

		ColObject = td->terrainorig;
		if (ColObject == nullptr)
		{
			continue;
		}

		Pos.x = collision->newpos.x;
		Pos.y = collision->newpos.y;
		Pos.z = 0.0f;
		math3dInvertMatrix(&InvMat,&td->terrainorig->mat);
		math3dVectorMultiply(&Pos,&Pos,&InvMat);

		Width 	= xstep * 2.0f;
		Height 	= ystep * 2.0f;

		// see if collision point inside current supercell
		piVectorConstPtr objPos = ColObject->RenderPositions();

		if (Pos.x <  objPos[1].x ||
			Pos.x >= objPos[1].x + Width * td->numoctetsacross ||
			Pos.y <  objPos[1].y ||
			Pos.y >= objPos[1].y + Height * td->numoctetsdown)
		{
			// nope, so move on
			continue;
		}

		// ok determine which octet
		x = (SINT32)((Pos.x - objPos[1].x) / Width);
		y = (SINT32)((Pos.y - objPos[1].y) / Height);

		if (x < 0 || x >= td->numoctetsacross || y < 0 || y >= td->numoctetsdown )
			continue; // nope, not in here

		PolyNum = y * td->numoctetsacross + x;
		VertNum = PolyNum;
		PolyNum *= NUM_TRIANGLES_PER_OCTET;
		VertNum *= NUM_VERTICES_PER_OCTET;

		if (VertNum >= ColObject->numverts)
		{
			char str[256];

			snprintf(str, 255, 	"trncdCheckTerrainDown:  VertNum (%d) >= ColObject->numverts (%d) "
								" (%d, %d) (%f, %f) (%f, %f) (%d, %d) (%f, %f)",
					VertNum, (int)ColObject->numverts, (int)td->numoctetsacross,(int)td->numoctetsdown,
					Pos.x, Pos.y, objPos[1].x, objPos[1].y, x, y, Width, Height);

			piExitProgram (__FILE__, __LINE__, str);
		}

		CenterPos = objPos[VertNum];
		Found = 0;
		// determine which quadrant we're in
		if (Pos.x < CenterPos.x &&
			Pos.y >= CenterPos.y)
		{
			// upper left
			// determine which triangle
			Vec1.x = -1.0f;
			Vec1.y = 1.0f;

			Vec2.x = Pos.x - CenterPos.x;
			Vec2.y = Pos.y - CenterPos.y;

			if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
			{
				TriNum = 1;
			}
			else
			{
				TriNum = 2;
			}
		}
		else
		if (Pos.x >= CenterPos.x &&
			 Pos.y >= CenterPos.y)
		{
			// upper right
			// determine which triangle
			Vec1.x = 1.0f;
			Vec1.y = 1.0f;

			Vec2.x = Pos.x - CenterPos.x;
			Vec2.y = Pos.y - CenterPos.y;

			if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
			{
				TriNum = 3;
			}
			else
			{
				TriNum = 4;
			}
		}
		else
		if (Pos.x < CenterPos.x &&
			Pos.y < CenterPos.y)
		{
			// lower left
			// determine which triangle
			Vec1.x = -1.0f;
			Vec1.y = -1.0f;

			Vec2.x = Pos.x - CenterPos.x;
			Vec2.y = Pos.y - CenterPos.y;

			if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
			{
				TriNum = 7;
			}
			else
			{
				TriNum = 0;
			}
		}
		else
		{
			// lower right
			// determine which triangle
			Vec1.x = 1.0f;
			Vec1.y = -1.0f;

			Vec2.x = Pos.x - CenterPos.x;
			Vec2.y = Pos.y - CenterPos.y;

			if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
			{
				TriNum = 5;
			}
			else
			{
				TriNum = 6;
			}
		}

		// now set up return stuff
		// determine polynum
		PolyNum += TriNum;
		// determine collision point
		pn = &ColObject->psnorms[PolyNum];
		d = math3dDotProduct(&dir, pn);
		if (d == 0)
		{
			d = 1.0f;
		}

		pv1 = ColObject->RenderPositions()[ColObject->ptris[PolyNum].v1];


		d1 = math3dDotProduct(&pv1, pn);
		d2 = math3dDotProduct(&Pos, pn);
		math3dScaleVector(&m, &dir, 1.0f / -d);
		math3dScaleVector(&q, &m, d2 - d1);
		math3dAddVectors(&tp, &Pos, &q);

		collision->colobj = ColObject;
		collision->colpoly = PolyNum;
		collision->colpos = tp;
		collision->colnormal = *pn;
		collision->colangle = d;
		return (TRUE);
	}

	return(retval);
}

// has a bug near the edges, needs to add an error factor to catch
// collision rays within the margin of error for a FLOAT32
//
//	this bug is probably fixed, haven't had time to determine if so
//
// assumes several things
// it assumes terrainorig arranged in the following way:
// octets arranged from left to right, then from top to bottom
// each octet comprised of 10 vertecies, 8 triangles
LOGICAL trncdCheckTerrainDownWithDTL(COLLISION* collision, TERRAIN_DETAIL* lpDTL)
{
//LOGICAL        retval=FALSE;
TERRAIN_DETAIL*	td;
OBJECT* ColObject;
piVECTOR Pos;
FLOAT32 Width;
FLOAT32 Height;
SINT32 x,y;
SINT32 PolyNum;
SINT32 VertNum;
piVECTOR const*  pn;
piVECTOR  pv1;
FLOAT32 d,d1,d2;
piVECTOR m,q,tp;
piVECTOR dir;
FLOAT32 eps;
piMATRIX InvMat;
UINT32 Found;
piVECTOR CenterPos;
piVECTOR Vec1,Vec2;
UINT32 TriNum;

	dir.z = 1.0f;
	dir.x = 0.0f;
	dir.y = 0.0f;
	eps = 10.0f;

    td = lpDTL;

    ColObject = td->terrainorig;
    if (ColObject == nullptr)
    {
	    return FALSE;
    }

	Pos.x = collision->newpos.x;
	Pos.y = collision->newpos.y;
	Pos.z = 0.0f;
	math3dInvertMatrix(&InvMat,&td->terrainorig->mat);
	math3dVectorMultiply(&Pos,&Pos,&InvMat);

	Width 	= xstep * 2.0f;
	Height 	= ystep * 2.0f;

	// see if collision point inside current supercell
	piVectorConstPtr objPos = ColObject->RenderPositions();

	if (Pos.x <  objPos[1].x ||
		Pos.x >= objPos[1].x + Width * td->numoctetsacross ||
		Pos.y <  objPos[1].y ||
		Pos.y >= objPos[1].y + Height * td->numoctetsdown)
	{
		// nope, so try it with all supercells
		return trncdCheckTerrainDown(collision);
	}

    x = FloatToInt(Pos.x - objPos[1].x) / FloatToInt(Width);
    y = FloatToInt(Pos.y - objPos[1].y) / FloatToInt(Height);

	if (x < 0 || x >= td->numoctetsacross || y < 0 || y >= td->numoctetsdown )
		return FALSE; // nope, not in here

	PolyNum = y * td->numoctetsacross + x;
	VertNum = PolyNum;
	PolyNum *= NUM_TRIANGLES_PER_OCTET;
	VertNum *= NUM_VERTICES_PER_OCTET;

	if (VertNum >= ColObject->numverts)
	{
		char str[256];

		snprintf(str, 255, 	"trncdCheckTerrainDown:  VertNum (%d) >= ColObject->numverts (%d) "
							" (%d, %d) (%f, %f) (%f, %f) (%d, %d) (%f, %f)",
				VertNum, (int)ColObject->numverts, (int)td->numoctetsacross,(int)td->numoctetsdown,
				Pos.x, Pos.y, objPos[1].x, objPos[1].y, x, y, Width, Height);

		piExitProgram (__FILE__, __LINE__, str);
	}

	CenterPos = objPos[VertNum];
	Found = 0;
	// determine which quadrant we're in
	if (Pos.x < CenterPos.x &&
		Pos.y >= CenterPos.y)
	{
		// upper left
		// determine which triangle
		Vec1.x = -1.0f;
		Vec1.y = 1.0f;

		Vec2.x = Pos.x - CenterPos.x;
		Vec2.y = Pos.y - CenterPos.y;

		if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
		{
			TriNum = 1;
		}
		else
		{
			TriNum = 2;
		}
	}
	else
	if (Pos.x >= CenterPos.x &&
			Pos.y >= CenterPos.y)
	{
		// upper right
		// determine which triangle
		Vec1.x = 1.0f;
		Vec1.y = 1.0f;

		Vec2.x = Pos.x - CenterPos.x;
		Vec2.y = Pos.y - CenterPos.y;

		if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
		{
			TriNum = 3;
		}
		else
		{
			TriNum = 4;
		}
	}
	else
	if (Pos.x < CenterPos.x &&
		Pos.y < CenterPos.y)
	{
		// lower left
		// determine which triangle
		Vec1.x = -1.0f;
		Vec1.y = -1.0f;

		Vec2.x = Pos.x - CenterPos.x;
		Vec2.y = Pos.y - CenterPos.y;

		if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
		{
			TriNum = 7;
		}
		else
		{
			TriNum = 0;
		}
	}
	else
	{
		// lower right
		// determine which triangle
		Vec1.x = 1.0f;
		Vec1.y = -1.0f;

		Vec2.x = Pos.x - CenterPos.x;
		Vec2.y = Pos.y - CenterPos.y;

		if ((Vec1.x * Vec2.y - Vec1.y * Vec2.x) > 0)
		{
			TriNum = 5;
		}
		else
		{
			TriNum = 6;
		}
	}

	// now set up return stuff
	// determine polynum
	PolyNum += TriNum;
	// determine collision point
	pn = &ColObject->psnorms[PolyNum];
	d = math3dDotProduct(&dir, pn);
	if (d == 0)
	{
		d = 1.0f;
	}

	pv1 = ColObject->RenderPositions()[ColObject->ptris[PolyNum].v1];

	d1 = math3dDotProduct(&pv1, pn);
	d2 = math3dDotProduct(&Pos, pn);
	math3dScaleVector(&m, &dir, 1.0f / -d);
	math3dScaleVector(&q, &m, d2 - d1);
	math3dAddVectors(&tp, &Pos, &q);

	collision->colobj = ColObject;
	collision->colpoly = PolyNum;
	collision->colpos = tp;
	collision->colnormal = *pn;
	collision->colangle = d;

    return(TRUE);
}

// has a bug near the edges, needs to add an error factor to catch
// collision rays within the margin of error for a FLOAT32
LOGICAL trncdCheckTerrain(COLLISION* collision)
{
LOGICAL			retval=FALSE, indirection;
int				i, j, k, l, m, numverts;
float			/*x, y, z, d, */xo, yo, zo, xn, yn, zn, xc, yc, zc, dp;
float			xo1, yo1, zo1, xn1, yn1, zn1;
float			mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR		mn, mx;
piMATRIX		InvMat;
piVECTOR		TempVec;
OBJECT*		lpobj;
piVECTOR*		op;
piVECTOR*		np;
piVECTOR*		lpcrnr;
piVECTOR		op1;
piVECTOR		dir, vt, pv;
TERRAIN_DETAIL*	td;
SINT32			texid;


	td = trndtl;

	op      = &collision->oldpos;
	np      = &collision->newpos;

	// cannot collide if standing still
	if (op->x == np->x &&
		op->y == np->y &&
		op->z == np->z)
	{
		return(FALSE);
	}

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	xo1 = op->x;
	yo1 = op->y;
	zo1 = op->z;
	xn1 = np->x;
	yn1 = np->y;
	zn1 = np->z;

//	texid = piGetNamedTexture("null1.bmp");
	texid = _NullTexId;
	for (l=0; l < NUM_TERRAIN_RECTANGLES && !retval; l++, td++)
	{
		lpobj = td->terrain;
		if (lpobj == nullptr)
		{
			continue;
		}

		for (m=0; m < MAX_TERRAIN_TEXTURES && !retval; lpobj=td->nonreplacementtiles[m++])
		{
			if (lpobj == nullptr)
			{
				continue;
			}

			if (lpobj->texid == texid)
			{
				lpobj->flags |= OBJECT_SKIPDRAWING;
				//continue;
			}

			if (!(lpobj->flags & OBJECT_VISIBLE))
			{
				continue;
			}

			if (!(lpobj->flags & OBJECT_COLLISION))
			{
				continue;
			}

			if (lpobj->flags & OBJECT_USEMATRIX)
			{
				math3dInvertMatrix(&InvMat,&lpobj->mat);
				TempVec.x = xo1;
				TempVec.y = yo1;
				TempVec.z = zo1;
				math3dVectorMultiply(&TempVec,&TempVec,&InvMat);
				xo = TempVec.x;
				yo = TempVec.y;
				zo = TempVec.z;

				TempVec.x = xn1;
				TempVec.y = yn1;
				TempVec.z = zn1;
				math3dVectorMultiply(&TempVec,&TempVec,&InvMat);
				xn = TempVec.x;
				yn = TempVec.y;
				zn = TempVec.z;

				math3dVectorMultiply(&op1,op,&InvMat);
			}
			else
			{
				xo = xo1;
				yo = yo1;
				zo = zo1;

				xn = xn1;
				yn = yn1;
				zn = zn1;

				op1 = *op;
			}

			// center of this object
			xc = lpobj->pos.x;
			yc = lpobj->pos.y;
			zc = lpobj->pos.z;

			// very simple in-the-direction-of-movement check
			indirection = FALSE;
			if (lpobj->numverts <= 8)
			{
				piVectorConstPtr objPos = lpobj->RenderPositions();
				numverts  = lpobj->numverts;
				for (i=0; i < numverts; i++, ++objPos)
				{
					math3dSubtractVectors(&vt, objPos.ptr, &op1);
					dp = math3dDotProduct(&dir, &vt);
					if (dp > 0.0f)
					{
						indirection = TRUE;
						break;
					}
				}
			}
			else
			{
				lpcrnr = lpobj->bbox;
				for (i=0; i < 2 && !indirection; i++)
				{
					vt.x = xc + lpcrnr[i].x;
					for (j=0; j < 2 && !indirection; j++)
					{
						vt.y = yc + lpcrnr[j].y;
						for (k=0; k < 2; k++)
						{
							vt.z = zc + lpcrnr[k].z;
							math3dSubtractVectors(&pv, &vt, &op1);
							dp = math3dDotProduct(&dir, &pv);
							if (dp > 0.0f)
							{
								indirection = TRUE;
								break;
							}
						}
					}
				}
			}

			if (!indirection)
			{
				continue;
			}

			// FIX ME! This can save a bunch of time;
			// at the very edge of an object, dp goes negative
			// or the distance becomes too great (it shouldn't)
/*
			if (0)
			{
				// Reject if path (a line) is completely
				// outside radius from center of object
				vt.x = x = xc - xo;
				vt.y = y = yc - yo;
				vt.z = z = zc - zo;
				math3dNormalize(&vt);

				d = x * x + y * y + z * z;
				dp = math3dDotProduct(&vt, &dir);
				if (dp <= 0.0f)
				{
					continue;
				}

				dp *= d;
				d = (float)sqrt(dp);	// extract sine from hypotenus and cosine

				x = xc - (dir.x * d + xo);
				y = yc - (dir.y * d + yo);
				z = zc - (dir.z * d + zo);

				d = x * x + y * y + z * z;
				if (d > lpobj->maxrad)
				{
					continue;
				}
			}
*/
			// If inside bounding box, evaluate each polygon
			mn = lpobj->bbox[0];
			mnx = mn.x + xc;
			mny = mn.y + yc;
			mnz = mn.z + zc;
			mx = lpobj->bbox[1];
			mxx = mx.x + xc;
			mxy = mx.y + yc;
			mxz = mx.z + zc;

			if ((mnx <= xn &&
				mxx >= xn) ||
				(mnx <= xo &&
				mxx >= xo))
			{
				if ((mny <= yn &&
					mxy >= yn) ||
					(mny <= yo &&
					mxy >= yo))
				{
					{
						if (trncdCheckTerrainCollision(lpobj, collision))
						{
							retval = TRUE;
							break;
						}

						continue;
					}
				}
			}

			if ((mnx <= xn &&
				mxx >= xo) ||
				(mnx <= xo &&
				mxx >= xn))
			{
				if ((mny <= yn &&
					mxy >= yo) ||
					(mny <= yo &&
					mxy >= yn))
				{
					{
						if (trncdCheckTerrainCollision(lpobj, collision))
						{
							retval = TRUE;
							break;
						}

						continue;
					}
				}
			}

			continue;
		}
	}

	return(retval);
}

/*
// has a bug near the edges, needs to add an error factor to catch
// collision rays within the margin of error for a FLOAT32
LOGICAL trncdCheckOriginalTerrain(COLLISION* collision)
{
LOGICAL			retval=FALSE, indirection;
int				i, j, k, l, numverts;
float			//x, y, z, d, //xo, yo, zo, xn, yn, zn, xc, yc, zc, dp;
float			xo1, yo1, zo1, xn1, yn1, zn1;
float			mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR		mn, mx;
piMATRIX		InvMat;
piVECTOR		TempVec;
OBJECT*		lpobj;
piVECTOR*		op, np, lpcrnr;
piVECTOR		op1;
piVECTOR		dir, vt, pv;
TERRAIN_DETAIL*	td;



	td = trndtl;

	op      = &collision->oldpos;
	np      = &collision->newpos;

	// cannot collide if standing still
	if (op->x == np->x &&
		op->y == np->y &&
		op->z == np->z)
	{
		return(FALSE);
	}

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	xo1 = op->x;
	yo1 = op->y;
	zo1 = op->z;
	xn1 = np->x;
	yn1 = np->y;
	zn1 = np->z;

	for (l=0; l < NUM_TERRAIN_RECTANGLES; l++, td++)
	{
		lpobj = td->terrainorig;
		if (lpobj == nullptr)
		{
			continue;
		}

		if (!(lpobj->flags & OBJECT_COLLISION))
		{
			continue;
		}

		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			math3dInvertMatrix(&InvMat,&lpobj->mat);
			TempVec.x = xo1;
			TempVec.y = yo1;
			TempVec.z = zo1;
			math3dVectorMultiply(&TempVec,&TempVec,&InvMat);
			xo = TempVec.x;
			yo = TempVec.y;
			zo = TempVec.z;

			TempVec.x = xn1;
			TempVec.y = yn1;
			TempVec.z = zn1;
			math3dVectorMultiply(&TempVec,&TempVec,&InvMat);
			xn = TempVec.x;
			yn = TempVec.y;
			zn = TempVec.z;

			math3dVectorMultiply(&op1,op,&InvMat);
		}
		else
		{
			xo = xo1;
			yo = yo1;
			zo = zo1;

			xn = xn1;
			yn = yn1;
			zn = zn1;

			op1 = *op;
		}

		// center of this object
		xc = lpobj->pos.x;
		yc = lpobj->pos.y;
		zc = lpobj->pos.z;

		// very simple in-the-direction-of-movement check
		indirection = FALSE;
		if (lpobj->numverts <= 8)
		{
			piVectorConstPtr objPos = lpobj->RenderPositions();
			numverts  = lpobj->numverts;
			for (i=0; i < numverts; i++, ++objPos)
			{
				math3dSubtractVectors(&vt, objPos.ptr, &op1);
				dp = math3dDotProduct(&dir, &vt);
				if (dp > 0.0f)
				{
					indirection = TRUE;
					break;
				}
			}
		}
		else
		{
			lpcrnr = lpobj->bbox;
			for (i=0; i < 2 && !indirection; i++)
			{
				vt.x = xc + lpcrnr[i].x;
				for (j=0; j < 2 && !indirection; j++)
				{
					vt.y = yc + lpcrnr[j].y;
					for (k=0; k < 2; k++)
					{
						vt.z = zc + lpcrnr[k].z;
						math3dSubtractVectors(&pv, &vt, &op1);
						dp = math3dDotProduct(&dir, &pv);
						if (dp > 0.0f)
						{
							indirection = TRUE;
							break;
						}
					}
				}
			}
		}

		if (!indirection)
		{
			continue;
		}

		// FIX ME! This can save a bunch of time;
		// at the very edge of an object, dp goes negative
		// or the distance becomes too great (it shouldn't)
//
		if (0)
		{
			// Reject if path (a line) is completely
			// outside radius from center of object
			vt.x = x = xc - xo;
			vt.y = y = yc - yo;
			vt.z = z = zc - zo;
			math3dNormalize(&vt);

			d = x * x + y * y + z * z;
			dp = math3dDotProduct(&vt, &dir);
			if (dp <= 0.0f)
			{
				continue;
			}

			dp *= d;
			d = (float)sqrt(dp);	// extract sine from hypotenus and cosine

			x = xc - (dir.x * d + xo);
			y = yc - (dir.y * d + yo);
			z = zc - (dir.z * d + zo);

			d = x * x + y * y + z * z;
			if (d > lpobj->maxrad)
			{
				continue;
			}
		}
//
		// If inside bounding box, evaluate each polygon
		mn = lpobj->bbox[0];
		mnx = mn.x + xc;
		mny = mn.y + yc;
		mnz = mn.z + zc;
		mx = lpobj->bbox[1];
		mxx = mx.x + xc;
		mxy = mx.y + yc;
		mxz = mx.z + zc;

		if ((mnx <= xn &&
			mxx >= xn) ||
			(mnx <= xo &&
			mxx >= xo))
		{
			if ((mny <= yn &&
				mxy >= yn) ||
				(mny <= yo &&
				mxy >= yo))
			{
//				if ((mnz <= zn &&
//					mxz >= zn) ||
//					(mnz <= zo &&
//					mxz >= zo))
				{
					//Write_Debug("Non-flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorWHITEALPHA);
					if (trncdCheckTerrainCollision(lpobj, collision))
//					if (obcdCheckObjectCollision(lpobj,collision))
					{
						retval = TRUE;
						break;
					}

					continue;
				}
			}
		}

		if ((mnx <= xn &&
			mxx >= xo) ||
			(mnx <= xo &&
			mxx >= xn))
		{
			if ((mny <= yn &&
				mxy >= yo) ||
				(mny <= yo &&
				mxy >= yn))
			{
//				if ((mnz <= zn &&
//					mxz >= zo) ||
//					(mnz <= zo &&
//					mxz >= zn))
				{
					//Write_Debug("Flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorREDALPHA);
					if (trncdCheckTerrainCollision(lpobj, collision))
					{
						retval = TRUE;
						break;
					}

					continue;
				}
			}
		}
	}

	return(retval);
}

LOGICAL trncdPlayerToTerrain(piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL		retval;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = -oldpos->x;
	collision.oldpos.y = -oldpos->y;
	collision.oldpos.z = -oldpos->z;
	collision.newpos.x = -newpos->x;
	collision.newpos.y = -newpos->y;
	collision.newpos.z = -newpos->z;

	retval = trncdCheckTerrain(&collision);

	if (retval)
	{
		newpos->x = -collision.colpos.x;
		newpos->y = -collision.colpos.y;
		newpos->z = -collision.colpos.z;
	}
	else
	{
		newpos->x = -collision.newpos.x;
		newpos->y = -collision.newpos.y;
		newpos->z = -collision.newpos.z;
	}

	return(retval);
}
*/

// FIXME: optimize
LOGICAL trncdObjectToTerrain(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL			retval;
COLLISION		collision;

#ifdef EVENT_LOG_TIMES
LARGE_INTEGER	count1, count2;
static __int64	count=0;
static int		number=0;
//static int		npolys=0;
#endif


#ifdef EVENT_LOG_TIMES
	if (!QueryPerformanceCounter(&count1))
	{
	}
#endif

	memset(&collision, 0, sizeof(collision));
	collision.object   = object;
	collision.oldpos.x = oldpos->x;
	collision.oldpos.y = oldpos->y;
	collision.oldpos.z = oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;

	retval = trncdCheckTerrain(&collision);

	if (retval)
	{
		newpos->x = collision.colpos.x;
		newpos->y = collision.colpos.y;
		newpos->z = collision.colpos.z;
		if (collision.colobj != nullptr)
		{
			newpos->x += collision.colobj->mat._41;
			newpos->y += collision.colobj->mat._42;
			newpos->z += collision.colobj->mat._43;
		}

	}
	else
	{
		newpos->x = collision.newpos.x;
		newpos->y = collision.newpos.y;
		newpos->z = collision.newpos.z;
	}

#ifdef EVENT_LOG_TIMES
	if (QueryPerformanceCounter(&count2))
	{
		count += count2.QuadPart - count1.QuadPart;
		if (++number == 128)
		{
			QueryPerformanceFrequency(&count2);
//			Write_Debug("Average object to terrain collision detect time: %d us",
//				(DWORD)(count / 128) * 1000 / (DWORD)(count2.LowPart / 1000));
			number = 0;
			count = 0;
//			npolys = 0;
		}
	}
#endif

	return(retval);
}

//
// Object Collision Routines
//

LOGICAL trncdCheckObjectCollision(OBJECT* lpobj, COLLISION* collision)
{
LOGICAL		retval=FALSE;
int			i, numtris;
float		d, d1, d2, d3, eps;
piVECTOR	tp, dir;
piVECTOR	pv1, pv2, pv3;
piVECTOR	m, q;
piVECTOR*	op;
piVECTOR*	np;
piVECTOR const* pn;
piVECTOR const*	pcn;
piINDICES*	pt;
piVECTOR*	lpcrnr;

#ifdef EVENT_LOG_TIMES
LARGE_INTEGER count1, count2;
static __int64    count=0;
static int        number=0;
static int		  npolys=0;
#endif

	if((lpobj->flags & OBJECT_NOTOBJECT)) return FALSE;

#ifdef EVENT_LOG_TIMES
	if (!QueryPerformanceCounter(&count1))
	{
	}
#endif


	op = &collision->oldpos;
	np = &collision->newpos;

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	numtris = lpobj->numtris;
	piVectorConstPtr objPos = lpobj->RenderPositions();
	pt      = lpobj->ptris;
	pn      = lpobj->psnorms;
	pcn     = lpobj->pscdn;
	lpcrnr  = lpobj->bbox;

	eps = FLT_EPSILON * 16.0f;
	for (i=0; i < numtris; i++, pt++, pn++, pcn+=3)
	{
		d = math3dDotProduct(&dir, pn);
		if (d >= 0.0f)
		{
			continue;
		}

#ifdef EVENT_LOG_TIMES
		npolys++;
#endif

		math3dVectorMultiply(&pv1, &objPos[pt->v1], &lpobj->mat);
		d1 = math3dDotProduct(&pv1, pn);
		d2 = math3dDotProduct(op, pn);
		d3 = math3dDotProduct(np, pn);

		if (d2 >= d1 && d3 <= d1)
		{
			math3dVectorMultiply(&pv2, &objPos[pt->v2], &lpobj->mat);
			math3dVectorMultiply(&pv3, &objPos[pt->v3], &lpobj->mat);

			// create a vertex where the vector crosses the poly plane
			math3dScaleVector(&m, &dir, 1.0f / -d);
			math3dScaleVector(&q, &m, d2 - d1);
			math3dAddVectors(&tp, op, &q);

#ifdef CD_POLY_FENCE
			// polygon fence algorithm
			// used to determine whether the point is within the polygon bounds
			{
			float       da, db;

				da = math3dDotProduct(&pcn[0], &pv1);
				db = math3dDotProduct(&pcn[0], &tp);
				if (db > da)
				{
					continue;
				}

				da = math3dDotProduct(&pcn[1], &pv2);
				db = math3dDotProduct(&pcn[1], &tp);
				if (db > da)
				{
					continue;
				}

				da = math3dDotProduct(&pcn[2], &pv3);
				db = math3dDotProduct(&pcn[2], &tp);
				if (db > da)
				{
					continue;
				}
			}
#else
			// sum of the angles algorithm
			// used to determine whether the point is within the polygon bounds
			{
				piVECTOR		w1, w2, w3;

				math3dSubtractVectors(&w1, pv1, &tp);
				math3dSubtractVectors(&w2, pv2, &tp);
				math3dSubtractVectors(&w3, pv3, &tp);

				math3dNormalize(&w1);
				math3dNormalize(&w2);
				math3dNormalize(&w3);

				d1 = math3dDotProduct(&w1, &w2);
				d2 = math3dDotProduct(&w1, &w3);
				d3 = math3dDotProduct(&w2, &w3);
			}

			if (d1 + d2 + d3 <= -1.0f)
#endif
			{
				//OutlineTriangle(pv, pt, colorCYANALPHA);
				//HighliteView(&tp, colorCYANALPHA);
				if (collision->collide)
				{
				FLOAT32		dist1, dist2;
				piVECTOR	v1;

					math3dSubtractVectors(&v1, &collision->colpos, op);
					dist1 = math3dDotProduct(&v1, &v1);
					math3dSubtractVectors(&v1, &tp, op);
					dist2 = math3dDotProduct(&v1, &v1);
					if (dist2 < dist1)
					{
						collision->colobj = lpobj;
						collision->colpoly = i;
						collision->colpos = tp;
						collision->colnormal = *pn;
						collision->colangle = d;
						retval = TRUE;
					}

					continue;
				}

				collision->colobj = lpobj;
				collision->colpoly = i;
				collision->colpos = tp;
				collision->colnormal = *pn;
				collision->colangle = d;
				collision->collide = TRUE;
				retval = TRUE;
				continue;
			}
		}
	}

#ifdef EVENT_LOG_TIMES
	if (QueryPerformanceCounter(&count2))
	{
		count += count2.QuadPart - count1.QuadPart;
		if (++number == 1024)
		{
			QueryPerformanceFrequency(&count2);
#ifdef TERRAIN_EDITOR
			Write_Debug("Average object polygon collision detect time: %d us, polys = %d",
				(DWORD)(count / 1024) * 1000 / (DWORD)(count2.LowPart / 1000), (DWORD)(npolys / number));
#endif
			number = 0;
			count = 0;
			npolys = 0;
		}
	}
#endif

	return(retval);
}

static LOGICAL CheckObjectImplement(OBJECT* lpobj, COLLISION* collision)
{
LOGICAL			indirection;
int				i, j, k, numverts;
float			/*x, y, z, d, */xo, yo, zo, xn, yn, zn, xc, yc, zc, dp;
float			mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR		mn, mx, pos, mvd;
piVECTOR*		op;
piVECTOR*		np;
piVECTOR*		lpcrnr;
piVECTOR		dir, vt, pv;

	op = &collision->oldpos;
	np = &collision->newpos;

	xo = op->x;
	yo = op->y;
	zo = op->z;
	xn = np->x;
	yn = np->y;
	zn = np->z;

	math3dSubtractVectors(&dir, np, op);

	// length of dir unnecessary for bounds checking until the FIX ME below is fixed
//	math3dNormalize(&dir);

	do
	{
		if (lpobj->flags & OBJECT_NOTOBJECT)
		{
			continue;
		}

		if (!(lpobj->flags & OBJECT_VISIBLE))
		{
			continue;
		}

		if (!(lpobj->flags & OBJECT_COLLISION))
		{
			continue;
		}

		if (lpobj->flags & OBJECT_USEMATRIX)
		{
		}

		// center of this object
		pos = lpobj->pos;
		math3dVectorMultiply(&pos, &pos, &lpobj->mat);

		xc = pos.x;
		yc = pos.y;
		zc = pos.z;

		// very simple in-the-direction-of-movement check
		indirection = FALSE;
		if (lpobj->numverts <= 8)
		{
			piVectorConstPtr objPos = lpobj->RenderPositions();
			numverts  = lpobj->numverts;
			for (i=0; i < numverts; i++, ++objPos)
			{
				math3dAddVectors(&mvd, objPos.ptr, &pos);
				math3dSubtractVectors(&vt, &mvd, op);
				dp = math3dDotProduct(&dir, &vt);
				if (dp > 0.0f)
				{
					indirection = TRUE;
					break;
				}
			}
		}
		else
		{
			lpcrnr = lpobj->bbox;
			for (i=0; i < 2 && !indirection; i++)
			{
				vt.x = xc + lpcrnr[i].x;
				for (j=0; j < 2 && !indirection; j++)
				{
					vt.y = yc + lpcrnr[j].y;
					for (k=0; k < 2; k++)
					{
						vt.z = zc + lpcrnr[k].z;
						math3dSubtractVectors(&pv, &vt, op);
						dp = math3dDotProduct(&dir, &pv);
						if (dp > 0.0f)
						{
							indirection = TRUE;
							break;
						}
					}
				}
			}
		}

		if (!indirection)
		{
			continue;
		}

		// FIX ME! This can save a bunch of time;
		// at the very edge of an object, dp goes negative
		// or the distance becomes too great (it shouldn't)
/*
		if (0)
		{
			// Reject if path (a line) is completely
			// outside radius from center of object
			vt.x = x = xc - xo;
			vt.y = y = yc - yo;
			vt.z = z = zc - zo;
			math3dNormalize(&vt);

			d = x * x + y * y + z * z;
			dp = math3dDotProduct(&vt, &dir);
			if (dp <= 0.0f)
			{
				continue;
			}

			dp *= d;
			d = (float)sqrt(dp);	// extract sine from hypotenus and cosine

			x = xc - (dir.x * d + xo);
			y = yc - (dir.y * d + yo);
			z = zc - (dir.z * d + zo);

			d = x * x + y * y + z * z;
			if (d > lpobj->maxrad)
			{
				continue;
			}
		}
*/
		// If inside bounding box, evaluate each polygon
		mn = lpobj->bbox[0];
		mnx = mn.x + xc;
		mny = mn.y + yc;
		mnz = mn.z + zc;
		mx = lpobj->bbox[1];
		mxx = mx.x + xc;
		mxy = mx.y + yc;
		mxz = mx.z + zc;

		if ((mnx <= xn &&
			mxx >= xn) ||
			(mnx <= xo &&
			mxx >= xo))
		{
			if ((mny <= yn &&
				mxy >= yn) ||
				(mny <= yo &&
				mxy >= yo))
			{
				if ((mnz <= zn &&
					mxz >= zn) ||
					(mnz <= zo &&
					mxz >= zo))
				{
					//Write_Debug("Non-flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorWHITEALPHA);
					if (trncdCheckObjectCollision(lpobj, collision))
					{
						return(TRUE);
					}

					continue;
				}
			}
		}

		if ((mnx <= xn &&
			mxx >= xo) ||
			(mnx <= xo &&
			mxx >= xn))
		{
			if ((mny <= yn &&
				mxy >= yo) ||
				(mny <= yo &&
				mxy >= yn))
			{
				if ((mnz <= zn &&
					mxz >= zo) ||
					(mnz <= zo &&
					mxz >= zn))
				{
					//Write_Debug("Flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorREDALPHA);
					if (trncdCheckObjectCollision(lpobj, collision))
					{
						return(TRUE);
					}

					continue;
				}
			}
		}
	}
	while(FALSE);

	return(FALSE);
}

static LOGICAL CheckObjectInterate(OBJECT* lpobj, COLLISION* collision, LOGICAL* collide)
{
OBJECT*	lpatt;

	while (lpobj)
	{
		if (lpobj->sibling)
		{
			CheckObjectInterate(lpobj->sibling, collision, collide);
		}

		lpatt = lpobj->attached;
		while (lpatt)
		{
			if (CheckObjectImplement(lpatt, collision))
			{
				*collide = TRUE;
			}

			lpatt = lpatt->attached;
		}

		if (CheckObjectImplement(lpobj, collision))
		{
			*collide = TRUE;
		}

		lpobj = lpobj->child;
	}

	return(TRUE);
}

LOGICAL trncdCheckObject(COLLISION* collision)
{
LOGICAL				retval=FALSE;
SINT8				objname[64];
int					l, obnum;
OBJECT*			lpobj;
piVECTOR*		op;
piVECTOR*		np;
TERRAIN_DETAIL*	td = trndtl;

	op = &collision->oldpos;
	np = &collision->newpos;

	// cannot collide if standing still
	if (op->x == np->x &&
		op->y == np->y &&
		op->z == np->z)
	{
		return(FALSE);
	}

	for (l=0; l < NUM_TERRAIN_RECTANGLES; l++, td++)
	{
		for (obnum=0; obnum < td->tobndx; obnum++)
		{
			// could optimize this via some array or list of object names bypassing the
			// iterate-through-each-object-in-database scan
			sprintf(objname, "%s_%d_%d", td->trnobj[obnum].objname, (SINT32)td->trnobj[obnum].pos.x, (SINT32)td->trnobj[obnum].pos.y);
			lpobj = obmainFindTopmostObject(objname);
			if (lpobj == nullptr)
			{
				continue;
			}

			CheckObjectInterate(lpobj, collision, &retval);
		}
	}


	return(retval);
}
/*

LOGICAL trncdPlayerToObject1(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly)
{
LOGICAL		retval;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = -oldpos->x;
	collision.oldpos.y = -oldpos->y;
	collision.oldpos.z = -oldpos->z;
	collision.newpos.x = -newpos->x;
	collision.newpos.y = -newpos->y;
	collision.newpos.z = -newpos->z;

	retval = trncdCheckObject(&collision);

	if (retval)
	{
		*retpos = collision.colpos;
		*lpobj = collision.colobj;
		*poly = collision.colpoly;
	}


	return(retval);
}


LOGICAL trncdPlayerToObject(piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL		retval;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = -oldpos->x;
	collision.oldpos.y = -oldpos->y;
	collision.oldpos.z = -oldpos->z;
	collision.newpos.x = -newpos->x;
	collision.newpos.y = -newpos->y;
	collision.newpos.z = -newpos->z;

	retval = trncdCheckObject(&collision);

	if (retval)
	{
		newpos->x = -collision.colpos.x;
		newpos->y = -collision.colpos.y;
		newpos->z = -collision.colpos.z;
	}
	else
	{
		newpos->x = -collision.newpos.x;
		newpos->y = -collision.newpos.y;
		newpos->z = -collision.newpos.z;
	}

	return(retval);
}

LOGICAL trncdObjectToObject(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL		retval;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.object   = object;
	collision.oldpos.x = oldpos->x;
	collision.oldpos.y = oldpos->y;
	collision.oldpos.z = oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;

	retval = trncdCheckObject(&collision);

	if (retval)
	{
		newpos->x = collision.colpos.x;
		newpos->y = collision.colpos.y;
		newpos->z = collision.colpos.z;
	}
	else
	{
		newpos->x = collision.newpos.x;
		newpos->y = collision.newpos.y;
		newpos->z = collision.newpos.z;
	}

	return(retval);
}

// has not been converted to use terrain matrix
LOGICAL trncdPlayerOnGround(piVECTOR* player, piVECTOR* oldpos, piVECTOR* newpos)
{
//LOGICAL		retval1, retval2;
//FLOAT32		dist1, dist2;
//piVECTOR	tmp1, tmp2, pp;
COLLISION	collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = -oldpos->x;
	collision.oldpos.y = -oldpos->y;
	collision.oldpos.z = -oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;

//	retval1 = trncdCheckTerrain(&collision);
	retval1 = trncdCheckTerrainDown(&collision);

	newpos->x = collision.colpos.x;
	newpos->y = collision.colpos.y;
	newpos->z = collision.colpos.z;

	return(retval1);

if (retval1)
	{
		collision.newpos = collision.colpos;
		tmp1.x = -collision.colpos.x;
		tmp1.y = -collision.colpos.y;
		tmp1.z = -collision.colpos.z;
		math3dSubtractVectors(&pp, &tmp1, player);
		dist1 = math3dDotProduct(&pp, &pp);

//		retval2 = trncdCheckObject(&collision);
		retval2 = trncdCheckTerrainDown(&collision);

		if (retval2)
		{
			tmp2.x = -collision.colpos.x;
			tmp2.y = -collision.colpos.y;
			tmp2.z = -collision.colpos.z;
			math3dSubtractVectors(&pp, &tmp2, player);
			dist2 = math3dDotProduct(&pp, &pp);
			if (dist1 < dist2)
			{
				newpos->x = tmp1.x;
				newpos->y = tmp1.y;
				newpos->z = tmp1.z;
			}
			else
			{
				newpos->x = tmp2.x;
				newpos->y = tmp2.y;
				newpos->z = tmp2.z;
			}
		}
		else
		{
			newpos->x = tmp1.x;
			newpos->y = tmp1.y;
			newpos->z = tmp1.z;
		}
	}
	else
	{
		newpos->x = -collision.newpos.x;
		newpos->y = -collision.newpos.y;
		newpos->z = -collision.newpos.z;
		return(FALSE);
	}

	return(retval1 || retval2);
}
*/

/*
// has not been converted to use terrain object matrix
LOGICAL trncdCollideOnGround(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly)
{
LOGICAL		retval1, retval2;
FLOAT32		dist1, dist2;
piVECTOR	tmp1, tmp2, pp;
COLLISION	collision1, collision2;

	memset(&collision1, 0, sizeof(collision1));
	collision1.oldpos.x = oldpos->x;
	collision1.oldpos.y = oldpos->y;
	collision1.oldpos.z = oldpos->z;
	collision1.newpos.x = newpos->x;
	collision1.newpos.y = newpos->y;
	collision1.newpos.z = newpos->z;

	retval1 = trncdCheckTerrain(&collision1);

	if (retval1)
	{
		tmp1.x = collision1.colpos.x;
		tmp1.y = collision1.colpos.y;
		tmp1.z = collision1.colpos.z;
		math3dSubtractVectors(&pp, &tmp1, oldpos);
		dist1 = math3dDotProduct(&pp, &pp);

		collision2  = collision1;
		collision2.newpos = collision2.colpos;
		retval2 = trncdCheckObject(&collision2);

		if (retval2)
		{
			tmp2.x = collision2.colpos.x;
			tmp2.y = collision2.colpos.y;
			tmp2.z = collision2.colpos.z;
			math3dSubtractVectors(&pp, &tmp2, oldpos);
			dist2 = math3dDotProduct(&pp, &pp);
			if (dist1 < dist2)
			{
				*retpos = collision1.colpos;
				*lpobj = collision1.colobj;
				*poly = collision1.colpoly;
			}
			else
			{
				*retpos = collision2.colpos;
				*lpobj = collision2.colobj;
				*poly = collision2.colpoly;
			}
		}
		else
		{
			*retpos = collision1.colpos;
			*lpobj = collision1.colobj;
			*poly = collision1.colpoly;
		}
	}

	return(retval1 || retval2);
}
*/

UINT32 trncdGetTOHead(trnCDTerrainObject** TOHead)
{
	*TOHead = _TerrainObjectsHead.Next;

	return 0;
}

// steps through the object list until out of objects or a collision occurs
// different from other collider of similar name in that the exact objects to check are passed in
UINT32 trncdCollideWithListOfTerrainObjects1(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											OBJECT** CollideObjects, trnCDTerrainObject** TOList,
											UINT32 MaxNumInTOList, piVECTOR* OldPnt, piVECTOR* NewPnt,
											UINT32 DontIgnoreBackface, UINT32 Flags)
{
OBJECT*				Object2;
trnCDTerrainObject*	TO;
UINT32				Count;
FLOAT32				mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR			OldPnt1, NewPnt1, OldPnt2, NewPnt2;

	Count = 0;

    if(!g_bIgnoreTreeCollide && (!Flags || (Flags & COLLIDER_TRN_TREE)))
        sptCollide(*OldPnt, *NewPnt, CollisionList, *NumCollisionSlotsLeft);

#ifdef TRNCD_USE_ODE
	_odeCollide(*OldPnt, *NewPnt, CollisionList, *NumCollisionSlotsLeft);
	return 0;
#endif

	while (Count < MaxNumInTOList)
	{
		Object2 = CollideObjects[Count];
		TO = TOList[Count];
		Count++;

		if (TO == nullptr || Object2 == nullptr)
		{
			break;
		}

		if (!(Object2->flags & OBJECT_VISIBLE))
		{
			continue;
		}

		if (TO->InUse == 0)
		{
			continue;
		}

		if (TO->DamageLevel == FB_CLOSED_VISLEVEL)
		{
			continue;
		}

		if (!TO->IsCollidable(Flags))
		{
			continue;
		}

        if(Object2->flag2 & OBJECT_SPEEDTREE && !g_bIgnoreTreeCollide && (!Flags || (Flags & COLLIDER_TRN_TREE)))
        {
			trncdCollideSpeedTree(Object2,
                                  Object2,
                                  CollisionList,
                                  NumCollisionSlotsLeft,
                                  OldPnt,
                                  NewPnt);
            continue;
        }

		// check bounding box
		// note that the bounding boxes are relative to the center of the vertecies
		mnx = Object2->bbox[0].x;
		mny = Object2->bbox[0].y;
		mnz = Object2->bbox[0].z;

		mxx = Object2->bbox[1].x;
		mxy = Object2->bbox[1].y;
		mxz = Object2->bbox[1].z;


		// try moving the vector multiply to the actual poly-check function level
		// via a slightly bigger bounding box check
		math3dVectorMultiply(&OldPnt2,OldPnt,&Object2->matint);
		math3dVectorMultiply(&NewPnt2,NewPnt,&Object2->matint);

		mathSubVectors(&OldPnt1,&OldPnt2,&Object2->pos);
		mathSubVectors(&NewPnt1,&NewPnt2,&Object2->pos);


		if ((mnx <= NewPnt1.x &&
			mxx >= NewPnt1.x) ||
			(mnx <= OldPnt1.x &&
			mxx >= OldPnt1.x))
		{
			if ((mny <= NewPnt1.y &&
				mxy >= NewPnt1.y) ||
				(mny <= OldPnt1.y &&
				mxy >= OldPnt1.y))
			{
				if ((mnz <= NewPnt1.z &&
					mxz >= NewPnt1.z) ||
					(mnz <= OldPnt1.z &&
					mxz >= OldPnt1.z))
				{
					// ok lets check the polygons
					trncdCollideWithSpecificTerrainObject(CollisionList,
													  NumCollisionSlotsLeft,
													  Object2,
													  TO->TerrainMasterObject,
													  TO,
													  &OldPnt2,
													  &NewPnt2,
													  DontIgnoreBackface,
													  Flags);


					continue;
				}
			}
		}

		if ((mnx <= NewPnt1.x &&
			mxx >= OldPnt1.x) ||
			(mnx <= OldPnt1.x &&
			mxx >= NewPnt1.x))
		{
			if ((mny <= NewPnt1.y &&
				mxy >= OldPnt1.y) ||
				(mny <= OldPnt1.y &&
				mxy >= NewPnt1.y))
			{
				if ((mnz <= NewPnt1.z &&
					mxz >= OldPnt1.z) ||
					(mnz <= OldPnt1.z &&
					mxz >= NewPnt1.z))
				{
					// ok let's check the polygons
					trncdCollideWithSpecificTerrainObject(CollisionList,
													  NumCollisionSlotsLeft,
													  Object2,
													  TO->TerrainMasterObject,
													  TO,
													  &OldPnt2,
													  &NewPnt2,
													  DontIgnoreBackface,
													  Flags);


					continue;
				}
			}
		}
	}

	return 0;
}


UINT32 trncdCollideWithAllTerrainObjects(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											piVECTOR* OldPnt, piVECTOR* NewPnt, UINT32 DontIgnoreBackface, UINT32 Flags)
{
trnCDTerrainObject*	TO;
UINT32				Count;
FLOAT32				mnx, mny, mnz, mxx, mxy, mxz;
FLOAT64VECTOR		WorldOriginOffset;

	if (CollisionList == nullptr)
	{
		return 1;
	}

	if (NumCollisionSlotsLeft == 0)
	{
		return 1;
	}

#ifdef TRNCD_USE_ODE
	_odeCollide(*OldPnt, *NewPnt, CollisionList, *NumCollisionSlotsLeft);
	return 0;
#endif

	trnGetWorldOriginOffset(&WorldOriginOffset);

	Count = 0;

	TO = &_TerrainObjectsHead;
	while (TO != nullptr)
	{
		TO = TO->Next;

//		TO = TOList[Count];
		Count++;

		if (TO == nullptr)
		{
			break;
		}

		if (TO->InUse == 0)
		{
			continue;
		}

		if (TO->DamageLevel == FB_CLOSED_VISLEVEL)
		{
			continue;
		}

		if (!TO->IsCollidable(Flags))
		{
			continue;
		}

		// first check the overall bounding box
		mnx = (FLOAT32)(TO->BBox1.x + TO->Center.x - TO->OrigCenter.x);
		mny = (FLOAT32)(TO->BBox1.y + TO->Center.y - TO->OrigCenter.y);
		mnz = (FLOAT32)(TO->BBox1.z + TO->Center.z - TO->OrigCenter.z);

		mxx = (FLOAT32)(TO->BBox2.x + TO->Center.x - TO->OrigCenter.x);
		mxy = (FLOAT32)(TO->BBox2.y + TO->Center.y - TO->OrigCenter.y);
		mxz = (FLOAT32)(TO->BBox2.z + TO->Center.z - TO->OrigCenter.z);

		if ((mnx <= NewPnt->x &&
			mxx >= NewPnt->x) ||
			(mnx <= OldPnt->x &&
			mxx >= OldPnt->x))
		{
			if ((mny <= NewPnt->y &&
				mxy >= NewPnt->y) ||
				(mny <= OldPnt->y &&
				mxy >= OldPnt->y))
			{
				if ((mnz <= NewPnt->z &&
					mxz >= NewPnt->z) ||
					(mnz <= OldPnt->z &&
					mxz >= OldPnt->z))
				{
					// now check the object tree itself
					// first convert collision line to object's frame of reference
//					math3dVectorMultiply(&RotatedOldPnt,OldPnt,&TO->InvMat);
//					math3dVectorMultiply(&RotatedNewPnt,NewPnt,&TO->InvMat);
					// now do the actual check
					if (TO->DamageLevel == 0)
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->CollisionObject,
												  TO->TerrainObject,
												  TO,
												  //TO->TerrainMasterObject,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}
					else
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->DamageCollisionObject,
												  TO->TerrainObject,
												  TO,
												  //TO->TerrainMasterObject,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);

					}

					continue;
				}
			}
		}

		if ((mnx <= NewPnt->x &&
			mxx >= OldPnt->x) ||
			(mnx <= OldPnt->x &&
			mxx >= NewPnt->x))
		{
			if ((mny <= NewPnt->y &&
				mxy >= OldPnt->y) ||
				(mny <= OldPnt->y &&
				mxy >= NewPnt->y))
			{
				if ((mnz <= NewPnt->z &&
					mxz >= OldPnt->z) ||
					(mnz <= OldPnt->z &&
					mxz >= NewPnt->z))
				{
					// now check the object tree itself
					// first convert collision line to object's frame of reference
//					math3dVectorMultiply(&RotatedOldPnt,OldPnt,&TO->InvMat);
//					math3dVectorMultiply(&RotatedNewPnt,NewPnt,&TO->InvMat);
					// now do the actual check
					if (TO->DamageLevel == 0)
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->CollisionObject,
												  TO->TerrainObject,
												  TO,
												  //TO->TerrainMasterObject,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}
					else
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->DamageCollisionObject,
												  TO->TerrainObject,
												  TO,
												  //TO->TerrainMasterObject,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}

					continue;
				}
			}
		}
	}

	return 0;
}

LOGICAL trncdSphereTest(trnCDTerrainObject* to, OBJECT* o)
{
    piVECTOR pos2;

    FLOAT32 radius;

    if(o->flag2 & OBJECT_ISFPS)
    {
        radius = o->maxrad;
    }
    else
    {
        radius = math3dFastSqrt(o->maxrad);
    }

    math3dVectorMultiply(&pos2, &o->pos, &o->mat);

    piVECTOR relPos =
    {
        to->Center.x - pos2.x,
        to->Center.y - pos2.y,
        to->Center.z - pos2.z
    };

    FLOAT32 dist2 = mathVectDotProduct(&relPos, &relPos);

    FLOAT32 r = to->Radius + radius;

    return dist2 < r * r;
}

// steps through the list until out of objects or a Terrain object is collided
UINT32 trncdCollideWithListOfTerrainObjects(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											trnCDTerrainObject** TOList, UINT32 MaxNumInTOList,
											piVECTOR const* OldPnt, piVECTOR const* NewPnt,
											UINT32 DontIgnoreBackface, UINT32 Flags)
{
trnCDTerrainObject*	TO;
UINT32				Count;
FLOAT32				mnx, mny, mnz, mxx, mxy, mxz;
FLOAT64VECTOR		WorldOriginOffset;


	if (CollisionList == nullptr || TOList == nullptr)
	{
		return 1;
	}

	if (NumCollisionSlotsLeft == 0 || MaxNumInTOList == 0)
	{
		return 1;
	}

    if(!g_bIgnoreTreeCollide && (!Flags || (Flags & COLLIDER_TRN_TREE)))
        sptCollide(*OldPnt, *NewPnt, CollisionList, *NumCollisionSlotsLeft);

#ifdef TRNCD_USE_ODE
	_odeCollide(*OldPnt, *NewPnt, CollisionList, *NumCollisionSlotsLeft);
#endif

	trnGetWorldOriginOffset(&WorldOriginOffset);

	Count = 0;
	while (Count < MaxNumInTOList)
	{
		TO = TOList[Count];
		Count++;

		if (TO == nullptr)
		{
			break;
		}

		if (TO->InUse == 0)
		{
			continue;
		}

		if (TO->DamageLevel == FB_CLOSED_VISLEVEL)
		{
			continue;
		}

		if (!TO->IsCollidable(Flags))
		{
			continue;
		}

		// first check the overall bounding box
		mnx = (FLOAT32)(TO->BBox1.x + TO->Center.x - TO->OrigCenter.x);
		mny = (FLOAT32)(TO->BBox1.y + TO->Center.y - TO->OrigCenter.y);
		mnz = (FLOAT32)(TO->BBox1.z + TO->Center.z - TO->OrigCenter.z);

		mxx = (FLOAT32)(TO->BBox2.x + TO->Center.x - TO->OrigCenter.x);
		mxy = (FLOAT32)(TO->BBox2.y + TO->Center.y - TO->OrigCenter.y);
		mxz = (FLOAT32)(TO->BBox2.z + TO->Center.z - TO->OrigCenter.z);

		if ((mnx <= NewPnt->x &&
			mxx >= NewPnt->x) ||
			(mnx <= OldPnt->x &&
			mxx >= OldPnt->x))
		{
			if ((mny <= NewPnt->y &&
				mxy >= NewPnt->y) ||
				(mny <= OldPnt->y &&
				mxy >= OldPnt->y))
			{
				if ((mnz <= NewPnt->z &&
					mxz >= NewPnt->z) ||
					(mnz <= OldPnt->z &&
					mxz >= OldPnt->z))
				{
					// now check the object tree itself
					// first convert collision line to object's frame of reference
//					math3dVectorMultiply(&RotatedOldPnt,OldPnt,&TO->InvMat);
//					math3dVectorMultiply(&RotatedNewPnt,NewPnt,&TO->InvMat);
					// now do the actual check
					if (TO->DamageLevel == 0)
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->CollisionObject,
												  TO->TerrainObject,
												  TO,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}
					else
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->DamageCollisionObject,
												  TO->TerrainObject,
												  TO,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);

					}

					continue;
				}
			}
		}

		if ((mnx <= NewPnt->x &&
			mxx >= OldPnt->x) ||
			(mnx <= OldPnt->x &&
			mxx >= NewPnt->x))
		{
			if ((mny <= NewPnt->y &&
				mxy >= OldPnt->y) ||
				(mny <= OldPnt->y &&
				mxy >= NewPnt->y))
			{
				if ((mnz <= NewPnt->z &&
					mxz >= OldPnt->z) ||
					(mnz <= OldPnt->z &&
					mxz >= NewPnt->z))
				{
					// now check the object tree itself
					// first convert collision line to object's frame of reference
//					math3dVectorMultiply(&RotatedOldPnt,OldPnt,&TO->InvMat);
//					math3dVectorMultiply(&RotatedNewPnt,NewPnt,&TO->InvMat);
					// now do the actual check
					if (TO->DamageLevel == 0)
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->CollisionObject,
												  TO->TerrainObject,
												  TO,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}
					else
					{
						_CollideWithTerrainObject(CollisionList,
												  NumCollisionSlotsLeft,
												  TO->DamageCollisionObject,
												  TO->TerrainObject,
												  TO,
												  OldPnt,
												  NewPnt,
												  1,
												  DontIgnoreBackface,
												  Flags);
					}

					continue;
				}
			}
		}
	}

	return 0;
}


static UINT32 _CollideWithTerrainObject(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft, OBJECT* CurrentObject, OBJECT* TopObject,
										trnCDTerrainObject* TO, piVECTOR const* OldPnt, piVECTOR const* NewPnt, UINT32 NoSiblings,
										UINT32 DontIgnoreBackface, UINT32 Flags)
{
OBJECT*		Object1, *Object2;
FLOAT32		mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR	OldPnt1, NewPnt1, OldPnt2, NewPnt2;

	if (CurrentObject == nullptr)
	{
		return 0;
	}

	Object1 = CurrentObject;
/*
	if (0)
	{
	UCHAR s[256];
	static int _count;

		sprintf((char*)s,loclGetLocalizationStringByID(loclBBOX_TOPOBJECT_MSG),TopObject->name,(_count++ % 1024));
		rnmainListText((char*)s);
	}
*/
	do
	{
		if (!(Object1->flags & OBJECT_NOTOBJECT) &&
			(Object1->flags & OBJECT_VISIBLE))
		{
			Object2 = Object1;

			do
			{
                if(Object2->flag2 & OBJECT_SPEEDTREE &&
                   !g_bIgnoreTreeCollide &&
                   !g_bCheckTreeBox &&
				    (!Flags || (Flags & COLLIDER_TRN_TREE)))
                {
					trncdCollideSpeedTree(Object2,
                                          TopObject,
                                          CollisionList,
                                          NumCollisionSlotsLeft,
                                          OldPnt,
                                          NewPnt);

                    Object2 = Object2->attached;
                    continue;
                }

				// check bounding box
				// note that the bounding boxes are relative to the center of the vertecies
				mnx = Object2->bbox[0].x;
				mny = Object2->bbox[0].y;
				mnz = Object2->bbox[0].z;

				mxx = Object2->bbox[1].x;
				mxy = Object2->bbox[1].y;
				mxz = Object2->bbox[1].z;

				// Move the points from world space into the space of the object
				math3dVectorMultiply(&OldPnt2, OldPnt,  &Object2->matint);
				math3dVectorMultiply(&NewPnt2, NewPnt,  &Object2->matint);

				mathSubVectors(&OldPnt1,&OldPnt2,&Object2->pos);
				mathSubVectors(&NewPnt1,&NewPnt2,&Object2->pos);

				if ((mnx <= NewPnt1.x &&
					mxx >= NewPnt1.x) ||
					(mnx <= OldPnt1.x &&
					mxx >= OldPnt1.x))
				{
					if ((mny <= NewPnt1.y &&
						mxy >= NewPnt1.y) ||
						(mny <= OldPnt1.y &&
						mxy >= OldPnt1.y))
					{
						if ((mnz <= NewPnt1.z &&
							mxz >= NewPnt1.z) ||
							(mnz <= OldPnt1.z &&
							mxz >= OldPnt1.z))
						{

                            if(Object2->flag2 & OBJECT_SPEEDTREE)
							{
                                if(!g_bIgnoreTreeCollide && (!Flags || (Flags & COLLIDER_TRN_TREE)))
                                {
									trncdCollideSpeedTree(Object2, TopObject, CollisionList, NumCollisionSlotsLeft, OldPnt, NewPnt);
                                }
							}else
							{
									trncdCollideWithSpecificTerrainObject(CollisionList,
																	  NumCollisionSlotsLeft,
																	  Object2,
																	  TopObject,
																	  TO,
																	  &OldPnt2,
																	  &NewPnt2,
																	  DontIgnoreBackface,
																	  Flags);
							}
							Object2 = Object2->attached;
							continue;
						}
					}
				}

				if ((mnx <= NewPnt1.x &&
					mxx >= OldPnt1.x) ||
					(mnx <= OldPnt1.x &&
					mxx >= NewPnt1.x))
				{
					if ((mny <= NewPnt1.y &&
						mxy >= OldPnt1.y) ||
						(mny <= OldPnt1.y &&
						mxy >= NewPnt1.y))
					{
						if ((mnz <= NewPnt1.z &&
							mxz >= OldPnt1.z) ||
							(mnz <= OldPnt1.z &&
							mxz >= NewPnt1.z))
						{
							if(Object2->flag2 & OBJECT_SPEEDTREE)
							{
                                if(!g_bIgnoreTreeCollide && (!Flags || (Flags & COLLIDER_TRN_TREE)))
                                {
									trncdCollideSpeedTree(Object2, TopObject, CollisionList, NumCollisionSlotsLeft, OldPnt, NewPnt);
                                }
							}else
							{
								trncdCollideWithSpecificTerrainObject(CollisionList,
															  NumCollisionSlotsLeft,
															  Object2,
															  TopObject,
															  TO,
															  &OldPnt2,
															  &NewPnt2,
															  DontIgnoreBackface,
															  Flags);
							}

							Object2 = Object2->attached;
							continue;
						}
					}
				}

				Object2 = Object2->attached;

			} while (Object2 != nullptr);
		}

		if (Object1->child != nullptr)
		{
			_CollideWithTerrainObject(CollisionList,
									  NumCollisionSlotsLeft,
									  Object1->child,
									  TopObject,
									  TO,
									  OldPnt,
									  NewPnt,
									  0,
									  DontIgnoreBackface,
									  Flags);
		}

		Object1 = Object1->sibling;

	} while (Object1 != nullptr && NoSiblings == 0);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: trncdCollideSpeedTree
// Description: Yet another collision routine.
//////////////////////////////////////////////////////////////////////
SINT32 trncdCollideSpeedTree(OBJECT* o,
                             OBJECT* top,
                             FASTCOLLIDE** collisionlist,
                             UINT32* numslotsleft,
                             piVECTOR const* p1,
                             piVECTOR const* p2)
{
    if(gCustomTerrain)
        return 0;

    piVECTOR collidept;
    FASTCOLLIDE* list = nullptr;
    bool collided = false;

    if(*numslotsleft <= 4)
    {
        return -1;
    }

    list = *collisionlist;

    FLOAT32 time;

    SINT32 tri = 0;

    if(o->flag3 & OBJECT_TREE_CLUSTER)
    {
        if(sptCollideWithCluster(o, p1, p2, &collidept, &time, &tri))
        {
            collided = true;
        }
    }
    else
    {
        if(!o->treeID)
        {
            return -1;
        }

        // Tree collision is just a cylinder collide. Sometimes rifle shots up close, or
        // any projectile with high velocity will fail the bounding box test, since the
        // start and end points may lie outside the box, even though it passes through it.
        // So just pass it to the cylinder collide for an accurate collision test.
        if(sptCollideWithTree(o->treeID, p1, p2, &collidept, &time))
        {
            collided = true;
        }
    }

    if(collided)
    {
        piVECTOR start, end;

        // Trees are not rotated...just translate to origin to put into object space
        piVECTOR pos = {o->mat._41, o->mat._42, o->mat._43};

        mathSubVectors(&start, p1, &pos);
        mathSubVectors(&end, p2, &pos);

        mathSubVectors(&start, &start, &o->pos);
        mathSubVectors(&end, &end, &o->pos);

        list->CollideObject = o;
        list->TopLevelObject = top;
        list->CollideTime = time;
        list->PolyNum = tri;
        list->CollidePnt = collidept;
        list->NewPos = end;
        list->OldPos = start;
        list->Alpha = 1.0f;

        if(!o->psnorms)
        {
            o->psnorms = (piVECTOR*)memAlloc(o->numtris * sizeof(piVECTOR));
        }

        if(o->psnorms)
        {
            piVECTOR normal;

            // Determine normal from collide point to original point
            math3dVectorMultiply(&collidept, &collidept, &o->mat);
            mathSubVectors(&normal, p1, &collidept);
            math3dNormalize(&normal);

            // Put in object space
            math3dMultiplyRotation(&normal, &normal, &o->matint);

            o->psnorms[tri].x = normal.x;
            o->psnorms[tri].y = normal.y;
            o->psnorms[tri].z = normal.z;
        }

        list++;
		(*numslotsleft)--;

        *collisionlist = list;

        // Collide
        return 0;
    }

    return -1;
}

static bool CollideUsingODE(const Vec3d& p1,
                            const Vec3d& p2,
                            FASTCOLLIDE** list,
                            unsigned int& SlotsLeft,
                            const OBJECT& o,
                            const OBJECT& TopObject,
                            const trnCDTerrainObject& to,
                            const unsigned int collidebits)
{
    if(!o.collider)
        return false;

    static colliderQuery* collider = nullptr;

    FASTCOLLIDE* collidelist = *list;

    if(!collider)
    {
        collider = new colliderQuery(0, false);
        collider->setMaxContactsPerHit(1);
        collider->setUseBoundingBox(false);

        dGeomID ray = dCreateRay(collider->space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1);
        collider->addGeom(ray, 0, 0);
    }

    collider->setGeomToCollideWith(o.collider->id(0));

    Vec3d pt1 = p1 * Matrixf(o.mat) + WorldOriginOffset;
    Vec3d pt2 = p2 * Matrixf(o.mat) + WorldOriginOffset;

    pt1 = physicsGameToPhysical(pt1);
    pt2 = physicsGameToPhysical(pt2);

    Vec3d dir = pt2 - pt1;
    double len = dir.length();

    dir.z() = -dir.z(); // <-- reversed because of our stupid coordinate system
    dir.normalize();

    colliderQuery::dGeomList& geoms = collider->geoms();

    dGeomRaySet(geoms[0], pt1.x(), pt1.y(), -pt1.z(), dir.x(), dir.y(), dir.z());
    dGeomRaySetLength(geoms[0], len);
    colliderSetBits(geoms[0], 0, collidebits);

    collider->setDirty(true);
    dGeomID current = nullptr;

    if(collider->findCollisions(true))
    {
        colliderQuery::CollisionList& collisions = collider->getCollisions();
        Vec3d p;

        for(unsigned int i = 0; i < collisions.size(); i++)
        {
            if(current == collisions[i].g1)
                continue;

            current = collisions[i].g1;

			// don't use blocks
			if(colliderIsType(collisions[i].g2, COLLIDER_BLOCK))
				continue;

            if(colliderIsType(collisions[i].g2, collidebits))
            {
				//colliderDebugContact(collisions[i], colorCYANALPHA, nullptr, .3f);

                p = physicsPhysicalToGame(collisions[i].pos);

                OBJECT* oo = (OBJECT*)dGeomGetData(collisions[i].g2);

		        collidelist->CollideObject = oo;
		        collidelist->TopLevelObject = (OBJECT*)&TopObject;
		        collidelist->TO = (trnCDTerrainObject*)&to;
		        collidelist->CollideTime = (float)(collisions[i].depth / len);
                collidelist->PolyNum = collisions[i].triindex;
                collidelist->Alpha = 1.0f;

                // Have to put the collide point back to object space for the game to handle.
                p -= WorldOriginOffset;
                p.z() = -p.z();

                p = p * Matrixf(oo->matint);

		        collidelist->CollidePnt.set((float)p.x(), (float)p.y(), (float)p.z());
                collidelist->OldPos.set((float)p1.x(), (float)p1.y(), (float)p1.z());
                collidelist->NewPos.set((float)p2.x(), (float)p2.y(), (float)p2.z());

                ++collidelist;
                SlotsLeft--;

                // Legacy crap
                if(SlotsLeft <= 4)
                    break;
            }
        }

        *list = collidelist;

        return true;
    }

    return false;
}

UINT32 trncdCollideWithSpecificTerrainObject(FASTCOLLIDE** CollisionList,
                                             UINT32* NumCollisionSlotsLeft,
											 OBJECT* CurrentObject,
                                             OBJECT* TopObject,
											 trnCDTerrainObject* TO,
											 const piVECTOR* OldPnt,
                                             const piVECTOR* NewPnt,
                                             UINT32 DontIgnoreBackface,
											 UINT32 Flags)
{
UINT32		i;
piINDICES*	CurrentTri;
piVECTOR	V1,V2,V3;
piVECTOR	Vec1,Vec2;
FLOAT32		D1,D2;
FLOAT32		Time;
piVECTOR const* pcn;
piVECTOR const* psn;
LOGICAL		HasAlpha;
FASTCOLLIDE* CollideList;

	if (!CurrentObject || *NumCollisionSlotsLeft <= 4)
	{
		return 0;
	}

	if(CurrentObject->collider && !CurrentObject->collider->isCollidable(Flags))
		return 0;

    if(CurrentObject->modelInstance && gCustomTerrain)
    {
        if(CurrentObject->modelInstance->GetModelType() == ModelTypes::TERRAIN_TILE)
        {
            CollideUsingODE(Vec3d(*OldPnt),
                            Vec3d(*NewPnt),
                            CollisionList,
                           *NumCollisionSlotsLeft,
                           *CurrentObject,
                           *TopObject,
                           *TO,
                            COLLIDER_TILE);
            return 0;
        }
    }

	CurrentTri = CurrentObject->ptris;
	pcn = CurrentObject->pscdn;
	psn = CurrentObject->psnorms;
	piVectorConstPtr objPos = CurrentObject->RenderPositions();
	piTexCoordConstPtr objUV = CurrentObject->RenderTexCoords();

    if(!psn)
    {
        return 0;
    }

	HasAlpha = piTextureHasAlpha(CurrentObject->texid);
	CollideList = *CollisionList;
	for (i=0; i < CurrentObject->numtris; i++, CurrentTri++, pcn+=3, psn++)
	{
#ifdef TERRAIN_EDITOR
		_CollisionTriCnt++;
#endif
		// first step is to determine if line passes thru the plane of the triangle, and
		// in the direction of the normal
		// if not no collision
		V1 = objPos[CurrentTri->v1];

		mathSubVectors(&Vec1,OldPnt,&V1);

		D1 = math3dDotProduct(&Vec1, psn);

		if (D1 < 0.0 && DontIgnoreBackface == 0)
		{
			continue;
		}

		mathSubVectors(&Vec2,NewPnt,&V1);
		D2 = math3dDotProduct(&Vec2, psn);

		if ((D1 > 0.001f && D2 < 0.0f) || (D2 > 0.0f && D1 < -0.001f))
		{
			// ok line intersects the plane of the triangle
			// first determine the exact point
			Time = (-D1) / (D2 - D1);

			mathSubVectors(&Vec1,NewPnt,OldPnt);
			Vec1.x = Vec1.x * Time + OldPnt->x;
			Vec1.y = Vec1.y * Time + OldPnt->y;
			Vec1.z = Vec1.z * Time + OldPnt->z;

			V2 = objPos[CurrentTri->v2];
			V3 = objPos[CurrentTri->v3];

#ifndef CD_POLY_FENCE
			// next few lines would be the way to go if we couldn't use the fence method
			// NOTE:  THIS HAS NOT BEEN TESTED OR DEBUGGED!!!!!
			mathSubVectors(&W1,V1,&Vec1);
			mathSubVectors(&W2,V2,&Vec1);
			mathSubVectors(&W3,V3,&Vec1);
			mathVectDoCrossProduct(&X1,&W1,&W2);
			mathVectDoCrossProduct(&X2,&W2,&W3);
			mathVectDoCrossProduct(&X3,&W3,&W1);
			mathAddVectors(&Vec1,&X1,&X2);
			mathAddVectors(&Vec1,&Vec1,&X3);
			Num = Vec1.x + Vec1.y + Vec1.z;
			if (fabs(Num) < 0.0001)
			{
				continue;
			}
#else
			// quick, dirty, cheap, and accurate
			mathSubVectors(&Vec2,&Vec1,&V1);
			D1 = math3dDotProduct(&pcn[0],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}

			mathSubVectors(&Vec2,&Vec1,&V2);
			D1 = math3dDotProduct(&pcn[1],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}

			mathSubVectors(&Vec2,&Vec1,&V3);
			D1 = math3dDotProduct(&pcn[2],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}
#endif
			// collision

			CollideList->CollideObject = CurrentObject;
			CollideList->TopLevelObject = TopObject;
			CollideList->TO = TO;
			CollideList->CollideTime = Time;
			CollideList->PolyNum = i;
			CollideList->CollidePnt = Vec1;

			CollideList->NewPos = *NewPnt;
			CollideList->OldPos = *OldPnt;

			// determine alpha value for transparancy
			if (HasAlpha)
			{
			FLOAT32		L1,L2;
			FLOAT32		Delta;
			piVECTOR	P;
			FLOAT32		N1,N2;
            FLOAT32     fDiff;
			FLOAT32		u1,v1,u2,v2;
			piUV	Ver1,Ver2,Ver3;
			UINT32		Alpha = 255;

				Ver1 = objUV[CurrentTri->v1];
				Ver2 = objUV[CurrentTri->v2];
				Ver3 = objUV[CurrentTri->v3];

				P = Vec1;

				mathSubVectors(&Vec1,&V3,&V1);
				N1 = -math3dDotProduct(&pcn[0],&Vec1);

                fDiff = math3dDotProduct(&Vec1, &Vec1) - N1 * N1;

                if(fDiff < 0.000001f)
                {
                    L1 = 0.0f;
                }
                else
                {
				    L1 = (FLOAT32)sqrt(fDiff);
                }

                mathSubVectors(&Vec2,&V2,&V1);
				L2 = (FLOAT32)mathExactVectorLength(&Vec2);
				Delta = L1 / L2;
				u1 = (Ver2.u - Ver1.u) * Delta + Ver1.u;
				v1 = (Ver2.v - Ver1.v) * Delta + Ver1.v;

				mathSubVectors(&Vec1,&P,&V1);
				N2 = -math3dDotProduct(&pcn[0],&Vec1);
				Delta = N2 / N1;
				u2 = (Ver3.u - u1) * Delta;
				v2 = (Ver3.v - v1) * Delta;

				math3dNormalize(&Vec2);
				mathSubVectors(&Vec1,&P,&V1);
				N1 = math3dDotProduct(&Vec1,&Vec2);
				Delta = N1 / L2;
				u1 = (Ver2.u - Ver1.u) * Delta + Ver1.u;
				v1 = (Ver2.v - Ver1.v) * Delta + Ver1.v;

				u1 += u2;
				v1 += v2;

				piGetTextureUVAlpha(&Alpha, CurrentObject->texid, 0, u1, v1);
				CollideList->Alpha = (FLOAT32)(Alpha / 255.0f);
			}
			else
			{
				CollideList->Alpha = 1.0f;
			}

			CollideList++;
			(*NumCollisionSlotsLeft)--;
			if (*NumCollisionSlotsLeft <= 4)
			{
				break;
			}
		}
	}

	*CollisionList = CollideList;

	return 0;
}


UINT32 trncdCollideLineCollisionObjectWithTerrain(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
												  trnCDTerrainObject** TOList, UINT32 MaxNumInTOList,
												  OBJECT* CollideObject, UINT32 Flags)
{
trnCDTerrainObject*	TO;
UINT32				Count;
piVECTOR			Vec1,Vec2;
FLOAT32				Radius;
piMATRIX			InvMat;


	if (CollisionList == nullptr || TOList == nullptr)
	{
		return 1;
	}

	if (NumCollisionSlotsLeft == 0 || MaxNumInTOList == 0)
	{
		return 1;
	}

	if (CollideObject == nullptr)
	{
		return 1;
	}

	Vec1 = CollideObject->bbox[0];
	Vec2 = CollideObject->bbox[1];
	mathSubVectors(&Vec1,&Vec1,&Vec2);
	Radius = (FLOAT32)mathExactVectorLength(&Vec1);
	Vec1.x = CollideObject->mat._41;
	Vec1.y = CollideObject->mat._42;
	Vec1.z = CollideObject->mat._43;

	Count = 0;
	while (Count < MaxNumInTOList)
	{
		TO = TOList[Count];
		Count++;

		if (TO == nullptr)
		{
			break;
		}

		if (TO->InUse == 0)
		{
			continue;
		}

		if (!TO->IsCollidable(Flags))
		{
			continue;
		}

		mathSubVectors(&Vec2,&Vec1,&(TO->Center));
		if (mathExactVectorLength(&Vec2) < Radius + TO->Radius)
		{
			math3dInvertMatrix(&InvMat,&CollideObject->mat);
			_CheckObjectTreeForLineCollision(CollisionList,
											 NumCollisionSlotsLeft,
											 TO->TerrainObject,
											 TO->TerrainMasterObject,
											 CollideObject,
											 &InvMat,
											 Flags);
		}
	}

	return 0;
}

static UINT32 _CheckObjectTreeForLineCollision(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
												OBJECT* Object, OBJECT* TopObject, OBJECT* LineCollideObject,
												piMATRIX* InvMat, UINT32 Flags)
{
OBJECT*		ObjectSib, *ObjectAtt;
UINT32		i;
piVECTOR	OldPnt,NewPnt;
piMATRIX	Mat1;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjectSib = Object;

	do
	{
		ObjectAtt = ObjectSib;

		if (!(ObjectAtt->flags & OBJECT_NOTOBJECT) &&
			 (ObjectAtt->flags & OBJECT_VISIBLE))
		{
			do
			{

				if (ObjectAtt->flags & OBJECT_LINECLOSED || ObjectAtt->flags & OBJECT_LINESTRIP)
				{
					if(ObjectAtt->collider && !ObjectAtt->collider->isCollidable(Flags))
						continue;

					//math3dMatrixMultiplyOptimized(&Mat1,&ObjectAtt->mat,InvMat);
					math3dMatrixMultiply(&Mat1,&ObjectAtt->mat,InvMat);
					piVectorConstPtr objPos = ObjectAtt->RenderPositions();
					for (i=0; i < ObjectAtt->numtris; i++)
					{

						OldPnt = objPos[ObjectAtt->ptris[i].v2];
						NewPnt = objPos[ObjectAtt->ptris[i].v3];

						if (OldPnt.x - NewPnt.x < 0.0001f &&
							OldPnt.x - NewPnt.x > -0.0001f &&
							OldPnt.y - NewPnt.y < 0.0001f &&
							OldPnt.y - NewPnt.y > -0.0001f &&
							OldPnt.z - NewPnt.z < 0.0001f &&
							OldPnt.z - NewPnt.z > -0.0001f)
						{
							continue;
						}

						math3dVectorMultiply(&OldPnt,&OldPnt,&Mat1);
						math3dVectorMultiply(&NewPnt,&NewPnt,&Mat1);

						// we're colliding this line segment with the line collider object
						trncdCollideWithSpecificTerrainObject(CollisionList,
														  NumCollisionSlotsLeft,
														  LineCollideObject, // line collider object
														  TopObject,
														  nullptr,
														  &OldPnt,
														  &NewPnt,
														  1,
														  Flags);


						OldPnt = objPos[ObjectAtt->ptris[i].v3];
						NewPnt = objPos[ObjectAtt->ptris[i].v1];
						math3dVectorMultiply(&OldPnt,&OldPnt,&Mat1);
						math3dVectorMultiply(&NewPnt,&NewPnt,&Mat1);

						// we're colliding this line segment with the line collider object
						trncdCollideWithSpecificTerrainObject(CollisionList,
														  NumCollisionSlotsLeft,
														  LineCollideObject, // line collider object
														  TopObject,
														  nullptr,
														  &OldPnt,
														  &NewPnt,
														  1,
														  Flags);

						OldPnt = objPos[ObjectAtt->ptris[i].v1];
						NewPnt = objPos[ObjectAtt->ptris[i].v2];
						math3dVectorMultiply(&OldPnt,&OldPnt,&Mat1);
						math3dVectorMultiply(&NewPnt,&NewPnt,&Mat1);

						// we're colliding this line segment with the line collider object
						trncdCollideWithSpecificTerrainObject(CollisionList,
														  NumCollisionSlotsLeft,
														  LineCollideObject, // line collider object
														  TopObject,
														  nullptr,
														  &OldPnt,
														  &NewPnt,
														  1,
														  Flags);
					}
				}

				ObjectAtt = ObjectAtt->attached;

			} while (ObjectAtt != nullptr);
		}

		if (ObjectSib->child != nullptr)
		{
			_CheckObjectTreeForLineCollision(CollisionList,
											 NumCollisionSlotsLeft,
											 ObjectSib->child,
											 TopObject,
											 LineCollideObject,
											 InvMat,
											 Flags);
		}

		ObjectSib = ObjectSib->sibling;

	} while (ObjectSib != nullptr);

	return 0;
}

UINT32 trncdInitSingleTerrainObject(trnCDTerrainObject* TO)
{
	if(!TO)
		return 0;

	if (TO->TerrainObject != nullptr)
	{
		if(TO->TerrainObject)
			_UpdateObjectInvMatrix(TO->TerrainObject);
	}

	return 0;
}

UINT32 trncdAdjustTerrainObjects(piVECTOR* CameraAdjustment)
{
	trnCDTerrainObject* TO;

	// next line should be called from the function that calls this function
	persAdjustExternalFixedCamera(CameraAdjustment);

		for (TO = _TerrainObjectsHead.Next; TO != nullptr; TO = TO->Next)
		{
			if (TO->TerrainObject != nullptr)
			{
				mathAddVectors(&(TO->Center),&(TO->Center),CameraAdjustment);

				OBJECT* childObj = TO->TerrainObject->child;
				assert(childObj);

                if(gCustomTerrain)
                {
                    if(childObj->flag3 & (OBJECT_CLOUDS | OBJECT_RAINCLOUDS))
                    {
                        continue;
                    }
                }

				_AdjustObjectMatrixPosition(childObj, CameraAdjustment);
		}
	}

	return 0;
}

// collides with a list of polygon/object pairs
UINT32 trncdCollideWithPolyList(piVECTOR* OldPos, piVECTOR* NewPos, piVECTOR* CollidePnt, OBJECT** CollideObject,
								UINT32* PolyNum, OBJECT** CollideObjectList, UINT32* PolyNumList, UINT32 NumPolys, UINT32 Flags)
{
piVECTOR	OldPnt1,NewPnt1;
piVECTOR*	OldPnt,*NewPnt;
piMATRIX	InvMat;
OBJECT*		CurrentObject;
UINT32		CurrentPoly;
UINT32		i;
piINDICES*	CurrentTri;
piVECTOR	V1,V2,V3;
piVECTOR	Vec1,Vec2;
FLOAT32		D1,D2;
FLOAT32		Time;
piVECTOR const*	pcn;
piVECTOR const*	psn;

	for (i=0; i < NumPolys; i++)
	{
		CurrentObject = CollideObjectList[i];
		CurrentPoly = PolyNumList[i];

		if (CurrentObject->flags & OBJECT_SKIPDRAWING)
		{
			continue;
		}

		if(CurrentObject->collider && !CurrentObject->collider->isCollidable(Flags))
			continue;

		piVectorConstPtr objPos = CurrentObject->RenderPositions();

		math3dInvertMatrix(&InvMat,&CurrentObject->mat);

		math3dVectorMultiply(&OldPnt1,OldPos,&InvMat);
		math3dVectorMultiply(&NewPnt1,NewPos,&InvMat);
		OldPnt = &OldPnt1;
		NewPnt = &NewPnt1;

		CurrentTri = &(CurrentObject->ptris[CurrentPoly]);
		pcn = &(CurrentObject->pscdn[CurrentPoly*3]);
		psn = &(CurrentObject->psnorms[CurrentPoly]);

		// first step is to determine if line passes thru the plane of the triangle, and
		// in the direction of the normal
		// if not no collision
		V1 = objPos[CurrentTri->v1];

		mathSubVectors(&Vec1,OldPnt,&V1);

		D1 = math3dDotProduct(&Vec1, psn);

		if (D1 < 0.0)
		{
			continue;
		}

		if (D1 < 0.001f && D1 > -0.001f)
		{
			continue;
		}

		mathSubVectors(&Vec2,NewPnt,&V1);
		D2 = math3dDotProduct(&Vec2, psn);

		if ((D1 > 0 && D2 < 0) || (D2 > 0 && D1 < 0))
		{
			// ok line intersects the plane of the triangle
			// first determine the exact point
			Time = (-D1) / (D2 - D1);

			mathSubVectors(&Vec1,NewPnt,OldPnt);
			Vec1.x = Vec1.x * Time + OldPnt->x;
			Vec1.y = Vec1.y * Time + OldPnt->y;
			Vec1.z = Vec1.z * Time + OldPnt->z;

			V2 = objPos[CurrentTri->v2];
			V3 = objPos[CurrentTri->v3];

#ifndef CD_POLY_FENCE
			// next few lines would be the way to go if we couldn't use the fence method
			// NOTE:  THIS HAS NOT BEEN TESTED OR DEBUGGED!!!!!
			mathSubVectors(&W1,V1,&Vec1);
			mathSubVectors(&W2,V2,&Vec1);
			mathSubVectors(&W3,V3,&Vec1);
			mathVectDoCrossProduct(&X1,&W1,&W2);
			mathVectDoCrossProduct(&X2,&W2,&W3);
			mathVectDoCrossProduct(&X3,&W3,&W1);
			mathAddVectors(&Vec1,&X1,&X2);
			mathAddVectors(&Vec1,&Vec1,&X3);
			Num = Vec1.x + Vec1.y + Vec1.z;
			if (fabs(Num) < 0.0001)
			{
				continue;
			}
#else
			// quick, dirty, cheap, and accurate
			mathSubVectors(&Vec2,&Vec1,&V1);
			D1 = math3dDotProduct(&pcn[0],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}

			mathSubVectors(&Vec2,&Vec1,&V2);
			D1 = math3dDotProduct(&pcn[1],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}

			mathSubVectors(&Vec2,&Vec1,&V3);
			D1 = math3dDotProduct(&pcn[2],&Vec2);
			if (D1 > 0.001)
			{
				continue;
			}
#endif
			// collision
			*CollidePnt = Vec1;
			*CollideObject = CurrentObject;
			*PolyNum = CurrentPoly;
			return 1;
		}
	}

	return 0;
}

UINT32 trnUpdateTerrainObjectLODs(UINT32 Override)
{
trnCDTerrainObject*	TO;
UINT32				i;
UINT32				HideLOD;
OBJECT*				Obj;
LOGICAL LODChanged;
SINT32 Count;

static UINT32	_Count;

	// only update LODs every half second or so

	// hook in here and process ode colliders that are queued for deletion
	if (ft200msticks == 0)
		trncdProcessODEQueue();

	if (Override == 0)
	{
		if (ft500msticks == 0)
		{
			//Write_Debug("Returning");
			return 0;
		}
	}

	//Write_Debug("LODing");
	_Count++;

	if (_Count >= 6)
	{
		_Count = 0;
	}

	TO = _TerrainObjectsHead.Next;

	// remember: count will increase by an average of 2 for each LOD change
	Count = 0;

	while (TO != nullptr && (LODLimitPerFrame == FALSE || Count < 1000))
	{
		for (i=0; i < TRN_MAX_LOD; i++)
		{
			if (TO->LOD[i] == nullptr)
			{
				break;
			}

			if (TO->LODDamageLevel[i] != TO->DamageLevel)
			{
				HideLOD = 1;
			}
			else
			{
				HideLOD = 0;
			}

            if(TO->TerrainMasterObject)
            {
                if(TO->TerrainMasterObject->flag3 & OBJECT_RAINCLOUDS)
                {
                    if(!fxWeatherIsCloudy())
                    {
                        // Hide rain clouds if not raining
                        HideLOD = 1;
                    }
                }
            }

            // Ticket# 5839: Reset on cp rebuild
            if(TO->DamageLevel == 0 && TO->LastFXDamageLevel == 1)
            {
                TO->LastFXDamageLevel = 0;
            }

 			if (TO->DamageLevel != 0 && TO->DamageLevel != FB_CLOSED_VISLEVEL &&
				ft1000msticks != 0 &&
				TO->ObjId.objType != BUILDING_CAPTURE &&
				TO->FirstUpdate == 0)
			{
			    FLOAT64VECTOR Pos;

				Pos.x = TO->Center.x + WorldOriginOffset.x;
				Pos.y = TO->Center.y + WorldOriginOffset.y;
				Pos.z = TO->Center.z;

				if (TO->LastFXDamageLevel == 0 && TO->DamageLevel != 0)
				{
                    if(!gCustomTerrain)
                    {
    				    TO->LastFXDamageLevel = TO->DamageLevel;

                        int id = fxSndGetNewSound();

                        if(id > 0)
                        {
                            fxSndSetSoundName(id, "sounds/destroy.wav");
                            fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
                            fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
                            fxSndSetPosition(id, Pos.x, Pos.y, -Pos.z);
                            fxSndSetPitch(id, 1.0f);
                            fxSndSetVolume(id, 1.0f);
                            fxSndPushFlags(id, DEF_SOUND_STOPS_AMBIENCE);
                            fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
                            fxSndSetMinDistance(id, 25.0f);
                            fxSndSetMaxDistance(id, 3218.0f);

                            fxSndInitSound(id);
                        }

                        TransformEffect* te = TransformEffect::AddEffect("genericbuildingexplosion", Vec3f(0, 0, -1), 0, nullptr);

                        if(te)
                        {
                            te->enableemitters(true);;
                            te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                            te->setmatrix(Matrixf::translate(Pos));
                        }
                    }
				}
			}

			Obj = TO->LOD[i];
			if (Obj == nullptr)
			{
				continue;
			}
			while (Obj->child != nullptr)
			{
				Obj = Obj->child;
			}
			if (HideLOD == 1)
			{
				Obj->minlod = 0.0f;
				Obj->maxlod = 0.0f;
			}
			else
			{
				Obj->minlod = TO->LODSavedMin[i];
				Obj->maxlod = TO->LODSavedMax[i];
			}

			obdrawProcessLODCallback(TO->LOD[i], 0, &TO->Center,TRUE,TO->Radius,&LODChanged);

            if(TO->LOD[i]->flag2 & OBJECT_AIRRAIDSIREN)
            {
                // Play siren if necessary
                trnFxPlayAirWarnSiren(TO, i);
            }

			if (LODChanged == TRUE)
			{
				Count++;
			}

		}

		TO = TO->Next;
	}

	return 0;
}

UINT32 trnSetDamageLevel(trnCDTerrainObject* TO, UINT32 DamageLevel)
{
	if (TO == nullptr)
	{
		return 0;
	}

	if (TO->TerrainObject == nullptr)
	{
		return 0;
	}

	if(DamageLevel != TO->DamageLevel && (TO->uFlags & TRN_OBJECT_ONMEDIUMLIST))
		_updateCollideLists = true;

	TO->DamageLevel = DamageLevel;

	if (TO->TerrainObject != nullptr)
	{
		if (TO->TerrainObject->child != nullptr)
		{
			_SetDamageLevel(TO->TerrainObject->child,DamageLevel);
		}
	}

	return 0;
}

static UINT32 _SetDamageLevel(OBJECT* Obj, UINT32 DamageLevel)
{
#ifdef OLD
OBJECT*	ObjSib;
UCHAR*	RetPtr;
UINT32	CurrentDamageNum;

	if (Obj == nullptr)
	{
		return 0;
	}

	ObjSib = Obj;

	do
	{

		if (obmainFindCommentName((char **)&RetPtr,ObjSib->comment,"damagelevel") == TRUE)
		{
			CurrentDamageNum = atoi((char*)RetPtr);

			if (CurrentDamageNum != DamageLevel)
			{
				obmainClearFlagsInThisTree(ObjSib,OBJECT_VISIBLE);
			}
			else
			{
				obmainSetFlagsInThisTree(ObjSib,OBJECT_VISIBLE);
			}
		}
		else
		{
			if (ObjSib->child != nullptr)
			{
				_SetDamageLevel(ObjSib->child,DamageLevel);
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);
#endif

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: trnSetBuilderFlag
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 trnSetBuilderFlag(SINT32 x, SINT32 y, UINT32 id)
{
    TERRAINID ID = {x, y, id};

    mapTOList::iterator I = mapTO.find(ID);

    if(I == mapTO.end())
        return -1;

    // trnCDTerrainObject
    I->second->uFlags |= TRN_OBJECT_BUILDER;

    return 0;
}

// Update a strat object's flags
// Called from strtClientMessages.cpp::_applySCStatus()
UINT32 trnUpdateObjectFlags(SINT32 x, SINT32 y, UINT32 id, SINT16 ownership, SINT32 flags)
{
	sdm_full_obj_id fullid;
	memset(&fullid, 0x0, sizeof(sdm_full_obj_id));
	fullid.cell.ox = x << 4;
	fullid.cell.oy = y << 4;
	fullid.id = id;

	StratDB::StratObject* stratobj = StratDB::FindStratObject(fullid);
	if(stratobj)
	{
		if ( ownership >= 0 )
			stratobj->SetCountry(ownership);
		stratobj->SetStratFlags((UINT32)flags);
	}

	return 0;
}

// update a strat level object's status
UINT32 trnUpdateObjectStatus(SINT32 x, SINT32 y, UINT32 id, SINT32 country, SINT32 vislevel)
{
	// update the object from the strat db object if -1's passed in
	if(country == -1 || vislevel == -1)
		trnGetStratData((SINT16)x, (SINT16)y, id, &country, &vislevel);

	trnCDTerrainObject*	TO;
	UINT32 Found;

	Found = 0;

	for (TO = _TerrainObjectsHead.Next; TO != nullptr; TO = TO->Next)
	{
		if (TO->StratSupercellX == x && TO->StratSupercellY == y &&
			TO->StratId == id)
		{
			Found = 1;

			if ((TO->ObjId.objClass == CLASS_CP) &&
				(TO->ObjId.objType == CP_BRIDGE))
			{
				// HACK for making bridges appear/disappear
				Write_Log (LOG_COMM, LOG_INFO, "Bridge status change .. %d %d %d -> %d (%d)", x, y, id, country, vislevel);

				// FIXME use values from strtTypes.h once it's properly shared in source control
				if (country == BRIDGE_DESTROYED_COUNTRY)
					vislevel = 1;
				else if (country == BRIDGE_BUILT_COUNTRY)
					vislevel = 0;

				country = WWII_CHINA;
			}

			if (country >= 0)
			{
				if (TO->Country >= MAX_COUNTRIES || TO->Country == WWII_NO_COUNTRY)
				{
					TO->FirstUpdate = 1;
				}
				else
				{
					TO->FirstUpdate = 0;
				}

				TO->Country = country;
				_SetTOObjectCountry(TO->TerrainObject,country,0);
			}

			if (vislevel >= 0)
			{
				// Be wary, this code already caused one bug, colliding with invisible FB's.
				// Server sends both 0xf and 0xfffff to denote this state.
				if ((vislevel & 0xf) == 0xf)
                {
                   // Write_Debug("StratID %d is tagged as FB_CLOSED_VISLEVEL (%x)", TO->StratId, vislevel);
					vislevel = FB_CLOSED_VISLEVEL;
                }

                // Separate damage level from vislevel, which also contains smoke levels.
				SINT32 newLevel = _GetDamageLevel(vislevel);

				if(newLevel != TO->DamageLevel && (TO->uFlags & TRN_OBJECT_ONMEDIUMLIST))
					_updateCollideLists = true;

				TO->DamageLevel = newLevel;

				if (! TO->LevelInited)
				{
					// set it as a past event
                    if (TO->Country > WWII_NO_COUNTRY &&
                        TO->Country < MAX_COUNTRIES)
                    {
						TO->LevelInited = TRUE;
                    }

                    TO->LastFXDamageLevel = TO->DamageLevel;
					TO->FireStartTime = frametime - (TRN_OBJ_FIRE_TIME2 + 10);
				}
				else
				{

					if (TO->DamageLevel != 0 &&
						TO->FireStartTime == 0)
					{
						TO->FireStartTime = frametime;
					}
					if (TO->DamageLevel == 0)
					{
						TO->FireStartTime = 0;
					}
				}
			}

			if (TO->TrnAutoGun != nullptr)
			{
				trnKillAutogun(&(TO->TrnAutoGun));
				TO->TrnAutoGun = nullptr;
				trnAddAutoGun(TO);

				if (TO->TrnAutoGun != nullptr)
				{
					trnUpdateAutogunStatus(TO->TrnAutoGun,
                                           country,
                                           TO->DamageLevel);
				}
			}
			else
			{
				if (TO->ObjId.objClass == CLASS_BUILDING &&
					TO->ObjId.objType == BUILDING_AUTOGUNNER)
				{
					trnAddAutoGun(TO);
					if (TO->TrnAutoGun != nullptr)
					{
						trnUpdateAutogunStatus(TO->TrnAutoGun,
                                               country,
                                               TO->DamageLevel);
					}
				}
			}

			// we found the terrain object, no reason to keep spinning right?
            // [DCF] Wrong! Need to keep spinning for multiple LODs.
			//break;
		}

	}

	if (Found == 0)
		_AddStratMsgToQue(x, y, id, country, vislevel);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: _GetDamageLevel
// Description: Damage level is 0, 1 or 255 for buildings
//////////////////////////////////////////////////////////////////////
SINT32 _GetDamageLevel(SINT32 visLevel)
{
    switch(visLevel)
    {
    case VISLEVEL_HIDDEN:
        {
            return VISLEVEL_HIDDEN;
        }
    case VISLEVEL_NORMAL:
        {
            return VISLEVEL_NORMAL;
        }
    default:
        {
            return VISLEVEL_DAMAGED;
        }
    }
}

static UINT32 _AddStratMsgToQue(SINT32 scx,SINT32 scy,UINT32 Id,SINT32 Country,SINT32 VisLevel)
{
_StratMessageQue * StratMsg;

	if (TerrainStable == TRUE)
	{
		if (Id == 0)
		{
			Write_Debug("Id number zero! scx = %d, scy = %d",scx,scy);
			return 0;
		}
		Write_Debug("Unable to find strat object scx=%d,scy=%d,id=%d, updated with country"
					"=%d,VisLevel=%d",
					scx,
					scy,
					Id,
					Country,
					VisLevel);
		return 0;
	}

	StratMsg = _GetNewStratMsg();

	if (StratMsg == nullptr)
	{
		Write_Debug("Unable to allocate new strat message que element");
		return 0;
	}

	StratMsg->scx = scx;
	StratMsg->scy = scy;
	StratMsg->Id = Id;
	StratMsg->Country = Country;
	StratMsg->VisLevel = VisLevel;

	return 0;
}

UINT32 trnSetCountry(trnCDTerrainObject * TO, UINT32 Country)
{
	if (TO == nullptr || TO->TerrainObject == nullptr)
		return 0;

	if(!(TO->TerrainObject->flag3 & OBJECT_HANDLES_COUNTRY))
	{
		// set our country flag
		_SetTOObjectCountry(TO->TerrainObject, Country, 0);
		TO->Country = Country;
		return 1;
	}

	return 0;
}

static UINT32 _SetTOObjectCountry(OBJECT * Object, SINT32 Country, UINT32 IncludeSibs)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;
UINT32 i;

	if (Country < 0 || Country >= _MAX_COUNTRIES)
	{
		return 0;
	}

	if (_CountryTexIds[Country] == 0)
	{
		return 0;
	}

	if (Object == nullptr)
	{
		return 0;
	}

    if(Object->flag2 & OBJECT_SPEEDTREE)
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
				for (i=0;i<_MAX_COUNTRIES;i++)
				{
					if (_CountryTexIds[i] == 0)
					{
						continue;
					}

					if (ObjAtt->texid == _CountryTexIds[i])
					{
						piAddTexRef(_CountryTexIds[Country]);
						piFreeMipTextures(ObjAtt->texid);
						ObjAtt->texid = _CountryTexIds[Country];

                        if(ObjAtt->material)
                        {
                            if(ObjAtt->material->IsPool())
                            {
#if USE_STATESET
                                // Cannot use pool object for this. // Ticket #4748
                                ObjAtt->material = new RenderStateSet(*ObjAtt->material);
                                ObjAtt->material->SetIsPool(false);
#else
                                // Cannot use pool object for this. // Ticket #4748
                                ObjAtt->material = new oglMaterial(*ObjAtt->material);
                                ObjAtt->material->SetIsPoolMaterial(false);
#endif
                            }

#if USE_STATESET
                            TextureUnit* tu = dynamic_cast<TextureUnit*>(ObjAtt->material->GetRenderState(RS_TEXTURE_UNIT0));
                            tu->SetTexture(ObjAtt->texid);
#else
                            ObjAtt->material->AddTexture(0, ObjAtt->texid);
#endif
                        }

						break;
					}
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_SetTOObjectCountry(ObjSib->child,Country,1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && IncludeSibs != 0);


	return 0;
}

LOGICAL trncdCollideDownShortList(COLLISION& c, FASTCOLLIDE list[32], UINT32& numcollides, UINT32 Flags)
{
    mlVehicleData*	VehicleData = persGetPersonaModelDataPtr();

    if(!VehicleData)
    {
        return FALSE;
    }

    FASTCOLLIDE*		CollideListPtr = list;
	numcollides = 32;

	trncdCollideWithListOfTerrainObjects(&CollideListPtr,
										 &numcollides,
                                         VehicleData->ShortTrnList,
										 ML_NUM_SHORT_LIST_ELEMENTS,
                                         &c.oldpos,
									  	 &c.newpos,
										 0,
										 Flags);

	numcollides = 32 - numcollides;

    if(numcollides > 0)
    {
        mlUtilitySortCollideList(list, numcollides);
    }

	return numcollides > 0;
}

LOGICAL trncdCollideDownMediumList(piVECTOR* OldPos, piVECTOR* NewPos, UINT32 Flags)
{
	mlVehicleData*	VehicleData = persGetPersonaModelDataPtr();
	UINT32			NumCollides;
	FASTCOLLIDE		CollisionList[32];
	FASTCOLLIDE*		CollideListPtr;
	COLLISION		collision;
	UINT32 i;

	if (VehicleData != nullptr)
	{
		NumCollides = 32;
		CollideListPtr = CollisionList;
		trncdCollideWithListOfTerrainObjects(&CollideListPtr,
											 &NumCollides,
											 VehicleData->MedTrnList,
											 ML_NUM_MED_LIST_ELEMENTS,
											 OldPos,
											 NewPos,
											 0,
											 Flags);

		NumCollides = 32 - NumCollides;
		if (NumCollides > 0 && NumCollides < 32)
		{
			mlUtilitySortCollideList(&CollisionList[0],NumCollides);
			memset(&collision, 0, sizeof(COLLISION));

			for (i=0;i<NumCollides;i++)
			{
				if (CollisionList[i].Alpha > 0.5f)
				{
					*NewPos = CollisionList[i].CollidePnt;

                    if(CollisionList[i].CollideObject)
					    math3dVectorMultiply(NewPos,NewPos,&CollisionList[i].CollideObject->mat);

					return (TRUE);
				}

			}
		}
	}

	if (trncdObjectToTerrain(nullptr, OldPos, NewPos))
	{
		return(TRUE);
	}

	return(FALSE);
}

UINT32 trncdGetNumTOs(void)
{
	return _NumTO;
}

UINT32 trncdUpdateStratMsgQue(void)
{
_StratMessageQue * StratMsg;
_StratMessageQue * PrevStratMsg;

	// flush que if no more terrain objects loading
	if (TerrainStable == TRUE)
	{
		for (StratMsg = _StratMsgTail;StratMsg != nullptr && StratMsg != &_StratMsgHead;)
		{
			PrevStratMsg = StratMsg->prev;

			trnUpdateObjectStatus(StratMsg->scx,
									StratMsg->scy,
									StratMsg->Id,
									StratMsg->Country,
									StratMsg->VisLevel);

			_FreeStratMsg(StratMsg);
			StratMsg = PrevStratMsg;
		}
	}

	return 0;
}

// puts collision result in offset spacein V1
// puts collision point relative to object in collisionpnt
// note: this algorithm* doesn't* take into account target vehicle velocity
//
// note also: CollisionPnt,CollisionObject,and CollisionPolyNum are all optional
// put nullptr for these values if not applicable
//
// if CollideWithAllTerrainObjects is true, *all* currently tracked TO's are checked
// this is really slow, so don't use it on a frame-by-frame basis
LOGICAL trncdCollideWithVehiclesAndTerrain(piVECTOR* V1,
										   piVECTOR* V2,
										   piVECTOR* CollisionPnt,
										   OBJECT ** CollisionObject,
										   SINT32* CollisionPolyNum,
										   void* CollisionVehicle,
										   trnCDTerrainObject** TO,
										   LOGICAL CollideWithAllTerrainObjects,
										   LOGICAL IgnoreVehicles,
										   LOGICAL CollideWithInfantry,
										   UINT32 Flags)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

    if(!VehicleData)
        return FALSE;

	piVECTOR OldPos,NewPos;
	FASTCOLLIDE CollisionList[32];
	FASTCOLLIDE * CollideListPtr;
	UINT32 TempNum;
	UINT32 NumCollides;
	piVECTOR CollidePnt;
	OBJECT * CollideObject;
	SINT32 PolyNum;
	FLOAT64VECTOR WorldOriginOffset;
	UINT32 i,j;
	UINT32 TempNum1;

	OldPos = *V1;
	NewPos = *V2;

	TempNum = 32;
	CollideListPtr = CollisionList;

	memset (CollisionList, 0, sizeof (CollisionList));

	NumCollides = 0;

	for (j=0;j<32;j++)
	{
//		CollisionList[j].Flags = 0;
		CollisionList[j].CollideTime = 10000000000000000000.0f;
	}

	trnGetWorldOriginOffset(&WorldOriginOffset);
	WorldOriginOffset.z = -WorldOriginOffset.z;

	NumCollides = 0;

	if (IgnoreVehicles == FALSE && (!Flags || Flags & COLLIDER_VEHICLE_MASK))
	{
		if (VehicleData != nullptr)
		{
			weapCollideRoundWithVehicles(&CollideListPtr,
								 &TempNum,
								 &OldPos,
								 &NewPos,
								 frametime,
								 MS_TO_S(frametime),
								 0,
								 0,
								 &WorldOriginOffset,
								 0,
								 0);

			NumCollides = 32 - TempNum;
		}

		for (j=0;j<NumCollides;j++)
		{
			CollisionList[j].Flags |= _AMMOCD_FLAG_ISAVEHICLE;
		}
	}

	if (NumCollides < 32)
	{
		if (trnopCollide(&OldPos,&NewPos,&CollidePnt,&CollideObject,&PolyNum) == TRUE)
		{
			CollisionList[NumCollides].CollideObject = CollideObject;
			CollisionList[NumCollides].PolyNum = PolyNum;
			CollisionList[NumCollides].CollideTime = 1.0f;
			CollisionList[NumCollides].TopLevelObject = CollideObject;
			CollisionList[NumCollides].CollidePnt = CollidePnt;
			CollisionList[NumCollides].Alpha = 1.0f;
			NumCollides++;
			TempNum--;
			CollideListPtr++;
		}
	}

	TempNum1 = NumCollides;

	if (NumCollides < 32)
	{
		if (CollideWithAllTerrainObjects == TRUE)
		{
			trncdCollideWithAllTerrainObjects(&CollideListPtr,
												 &TempNum,
												 &OldPos,
												 &NewPos,
												 0,
												 Flags);
		}
		else
		{
			trncdCollideWithListOfTerrainObjects(&CollideListPtr,
												 &TempNum,
												 VehicleData->MedTrnList,
												 ML_NUM_MED_LIST_ELEMENTS,
												 &OldPos,
												 &NewPos,
												 0,
												 Flags);
		}

		NumCollides = 32 - TempNum;
	}

	for (i=TempNum1;i<NumCollides;i++)
	{
		CollisionList[i].Flags |= _AMMOCD_FLAG_ISABUILDING;
	}

	NumCollides = 32 - TempNum;

	if (NumCollides == 0)
	{
		return FALSE;
	}

	mlUtilitySortCollideList(CollisionList,NumCollides);

	for (i=0;i<NumCollides;i++)
	{
		if ((CollisionList[i].Alpha > 0.5f) && (CollideWithInfantry || !(CollisionList[i].Flags & _AMMOCD_FLAG_ISPERSONA)))
		{
			*V1 = CollisionList[i].CollidePnt;
			math3dVectorMultiply(V1,V1,&CollisionList[i].CollideObject->mat);

			if (CollisionPnt != nullptr)
			{
				*CollisionPnt = CollisionList[i].CollidePnt;
			}
			if (CollisionObject != nullptr)
			{
				*CollisionObject = CollisionList[i].CollideObject;
			}
			if (CollisionPolyNum != nullptr)
			{
				*CollisionPolyNum = CollisionList[i].PolyNum;
			}
			if (CollisionVehicle != nullptr)
			{
				if (CollisionList[i].Flags & _AMMOCD_FLAG_ISAVEHICLE)
				{
					*((vehVehicle**)(CollisionVehicle)) = (vehVehicle*)(CollisionList[i].TopLevelObject);
				}
				else
				{
					*((vehVehicle**)(CollisionVehicle)) = nullptr;
				}
			}
			if (TO != nullptr)
			{
				if (CollisionList[i].Flags & _AMMOCD_FLAG_ISABUILDING)
				{
					*TO = CollisionList[i].TO;
				}
				else
				{
					*TO = nullptr;
				}
			}

			// very important return here .. makes us return the first collision
			return TRUE;
		}
	}


	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: trncdResetFXDamageFlags
// Description: This function is only used for debugging by using the .reset
// command after the .stratm pwncp command. This will reset the fx damage
// IDs so the effect can be replayed for testing.
//////////////////////////////////////////////////////////////////////
void trncdResetFXDamageFlags()
{
    trnCDTerrainObject* TO;
    SINT32 Count = 0;
    SINT32 i;

    TO = _TerrainObjectsHead.Next;

    if(TO == nullptr) return;

    while(TO != nullptr && (LODLimitPerFrame == FALSE || Count < 1000))
	{
		for(i=0; i < TRN_MAX_LOD; i++)
		{
			if(TO->LOD[i] == nullptr)
			{
				break;
			}

            if(TO->LastFXDamageLevel == 1)
            {
                TO->LastFXDamageLevel = 0;
            }

        }

        TO = TO->Next;
    }
}

#ifdef _MWERKS
#pragma mark -
#endif
// new collider stuff

/************************************************************************************************
	trncdDTLCollider
*************************************************************************************************/

typedef struct trncdDTLCollider
{
	UCHAR IsReplacementOctet[SUPERCELL_OCTETS_XY];
	OBJECT* txtobj[SUPERCELL_OCTETS_XY];
} trncdDTLCollider;

typedef struct
{
	float originX, originY; // bottom left
	float cellSizeX, cellSizeY;
	int   numcols, numrows;
} collideGrid;

static int collideWithSuperCell(TERRAIN_DETAIL* td, FASTCOLLIDE* collisionList, SINT32 maxcols,
									piVECTOR const* start, piVECTOR const* end);
static int collideWithGrid(collideGrid const* grid, int maxcols, int* cellCollideX, int* cellCollideY,
							float xstart, float ystart, float xend, float yend, LOGICAL onecellchek);
static int getCellsIntersected(float cellSizeX, float cellSizeY, int numcols, int numrows,
								int maxcol, int* cellCollideX, int* cellCollideY,
								float xstart, float ystart, float xend, float yend);
static LOGICAL lineTriCollision(piVECTOR const* start, piVECTOR const* end,  piVECTOR const* v1, piVECTOR const* v2, piVECTOR const* v3,
									piVECTOR const* pn, piVECTOR const* pcn1, piVECTOR const* pcn2, piVECTOR const* pcn3,
									piVECTOR* colpos, piVECTOR* colnorm, FLOAT32* t);

#if defined(BETA_RELEASE)
static void drawOctetDebug(TERRAIN_DETAIL*, int ox, int oy, int trinum, piVECTOR const* v1, piVECTOR const* v2, piVECTOR const* v3);
#endif

/************************************************************************************************
	trncdDTLColliderAlloc
*************************************************************************************************/

trncdDTLColliderRef trncdDTLColliderAlloc(TERRAIN_DETAIL* td)
{
	if(td == nullptr) return nullptr;
	if(td->trindx != SUPERCELL_OCTETS_XY)
	{
		char s[64];
		snprintf(s, 63, "Unexpected size: %d, cell: %d, %d", td->trindx, td->scx, td->scy);
		piExitProgram(__FILE__, __LINE__, s);
	}

	trncdDTLColliderRef ref = (trncdDTLColliderRef)memAlloc(sizeof(trncdDTLCollider));

	if(ref)
	{
		int i, nullIdx = -1;
		for(i = 0; i < MAX_TERRAIN_TEXTURES; i++)
		{
			if(td->triname[i][0] && !strcmp(td->triname[i], "null1.bmp"))
			{
				nullIdx = i;
				break;
			}
		}

		// find which tiles were replaced
		for(i = 0; i < SUPERCELL_OCTETS_XY; i++)
		{
			ref->IsReplacementOctet[i] = (td->trinfo[i].txndx == nullIdx);
			ref->txtobj[i] = td->nonreplacementtiles[td->trinfo[i].txndx]; // for the future
		}
	}

	return ref;
}

/************************************************************************************************
	trncdDTLColliderFree
*************************************************************************************************/

void trncdDTLColliderFree(trncdDTLColliderRef ref)
{
	if(ref)
		memFree(ref);
}

/************************************************************************************************
	trncdCollideWithBaseTerrain
*************************************************************************************************/
// collider for base terrain octets

SINT32 trncdCollideWithBaseTerrain(FASTCOLLIDE* collisionList, SINT32 maxcols,
									piVECTOR const* start, piVECTOR const* end)
{
#if (TERRAIN_RECTANGLES_X != 5 ||  TERRAIN_RECTANGLES_Y != 5)
#error "terrain cell layout changed!"
#endif
	int scx[8], scy[8];
	TERRAIN_DETAIL* td;
	int i, numSC, numcols;
	collideGrid trnGrid;

	logTimerBeginMacro();

	td = &trndtl[12]; // center terrain tile;
	if(td == nullptr)
		piExitProgram(__FILE__, __LINE__, "center tile is nullptr!");

	// get which supercells to test
	trnGrid.originX = (FLOAT32)((td->scx - 2) * SUPERCELL_WIDTH  - WorldOriginOffset.x); // relative to
	trnGrid.originY = (FLOAT32)((td->scy - 2) * SUPERCELL_HEIGHT - WorldOriginOffset.y); // bottom-left supercell
	trnGrid.cellSizeX = SUPERCELL_WIDTH;
	trnGrid.cellSizeY = SUPERCELL_HEIGHT;
	trnGrid.numcols = trnGrid.numrows = 5;

	numSC = collideWithGrid(&trnGrid, 8, scx, scy, start->x, start->y, end->x, end->y, TRUE);

	// collide with individual supercells
	for(numcols = 0, i = 0; i < numSC && numcols < maxcols; i++)
	{
		td = &trndtl[(4 - scy[i]) * 5 + scx[i]];
		if(td == nullptr) continue;
		if(td->collider == nullptr) td->collider = trncdDTLColliderAlloc(td);
		if(td->collider == nullptr) continue;

		numcols += collideWithSuperCell(td, &collisionList[numcols], maxcols - numcols, start, end);
	}
	logTimerEndMacro();

	return numcols;
}

/************************************************************************************************
	collideWithSuperCell
*************************************************************************************************/

static int collideWithSuperCell(TERRAIN_DETAIL* td, FASTCOLLIDE* collisionList, SINT32 maxcols,
									piVECTOR const* start, piVECTOR const* end)
{
	int ox[64], oy[64];
	int i, j, numOctets, numcols;
	collideGrid trnGrid;

	trnGrid.originX = (FLOAT32)(td->scx * SUPERCELL_WIDTH  - WorldOriginOffset.x);
	trnGrid.originY = (FLOAT32)(td->scy * SUPERCELL_HEIGHT - WorldOriginOffset.y);
	trnGrid.cellSizeX = xstep;
	trnGrid.cellSizeY = ystep;
	trnGrid.numcols = SUPERCELL_SIZE_X1;
	trnGrid.numrows = SUPERCELL_SIZE_Y1;

	// get which octets to test (actually quarter octets)
	numOctets = collideWithGrid(&trnGrid, 128, ox, oy, start->x, start->y, end->x, end->y, FALSE);

	// and finally, test the two triangles in each quarter octet
	for(numcols = 0, i = 0; i < numOctets && numcols < maxcols; i++)
	{
		int idx = (oy[i] >> 1) * SUPERCELL_OCTETS_X + (ox[i] >> 1);
		if(td->collider->IsReplacementOctet[idx]) continue;

		// collide with polys in terrain object since we've got them lying around
		// rather than using a heightfield directly
		piVectorConstPtr pos = td->terrainorig->RenderPositions();
		piINDICES* ptris = td->terrainorig->ptris;
		piVECTOR* pcn = td->terrainorig->pscdn;
		piVECTOR* psn = td->terrainorig->psnorms;
		int PolyNum = idx * NUM_TRIANGLES_PER_OCTET;

		// figure out index from octet triangle layout (basic logic from trncdCheckTerrainDown)
		int tri[2];
		if(ox[i] & 0x1) { // right
			if(oy[i] & 0x1) { // upper
				tri[0] = 3; tri[1] = 4;
			}else { // lower
				tri[0] = 5; tri[1] = 6;
			}
		}else {// left
			if(oy[i] & 0x1) {// upper
				tri[0] = 1; tri[1] = 2;
			}else { // lower
				tri[0] = 7; tri[1] = 0;
			}
		}

		// and finally collide with triangles
		FLOAT32 t;
		piVECTOR colpos, colnorm, v1, v2;

		v1.x = (FLOAT32)(start->x - td->terrainorig->mat._41);
		v1.y = (FLOAT32)(start->y - td->terrainorig->mat._42);
		v1.z = (FLOAT32)(start->z - td->terrainorig->mat._43);
		v2.x = (FLOAT32)(end->x   - td->terrainorig->mat._41);
		v2.y = (FLOAT32)(end->y   - td->terrainorig->mat._42);
		v2.z = (FLOAT32)(end->z   - td->terrainorig->mat._43);

		for(j = 0; j < 2 && numcols < maxcols; j++)
		{
			if(lineTriCollision(&v1, &v2, &pos[ptris[PolyNum+tri[j]].v1], &pos[ptris[PolyNum+tri[j]].v2], &pos[ptris[PolyNum+tri[j]].v3],
	 							 &psn[PolyNum+tri[j]], &pcn[3*(PolyNum+tri[j])+0],  &pcn[3*(PolyNum+tri[j])+1], &pcn[3*(PolyNum+tri[j])+2], &colpos, &colnorm, &t))
	 		{
#ifdef BETA_RELEASE
				if(VehicleObjectAnalysis)
					drawOctetDebug(td, ox[i], oy[i], tri[j], &pos[ptris[PolyNum+tri[j]].v1], &pos[ptris[PolyNum+tri[j]].v2], &pos[ptris[PolyNum+tri[j]].v3]);
#endif

	 			FASTCOLLIDE* collision = &collisionList[numcols];

	 			collision->TopLevelObject = nullptr;
				collision->CollideObject = td->terrainorig; // grr can't export texture information correctly will have to
				collision->PolyNum = PolyNum + tri[j];		// wait until entire collider API is redone and can use the txtobj field
				if(td->collider->txtobj[idx] && td->collider->txtobj[idx]->texname) // fairly horrible hack in the meantime,
					td->terrainorig->texname = td->collider->txtobj[idx]->texname; // mainly so IsWater checks will work
				collision->CollideTime = t;
				collision->CollidePnt = colpos;
				collision->OldPos = *start;
				collision->NewPos = *end;
				collision->Flags = 0;
				collision->Alpha = 1.0;

	 			numcols++;
	 		}
	 	}
	}

	return numcols;
}

/************************************************************************************************
	collideWithGrid
*************************************************************************************************/

static int collideWithGrid(collideGrid const* grid, int maxcol, int* cellCollideX, int* cellCollideY,
					float xstart, float ystart, float xend, float yend, LOGICAL onecellcheck)
{
	// transform relative to grid
	xstart -= grid->originX; xend -= grid->originX;
	ystart -= grid->originY; yend -= grid->originY;

	if(onecellcheck)
	{
		// optimized for the case of line entirely within one cell
		// good for supercells where line will be small compared to cell size
		int cellStartX, cellEndX;
		cellStartX = (int)(xstart / grid->cellSizeX);
		cellEndX   = (int)(xend   / grid->cellSizeX);

		if(cellStartX == cellEndX)
		{
			if(cellStartX < 0 || cellStartX >= grid->numcols) return 0;

			int cellStartY, cellEndY;

			cellStartY = (int)(ystart / grid->cellSizeY);
			cellEndY   = (int)(yend   / grid->cellSizeY);

			if(cellStartY == cellEndY)
			{
				if(cellStartY < 0 || cellStartY >= grid->numrows) return 0;

				cellCollideX[0] = cellStartX;
				cellCollideY[0] = cellStartY;
				return 1;
			}
		}
	}

	// intersects more than one cell, do the extra work
	float dx, dy;
	dx = xend - xstart;	if(dx < 0) dx = -dx;
	dy = yend - ystart;	if(dy < 0) dy = -dy;

	if(dx > dy) // reduce number of outer loops by iterating along "long" axis
	{
		return getCellsIntersected(grid->cellSizeX, grid->cellSizeY, grid->numcols, grid->numrows,
									maxcol, cellCollideX, cellCollideY, xstart, ystart, xend, yend);
	}else
	{
		return getCellsIntersected(grid->cellSizeY, grid->cellSizeX, grid->numrows, grid->numcols,
									maxcol, cellCollideY, cellCollideX, ystart, xstart, yend, xend);

	}
}

/************************************************************************************************
	getCellsIntersected
*************************************************************************************************/

static int getCellsIntersected(float cellSizeX, float cellSizeY, int numcols, int numrows,
								int maxcols, int* cellCollideX, int* cellCollideY,
								float xstart, float ystart, float xend, float yend)
{
	int i, done;
	float slope;
	float swaptmp;
	float x1, y1, x2, y2, xo, yo;
	int cellMinX, cellMaxX;
	int cellMinY, cellMaxY;
	int cellStartY, cellEndY;
	int cellX, cellY;

	// flip x so we go from left to right
	if(xend < xstart)
	{
		swaptmp = xend; xend = xstart; xstart = swaptmp;
		swaptmp = yend; yend = ystart; ystart = swaptmp;
	}

// line equation y = slope * (x - xo) + yo
	if(xend - xstart < .001f)
		slope = 0.0f;
	else
		slope = (yend - ystart) / (xend - xstart);

	xo = xstart;
	yo = ystart;

	// calc bounds
	if(slope >= 0)
	{
		if(ystart < 0)
		{
			ystart = 0;
			if(slope != 0)
				xstart = (ystart - yo) / slope + xo;
		}

		cellMinY = (int)(ystart / cellSizeY);
		cellMaxY = (int)(yend   / cellSizeY);
	}else
	{
		if(ystart > cellSizeY * numrows)
		{
			ystart = cellSizeY * numrows;
			xstart = (ystart - yo) / slope + xo;
		}

		cellMinY = (int)(yend   / cellSizeY);
		cellMaxY = (int)(ystart / cellSizeY);
	}

	cellMinX = (int)(xstart / cellSizeX);
	cellMaxX = (int)(xend   / cellSizeX);

	if (cellMinX < 0) 			cellMinX = 0;
	if (cellMinY < 0) 			cellMinY = 0;
	if (cellMaxX > numcols - 1)	cellMaxX = numcols - 1;
	if (cellMaxY > numrows - 1)	cellMaxY = numrows - 1;

	x2 = cellMinX * cellSizeX;
	y2 = slope * (x2 - xo) + yo;
	if(slope > 0)
	{
		cellStartY = 0;
		cellEndY   = (int)(y2 / cellSizeY);
	}
	else
	{
		cellStartY = (int)(y2 / cellSizeY);
		cellEndY = 0;
	}

	for(done = 0, i = 0, cellX = cellMinX; !done && cellX <= cellMaxX; cellX++)
	{
		x1 = x2;
		y1 = y2;
		x2 = (cellX + 1) * cellSizeX;
		y2 = slope * (x2 - xo)  + yo;

		if(slope > 0)
		{
			cellStartY = cellEndY;
			cellEndY   = (int)(y2 / cellSizeY);
		}else
		{
			cellEndY   = cellStartY;
			cellStartY = (int)(y2 / cellSizeY);
		}

		if(cellStartY < cellMinY)
		{
			cellStartY = cellMinY;
			if(slope < 0)
				done = 1;
		}

		if(cellEndY > cellMaxY)
		{
			cellEndY = cellMaxY;
			if(slope > 0)
				done = 1;
		}

		for(cellY = cellStartY; i < maxcols && cellY <= cellEndY; cellY++, i++)
		{
			cellCollideX[i] = cellX;
			cellCollideY[i] = cellY;
		}
	}

	return i;
}

/************************************************************************************************
	lineTriCollision
*************************************************************************************************/
// basic algorithm copied from trncdCheckTerrainDown

static LOGICAL lineTriCollision(piVECTOR const* start, piVECTOR const* end,  piVECTOR const* v1, piVECTOR const* v2, piVECTOR const* v3,
									piVECTOR const* pn, piVECTOR const* pcn1, piVECTOR const* pcn2, piVECTOR const* pcn3,
									piVECTOR* colpos, piVECTOR* colnorm, FLOAT32* t)
{
	FLOAT32 d1, d2, d3, d;
	piVECTOR dir;

	math3dSubtractVectors(&dir, end, start);
	d = math3dDotProduct(&dir, pn);

	// one sided
	if (d >= 0.0f) return FALSE;

	d1 = math3dDotProduct(v1, pn);
	d2 = math3dDotProduct(start, pn);
	d3 = math3dDotProduct(end, pn);

	// vector crosses the poly plane, do some work
	if(!(d2 >= d1 && d3 <= d1)) return FALSE;

	piVECTOR m, q, tp;

	// create a vertex where the vector crosses the poly plane
	math3dScaleVector(&m, &dir, 1.0f / -d);
	math3dScaleVector(&q, &m, d2 - d1);
	math3dAddVectors(&tp, start, &q);

	// polygon fence algorithm
	// used to determine whether the point is within the polygon bounds
	FLOAT32 da, db;

	da = math3dDotProduct(pcn1, v1);
	db = math3dDotProduct(pcn1, &tp);
	if (db > da) return FALSE;

	da = math3dDotProduct(pcn2, v2);
	db = math3dDotProduct(pcn2, &tp);
	if (db > da) return FALSE;

	da = math3dDotProduct(pcn3, v3);
	db = math3dDotProduct(pcn3, &tp);
	if (db > da) return FALSE;

	*colpos = tp;
	*colnorm = *pn;
	*t = d2 - d1;

	return TRUE;
}

/************************************************************************************************
	drawOctetDebug
*************************************************************************************************/
// let's see what the heck we actually collided with

#if defined(BETA_RELEASE)
static void drawOctetDebug(TERRAIN_DETAIL* td, int ox, int oy, int trinum, piVECTOR const* v1, piVECTOR const* v2, piVECTOR const* v3)
{
	char drawname[32];
	FLOAT64VECTOR v1_64, v2_64, v3_64;

	math32To64Vec(&v1_64, v1); mathAddVectors(&v1_64, &v1_64, &WorldOriginOffset);
	math32To64Vec(&v2_64, v2); mathAddVectors(&v2_64, &v2_64, &WorldOriginOffset);
	math32To64Vec(&v3_64, v3); mathAddVectors(&v3_64, &v3_64, &WorldOriginOffset);

	v1_64.x += td->terrainorig->mat._41; v1_64.y += td->terrainorig->mat._42; v1_64.z += td->terrainorig->mat._43;
	v2_64.x += td->terrainorig->mat._41; v2_64.y += td->terrainorig->mat._42; v2_64.z += td->terrainorig->mat._43;
	v3_64.x += td->terrainorig->mat._41; v3_64.y += td->terrainorig->mat._42; v3_64.z += td->terrainorig->mat._43;

	snprintf(drawname, 31, "oct%d,%d,%d,%d,%d-1", td->scx, td->scy, ox, oy, trinum);
	draw3dSetWorldDebugLine(&v1_64, &v2_64, drawname, colorYELLOWALPHA);

	snprintf(drawname, 31, "oct%d,%d,%d,%d,%d-2", td->scx, td->scy, ox, oy, trinum);
	draw3dSetWorldDebugLine(&v2_64, &v3_64, drawname, colorYELLOWALPHA);

	snprintf(drawname, 31, "oct%d,%d,%d,%d,%d-3", td->scx, td->scy, ox, oy, trinum);
	draw3dSetWorldDebugLine(&v3_64, &v1_64, drawname, colorYELLOWALPHA);
}
#endif

void trncdIgnoreTreeCollide(LOGICAL bIgnore)
{
    g_bIgnoreTreeCollide = bIgnore;
}

void trncdCheckTreeBox(LOGICAL bCheckTreeBox)
{
	g_bCheckTreeBox = bCheckTreeBox;
}

static inline void _recurseanddrawlines(OBJECT* o)
{
    if(o->sibling)
        _recurseanddrawlines(o->sibling);
    if(o->child)
        _recurseanddrawlines(o->child);
    if(o->attached)
        _recurseanddrawlines(o->attached);

    if(o->flags & OBJECT_NOTOBJECT)
        return;

    piVectorConstPtr p = o->RenderPositions();

    oglSetWorldMatrix(&o->mat);

    for(int ii = 0; ii < o->numtris; ii++)
    {
        draw3dLine(&p[o->ptris[ii].v1], &p[o->ptris[ii].v2], colorBLUEALPHA);
        draw3dLine(&p[o->ptris[ii].v2], &p[o->ptris[ii].v3], colorBLUEALPHA);
        draw3dLine(&p[o->ptris[ii].v3], &p[o->ptris[ii].v1], colorBLUEALPHA);
    }
}

void trncdDrawShortCollisionObjects()
{
    mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
    {
        return;
    }

    trnCDTerrainObject* to = nullptr;

#if USE_STATESET
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetMode(GL_LIGHTING, 0);
    }

    stateset->Apply();
#endif

    for(int i = 0; i < ML_NUM_SHORT_LIST_ELEMENTS; i++)
    {
        to = data->ShortTrnList[i];

        if(!to)
        {
            continue;
        }

        if(!to->InUse)
        {
            continue;
        }

        if(to->DamageLevel == FB_CLOSED_VISLEVEL)
        {
            continue;
        }

        if(to->CollisionObject)
        {
            OBJECT* o = to->CollisionObject;

#if USE_STATESET
            _recurseanddrawlines(o->child);
#else
            oglDisableTexture();
            oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

            _recurseanddrawlines(o->child);

            oglEnableTexture();
            oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
#endif
        }
    }
}

void trncdDrawShortTOBoundingBox()
{
    mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
    {
        return;
    }

    trnCDTerrainObject* to = nullptr;

    for(SINT32 i = 0; i < ML_NUM_SHORT_LIST_ELEMENTS; i++)
    {
        to = data->ShortTrnList[i];
        if(!to)
        {
            continue;
        }

        if(!to->InUse)
        {
            continue;
        }

        if(to->DamageLevel == FB_CLOSED_VISLEVEL)
        {
            continue;
        }

        piVECTOR v1;
        piVECTOR v2;

        v1.x = to->BBox1.x + to->Center.x - to->OrigCenter.x;
        v1.y = to->BBox1.y + to->Center.y - to->OrigCenter.y;
        v1.z = to->BBox1.z + to->Center.z - to->OrigCenter.z;

        v2.x = to->BBox2.x + to->Center.x - to->OrigCenter.x;
        v2.y = to->BBox2.y + to->Center.y - to->OrigCenter.y;
        v2.z = to->BBox2.z + to->Center.z - to->OrigCenter.z;

        piSetWorldMatrix(&IdentityMatrix);

        piVECTOR p1, p2;
        p1.x = v1.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v1.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v1.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorORANGEALPHA);
    }
}

// calling this function resets the check
bool trnCDUpdateCollideLists(bool set)
{
	if(set)
	{
		_updateCollideLists = true;
		return false;
	}
	else
	{
		if(_updateCollideLists)
		{
			_updateCollideLists = false;
			return true;
		}

		return false;
	}
}

/*-----------------------------------------------------------------------
Collide against terrain/vehicles with mouse click.
TODO: Rename function???
TODO: Return objects? Queue objects?
TODO: Pass in collision bits???
etc...
------------------------------------------------------------------------*/
#include "ui/uiChat.h"
bool trncdCollideWithMouse(int sx, int sy)
{
#if(0) // Debugging
    extern HWND myhwnd;

    POINT mouse;
    // Get screen mouse position
    GetCursorPos(&mouse);
    // Map relative to viewport
    ScreenToClient(myhwnd, &mouse);

    sx = mouse.x;
    sy = mouse.y;
#endif

    double x, y, z;

    oglGetScreenToWorld(sx, sy, x, y, z);

    static colliderQuery* cq = nullptr;

    if(!cq)
    {
        cq = new colliderQuery(0, false);
        cq->addGeomToCollideWith((dGeomID)colliderWorldSpace());
        cq->setMaxContactsPerHit(1);

        dGeomID ray = dCreateRay(cq->space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        cq->addGeom(ray, 0, COLLIDER_ALL);
    }

    // Backwards because of our f'd up coordinate system
    Vec3d end(x, y, -z);
    Vec3d start(playpos);
    start.x() = -start.x();
    start.y() = -start.y();

    // Now to get direction...
    Vec3d dir = end - start;
    dir.normalize();

    // World Space our start point
    start += Vec3d(WorldOriginOffset);

    // ODE Space our start point
    start = physicsGameToPhysical(start);

    colliderQuery::dGeomList& geoms = cq->geoms();

    // Set start point and direction
    dGeomRaySet(geoms[0], start.x(), start.y(), start.z(), dir.x(), dir.y(), dir.z());

    if(!cq->findCollisions(true))
    {
        return false;
    }

    colliderQuery::CollisionList& list = cq->getCollisions();

#ifdef BETA_RELEASE
	colliderQuery::Collision& hit = list.at(0);

	if(!Online && SCEN_MGR->IsActive())
	{
		Vec3d position = physicsPhysicalToGame(hit.pos);
		SCEN_MGR->AddEditPoint((float)position.x(), (float)position.y(), (float)position.z());
	}
#endif

    for(unsigned int j = 0; j < list.size(); j++)
    {
        // TODO: write contact point information or whatever
// Debugging
#if(0)
        struct colliderQuery::Collision& c = list[j];
        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);
        uiChatPrintLine("%s", o->modelInstance ? o->modelInstance->GetModelName() : o->name);
#endif
        // Can just use list[0], but keep loop for testing now...
        break;
    }

    return true;
}

/*-------------------------------------------------------------
Function: damageObject
Description:
--------------------------------------------------------------*/
static void damageObject(OBJECT* o)
{
    if(o->modelInstance)
    {
        OBJECT* oo = o->modelInstance->rootObject->child;

        while(oo)
        {
            trnCDTerrainObject* TO = trncdFindTerrainObject(oo);

            if(TO)
            {
                if(TO->DamageLevel == 0)
                    trnSetDamageLevel(TO, 1);
                else
                {
                    trnSetDamageLevel(TO, 0);

                    if(TO->LastFXDamageLevel == 1)
                    {
                        TO->LastFXDamageLevel = 0;
                    }
                }
            }

            oo = oo->sibling;
        }

        char model[128];

        PoolModel* pm = obmgrGetPoolModel(o->modelInstance->GetModelName());

        if(pm)
        {
            for(unsigned int i = 0; i < pm->subModelReferences.size(); i++)
            {
                sprintf(model, "%s_%d", pm->subModelReferences[i].poolModel->modelname.c_str(), i + 1);

                OBJECT* oo = obmainFindTopmostObject(model);

                if(!oo)
                    continue;

                oo = oo->child;

                while(oo)
                {
                    trnCDTerrainObject* TO = trncdFindTerrainObject(oo);

                    if(TO)
                    {
                        if(TO->DamageLevel == 0)
                            trnSetDamageLevel(TO, 1);
                        else
                        {
                            trnSetDamageLevel(TO, 0);

                            if(TO->LastFXDamageLevel == 1)
                            {
                                TO->LastFXDamageLevel = 0;
                            }
                        }
                   }

                    oo = oo->sibling;
                }
            }
        }
    }
}

/*-------------------------------------------------------------
Function: trncdToggleDamageWithMouseClick
Description: Tool for production use to toggle damage states
based on mouse click. Click for damage. Click again for no damage.
--------------------------------------------------------------*/
bool trncdToggleDamageWithMouseClick(int sx, int sy)
{
    if(!EnableMouseClickToDamageBuildings)
        return false;

#if(0) // Debugging
    extern HWND myhwnd;

    POINT mouse;
    // Get screen mouse position
    GetCursorPos(&mouse);
    // Map relative to viewport
    ScreenToClient(myhwnd, &mouse);

    sx = mouse.x;
    sy = mouse.y;
#endif

    double x, y, z;

    oglGetScreenToWorld(sx, sy, x, y, z);

    static colliderQuery* cq = nullptr;

    if(!cq)
    {
        cq = new colliderQuery(0, false);
        cq->addGeomToCollideWith((dGeomID)colliderWorldSpace());
        cq->setMaxContactsPerHit(1);

        dGeomID ray = dCreateRay(cq->space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        cq->addGeom(ray, 0, COLLIDER_ALL);
    }

    // Backwards because of our f'd up coordinate system
    Vec3d end(x, y, -z);
    Vec3d start(playpos);
    start.x() = -start.x();
    start.y() = -start.y();

    // Now to get direction...
    Vec3d dir = end - start;
    dir.normalize();

    // World Space our start point
    start += Vec3d(WorldOriginOffset);

    // ODE Space our start point
    start = physicsGameToPhysical(start);

    colliderQuery::dGeomList& geoms = cq->geoms();

    // Set start point and direction
    dGeomRaySet(geoms[0], start.x(), start.y(), start.z(), dir.x(), dir.y(), dir.z());

    if(!cq->findCollisions(true))
    {
        return false;
    }

    colliderQuery::CollisionList& list = cq->getCollisions();

    for(unsigned int j = 0; j < list.size(); j++)
    {
        struct colliderQuery::Collision& c = list[j];

        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

        if(o->modelInstance)
        {
            if(o->modelInstance->GetModelType() == ModelTypes::BUILDING)
            {
                damageObject(o);
            }
        }

        // Can just use list[0], but keep loop for testing now...
        break;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
//	BoundBox::CreateBounds
//////////////////////////////////////////////////////////////////////
void BoundBox::CreateBounds(const OBJECT* o, piVECTOR& min, piVECTOR& max, LOGICAL singleObject)
{
	if(o->numverts)
	{
		piVectorConstPtr	crtVertex	= o->ImmutablePositions();

		for(int i=0; i < o->numverts; i++, ++crtVertex)
		{
			if(crtVertex->x < min.x) min.x = crtVertex->x;
			if(crtVertex->y < min.y) min.y = crtVertex->y;
			if(crtVertex->z < min.z) min.z = crtVertex->z;

			if(crtVertex->x > max.x) max.x = crtVertex->x;
			if(crtVertex->y > max.y) max.y = crtVertex->y;
			if(crtVertex->z > max.z) max.z = crtVertex->z;
		}
	}

	if(!singleObject)
	{
		if(o->child)	CreateBounds(o->child, min, max, singleObject);
		if(o->attached)	CreateBounds(o->attached, min, max, singleObject);
		if(o->sibling)	CreateBounds(o->sibling, min, max, singleObject);
	}
}

//////////////////////////////////////////////////////////////////////
//	BoundBox::BoundBox
//////////////////////////////////////////////////////////////////////
BoundBox::BoundBox(const OBJECT* o, LOGICAL singleObject)
{
	minBounds.x = minBounds.y = minBounds.z = FLT_MAX;
	maxBounds.x = maxBounds.y = maxBounds.z = FLT_MIN;


	CreateBounds(o, minBounds, maxBounds, singleObject);
	Update();
}

/*
//////////////////////////////////////////////////////////////////////
//	BoundBox::BoundBox
//////////////////////////////////////////////////////////////////////
BoundBox::BoundBox(int hfile)
{
	fioRead(hfile, &minBounds, sizeof(piVECTOR));
	fioRead(hfile, &maxBounds, sizeof(piVECTOR));
	Update();
}
*/
/*
//////////////////////////////////////////////////////////////////////
//	BoundBox::SaveToFile
//////////////////////////////////////////////////////////////////////
void BoundBox::SaveToFile(int hfile)
{
	fioWrite(hfile, &minBounds, sizeof(piVECTOR));
	fioWrite(hfile, &maxBounds, sizeof(piVECTOR));
}
*/

//////////////////////////////////////////////////////////////////////
//	BoundBox::Update
//////////////////////////////////////////////////////////////////////
void BoundBox::Update()
{
	corners[0].x = minBounds.x;	corners[0].y = minBounds.y;	corners[0].z = maxBounds.z;
	corners[1].x = maxBounds.x;	corners[1].y = minBounds.y;	corners[1].z = maxBounds.z;
	corners[2].x = maxBounds.x;	corners[2].y = maxBounds.y;	corners[2].z = maxBounds.z;
	corners[3].x = minBounds.x;	corners[3].y = maxBounds.y;	corners[3].z = maxBounds.z;
	corners[4].x = minBounds.x;	corners[4].y = minBounds.y;	corners[4].z = minBounds.z;
	corners[5].x = maxBounds.x;	corners[5].y = minBounds.y;	corners[5].z = minBounds.z;
	corners[6].x = maxBounds.x;	corners[6].y = maxBounds.y;	corners[6].z = minBounds.z;
	corners[7].x = minBounds.x;	corners[7].y = maxBounds.y;	corners[7].z = minBounds.z;

	math3dSubtractVectors(&sideNormals[0], &corners[1], &corners[4]);
	math3dSubtractVectors(&sideNormals[1], &corners[4], &corners[1]);
	math3dSubtractVectors(&sideNormals[2], &corners[2], &corners[1]);
	math3dSubtractVectors(&sideNormals[3], &corners[1], &corners[2]);
	math3dSubtractVectors(&sideNormals[4], &corners[4], &corners[5]);
	math3dSubtractVectors(&sideNormals[5], &corners[5], &corners[4]);
}

#ifdef TRNCD_USE_ODE
bool _odeCollide(const piVECTOR& pt1, const piVECTOR& pt2, piVECTOR& collidept, float& time, int& tri)
{
	if(!_trnCollider)
	{
		_trnCollider = new colliderQuery(0, false);
		_trnCollider->addGeomToCollideWith((dGeomID)colliderTerrainSpace());
		_trnCollider->setMaxContactsPerHit(1);

		dGeomID ray = dCreateRay(_trnCollider->space(), 2000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        _trnCollider->addGeom(ray, 0, COLLIDER_TERRAIN_MASK);
	}

    dGeomID ray = _trnCollider->geom(0);

    Vec3d p1(pt1);
    Vec3d p2(pt2);

    p1 += WorldOriginOffset;
    p2 += WorldOriginOffset;

    p1 = physicsGameToPhysical(p1);
    p2 = physicsGameToPhysical(p2);

    Vec3d dir2 = p2 - p1;
    dir2.normalize();

    dGeomRaySet(ray, p1.x(), p1.y(), p1.z(), dir2.x(), dir2.y(), dir2.z());

    _trnCollider->setDirty(true);

    if(_trnCollider->findCollisions(true))
    {
        colliderQuery::CollisionList& collisions = _trnCollider->getCollisions();
        Vec3d p = physicsPhysicalToGame(collisions[0].pos);
        collidept.set(p.x(), p.y(), p.z());

        return true;
    }

    return false;
}

bool _odeCollide(const piVECTOR& pt1, const piVECTOR& pt2, FASTCOLLIDE** list, unsigned int& NumSlotsLeft)
{
    FASTCOLLIDE* collidelist = *list;

	if(!_trnCollider)
	{
		_trnCollider = new colliderQuery(0, false);
		_trnCollider->addGeomToCollideWith((dGeomID)colliderTerrainSpace());
		_trnCollider->setMaxContactsPerHit(1);

		dGeomID ray = dCreateRay(_trnCollider->space(), 2000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        _trnCollider->addGeom(ray, 0, COLLIDER_TERRAIN_MASK);
	}

    dGeomID ray = _trnCollider->geom(0);

    Vec3d p1(pt1);
    Vec3d p2(pt2);

    p1 += WorldOriginOffset;
    p2 += WorldOriginOffset;

    p1 = physicsGameToPhysical(p1);
    p2 = physicsGameToPhysical(p2);

    Vec3d dir = p2 - p1;
    dGeomRaySetLength(ray, dir.length());

    dir.normalize();

    dGeomRaySet(ray, p1.x(), p1.y(), p1.z(), dir.x(), dir.y(), dir.z());

    _trnCollider->setDirty(true);

    if(_trnCollider->findCollisions(true))
    {
        colliderQuery::CollisionList& collisions = _trnCollider->getCollisions();

        collidelist->CollideObject = colliderData<OBJECT>(collisions[0].g2);
        collidelist->CollideTime = collisions[0].depth;
        collidelist->Alpha = 1.0f;
        //collidelist->Flags = ODE_TREE_COLLISION;

        Vec3d p = physicsPhysicalToGame(collisions[0].pos);

        collidelist->CollidePnt.set(p.x(), p.y(), p.z());

        collidelist++;
        NumSlotsLeft--;

        *list = collidelist;

        return true;
    }

    return false;
}
#endif


/*
//////////////////////////////////////////////////////////////////////
//	BoundBox::Transform
//////////////////////////////////////////////////////////////////////
void BoundBox::Transform(const piMATRIX* m)
{
	math3dVectorMultiply(&minBounds, &minBounds, m);
	math3dVectorMultiply(&maxBounds, &maxBounds, m);
	Update();
}


//////////////////////////////////////////////////////////////////////
//	BoundBox::Translate
//////////////////////////////////////////////////////////////////////
void BoundBox::Translate(const piVECTOR* v)
{
	math3dAddVectors(&minBounds, &minBounds, v);
	math3dAddVectors(&maxBounds, &maxBounds, v);
	Update();
}
*/

#ifdef	_DEBUG
//////////////////////////////////////////////////////////////////////
//	BoundBox::Draw
//////////////////////////////////////////////////////////////////////
void BoundBox::Draw()	const
{
		//	corners[0] : left,	back,	bottom
		//	corners[1] : right, back,	bottom
		//	corners[2] : right,	front,	bottom
		//	corners[3] : left,	front,	bottom
		//	corners[4] : left,	back,	top
		//	corners[5] : right,	back,	top
		//	corners[6] : right,	front,	top
		//	corners[7] : left,	front,	top
	draw3dLine(&corners[0], &corners[1], colorBLUEALPHA);
	draw3dLine(&corners[1], &corners[2], colorBLUEALPHA);
	draw3dLine(&corners[2], &corners[3], colorBLUEALPHA);
	draw3dLine(&corners[3], &corners[0], colorBLUEALPHA);

	draw3dLine(&corners[4], &corners[5], colorBLUEALPHA);
	draw3dLine(&corners[5], &corners[6], colorBLUEALPHA);
	draw3dLine(&corners[6], &corners[7], colorBLUEALPHA);
	draw3dLine(&corners[7], &corners[4], colorBLUEALPHA);

	draw3dLine(&corners[0], &corners[4], colorBLUEALPHA);
	draw3dLine(&corners[1], &corners[5], colorBLUEALPHA);
	draw3dLine(&corners[2], &corners[6], colorBLUEALPHA);
	draw3dLine(&corners[3], &corners[7], colorBLUEALPHA);
}
#endif

    /*
#else

#define PRINT_TRACE printf("%s\n",__PRETTY_FUNCTION__)

BoundBox::BoundBox(const OBJECT* o, LOGICAL singleObject)
{
	PRINT_TRACE;
}

const char* trnCDTerrainObject::ObjName()
{
	PRINT_TRACE;
	return 0;
}

StratDB::StratObject* trnCDTerrainObject::StratObj()
{
	PRINT_TRACE;
	return 0;
}

bool trnCDUpdateCollideLists(bool set)
{
	PRINT_TRACE;
	return 0;
}

void trnFindCollisionObject(OBJECT ** const CollisionObject, OBJECT* Object)
{
	PRINT_TRACE;
}

UINT32 trnGetSpawnObject(OBJECT** o,
                         SINT32 SuperCellX,
                         SINT32 SuperCellY,
                         UINT32 Id,
                         UINT32 SpawnNum,
                         WEAPON_SPEC& spec)
{
	PRINT_TRACE;
	return 0;
}

SINT32 trnSetBuilderFlag(SINT32 x, SINT32 y, UINT32 id)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trnSetDamageLevel(trnCDTerrainObject* TO, UINT32 DamageLevel)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trnUpdateObjectFlags(SINT32 x, SINT32 y, UINT32 id, SINT16 ownership, SINT32 flags)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trnUpdateObjectStatus(SINT32 x, SINT32 y, UINT32 id, SINT32 country, SINT32 vislevel)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trnUpdateTerrainObjectLODs(UINT32 Override)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdAddTerrainObject(OBJECT* Object,SINT32 SuperCellX, SINT32 SuperCellY, UINT32 Id, REL_OBJ_ID * ObjId)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdRemoveTerrainObject(OBJECT* Object, SINT32 DummyNum1, SINT32 DummyNum2, UINT32 Id, REL_OBJ_ID * ObjId)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdAdjustTerrainObjects(piVECTOR* CameraAdjustment)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdCheckTerrain(COLLISION* collision)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdCheckTerrainDown(COLLISION* collision)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdCheckTerrainDownWithDTL(COLLISION* collision, TERRAIN_DETAIL* lpDTL)
{
	PRINT_TRACE;
	return 0;
}

void trncdCheckTreeBox(LOGICAL bCheckTreeBox)
{
	PRINT_TRACE;
}

LOGICAL trncdCollideDownMediumList(piVECTOR* OldPos, piVECTOR* NewPos)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdCollideDownShortList(COLLISION& c, FASTCOLLIDE list[32], UINT32& numcollides)
{
	PRINT_TRACE;
	return 0;
}

SINT32 trncdCollideSpeedTree(OBJECT* o,
                             OBJECT* top,
                             FASTCOLLIDE** collisionlist,
                             UINT32* numslotsleft,
                             piVECTOR const* p1,
                             piVECTOR const* p2)
{
	PRINT_TRACE;
	return 0;
}

SINT32 trncdCollideWithBaseTerrain(FASTCOLLIDE* collisionList, SINT32 maxcols, piVECTOR const* start, piVECTOR const* end)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdCollideWithListOfTerrainObjects(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											trnCDTerrainObject** TOList, UINT32 MaxNumInTOList,
											piVECTOR const* OldPnt, piVECTOR const* NewPnt, UINT32 DontIgnoreBackface)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdCollideWithListOfTerrainObjects1(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
                                             OBJECT** CollideObjects, trnCDTerrainObject** TOList,
                                             UINT32 MaxNumInTOList, piVECTOR* OldPnt, piVECTOR* NewPnt,
                                             UINT32 DontIgnoreBackface)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdCollideWithPolyList(piVECTOR* OldPos, piVECTOR* NewPos, piVECTOR* CollidePnt, OBJECT** CollideObject,
								UINT32* PolyNum, OBJECT** CollideObjectList, UINT32* PolyNumList, UINT32 NumPolys)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdCollideWithSpecificTerrainObject(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											 OBJECT* CurrentObject, OBJECT* TopObject,
											 trnCDTerrainObject* TO,
											 const piVECTOR* OldPnt, const piVECTOR* NewPnt, UINT32 DontIgnoreBackface)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdCollideWithVehiclesAndTerrain(piVECTOR* V1,
										   piVECTOR* V2,
										   piVECTOR* CollisionPnt,
										   OBJECT ** CollisionObject,
										   SINT32* CollisionPolyNum,
										   void* CollisionVehicle,
										   trnCDTerrainObject** TO,
										   LOGICAL CollideWithAllTerrainObjects,
										   LOGICAL IgnoreVehicles,
										   LOGICAL CollideWithInfantry)
{
	PRINT_TRACE;
	return 0;
}

void trncdDTLColliderFree(trncdDTLColliderRef ref)
{
	PRINT_TRACE;
}

void trncdDrawShortCollisionObjects()
{
	PRINT_TRACE;
}

void trncdDrawShortTOBoundingBox()
{
	PRINT_TRACE;
}

trnCDTerrainObject * trncdFindTerrainObject(OBJECT*O)
{
	PRINT_TRACE;
	return 0;
}

LOGICAL trncdGetListOfTerrainPolysNearBoundingBox(piVECTOR* BBox, OBJECT** CollideObj, UINT32* CollidePoly, UINT32* NumCollisionSlotsLeft)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdGetTOHead(trnCDTerrainObject** TOHead)
{
	PRINT_TRACE;
	return 0;
}

void trncdIgnoreTreeCollide(LOGICAL bIgnore)
{
	PRINT_TRACE;
}

UINT32 trncdInit()
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdInitSingleTerrainObject(trnCDTerrainObject* TO)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdKill()
{
	PRINT_TRACE;
	return 0;
}

void trncdProcessODEQueue(bool all)
{
	PRINT_TRACE;
}

void trncdRemoveODEColliderImmediate(trnCDTerrainObject& TO, bool release)
{
	PRINT_TRACE;
}

void trncdRemoveODEColliderQueued(trnCDTerrainObject& TO)
{
	PRINT_TRACE;
}

SINT32 trncdReplacementTileTextureID()
{
	PRINT_TRACE;
	return 0;
}

void trncdResetFXDamageFlags()
{
	PRINT_TRACE;
}

bool trncdSpawnDataExists(const char* buildingname)
{
	PRINT_TRACE;
	return 0;
}

UINT32 trncdUpdateStratMsgQue()
{
	PRINT_TRACE;
	return 0;
}

#endif
*/
