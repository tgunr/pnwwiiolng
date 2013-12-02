#include <syscfg.h> // Path is project specific.
//

#include "pi/pi.h"
#include <math.h>
#include "mem/mem.h"
#include "include/vars.h"
#include "list/list.h"
#include "ob/obmain.h"
#include "ob/obcd.h"
#include "ob/obcs.h"
#include "ob/obdraw.h"
#include "ob/obdlite.h"
#include "trn/trnmain.h"
#include "draw/draw3d.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "gr2/gr2Anim.h"
#include "veh/vehmain.h"
#include "ob/obcs.h"
#include "math/matheul.h"
#include "rn/rnvis.h"
#include "trn/trnwtr.h"
#include "rn/rnmain.h"
#include "ogl/oglGlobalStates.h"
#include "trn/trncd.h"
#include "trn/trnvars.h"
#include "ob/obMgr.h"
#include "shader/glslWater.h"
#include "osgImport/ShadowMap.h"
#include "scen/ScenarioManager.h"
#include "sdm/sdmDefines.h"

#include <vector>
#include <algorithm>

#if USE_STATESET
#include "ogl/oglShadow.h"
#endif

static Covert<LOGICAL> _doCull(TRUE);

typedef std::vector<SORTHEADER*> SortVector;

static void*         sortedlisttail;			// the end of the sorted list
static void*			unsortedlisttail;			// the end of the unsorted list
static void*			sortedcockpitlisttail;		// the end of the cockpit list
static void*			unsortedcockpitlisttail;	// the end of the unsorted cockpit list
static void*         tailMultiTexture;

static LISTHEADER	sortedlist;					// for transparancies, which are drawn 2nd
static LISTHEADER   unsortedlist;				// for non-transparancies and unfiltered transparancies, drawn 1st
static LISTHEADER	sortedcockpitlist;			// for the cockpit
static LISTHEADER	unsortedcockpitlist;		// for the cockpit, unsorted elements
static LISTHEADER   multiTextureList;

static LISTHEADER   sortfree;

static SINT32 _CullObjectCnt;
static SINT32 _DisplayedObjectCnt;

static SORTHEADER* lplast;
static SORTHEADER* lplastunsorted;
static SORTHEADER* lplastcockpit;
static SORTHEADER* lplastunsortedcockpit;
static SORTHEADER* lplastMultiTexture;

static LOGICAL UnsortBSP(SORTHEADER* lpsort);
#if(0)
static LOGICAL SortBSP(SORTHEADER* lpsort);
static LOGICAL SortCockpitBSP(SORTHEADER* lpsort);
static LOGICAL UnsortCockpitBSP(SORTHEADER* lpsort);
#endif
static LOGICAL unsortMultiTexture(SORTHEADER* s);

static std::vector<OBJECT*> _transparentbin;
static std::vector<OBJECT*> _transparentcockpitbin;
static std::vector<OBJECT*> _opaquebin;
static std::vector<OBJECT*> _opaquecockpitbin;
static std::vector<OBJECT*> _shadowbin;
static std::vector<OBJECT*> _multitexturebin;

static int _backToFront(OBJECT* left, OBJECT* right);
static int sortByMaterial(OBJECT* l, OBJECT* r);
static int _backReflectionToFront(OBJECT* left, OBJECT* right);

#include "osgImport/Polytope.h"

struct ShadowCull
{
    std::vector<OBJECT*> ShadowTransparentBin;
    std::vector<OBJECT*> ShadowOpaqueBin;
    std::vector<OBJECT*> ShadowMultiTextureBin;

    void Reset()
    {
        ShadowTransparentBin.clear();
        ShadowOpaqueBin.clear();
        ShadowMultiTextureBin.clear();

        if(ShadowMap::GetInstance().Enabled())
        {
            ShadowMap::GetInstance().setupCull();
        }
    }

    void Cull(OBJECT* o)
    {
        static piVECTOR pos;
        static BoundingSphere bs;

        if(o->modelInstance)
        {
            if(o->modelInstance->GetModelType() == ModelTypes::TERRAIN_TILE)
            {
                return;
            }
        }

        math3dVectorMultiply(&pos, &o->pos, &o->mat);
        bs.set(pos, math3dFastSqrt(o->maxrad));

        if(!ShadowMap::GetInstance().cull(bs))
        {
            o->shadowdist = math3dDotProduct(&pos, &playdir) + playtrans.z;

            if(o->distsqr < SDM_OCTET_SIZE || o->flag3 & (OBJECT_CLOUDS | OBJECT_RAINCLOUDS))
            {
                if(!(o->flag2 & OBJECT_HASALPHA) && obdrawGetMultiTextureOpcode(o->texid, NULL) == MT_SINGLEPASS)
                {
                    ShadowMultiTextureBin.push_back(o);
                }
                else if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
                {
                    ShadowTransparentBin.push_back(o);
                }
                else
                {
                    ShadowOpaqueBin.push_back(o);
                }
            }
        }
    }

    void Sort()
    {
        std::sort(ShadowOpaqueBin.begin(), ShadowOpaqueBin.end(), sortByMaterial);
    }
};

struct WaterReflectionCull
{
    std::vector<OBJECT*> ReflectionTransparentBin;
    std::vector<OBJECT*> ReflectionOpaqueBin;
    std::vector<OBJECT*> ReflectionMultiTextureBin;

    Polytope polytope;
    colliderQuery collider;
    bool firsttime;

    WaterReflectionCull() :
        firsttime(true)
    {
    }

    void Init()
    {
        if(firsttime)
        {
            firsttime = false;

            collider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
            dGeomID ray = dCreateRay(collider.space(), 10000.0f);
            dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
            collider.addGeom(ray, 0, COLLIDER_TRN_WATER);
        }
    }

    bool GetWaterHeight(float& height)
    {
        Init();

        colliderQuery::CollisionList collisions;

        Vec3d pos(-playpos.x, -playpos.y, playpos.z);
        pos += WorldOriginOffset;

        pos = physicsGameToPhysical(pos);
        pos.z() += 0.1f;

        colliderQuery::dGeomList& geoms = collider.geoms();
        dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, -1);

        collider.setDirty(true);

        if(collider.findCollisions(true))
        {
            collisions = collider.getCollisions();

            struct colliderQuery::Collision& c = collisions[0];

            c.pos = physicsPhysicalToGame(c.pos);

            height = (float)-c.pos.z();
            return true;
        }

        return false;
    }

    void Reset()
    {
        static float zf = 100.0f;

        ReflectionTransparentBin.clear();
        ReflectionOpaqueBin.clear();
        ReflectionMultiTextureBin.clear();

        if(g_wantWaterReflections && glslWaterCanReflect())
        {
            polytope.setToUnitFrustum();

            float planeheight = 0.0f;

            if(!GetWaterHeight(planeheight))
                planeheight = glslWaterGetClosestWaterHeight();

            oglSetWaterReflectionCamera(planeheight);

            float zn = 1.0f;

            Matrixf proj;

	        proj(0, 0) = 1.0f / tan(FieldOfView / 2.0f);
	        proj(1, 1) = 1.0f / tan(FieldOfView / 2.0f);
	        proj(2, 2) = (zf + zn) / (zf - zn);
	        proj(2, 3) = 1.0f;
	        proj(3, 2) = -2 * zf * zn / (zf - zn);
	        proj(3, 3) = 0.0f;

            const Matrixf& mv = oglGetWaterReflectionCamera();

            Matrixf mvp = mv * proj;
            polytope.transformProvidingInverse(mvp);
        }
    }

    void Cull(OBJECT* o)
    {
        if(g_wantWaterReflections && glslWaterCanReflect())
        {
            if(!(o->flag2 & (OBJECT_ISCOCKPIT | OBJECT_ISFPS)) &&
               !(o->flag3 & (OBJECT_COAST_WATER | OBJECT_RIVER_WATER)) &&
               !piIsMaterialInterior(o->matid))
            {
                static piVECTOR pos;
                static BoundingSphere bs;

                math3dVectorMultiply(&pos, &o->pos, &o->mat);
                bs.set(pos, math3dFastSqrt(o->maxrad));

                if(polytope.contains(bs))
                {
                    o->reflectiondist = math3dDotProduct(&pos, &playdir) + playtrans.z;

                    if(o->reflectiondist < SDM_OCTET_SIZE || o->flag3 & (OBJECT_CLOUDS | OBJECT_RAINCLOUDS))
                    {
                        if(!(o->flag2 & OBJECT_HASALPHA) && obdrawGetMultiTextureOpcode(o->texid, NULL) == MT_SINGLEPASS)
                        {
                            ReflectionMultiTextureBin.push_back(o);
                        }
                        else if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
                        {
                            ReflectionTransparentBin.push_back(o);
                        }
                        else
                        {
                            ReflectionOpaqueBin.push_back(o);
                        }
                    }
                }
            }
        }
    }

    void Sort()
    {
        if(g_wantWaterReflections && glslWaterCanReflect())
        {
            std::sort(ReflectionTransparentBin.begin(), ReflectionTransparentBin.end(), _backReflectionToFront);
            std::sort(ReflectionOpaqueBin.begin(), ReflectionOpaqueBin.end(), sortByMaterial);
            std::sort(ReflectionMultiTextureBin.begin(), ReflectionMultiTextureBin.end(), sortByMaterial);
        }
    }
};

static struct WaterReflectionCull waterreflection;
static struct ShadowCull shadowcull;

void obcsGetOpaqueBin(std::vector<OBJECT*>& bin)
{
    if(obdrawIsRenderPass(RENDER_REFLECTION))
        bin = waterreflection.ReflectionOpaqueBin;
    else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
        bin = shadowcull.ShadowOpaqueBin;
    else
        bin = _opaquebin;
}

void obcsGetOpaqueCockpitBin(std::vector<OBJECT*>& bin)
{
    bin = _opaquecockpitbin;
}

void obcsGetTransparentBin(std::vector<OBJECT*>& bin)
{
    if(obdrawIsRenderPass(RENDER_REFLECTION))
        bin = waterreflection.ReflectionTransparentBin;
    else
        bin = _transparentbin;
}

void obcsGetTransparentCockpitBin(std::vector<OBJECT*>& bin)
{
    bin = _transparentcockpitbin;
}

void obcsGetShadowBin(std::vector<OBJECT*>& bin)
{
    bin = _shadowbin;
}

void obcsGetMultiTextureBin(std::vector<OBJECT*>& bin)
{
    if(obdrawIsRenderPass(RENDER_REFLECTION))
        bin = waterreflection.ReflectionMultiTextureBin;
    else
        bin = _multitexturebin;
}

LOGICAL obcsGetObjectCullStats(SINT32 * CullObjectCnt,SINT32 * DisplayedObjectCnt)
{
	*CullObjectCnt = _CullObjectCnt;
	*DisplayedObjectCnt = _DisplayedObjectCnt;

	return TRUE;
}

LOGICAL obcsInit(void)
{
	sortfree.prev = sortfree.next = (LISTHEADER*)memAlloc(MINOBJECTS * sizeof(SORTHEADER));
	if (sortfree.next == NULL)
	{
		return(FALSE);
	}

	if (!listInitList((LISTHEADER*)&sortfree, MINOBJECTS, sizeof(SORTHEADER)))
	{
		return(FALSE);
	}

	sortedlist.prev = sortedlist.next = NULL;
	sortedlisttail = NULL;
	unsortedlist.prev = unsortedlist.next = NULL;
	unsortedlisttail = NULL;
	sortedcockpitlist.prev = sortedcockpitlist.next = NULL;
	sortedcockpitlisttail = NULL;
	unsortedcockpitlist.prev = unsortedcockpitlist.next = NULL;
	unsortedcockpitlisttail = NULL;

	return(TRUE);
}

void obcsKill(void)
{
	if (sortfree.prev != NULL)
	{
		memFree(sortfree.prev);
		sortfree.prev = NULL;
	}
}

void obcsDoCull(LOGICAL cull)
{
    _doCull = cull;
}

static int _frontToBack(OBJECT* left, OBJECT* right)
{
    return left->distsqr < right->distsqr;
}

static int _backToFront(OBJECT* left, OBJECT* right)
{
    return left->distsqr > right->distsqr;
}

static int _backReflectionToFront(OBJECT* left, OBJECT* right)
{
    return left->reflectiondist > right->reflectiondist;
}

static int sortByMaterial(OBJECT* l, OBJECT* r)
{
    return l->material < r->material;
}

static int sortByData(OBJECT* l, OBJECT* r)
{
    return l->RenderData() < r->RenderData();
}

#if(0)
static int sortByTexture(OBJECT* l, OBJECT* r)
{
  return l->texid < r->texid;
}
#endif

void obcsCullForWaterReflection(float planeheight)
{
    OBJECT* o = NULL;
    LISTHEADER* list = NULL;

    _transparentbin.clear();
    _opaquebin.clear();

	int numobjects = obmainGetObjects(&list);
	o = (OBJECT*)list->next;;

    piVECTOR p;

	for(; o != NULL; o = o->next)
	{
		// only operate on selected objects
		if(o->flags & (OBJECT_ATTACHED|OBJECT_TERRAIN))
		{
			continue;
		}

        unsigned int skipdrawflags = 0;
        OBJECT* attach = NULL;

		for(attach = o; attach != NULL; attach = attach->attached)
		{
            // Better for debugging to see which flags are set
            skipdrawflags |= !(attach->flags & OBJECT_VISIBLE);
            skipdrawflags |=  attach->flags & OBJECT_SKIPDRAWING;
            skipdrawflags |=  attach->flag2 & OBJECT_HIDDEN;

			if(skipdrawflags)
		        continue;

			if(attach->flag2 & OBJECT_ISCOCKPIT)
                continue;

            if(attach->flag3 & OBJECT_COAST_WATER)
                continue;

            if(attach->flag3 & OBJECT_RIVER_WATER)
                continue;

			if(!(attach->flag2 & OBJECT_ISFPS) && !rnvisFOVCheck(attach))
			{
				continue;
			}

            if(attach->flag2 & OBJECT_ANIMATION)
            {
                if(attach->distsqr > 50.0f)
                    continue;
            }

            if(attach->cullcallback)
            {
                // Returns true if object is to be culled
                if(attach->cullcallback(attach))
                {
                    continue;
                }
            }

            if(attach->flags & OBJECT_USEMATRIX)
            {
		        math3dVectorMultiply(&p, &attach->pos, &attach->mat);
	        }
	        else
	        {
		        p = attach->pos;
            }

            if(p.z > planeheight)
            {
                continue;
            }

            if((attach->flag2 & OBJECT_HASALPHA) && !(attach->flags & OBJECT_NOFILTER))
		    {
                _transparentbin.push_back(attach);
			}
			else
			{
                _opaquebin.push_back(attach);
            }
		}
	}

    sort(_transparentbin.begin(), _transparentbin.end(), _backToFront);
    sort(_opaquebin.begin(), _opaquebin.end(), sortByData);
}

struct AddToDrawList
{
    OBJECT* o;
    unsigned int mask;

    void Add(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & (OBJECT_NOTOBJECT | OBJECT_SKIPDRAWING)) &&
                 o->flags & OBJECT_VISIBLE &&
               !(o->flag2 & OBJECT_HIDDEN))
            {
                bool cull = false;

                if(mask != 0)
                    cull = !rnvisFOVCheck(o);

                if(!cull)
                {
                    if(o->cullcallback)
                    {
                        // Returns true if object is to be culled
                        cull = o->cullcallback(o);
                    }

                    if(!cull && piIsMaterialInterior(o->matid))
                    {
                        // No reason to draw interior objects if far away
                        if(o->distsqr * (FieldOfView / 90.0f) > 1000.0f)
                            cull = true;
                    }

                    if(!cull)
                    {
                        // Separate multitexture and water
                        if(!(o->flag2 & OBJECT_HASALPHA) && obdrawGetMultiTextureOpcode(o->texid, NULL) == MT_SINGLEPASS)
                        {
                            _multitexturebin.push_back(o);
                        }
                        else if(o->flag2 & OBJECT_ISCOCKPIT)
	                    {
		                    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
		                    {
			                    _transparentcockpitbin.push_back(o);
		                    }
		                    else
		                    {
                                _opaquecockpitbin.push_back(o);
		                    }
	                    }
	                    else
	                    {
		                    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
		                    {
                                _transparentbin.push_back(o);
                            }
		                    else
		                    {
                                _opaquebin.push_back(o);
		                    }
	                    }
                    }
                }
            }

            Add(o->child);
            Add(o->sibling);
            Add(o->attached);
        }
    }

    void Apply()
    {
        Add(o);
    }
};

void obcsCull()
{
    logTimerBeginMacro();

    _multitexturebin.clear();
    _opaquebin.clear();
    _transparentbin.clear();
    _transparentcockpitbin.clear();
    _opaquecockpitbin.clear();

    trnCDTerrainObject* to = NULL;

    trncdGetTOHead(&to);

    static BoundingBox bb;

    for(; to != NULL; to = to->Next)
    {
        unsigned int mask = CLIP_ALL_PLANES;
        static piVECTOR p1, p2;

        p1.x = to->BBox1.x + to->Center.x - to->OrigCenter.x;
        p1.y = to->BBox1.y + to->Center.y - to->OrigCenter.y;
        p1.z = to->BBox1.z + to->Center.z - to->OrigCenter.z;

        p2.x = to->BBox2.x + to->Center.x - to->OrigCenter.x;
        p2.y = to->BBox2.y + to->Center.y - to->OrigCenter.y;
        p2.z = to->BBox2.z + to->Center.z - to->OrigCenter.z;

        bb.set(p1, p2);

        if(!rnvisBoundBoxCull(mask, bb))
        {
            for(int i = 0; i < TRN_MAX_LOD; i++)
            {
                if(!to->LOD[i])
                    break;

                OBJECT* o = to->LOD[i];

                if(o->child)
                {
                    unsigned int skipdraw = 0;

                    skipdraw |= o->flags & OBJECT_SKIPDRAWING;
                    skipdraw |= o->flag2 & OBJECT_SPEEDTREE;

                    if(!skipdraw)
                    {
                        static AddToDrawList add;

                        add.o = o->child;
                        add.mask = mask;
                        add.Apply();
                    }
                }
            }
        }
    }

    vehVehicle* veh = NULL;
    vehGetVehicleHead(&veh);

    for(; veh != NULL; veh = veh->getNext())
    {
        piVECTOR pos;

        pos.x = (float)(veh->PosData.Pos.x - WorldOriginOffset.x);
        pos.y = (float)(veh->PosData.Pos.y - WorldOriginOffset.y);
        pos.z = (float)(veh->PosData.Pos.z - WorldOriginOffset.z);

        if(veh->VehicleObjectData)
        {
            OBJECT* o = veh->VehicleObjectData->PlaneObject;

            if(o && o->modelInstance)
            {
                unsigned int mask = CLIP_ALL_PLANES;

                float radius = o->modelInstance->GetBounds().radius();

                static piVECTOR p1, p2;

                math3dVectorMultiply(&p1, &o->bbox[0], &o->mat);
                math3dVectorMultiply(&p2, &o->bbox[0], &o->mat);

                bb.set(p1, p2);

                if(rnvisBoundBoxCull(mask, bb))
                {
                    for(int i = 0; i < ML_MAX_LODS; i++)
                    {
                        o = veh->VehicleObjectData->LODObjects[i];

                        if(!o)
                            break;

                        if(!(o->flags & OBJECT_SKIPDRAWING))
                        {
                            static AddToDrawList add;

                            add.o = o->child;
                            add.mask = mask;
                            add.Apply();
                        }
                    }
                }
            }
        }
    }

    if(ogl::UseDeprecatedPath())
    {
        std::sort(_multitexturebin.begin(), _multitexturebin.end(), sortByMaterial);
        std::sort(_opaquebin.begin(), _opaquebin.end(), sortByData);
        std::sort(_opaquecockpitbin.begin(), _opaquecockpitbin.end(), sortByData);
        std::sort(_transparentbin.begin(), _transparentbin.end(), _frontToBack); // Reversed when drawing
        std::sort(_transparentcockpitbin.begin(), _transparentcockpitbin.end(), _frontToBack); // Reversed when drawing
    }
    else
    {
        std::sort(_multitexturebin.begin(), _multitexturebin.end(), sortByMaterial);
        std::sort(_opaquebin.begin(), _opaquebin.end(), sortByMaterial);
        std::sort(_opaquecockpitbin.begin(), _opaquecockpitbin.end(), sortByMaterial);
        std::sort(_transparentbin.begin(), _transparentbin.end(), _frontToBack); // Reversed when drawing
        std::sort(_transparentcockpitbin.begin(), _transparentcockpitbin.end(), _frontToBack); // Reversed when drawing
    }

	_DisplayedObjectCnt = (int)_multitexturebin.size();
	_DisplayedObjectCnt += (int)_opaquebin.size();
	_DisplayedObjectCnt += (int)_opaquecockpitbin.size();
	_DisplayedObjectCnt += (int)_transparentbin.size();
	_DisplayedObjectCnt += (int)_transparentcockpitbin.size();

    logTimerEndMacro();
}

int obcsCullAndSortObjects(int numobjects, OBJECT* object)
{
    _CullObjectCnt = numobjects;

    if(!_doCull)
    {
        return _DisplayedObjectCnt;
    }

    logTimerBeginMacro();

    _multitexturebin.clear();
    _opaquebin.clear();
    _transparentbin.clear();
    _transparentcockpitbin.clear();
    _opaquecockpitbin.clear();

    waterreflection.Reset();

    if(gCustomTerrain)
        shadowcull.Reset();

	for(; object != NULL; object = object->next)
	{
		// only operate on selected objects
		if (object->flags & OBJECT_NOTOBJECT)
		{
			obmainMoveToNotObjectTree(object);
			continue;
		}

		// only operate on selected objects
		if(object->flags & (OBJECT_ATTACHED|OBJECT_TERRAIN))
		{
            continue;
		}

		unsigned int skipdrawflags = 0;

		for(OBJECT* lpobj = object; lpobj; lpobj = lpobj->attached)
		{
            // Better for debugging to see which flags are set
            skipdrawflags = !(lpobj->flags & OBJECT_VISIBLE);
            skipdrawflags |=  lpobj->flags & OBJECT_SKIPDRAWING;
            skipdrawflags |=  lpobj->flag2 & OBJECT_HIDDEN;
            skipdrawflags |=  lpobj->flags & OBJECT_DYNASHADOW && ShadowSizeSelect == 0;

            if(gCustomTerrain)
            {
                skipdrawflags |= lpobj->flags & OBJECT_DYNASHADOW;
                skipdrawflags |= (lpobj->flag3 & (OBJECT_CLOUDS | OBJECT_RAINCLOUDS));
            }

			if(skipdrawflags)
			{
                skipdrawflags = 0;
		        continue;
			}

            waterreflection.Cull(lpobj);

            if(gCustomTerrain)
                shadowcull.Cull(lpobj);

			if(!(lpobj->flag2 & OBJECT_ISFPS) && !rnvisFOVCheck(lpobj))
			{
				continue;
			}

            if(lpobj->cullcallback)
            {
                // Returns true if object is to be culled
                if(lpobj->cullcallback(lpobj))
                {
                    continue;
                }
            }

            if(piIsMaterialInterior(lpobj->matid))
            {
                // No reason to draw interior objects if far away
                if(lpobj->distsqr * (FieldOfView / 90.0f) > 1000.0f)
                    continue;
            }

            // Separate multitexture and water
            if(!(lpobj->flag2 & OBJECT_HASALPHA) && obdrawGetMultiTextureOpcode(lpobj->texid, NULL) == MT_SINGLEPASS)
            {
                _multitexturebin.push_back(lpobj);
                continue;
            }

			if(lpobj->flag2 & OBJECT_ISCOCKPIT)
			{
				if (lpobj->flag2 & OBJECT_HASALPHA &&
					!(lpobj->flags & OBJECT_NOFILTER))
				{
					_transparentcockpitbin.push_back(lpobj);
				}
				else
				{
                    _opaquecockpitbin.push_back(lpobj);
				}
			}
			else
			{
				if(lpobj->flag2 & OBJECT_HASALPHA &&
                 !(lpobj->flags & OBJECT_NOFILTER))
				{
                    _transparentbin.push_back(lpobj);
                }
				else
				{
                    _opaquebin.push_back(lpobj);
				}
			}
		}
	}

    if(ogl::UseDeprecatedPath())
    {
        std::sort(_multitexturebin.begin(), _multitexturebin.end(), sortByMaterial);
        std::sort(_opaquebin.begin(), _opaquebin.end(), sortByData);
        std::sort(_opaquecockpitbin.begin(), _opaquecockpitbin.end(), sortByData);
        std::sort(_transparentbin.begin(), _transparentbin.end(), _backToFront);
        std::sort(_transparentcockpitbin.begin(), _transparentcockpitbin.end(), _backToFront);
    }
    else
    {
        std::sort(_multitexturebin.begin(), _multitexturebin.end(), sortByMaterial);
        std::sort(_opaquebin.begin(), _opaquebin.end(), sortByMaterial);
        std::sort(_opaquecockpitbin.begin(), _opaquecockpitbin.end(), sortByMaterial);
        std::sort(_transparentbin.begin(), _transparentbin.end(), _backToFront);
        std::sort(_transparentcockpitbin.begin(), _transparentcockpitbin.end(), _backToFront);
    }

    waterreflection.Sort();

    if(gCustomTerrain)
        shadowcull.Sort();

	_DisplayedObjectCnt = (int)_multitexturebin.size();
	_DisplayedObjectCnt += (int)_opaquebin.size();
	_DisplayedObjectCnt += (int)_opaquecockpitbin.size();
	_DisplayedObjectCnt += (int)_transparentbin.size();
	_DisplayedObjectCnt += (int)_transparentcockpitbin.size();

    logTimerEndMacro();

	return _DisplayedObjectCnt;
}

// Used for shadow mapping
int obcsCullAndSortShadowObjects(int numobjects, OBJECT* lpobjorig, piVECTOR* lpdir)
{
    SINT32 numobj = 0;
    OBJECT*	object;
    OBJECT* lpobj;

    if(!_doCull)
        return (int)_shadowbin.size();

    _shadowbin.clear();

	for(object = lpobjorig; object; object = object->next)
	{
		_CullObjectCnt++;

		// only operate on selected objects
		if (object->flags & OBJECT_NOTOBJECT)
		{
			obmainMoveToNotObjectTree(object);
			continue;
		}

		// only operate on selected objects
		if(object->flags & OBJECT_ATTACHED)
		{
			continue;
		}

        unsigned int skipdrawflags = 0;

		for(lpobj = object; lpobj; lpobj = lpobj->attached)
		{
            // Better for debugging to see which flags are set
            skipdrawflags = !(lpobj->flags & OBJECT_VISIBLE);
            skipdrawflags |=  lpobj->flags & OBJECT_SKIPDRAWING;
            skipdrawflags |=  lpobj->flag2 & OBJECT_HIDDEN;

			if(skipdrawflags)
			{
                skipdrawflags = 0;
		        continue;
			}

            if(lpobj->flag2 & OBJECT_HASALPHA && !(lpobj->flags & OBJECT_NOFILTER))
            {
                continue;
            }

            // Only care about FPS now.
            if(!(lpobj->flag2 & (OBJECT_ISFPS | OBJECT_ANIMATION)))
            {
                continue;
            }

            if(!(lpobj->flag3 & OBJECT_CAST_SHADOW))
            {
                continue;
            }

			if(!rnvisFOVCheck(lpobj))
			{
				continue;
			}

            if(lpobj->cullcallback)
            {
                // Returns true if object is to be culled
                if(lpobj->cullcallback(lpobj))
                {
                    continue;
                }
            }

            _shadowbin.push_back(lpobj);

  		    numobj += 1;
		}
	}

    return numobj;
}

SORTHEADER* obcsGetSortedList(void)
{
	return((SORTHEADER*)sortedlist.next);
}

SORTHEADER* obcsGetUnsortedList(void)
{
	return((SORTHEADER*)unsortedlisttail);
}

SORTHEADER* obcsGetSortedCockpitList(void)
{
	return((SORTHEADER*)sortedcockpitlist.next);
}

SORTHEADER* obcsGetUnsortedCockpitList(void)
{
	return((SORTHEADER*)unsortedcockpitlisttail);
}

SORTHEADER* obcsGetMultiTextureList()
{
    return (SORTHEADER*)tailMultiTexture;
}

/* not used
LOGICAL unsortMultiTexture(SORTHEADER* lpsort)
{
	if (lplastMultiTexture == NULL || !bPerFrameBSP)
	{
		if (lplastMultiTexture == NULL)
		{
			tailMultiTexture = lpsort;
		}
		listAddItem((LISTHEADER*)&multiTextureList, (LISTHEADER*)lpsort);
		lplastMultiTexture = lpsort;
	}
	else
	{
		if (lpsort->layer > lplastMultiTexture->layer)
		{
			do
			{
				if (lpsort->layer < lplastMultiTexture->layer ||
					(lpsort->layer == lplastMultiTexture->layer &&
					lpsort->distsqr > lplastMultiTexture->distsqr))
				{
					listAddItemBefore((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
					lplastMultiTexture = lpsort;
					break;
				}
				if (lplastMultiTexture->next == NULL)
				{
					listAddItemAfter((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
					tailMultiTexture = lplastMultiTexture = lpsort;
					break;
				}
				lplastMultiTexture = lplastMultiTexture->next;
			}
			while (TRUE);
		}
		else
		if (lpsort->layer < lplastMultiTexture->layer)
		{
			while (lplastMultiTexture->prev)
			{
				if (lpsort->layer > lplastMultiTexture->layer ||
					(lpsort->layer == lplastMultiTexture->layer &&
					lpsort->distsqr < lplastMultiTexture->distsqr))
				{
					listAddItemAfter((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
					lplastMultiTexture = lpsort;
					break;
				}
				lplastMultiTexture = lplastMultiTexture->prev;
			}
			if (lplastMultiTexture->prev == NULL)
			{
				listAddItemBefore((LISTHEADER*)lplastMultiTexture->next, (LISTHEADER*)lpsort);
				lplastMultiTexture = lpsort;
			}
		}
		else
		{
			if (lpsort->distsqr < lplastMultiTexture->distsqr)
			{
				do
				{
					if (lpsort->layer < lplastMultiTexture->layer ||
						(lpsort->layer == lplastMultiTexture->layer &&
						lpsort->distsqr > lplastMultiTexture->distsqr))
					{
						listAddItemBefore((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
						lplastMultiTexture = lpsort;
						break;
					}
					if (lplastMultiTexture->next == NULL)
					{
						listAddItemAfter((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
						tailMultiTexture = lplastMultiTexture = lpsort;
						break;
					}
					lplastMultiTexture = lplastMultiTexture->next;
				}
				while (TRUE);
			}
			else
			{
				while (lplastMultiTexture->prev)
				{
					if (lpsort->layer > lplastMultiTexture->layer ||
						(lpsort->layer == lplastMultiTexture->layer &&
						lpsort->distsqr < lplastMultiTexture->distsqr))
					{
						listAddItemAfter((LISTHEADER*)lplastMultiTexture, (LISTHEADER*)lpsort);
						lplastMultiTexture = lpsort;
						break;
					}
					lplastMultiTexture = lplastMultiTexture->prev;
				}
				if (lplastMultiTexture->prev == NULL)
				{
					listAddItemBefore((LISTHEADER*)lplastMultiTexture->next, (LISTHEADER*)lpsort);
					lplastMultiTexture = lpsort;
				}
			}
		}
	}

    return TRUE;
}
*/

static LOGICAL UnsortBSP(SORTHEADER* lpsort)
{
    logTimerBeginMacro();

	if (lplastunsorted == NULL || !bPerFrameBSP )//|| true)
	{
		if (lplastunsorted == NULL)
		{
			unsortedlisttail = lpsort;
		}
		listAddItem((LISTHEADER*)&unsortedlist, (LISTHEADER*)lpsort);
		lplastunsorted = lpsort;
	}
	else
	{
		if (lpsort->layer > lplastunsorted->layer)
		{
			do
			{
				if (lpsort->layer < lplastunsorted->layer ||
					(lpsort->layer == lplastunsorted->layer &&
					lpsort->distsqr > lplastunsorted->distsqr))
				{
					listAddItemBefore((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
					lplastunsorted = lpsort;
					break;
				}
				if (lplastunsorted->next == NULL)
				{
					listAddItemAfter((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
					unsortedlisttail = lplastunsorted = lpsort;
					break;
				}
				lplastunsorted = lplastunsorted->next;
			}
			while (TRUE);
		}
		else
		if (lpsort->layer < lplastunsorted->layer)
		{
			while (lplastunsorted->prev)
			{
				if (lpsort->layer > lplastunsorted->layer ||
					(lpsort->layer == lplastunsorted->layer &&
					lpsort->distsqr < lplastunsorted->distsqr))
				{
					listAddItemAfter((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
					lplastunsorted = lpsort;
					break;
				}
				lplastunsorted = lplastunsorted->prev;
			}
			if (lplastunsorted->prev == NULL)
			{
				listAddItemBefore((LISTHEADER*)lplastunsorted->next, (LISTHEADER*)lpsort);
				lplastunsorted = lpsort;
			}
		}
		else
		{
			if (lpsort->distsqr < lplastunsorted->distsqr)
			{
				do
				{
					if (lpsort->layer < lplastunsorted->layer ||
						(lpsort->layer == lplastunsorted->layer &&
						lpsort->distsqr > lplastunsorted->distsqr))
					{
						listAddItemBefore((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
						lplastunsorted = lpsort;
						break;
					}
					if (lplastunsorted->next == NULL)
					{
						listAddItemAfter((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
						unsortedlisttail = lplastunsorted = lpsort;
						break;
					}
					lplastunsorted = lplastunsorted->next;
				}
				while (TRUE);
			}
			else
			{
				while (lplastunsorted->prev)
				{
					if (lpsort->layer > lplastunsorted->layer ||
						(lpsort->layer == lplastunsorted->layer &&
						lpsort->distsqr < lplastunsorted->distsqr))
					{
						listAddItemAfter((LISTHEADER*)lplastunsorted, (LISTHEADER*)lpsort);
						lplastunsorted = lpsort;
						break;
					}
					lplastunsorted = lplastunsorted->prev;
				}
				if (lplastunsorted->prev == NULL)
				{
					listAddItemBefore((LISTHEADER*)lplastunsorted->next, (LISTHEADER*)lpsort);
					lplastunsorted = lpsort;
				}
			}
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

#if(0)
static LOGICAL SortBSP(SORTHEADER* lpsort)
{
    logTimerBeginMacro();

	if (lplast == NULL || !bPerFrameBSP)
	{
		if (lplast == NULL)
		{
			sortedlisttail = lpsort;
		}
		listAddItem((LISTHEADER*)&sortedlist, (LISTHEADER*)lpsort);
		lplast = lpsort;
	}
	else
	{
		if (lpsort->layer > lplast->layer)
		{
			do
			{
				if (lpsort->layer < lplast->layer ||
					(lpsort->layer == lplast->layer &&
					lpsort->distsqr > lplast->distsqr))
				{
					listAddItemBefore((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
					lplast = lpsort;
					break;
				}
				if (lplast->next == NULL)
				{
					listAddItemAfter((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
					sortedlisttail = lplast = lpsort;
					break;
				}
				lplast = lplast->next;
			}
			while (TRUE);
		}
		else
		if (lpsort->layer < lplast->layer)
		{
			while (lplast->prev)
			{
				if (lpsort->layer > lplast->layer ||
					(lpsort->layer == lplast->layer &&
					lpsort->distsqr < lplast->distsqr))
				{
					listAddItemAfter((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
					lplast = lpsort;
					break;
				}
				lplast = lplast->prev;
			}
			if (lplast->prev == NULL)
			{
				listAddItemBefore((LISTHEADER*)lplast->next, (LISTHEADER*)lpsort);
				lplast = lpsort;
			}
		}
		else
		{
			if (lpsort->distsqr < lplast->distsqr)
			{
				do
				{
					if (lpsort->layer < lplast->layer ||
						(lpsort->layer == lplast->layer &&
						lpsort->distsqr > lplast->distsqr))
					{
						listAddItemBefore((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
						lplast = lpsort;
						break;
					}
					if (lplast->next == NULL)
					{
						listAddItemAfter((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
						sortedlisttail = lplast = lpsort;
						break;
					}
					lplast = lplast->next;
				}
				while (TRUE);
			}
			else
			{
				while (lplast->prev)
				{
					if (lpsort->layer > lplast->layer ||
						(lpsort->layer == lplast->layer &&
						lpsort->distsqr < lplast->distsqr))
					{
						listAddItemAfter((LISTHEADER*)lplast, (LISTHEADER*)lpsort);
						lplast = lpsort;
						break;
					}
					lplast = lplast->prev;
				}
				if (lplast->prev == NULL)
				{
					listAddItemBefore((LISTHEADER*)lplast->next, (LISTHEADER*)lpsort);
					lplast = lpsort;
				}
			}
		}
	}

    logTimerEndMacro();

	return(TRUE);
}
#endif

#if(0)
static LOGICAL SortCockpitBSP(SORTHEADER* lpsort)
{
    logTimerBeginMacro();

	if (lplastcockpit == NULL || !bPerFrameBSP)
	{
		if (lplastcockpit == NULL)
		{
			sortedcockpitlisttail = lpsort;
		}
		listAddItem((LISTHEADER*)&sortedcockpitlist, (LISTHEADER*)lpsort);
		lplastcockpit = lpsort;
	}
	else
	{
		if (lpsort->layer > lplastcockpit->layer)
		{
			do
			{
				if (lpsort->layer < lplastcockpit->layer ||
					(lpsort->layer == lplastcockpit->layer &&
					lpsort->distsqr > lplastcockpit->distsqr))
				{
					listAddItemBefore((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
					lplastcockpit = lpsort;
					break;
				}
				if (lplastcockpit->next == NULL)
				{
					listAddItemAfter((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
					sortedcockpitlisttail = lplastcockpit = lpsort;
					break;
				}
				lplastcockpit = lplastcockpit->next;
			}
			while (TRUE);
		}
		else
		if (lpsort->layer < lplastcockpit->layer)
		{
			while (lplastcockpit->prev)
			{
				if (lpsort->layer > lplastcockpit->layer ||
					(lpsort->layer == lplastcockpit->layer &&
					lpsort->distsqr < lplastcockpit->distsqr))
				{
					listAddItemAfter((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
					lplastcockpit = lpsort;
					break;
				}
				lplastcockpit = lplastcockpit->prev;
			}
			if (lplastcockpit->prev == NULL)
			{
				listAddItemBefore((LISTHEADER*)lplastcockpit->next, (LISTHEADER*)lpsort);
				lplastcockpit = lpsort;
			}
		}
		else
		{
			if (lpsort->distsqr < lplastcockpit->distsqr)
			{
				do
				{
					if (lpsort->layer < lplastcockpit->layer ||
						(lpsort->layer == lplastcockpit->layer &&
						lpsort->distsqr > lplastcockpit->distsqr))
					{
						listAddItemBefore((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
						lplastcockpit = lpsort;
						break;
					}
					if (lplastcockpit->next == NULL)
					{
						listAddItemAfter((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
						sortedcockpitlisttail = lplastcockpit = lpsort;
						break;
					}
					lplastcockpit = lplastcockpit->next;
				}
				while (TRUE);
			}
			else
			{
				while (lplastcockpit->prev)
				{
					if (lpsort->layer > lplastcockpit->layer ||
						(lpsort->layer == lplastcockpit->layer &&
						lpsort->distsqr < lplastcockpit->distsqr))
					{
						listAddItemAfter((LISTHEADER*)lplastcockpit, (LISTHEADER*)lpsort);
						lplastcockpit = lpsort;
						break;
					}
					lplastcockpit = lplastcockpit->prev;
				}
				if (lplastcockpit->prev == NULL)
				{
					listAddItemBefore((LISTHEADER*)lplastcockpit->next, (LISTHEADER*)lpsort);
					lplastcockpit = lpsort;
				}
			}
		}
	}

    logTimerEndMacro();

	return(TRUE);
}
#endif

#if(0)
static LOGICAL UnsortCockpitBSP(SORTHEADER* lpsort)
{
    logTimerBeginMacro();

	if (lplastunsortedcockpit == NULL || !bPerFrameBSP)
	{
		if (lplastunsortedcockpit == NULL)
		{
			unsortedcockpitlisttail = lpsort;
		}
		listAddItem((LISTHEADER*)&unsortedcockpitlist, (LISTHEADER*)lpsort);
		lplastunsortedcockpit = lpsort;
	}
	else
	{
		if (lpsort->layer > lplastunsortedcockpit->layer)
		{
			do
			{
				if (lpsort->layer < lplastunsortedcockpit->layer ||
					(lpsort->layer == lplastunsortedcockpit->layer &&
					lpsort->distsqr > lplastunsortedcockpit->distsqr))
				{
					listAddItemBefore((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
					lplastunsortedcockpit = lpsort;
					break;
				}
				if (lplastunsortedcockpit->next == NULL)
				{
					listAddItemAfter((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
					unsortedcockpitlisttail = lplastunsortedcockpit = lpsort;
					break;
				}
				lplastunsortedcockpit = lplastunsortedcockpit->next;
			}
			while (TRUE);
		}
		else
		if (lpsort->layer < lplastunsortedcockpit->layer)
		{
			while (lplastunsortedcockpit->prev)
			{
				if (lpsort->layer > lplastunsortedcockpit->layer ||
					(lpsort->layer == lplastunsortedcockpit->layer &&
					lpsort->distsqr < lplastunsortedcockpit->distsqr))
				{
					listAddItemAfter((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
					lplastunsortedcockpit = lpsort;
					break;
				}
				lplastunsortedcockpit = lplastunsortedcockpit->prev;
			}
			if (lplastunsortedcockpit->prev == NULL)
			{
				listAddItemBefore((LISTHEADER*)lplastunsortedcockpit->next, (LISTHEADER*)lpsort);
				lplastunsortedcockpit = lpsort;
			}
		}
		else
		{
			if (lpsort->distsqr < lplastunsortedcockpit->distsqr)
			{
				do
				{
					if (lpsort->layer < lplastunsortedcockpit->layer ||
						(lpsort->layer == lplastunsortedcockpit->layer &&
						lpsort->distsqr > lplastunsortedcockpit->distsqr))
					{
						listAddItemBefore((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
						lplastunsortedcockpit = lpsort;
						break;
					}
					if (lplastunsortedcockpit->next == NULL)
					{
						listAddItemAfter((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
						unsortedcockpitlisttail = lplastunsortedcockpit = lpsort;
						break;
					}
					lplastunsortedcockpit = lplastunsortedcockpit->next;
				}
				while (TRUE);
			}
			else
			{
				while (lplastunsortedcockpit->prev)
				{
					if (lpsort->layer > lplastunsortedcockpit->layer ||
						(lpsort->layer == lplastunsortedcockpit->layer &&
						lpsort->distsqr < lplastunsortedcockpit->distsqr))
					{
						listAddItemAfter((LISTHEADER*)lplastunsortedcockpit, (LISTHEADER*)lpsort);
						lplastunsortedcockpit = lpsort;
						break;
					}
					lplastunsortedcockpit = lplastunsortedcockpit->prev;
				}
				if (lplastunsortedcockpit->prev == NULL)
				{
					listAddItemBefore((LISTHEADER*)lplastunsortedcockpit->next, (LISTHEADER*)lpsort);
					lplastunsortedcockpit = lpsort;
				}
			}
		}
	}

    logTimerEndMacro();

	return(TRUE);
}
#endif

// Fast reset of the sorted list
void obcsResetSortedList(void)
{
    if(!_doCull) return;

    LISTHEADER*	tail;

	// reset the sort list
	tail = (LISTHEADER*)sortedlisttail;
	if (tail != NULL)
	{
		tail->next = sortfree.next;
		if (tail->next != NULL)
		{
			((LISTHEADER*)tail->next)->prev = tail;
		}

		sortfree.next = sortedlist.next;
		sortedlist.next = NULL;
		if (sortfree.next != NULL)
		{
			((LISTHEADER*)sortfree.next)->prev = &sortfree;
		}
	}

	lplast = NULL;
	sortedlisttail = NULL;
}

void obcsResetMultiTexture()
{
    if(!_doCull) return;

    LISTHEADER*	tail;

	tail = (LISTHEADER*)tailMultiTexture;
	if (tail != NULL)
	{
		tail->next = sortfree.next;
		if (tail->next != NULL)
		{
			((LISTHEADER*)tail->next)->prev = tail;
		}

		sortfree.next = multiTextureList.next;
		multiTextureList.next = NULL;
		if (sortfree.next != NULL)
		{
			((LISTHEADER*)sortfree.next)->prev = &sortfree;
		}
	}

	lplastMultiTexture = NULL;
	tailMultiTexture = NULL;
}

// Fast reset of the unsorted list
void obcsResetUnsortedList(void)
{
    if(!_doCull) return;

    LISTHEADER*	tail;

	tail = (LISTHEADER*)unsortedlisttail;
	if (tail != NULL)
	{
		tail->next = sortfree.next;
		if (tail->next != NULL)
		{
			((LISTHEADER*)tail->next)->prev = tail;
		}

		sortfree.next = unsortedlist.next;
		unsortedlist.next = NULL;
		if (sortfree.next != NULL)
		{
			((LISTHEADER*)sortfree.next)->prev = &sortfree;
		}
	}

	lplastunsorted = NULL;
	unsortedlisttail = NULL;
}

// Fast reset of the sorted cockpit list
void obcsResetSortedCockpitList(void)
{
    if(!_doCull) return;

    LISTHEADER*	tail;

	tail = (LISTHEADER*)sortedcockpitlisttail;
	if (tail != NULL)
	{
		tail->next = sortfree.next;
		if (tail->next != NULL)
		{
			((LISTHEADER*)tail->next)->prev = tail;
		}

		sortfree.next = sortedcockpitlist.next;
		sortedcockpitlist.next = NULL;
		if (sortfree.next != NULL)
		{
			((LISTHEADER*)sortfree.next)->prev = &sortfree;
		}
	}

	lplastcockpit = NULL;
	sortedcockpitlisttail = NULL;
}

// Fast reset of the unsorted cockpit list
void obcsResetUnsortedCockpitList(void)
{
    if(!_doCull) return;

    LISTHEADER*	tail;

	tail = (LISTHEADER*)unsortedcockpitlisttail;
	if (tail != NULL)
	{
		tail->next = sortfree.next;
		if (tail->next != NULL)
		{
			((LISTHEADER*)tail->next)->prev = tail;
		}

		sortfree.next = unsortedcockpitlist.next;
		unsortedcockpitlist.next = NULL;
		if (sortfree.next != NULL)
		{
			((LISTHEADER*)sortfree.next)->prev = &sortfree;
		}
	}

	lplastunsortedcockpit = NULL;
	unsortedcockpitlisttail = NULL;
}

void obcsAddToUnsortedList(SORTHEADER* lpsort)
{
	UnsortBSP(lpsort);
}

LOGICAL obcsBubbleSortObjects(int numobjects, OBJECT* lpobjorig, piVECTOR* lpdir)
{
FLOAT32			dp;
piVECTOR		pp, pv, pd, dir;
piMATRIX		mat;
OBJECT*	lpobj;
OBJECT* lpnxt;
OBJECT*	objprev;
OBJECT* nxtprev;

	pp.x = -playpos.x;
	pp.y = -playpos.y;
	pp.z = -playpos.z;

	dir = *lpdir;

	for (lpobj=lpobjorig; lpobj; lpobj=lpobj->next)
	{
		if (lpobj->flags & OBJECT_NOTOBJECT)
		{
			continue;
		}

		pv = pp;
		pd = dir;

		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			// Rotate/Translate position to object space

			math3dInvertMatrix(&mat, &lpobj->mat);


			// extract position
			math3dVectorMultiply(&pv, &pv, &mat);

			// nullify translation
			mat._41 = 0.0f;
			mat._42 = 0.0f;
			mat._43 = 0.0f;

			// extract direction
			math3dVectorMultiply(&pd, &pd, &mat);
		}

		math3dSubtractVectors(&pv, &pv, &lpobj->pos);
		dp = math3dDotProduct(&pd, &dir);
		lpobj->distsqr = dp * math3dDotProduct(&pv, &pv);
		lpobj->distsqr = math3dDotProduct(&pv, &pv);
	}

	lpobj = lpobjorig;
	if (lpobj != NULL && lpobj->next != NULL)
	{
		lpnxt = lpobj->next;
		while (lpnxt->flags & OBJECT_NOTOBJECT)
		{
			lpnxt = lpnxt->next;
			if (lpnxt == NULL)
			{
				break;
			}
		}

		for (; lpobj && lpnxt; )
		{
			if (lpobj->distsqr > lpnxt->distsqr)
			{
				objprev = lpobj->prev;
				if (lpobj->next != lpnxt)
				{
					nxtprev = lpnxt->prev;

					listDeleteOBJECT(lpobj);
					listDeleteOBJECT(lpnxt);

					listAddItemAfter((LISTHEADER*)objprev, (LISTHEADER*)lpnxt);
					listAddItemAfter((LISTHEADER*)nxtprev, (LISTHEADER*)lpobj);
				}
				else
				{
					listDeleteOBJECT(lpobj);
					listAddItemAfter((LISTHEADER*)lpnxt, (LISTHEADER*)lpobj);
				}

				lpnxt = lpobj;
			}

			lpobj = lpnxt;
			lpnxt = lpnxt->next;
			if (lpnxt != NULL)
			{
				while (lpnxt->flags & OBJECT_NOTOBJECT)
				{
					lpnxt = lpnxt->next;
					if (lpnxt == NULL)
					{
						break;
					}
				}
			}
		}
	}

	return(TRUE);
}

LOGICAL obcsSortObjectsByTexture(OBJECT* lpobjorig)
{
OBJECT*	lpobj;
OBJECT* lpnxt;
OBJECT*	objprev;
OBJECT* nxtprev;

	lpobj = lpobjorig;
	if (lpobj != NULL && lpobj->next != NULL)
	{
		lpnxt = lpobj->next;
		while (lpnxt->flags & OBJECT_NOTOBJECT)
		{
			lpnxt = lpnxt->next;
			if (lpnxt == NULL)
			{
				break;
			}
		}

		for (; lpobj && lpnxt; )
		{
			if (lpobj->texid > lpnxt->texid)
			{
				objprev = lpobj->prev;
				if (lpobj->next != lpnxt)
				{
					nxtprev = lpnxt->prev;

					listDeleteOBJECT(lpobj);
					listDeleteOBJECT(lpnxt);

					listAddItemAfter((LISTHEADER*)objprev, (LISTHEADER*)lpnxt);
					listAddItemAfter((LISTHEADER*)nxtprev, (LISTHEADER*)lpobj);
				}
				else
				{
					listDeleteOBJECT(lpobj);
					listAddItemAfter((LISTHEADER*)lpnxt, (LISTHEADER*)lpobj);
				}

				lpnxt = lpobj;
			}

			lpobj = lpnxt;
			lpnxt = lpnxt->next;
			if (lpnxt != NULL)
			{
				while (lpnxt->flags & OBJECT_NOTOBJECT)
				{
					lpnxt = lpnxt->next;
					if (lpnxt == NULL)
					{
						break;
					}
				}
			}
		}
	}

	return(TRUE);
}

LOGICAL obcsSortObjectTriangles(OBJECT* lpobj)
{
LOGICAL			sorted=FALSE;
SINT32			i, j, num;
FLOAT32			d1, d2;
piVECTOR		pv1, pv2, pv3, pp;
piMATRIX		mat;
piINDICES		ndx;
piINDICES*		pt;


	if (lpobj == NULL)
	{
		return(FALSE);
	}

	if (lpobj->flags & OBJECT_NOTOBJECT)
	{
		return(TRUE);
	}

	if (!(lpobj->flags & OBJECT_INDEXED) ||
		!(lpobj->flags & OBJECT_USEVERTEXBSP))
	{
		return(TRUE);
	}

	if (!ft100msticks)
	{
		return(TRUE);
	}

	pp.x = -playpos.x;
	pp.y = -playpos.y;
	pp.z = -playpos.z;
	if (lpobj->flags & OBJECT_USEMATRIX)
	{
		// Rotate/Translate position to object space
		math3dInvertMatrix(&mat, &lpobj->mat);

		math3dVectorMultiply(&pp, &pp, &mat);
	}

	piVectorConstPtr pos = lpobj->RenderPositions();
	num = lpobj->numtris;
	pt = lpobj->ptris;
	for (i=0; i < num-1; i++)
	{
		math3dSubtractVectors(&pv1, &pos[pt[i].v1], &pp);
		math3dSubtractVectors(&pv2, &pos[pt[i].v2], &pp);
		math3dSubtractVectors(&pv3, &pos[pt[i].v3], &pp);
		math3dAddVectors(&pv1, &pv1, &pv2);
		math3dAddVectors(&pv1, &pv1, &pv3);
		d1 = math3dDotProduct(&pv1, &pv1);

		for (j=i+1; j < num; j++)
		{
			math3dSubtractVectors(&pv1, &pos[pt[j].v1], &pp);
			math3dSubtractVectors(&pv2, &pos[pt[j].v2], &pp);
			math3dSubtractVectors(&pv3, &pos[pt[j].v3], &pp);
			math3dAddVectors(&pv1, &pv1, &pv2);
			math3dAddVectors(&pv1, &pv1, &pv3);
			d2 = math3dDotProduct(&pv1, &pv1);

			if (d1 < d2)
			{
				ndx = pt[i];
				pt[i] = pt[j];
				pt[j] = ndx;
				sorted = TRUE;
			}
		}

		if (sorted)
		{
//			break;
		}
	}

	return(TRUE);
}

SORTHEADER* obcsGetNextSortItem(void)
{
	return((SORTHEADER*)listGetItem((LISTHEADER*)&sortfree));
}

SORTHEADER* obcsGetNextOnList(SORTHEADER* list)
{
	if (list == NULL)
	{
		return(NULL);
	}

	return((SORTHEADER*)list->next);
}

SORTHEADER* obcsGetPrevOnList(SORTHEADER* list)
{
	if (list == NULL)
	{
		return(NULL);
	}

	if (list->prev->prev == NULL)
	{
		return(NULL);
	}

	return((SORTHEADER*)list->prev);
}

OBJECT* obcsGetListPart(SORTHEADER* list)
{
	if (list == NULL)
	{
		return(NULL);
	}

	return(list->part);
}

