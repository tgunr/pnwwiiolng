#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: trnfx.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "trn/trnfx.h"

#include "include/vars.h"
#include "math/math3d.h"
#include "sdm/StratDB.h"
#include "strt/strtCPFlags.h"
#include "fx/fxSnd.h"
#include "client/clientSTO.h"
#include "math/matheul.h"
#include "trn/trnvars.h"
#include "ob/obMgr.h"
#include "ob/obpool.h"
#include "fp/fpPPO.h"
#include "trn/trnop.h"
#include "ui/uiUTF.h"
#include "pers/persmain.h"
#include "ld/ldmdl.h"
#include "scen/ScenarioManager.h"

#ifdef BETA_RELEASE
#include "locl/loclApp.h"
#include "ui/uiChat.h"
#include "chat/chatLStringIDs.h"
#endif

#include "trn/trncd.h"

//////////////////////////////////////////////////////////////////////
// Function: _killSound
// Description:
//////////////////////////////////////////////////////////////////////
void _killSound(void* data)
{
    SINT32* handle = (SINT32*)data;

    if(handle == NULL)
        return;

    *handle = 0;
}

//////////////////////////////////////////////////////////////////////
// Function: hides/shows country flags for the objects owner
// Description:
//////////////////////////////////////////////////////////////////////
void trnFXSetCountryFlags(OBJECT* top, int owner)
{
	if(!top)
		return;

	obmainSetFlag3InObjectTree(top, OBJECT_HANDLES_COUNTRY);

	FindByName<OBJECT> finder;
	const char* flagNames[] = {"uk_flag", "us_flag", "fr_flag", "de_flag"};

	for(int flag = 0; flag < 4; flag++)
	{
		if(!flagNames[flag])
			continue;

		finder.set(flagNames[flag]);
		finder.find(top);

		if(finder.node)
		{
			if(owner == (flag + 1))
				((OBJECT*)(finder.node))->flags |= OBJECT_VISIBLE;
			else
				((OBJECT*)(finder.node))->flags &= ~OBJECT_VISIBLE;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Function: trnFxPlayAirWarnSiren
// Description:
//////////////////////////////////////////////////////////////////////
void trnFxPlayAirWarnSiren(trnCDTerrainObject* to, SINT32 lod)
{
    // Ticket# 4584
    if(!g_wantMessageAlert)
        return;

    OBJECT* o = to->LOD[lod];

    if(!o) return;

    while(o->flags & OBJECT_NOTOBJECT)
    {
        o = o->child;

        if(!o) return;
    }

    // Out of range. Stop sound if playing
    if(o->flags & OBJECT_SKIPDRAWING)
    {
        if(o->soundID > 0)
            fxSndStopSound(o->soundID);

        return;
    }

    // Check if HEAVY AIR warning
	sdm_full_obj_id fullid;

    fullid.cell.ox = to->StratSupercellX << 4;
    fullid.cell.oy = to->StratSupercellY << 4;
    fullid.id = to->ObjId.id;
	fullid.objClass = to->ObjId.objClass;
    fullid.objType = to->ObjId.objType;

    StratDB::StratObject* stratobj = StratDB::FindStratObject(fullid);

    LOGICAL playSiren = FALSE;

    if(stratobj)
    {
        StratDB::StratObject* parent = stratobj->ParentStratObject();

        // Loop until cp object is found
        while(parent)
        {
            if(parent->StratClass() == SDM_CLASS_CP)
                break;

            parent = parent->ParentStratObject();
        }

        if(parent)
        {
            if(parent->StratFlags() & CP_STAT_HEAVY_ENEMY_AIR)
            {
                if(o->soundID > 0)
                    return;

                playSiren = TRUE;
            }
        }
    }

    if(!playSiren)
    {
        if(o->soundID > 0)
        {
            fxSndStopSound(o->soundID);
        }

        return;
    }

    // Already playing?
    if(o->soundID > 0)
        return;

    // If not, play siren
    piVECTOR v;

    math3dVectorMultiply(&v, &o->pos, &o->mat);

    FLOAT32 z = math3dDotProduct(&v, &playdir) + playtrans.z;

    FLOAT32 distance = math3dFastSqrt(o->maxlod);

    // Too far
    if(z > distance)
        return;

    SINT32 ID = fxSndGetNewSound();

    if(ID == -1) return;

    fxSndSetSoundName(ID, (SCHAR*)"sounds\\airraidsiren.wav");
    fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
    fxSndSetPitch(ID, 1.0f);
    fxSndSetVolume(ID, 1.0f);
    fxSndSetCreateTime(ID, frametime);
    fxSndSetPlayType(ID, DEF_PLAYTYPE_OBJECT);
    fxSndSetEmitterObject(ID, o);
    fxSndSetPriority(ID, IDE_TP_PRIORITY_MEDIUM);
    fxSndSetPosition(ID, o->pos.x, o->pos.y, o->pos.z);
    fxSndSetMinDistance(ID, 100.0f);
    fxSndSetMaxDistance(ID, distance);
    fxSndSetKillCallback(ID, _killSound);
    fxSndSetKillData(ID, &o->soundID);

    if(!fxSndInitSound(ID))
        return;

    o->soundID = ID;
}

bool TerrainSTO::activate()
{
	if(activated()) return false;

#ifdef BETA_RELEASE
	if(0)
	{
		char buf[150] ;
		sprintf(buf, "[activate] Terrain model: %s id:%d", model(), (int)uid());
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_BETA, LSTR_GENERIC, UTF16(buf));
	}
#endif

	return (load()) ? ServerTrackedObject::activate(): false;
}

struct ScaleMatrix
{
    OBJECT* o;
    Vec3f scale;

    void Apply(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & OBJECT_NOTOBJECT))
            {
                if(o->numverts > 0)
                {
                    piMATRIX m = IdentityMatrix;

                    m._11 = scale.x();
                    m._22 = scale.y();
                    m._33 = scale.z();

                    piVectorPtr p = o->StaticPositions();

                    for(int i = 0; i < o->numverts; p++, i++)
                    {
                        math3dVectorMultiply(p.ptr, p.ptr, &m);
                    }
                }
            }

            Apply(o->child);
            Apply(o->sibling);
            Apply(o->attached);
        }
    }

    void DoScale()
    {
        Apply(o);
    }
};

bool TerrainSTO::load()
{
    extern void trnopSetSC(int);

	if(_to) return true; // we're already loaded

	piMATRIX transform = IdentityMatrix;

	piVECTOR attVec;
	attVec.x = attitude().x();
	attVec.y = attitude().y();
	attVec.z = attitude().z();

	mathEul_ToMatrix(&transform, EulOrdXYZr, &attVec);

    transform._41 = (FLOAT32)(m_position.x() - WorldOriginOffset.x);
    transform._42 = (FLOAT32)(m_position.y() - WorldOriginOffset.y);
    transform._43 = (FLOAT32)-(m_position.z() - WorldOriginOffset.z);

	PoolModel* masterModel = obmgrGetPoolModel(model());
	if(!masterModel) return false;

    int sx, sy, r, c;
    persGetMySupercellAndOctet(sx, sy, r, c);
    int sc = 12;

    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
    {
        if(!activeTerrainDetail[i])
            continue;

        if(trndtl[i].terrain == NULL)
            continue;

        if(trndtl[i].scx == sx && trndtl[i].scy == sy)
        {
            sc = i;
            break;
        }
    };

	_to = obmgrCreateModelInstance(model(), transform, true);
	if(!_to) return false;

	double slen2 = _scale.length2();

	// 0 length scale is bad data
	// 1 is no scale
    if(slen2 != 0 && slen2 != 1.0f)
    {
        ScaleMatrix scale;
        scale.o = _to;
        scale.scale = _scale;
        scale.DoScale();

        // Have to redo object bounding boxes.
        ldmdlProcessBounds(_to);
    }

	obmainSetFlag2InObjectTree(_to, OBJECT_TERRAIN_OBJECT);
	obmainSetFlag3InObjectTree(_to, OBJECT_STO);

	trndtl[sc].terrain->flags |= OBJECT_VISIBLE;
    trnopSetSC(sc);

    if(masterModel->IsStaticTerrainModel())
    {
	    obmgrHandleSpecialObjects(_to);

        if(masterModel->hastrees)
            trnopHandleSpeedTree(_to->child);
    }
	/*
    else
    {
	    // translate the object to its new position
        posxyz.x += (float)WorldOriginOffset.x;
        posxyz.y += (float)WorldOriginOffset.y;

        _to->trnum = sc;

	    if(!trnopOrientSubobjects(_to, &posxyz, true))
	    {
		    obpoolFreeObjectCallback(_to);
		    _to = NULL
	    }

        posxyz.x -= (float)WorldOriginOffset.x;
        posxyz.y -= (float)WorldOriginOffset.y;
    }
	*/

	preProcess();

	// add a TO object
	trncdAddTerrainObject(_to, 0, 0, 0, NULL);

	postProcess();

	// set some flags
	//int modelType = _to->modelInstance->GetModelType();

    // Force rebuild of inverse matrices.
	trncdInitSingleTerrainObject(trncdFindTerrainObject(_to->child));

	// make sure shadows get redone
    //_shadowmap.Reset();

	// make sure the collide lists get rebuilt right away
	trnCDUpdateCollideLists(true);

#ifdef BETA_RELEASE
	if(0)
	{
		char buf[150] ;
		sprintf(buf, "Added MSP terrain object %s at %.2f,%.2f,%.2f", _to->name, _to->mat._41, _to->mat._42, _to->mat._43);
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_BETA, LSTR_GENERIC, UTF16(buf));
	}
#endif

	/*
    trnopSetSC(12);

    // Ticket# 4701
    if(_to->modelInstance)
    {
        if(_to->modelInstance->GetModel()->hastrees)
        {
            static struct FindTree ft;

            ft.tree = NULL;
            ft.Find(_to->child);

            if(ft.tree)
            {
                obmainSetFlag2InThisTree(ft.tree, OBJECT_IS_ROTATED);
                trnopHandleSpeedTree(ft.tree);
            }
        }
    }
	*/

	return true;
}

void TerrainSTO::deactivate()
{
	if(!activated()) return;

#ifdef BETA_RELEASE
	if(0)
	{
		char buf[150] ;
		sprintf(buf, "[deactivate] Terrain model: %s id:%d", model(), (int)uid());
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_BETA, LSTR_GENERIC, UTF16(buf));
	}
#endif

	// kill terrain object
	if(_to)
	{
		trncdRemoveTerrainObject(_to, 0, 0, 0, NULL);
		obpoolFreeObjectCallback(_to);
		_to = NULL;
	}

	ServerTrackedObject::deactivate();
}

void TerrainSTO::postProcess()
{
	if(_to)
		_to->lpspare = this;
}

bool TerrainSTO::isCollidable()
{
	return (Online && !SCEN_MGR->IsActive() && age() < _collisionGrace) ? false: true;
}

void TerrainSTO::SetScale(float sx, float sy, float sz)
{
	_scale.set(sx, sy, sz);
}

void TerrainSTO::SetCountry(UINT32 country)
{
	if(!_to) return;

	OBJECT* o = _to;

	if(o->modelInstance)
    {
        OBJECT* oo = o->modelInstance->rootObject->child;

        while(oo)
        {
            trnCDTerrainObject* TO = trncdFindTerrainObject(oo);
            if(TO) trnSetCountry(TO, country);

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
                if(!oo) continue;

                oo = oo->child;

                while(oo)
                {
                    trnCDTerrainObject* TO = trncdFindTerrainObject(oo);
                    if(TO) trnSetCountry(TO, country);

                    oo = oo->sibling;
                }
            }
        }
    }
}

void TerrainSTO::ToggleDamage()
{
	if(!_to) return;

	OBJECT* o = _to;

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

