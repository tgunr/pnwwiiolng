#include <syscfg.h> // Path is project specific.
/*----------------------------------------------------------------------
Module: TransformEffect.cpp
Description:
-----------------------------------------------------------------------*/
#include "spt/Random.h"
#include "include/vars.h"

#include "fx/ParticleMgr.h"
#include "fx/TransformEffect.h"
#include "fx/EmitterMgr.h"
#include "veh/vehmain.h"
#include "trn/trnvars.h"
#include "math/math3d.h"
#include "ogrp/ogrptype.h"
#include "veh/vehanim.h"
#include "math/mathUnitConversion.h"
#include "gr2/gr2Anim.h"
#include "fx/fxWeather.h"
#include "ob/obdraw.h"
#include "pers/persmain.h"
#include "ml/mlCompon.h"
#include "ob/obMgr.h"
#include "fp/fpFPS.h"

#define MAX_TRANSFORMS  4096

static ParticleMgr* _particlemgr = nullptr;

using TransformList = std::set<TransformEffect*>;
static TransformEffect* TxEffects = nullptr;

static TransformList updatelist;
static TransformList removelist;

#ifdef BETA_RELEASE
#include "ui/uiChat.h"
static bool dontdrawfx = false;
#endif

#include <stack>
static std::stack<TransformEffect*> FreeTransforms;

#ifdef BETA_RELEASE
/*------------------------------------------------------------------------------
Function: DisableParticleDraw
Description:
-------------------------------------------------------------------------------*/
void DisableParticleDraw(bool draw)
{
    dontdrawfx = draw;
    uiChatPrintLine("Drawing FX %s", dontdrawfx ? "DISABLED" : "ENABLED");
}
#endif

/*------------------------------------------------------------------------------
Function: RainCheck
Description:
-------------------------------------------------------------------------------*/
static void RainCheck(TransformEffect* te)
{
    if(fxWeatherIsRaining())
        te->enable(false);
    else
    {
        if(!te->isEnabled())
        {
            te->enable(true);
            te->enableemitters(true);
        }
    }
}

/*------------------------------------------------------------------------------
Function: GetParticleMgr
Description:
-------------------------------------------------------------------------------*/
ParticleMgr* TransformEffect::GetParticleMgr()
{
    return _particlemgr;
}

/*------------------------------------------------------------------------------
Function: GetFreeTransform
Description:
-------------------------------------------------------------------------------*/
static TransformEffect* GetFreeTransform()
{
    if(FreeTransforms.empty())
    {
        return new TransformEffect;
    }

    TransformEffect* te = FreeTransforms.top();
    FreeTransforms.pop();

    te->reset();

    return te;
}

/*------------------------------------------------------------------------------
Function: FreeTransform
Description:
-------------------------------------------------------------------------------*/
static void FreeTransform(TransformEffect* te)
{
    te->releaseemitters();
    te->releaseparticles();

    FreeTransforms.push(te);
}

/*----------------------------------------------------------------------
Function: InitializeFxSystem
Description:
----------------------------------------------------------------------*/
void InitializeFxSystem()
{
    DataFxInit();
    EmitterMgrInit();

    _particlemgr = new ParticleMgr;
    _particlemgr->init(true);

    TxEffects = new TransformEffect[MAX_TRANSFORMS];

    for(int i = 0; i < MAX_TRANSFORMS; i++)
    {
        FreeTransform(&TxEffects[i]);
    }
}

/*----------------------------------------------------------------------
Function: DrawFxSystemDeprecated
Description:
----------------------------------------------------------------------*/
bool DrawFxSystemDeprecated(float dist2)
{
#ifdef BETA_RELEASE
    if(dontdrawfx)
    {
        return false;
    }
#endif

    if(obdrawIsRenderPass(RENDER_SCENE))
        return _particlemgr->drawdeprecated(dist2);

    return false;
}

/*----------------------------------------------------------------------
Function: DrawFxSystem
Description:
----------------------------------------------------------------------*/
bool DrawFxSystem(float dist2)
{
#ifdef BETA_RELEASE
    if(dontdrawfx)
    {
        return false;
    }
#endif

    if(obdrawIsRenderPass(RENDER_SCENE))
        return _particlemgr->draw(dist2);

    return false;
}

/*----------------------------------------------------------------------
Function: DrawSurfaceFx
Description: Special case particles for water surface effects
----------------------------------------------------------------------*/
void DrawSurfaceFx()
{
#ifdef BETA_RELEASE
    if(dontdrawfx)
    {
        return;
    }
#endif

    if(!obdrawIsRenderPass(RENDER_SCENE))
        _particlemgr->drawsurface();
}

/*----------------------------------------------------------------------
Function: DrawSurfaceFxDeprecated
Description: Special case particles for water surface effects
----------------------------------------------------------------------*/
void DrawSurfaceFxDeprecated()
{
#ifdef BETA_RELEASE
    if(dontdrawfx)
    {
        return;
    }
#endif

    if(obdrawIsRenderPass(RENDER_SCENE))
        _particlemgr->drawsurfacedeprecated();
}

/*----------------------------------------------------------------------
Function: CullFxSystem
Description:
----------------------------------------------------------------------*/
void CullFxSystem()
{
    if(_particlemgr)
    {
        _particlemgr->cleardraws();
        _particlemgr->cull();
    }
}

/*----------------------------------------------------------------------
Function: KillFxSystem
Description:
----------------------------------------------------------------------*/
void KillFxSystem()
{
    for(TransformList::iterator itr = updatelist.begin(); itr != updatelist.end(); ++itr)
    {
      FreeTransform(*itr);
    }
    updatelist.clear();

    EmitterMgrKill();

    if(_particlemgr)
    {
        delete _particlemgr;
        _particlemgr = nullptr;
    }

    DataFxKill();

    if(TxEffects)
    {
        delete[] TxEffects;
        TxEffects = nullptr;
    }

    Write_Log(LOG_OLD, LOG_INFO, "[KillFxSystem]");
}

/*----------------------------------------------------------------------
Function: UpdateFxSystem
Description:
----------------------------------------------------------------------*/
void UpdateFxSystem()
{
#ifdef DEBUG_WW2
    static bool noupdate = false;

    if(noupdate)
        return;
#endif

    if(!_particlemgr)
        return;

    if(!TxEffects)
        return;

    logTimerBeginMacro();

    _particlemgr->update();

    TransformList::iterator itr;

    TransformEffect* te = nullptr;

    for(itr = removelist.begin(); itr != removelist.end(); ++itr)
    {
        te = *itr;
        updatelist.erase(*itr);

        FreeTransform(te);
    }

    removelist.clear();

    for(itr = updatelist.begin(); itr != updatelist.end(); ++itr)
    {
        (*itr)->update();
    }

#if BETA_RELEASE
    // Check for xml file changes for dynamic debug of effects
    FxGet().update();
#endif

    logTimerEndMacro();
}

/*------------------------------------------------------------------------------
Function: TransformEffect::TransformEffect
Description:
-------------------------------------------------------------------------------*/
TransformEffect::TransformEffect()
{
    reset();
}

/*------------------------------------------------------------------------------
Function: TransformEffect::reset
Description:
-------------------------------------------------------------------------------*/
void TransformEffect::reset()
{
    _flags = 0;
    _lifetime = 0;
    _createtime = frametime;
    _damagelevel = 0;
    _gravcoef = 0.0f;
    _dragcoef = 0.0f;
    _dir = Vec3f(0, 0, -1);
    _isenabled = false;
    _stratobj = nullptr;
    _emittername = "noname";
    _effectname = "noname";
    _txobject = nullptr;
    _to = nullptr;
    _weapammo = nullptr;
    _dead = false;
    _lastwooffset = Vec3d(WorldOriginOffset);
    _state = nullptr;
    _enablestatevalue = 0;
    _lastdamagelevel = -1;
    _vehicle = nullptr;
    _updatecallback = nullptr;
}

/*----------------------------------------------------------------------
Function: TransformEffect::releaseemitters
Description:
-----------------------------------------------------------------------*/
void TransformEffect::releaseemitters()
{
    for(unsigned int i = 0; i < _emitters.size(); i++)
    {
        if(_flags & USER_CREATED_EFFECT)
        {
            _emitters[i]->releaseparticleeffect();
        }

        EmitterMgrFree(_emitters[i]);
    }

    _emitters.clear();
}

/*----------------------------------------------------------------------
Function: TransformEffect::releaseparticles
Description:
-----------------------------------------------------------------------*/
void TransformEffect::releaseparticles()
{
    for(unsigned int i = 0; i < _particles.size(); i++)
    {
        _particles[i]->_dead = true;
    }

    _particles.clear();
}

/*----------------------------------------------------------------------
Function: TransformEffect::~TransformEffect()
Description:
-----------------------------------------------------------------------*/
TransformEffect::~TransformEffect()
{
    releaseemitters();
    releaseparticles();
}

/*----------------------------------------------------------------------
Function: TransformEffect::enable
Description:
-----------------------------------------------------------------------*/
void TransformEffect::enable(bool enable)
{
    _isenabled = enable;
}

/*----------------------------------------------------------------------
Function: TransformEffect::enableemitters
Description:
-----------------------------------------------------------------------*/
void TransformEffect::enableemitters(bool enable)
{
    for(unsigned int i = 0; i < _emitters.size(); i++)
    {
        _emitters[i]->setenable(enable);
        _emitters[i]->setfirstrun(enable);
    }
}

/*----------------------------------------------------------------------
Function: TransformEffect::setstratobj
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::setstratobj(const StratDB::StratObject* so)
{
    for(unsigned int i = 0; i < _emitters.size(); i++)
    {
        _emitters[i]->_stratobj = so;
    }
}

/*----------------------------------------------------------------------
Function: TransformEffect::setstate
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::setstate(unsigned int* state)
{
    _state = state;
}

/*----------------------------------------------------------------------
Function: TransformEffect::setvehicle
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::setvehicle(const vehVehicle* v)
{
    _vehicle = v;
}

/*----------------------------------------------------------------------
Function: TransformEffect::setenablestatevalue
Description: Enable state at this value
-----------------------------------------------------------------------*/
void  TransformEffect::setenablestatevalue(unsigned int value)
{
    _enablestatevalue = value;
}

/*----------------------------------------------------------------------
Function: TransformEffect::setweapammo
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::setweapammo(const weapAmmo* weapammo)
{
    _weapammo = weapammo;
}

/*----------------------------------------------------------------------
Function: TransformEffect::settoobj
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::settoobj(const trnCDTerrainObject* to)
{
    _to = to;
}

/*----------------------------------------------------------------------
Function: TransformEffect::setdamagelevel
Description:
-----------------------------------------------------------------------*/
void  TransformEffect::setdamagelevel(int damagelevel)
{
    _damagelevel = damagelevel;
}

/*----------------------------------------------------------------------
Function: ApplyFuelFire
Description:
-----------------------------------------------------------------------*/
void ApplyFuelFire(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_FUELTANKS; i++)
    {
        if(!v->StateAnimation.FuelFireEmitter[i])
            continue;

        te = TransformEffect::AddEffect("fuelsparks", Vec3f(0, 0, 0), 0, v->StateAnimation.FuelFireEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.FuelFireOn[i]);
            te->setvehicle(v);
        }

        if(v->VehicleType->Type == VEH_TYPE_SEA)
        {
            te = TransformEffect::AddEffect("shipfuelfire1", Vec3f(0, 0, -1), 0, v->StateAnimation.FuelFireEmitter[i]);
        }
        else
        {
            te = TransformEffect::AddEffect("fuelfire1", Vec3f(0, 0, 0), 0, v->StateAnimation.FuelFireEmitter[i]);
        }

        if(te)
        {
            te->setstate(&v->StateAnimation.FuelFireOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyWingVortex
Description:
-----------------------------------------------------------------------*/
void ApplyWingVortex(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < 2; i++)
    {
        if(v->StateAnimation.WingtipVortexEmitter[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("wingtipvortex", Vec3f(0, 0, 0), 0, v->StateAnimation.WingtipVortexEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.WingtipVortexSmokeOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyGlycolLeak
Description:
-----------------------------------------------------------------------*/
void ApplyGlycolLeak(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_ENGINES; i++)
    {
        for(int j = 0; j < ML_MAX_RADIATORS_PER_ENGINE; j++)
        {
            if(v->StateAnimation.GlycolLeakSmokeEmitter[i][j] == nullptr)
                continue;

            te = TransformEffect::AddEffect("glycolleak1", Vec3f(0, 0, 0), 0, v->StateAnimation.GlycolLeakSmokeEmitter[i][j]);

            if(te)
            {
                te->setstate(&v->StateAnimation.GlycolLeakSmokeOn[i][j]);
                te->setvehicle(v);
            }
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyOilLeak
Description:
-----------------------------------------------------------------------*/
void ApplyOilLeak(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_ENGINES; i++)
    {
        if(v->StateAnimation.OilLeakSmokeEmitter[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("oilleak1", Vec3f(0, 0, 0), 0, v->StateAnimation.OilLeakSmokeEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.OilLeakSmokeOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyExhaust
Description:
-----------------------------------------------------------------------*/
void ApplyExhaust(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_ENGINES; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            if(v->StateAnimation.ExhaustSmokeEmitter[i][j] == nullptr)
                continue;

            te = TransformEffect::AddEffect("exhaust1", Vec3f(0, 0, 0), 0, v->StateAnimation.ExhaustSmokeEmitter[i][j]);

            if(te)
            {
                te->setstate(&v->StateAnimation.ExhaustSmokeOn[i]);
                te->setvehicle(v);
            }
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyFuelLeak
Description:
-----------------------------------------------------------------------*/
void ApplyFuelLeak(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_FUELTANKS; i++)
    {
        if(v->StateAnimation.FuelLeakSmokeEmitter[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("fuelleak1", Vec3f(0, 0, 0), 0, v->StateAnimation.FuelLeakSmokeEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.FuelLeakSmokeOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyEngineFire
Description:
-----------------------------------------------------------------------*/
void ApplyEngineFire(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < ML_MAX_ENGINES; i++)
    {
        if(v->StateAnimation.EngineFireEmitter[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("enginefire1", Vec3f(0, 0, 0), 0, v->StateAnimation.EngineFireEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.EngineFireOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyTankFire
Description:
-----------------------------------------------------------------------*/
void ApplyTankFire(struct vehVehicle* v)
{
    TransformEffect* te;

    char s[128];

    int num = vehAnimFindVehicleStateNumber(v, "tankfire");

    if(num == -1)
        return;

    for(int i = 0; i < VEH_MAX_FIREPOINTS; i++)
    {
        strprintf(s, "tfire%d", i+1);

        ogrpFindSubObjectByName(&v->StateAnimation.TankFireEmitterPoint[i],
						        v->VehicleObjectData->EffectsObject,
						        s);

        if(v->StateAnimation.TankFireEmitterPoint[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("tankfire", Vec3f(0, 0, -1), 0, v->StateAnimation.TankFireEmitterPoint[i]);

        if(te)
        {
            te->setstate(&v->PosData.States[num]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplySpecial
Description: Catch all for other states which do not follow the state rules
-----------------------------------------------------------------------*/
void ApplySpecial(struct vehVehicle* v)
{
    TransformEffect* te;

    static WEAPON_SPEC tank = NULL_WEAP_SPEC;
    static WEAPON_SPEC truck = NULL_WEAP_SPEC;

    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        tank = weapCreateSpec("GV_tank.1");
        truck = weapCreateSpec("GV_truck.1");
    }

    if(v->VehicleType->Type == VEH_TYPE_SEA)
    {
        const char* screws[2] =
        {
            "lscrew",
            "rscrew"
        };

        OBJECT* o = nullptr;

        // Ship screws
        for(int i = 0; i < 2; i++)
        {
            ogrpFindSubObjectByName(&o, v->VehicleObjectData->EffectsObject, screws[i]);

            if(!o)
                break;

            te = TransformEffect::AddEffect("boattrail", Vec3f(0, 0, 0), 0, o);

            if(te)
            {
                // Need to use this for screws too...since we don't have one for screws
                te->setstate(&v->StateAnimation.TreadDustOn[i]);
                te->setvehicle(v);
            }
        }
    }
    else if(v->VehicleType->Type == VEH_TYPE_GROUND && v->VehicleType->WeaponSpec.wClass == tank.wClass)
    {
        for(int i = 0; i < VEH_MAX_TREAD_DUST; i++)
        {
            if(v->StateAnimation.TreadDustEmitter[i] == nullptr)
                continue;

            if(!strncmp(v->StateAnimation.TreadDustEmitter[i]->name, "tdustl1", 7) ||
               !strncmp(v->StateAnimation.TreadDustEmitter[i]->name, "tdustr1", 7))
            {
                te = TransformEffect::AddEffect("tanktrack", Vec3f(0, 0, 0), 0, v->StateAnimation.TreadDustEmitter[i]);

                if(te)
                {
                    te->setstate(&v->StateAnimation.TreadDustOn[i]);
                    te->setvehicle(v);

                    ParticleEffect* pe = FxGet().cloneparticleeffect("tanktrackparticle");

                    static float size = 0.2f;

                    pe->setsize(size, size, size, size);
                    pe->separation = size * 1.5f;

                    te->getemitter(0)->setparticleeffect(pe);
                }
            }
        }
    }
    else if(v->VehicleType->Type == VEH_TYPE_GROUND && v->VehicleType->WeaponSpec.wClass == truck.wClass)
    {
        for(int i = 0; i < VEH_MAX_TREAD_DUST; i++)
        {
            if(v->StateAnimation.TreadDustEmitter[i] == nullptr)
                continue;

            if(!strncmp(v->StateAnimation.TreadDustEmitter[i]->name, "tdustl1", 7) ||
               !strncmp(v->StateAnimation.TreadDustEmitter[i]->name, "tdustr1", 7))
            {
                te = TransformEffect::AddEffect("tiretrack", Vec3f(0, 0, 0), 0, v->StateAnimation.TreadDustEmitter[i]);

                if(te)
                {
                    te->setstate(&v->StateAnimation.TreadDustOn[i]);
                    te->setvehicle(v);

                    ParticleEffect* pe = FxGet().cloneparticleeffect("tiretrackparticle");

                    const OBJECT* o = v->VehicleObjectData->LODObjects[0];
                    FindByComment<OBJECT> fbc;
                    fbc.set("wrotate");
                    fbc.find(o);

                    float size = 0.2f;

                    if(fbc.node)
                    {
                        o = ((OBJECT*)(fbc.node))->child;

                        if(o)
                            size = o->bbox[1].x;

                        if(size < 0.2f)
                            size = 0.2f;
                    }

                    pe->setsize(size, size, size, size);
                    pe->separation = size * 1.5f;

                    te->getemitter(0)->setparticleeffect(pe);
                }
            }
        }
    }
}


/*----------------------------------------------------------------------
Function: ApplyTreadDust
Description:
-----------------------------------------------------------------------*/
void ApplyTreadDust(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(v->StateAnimation.TreadDustEmitter[i] == nullptr)
            continue;

        if(v->VehicleType->Type == VEH_TYPE_SEA)
        {
            te = TransformEffect::AddEffect("water_treaddust", Vec3f(0, 0, 0), 0, v->StateAnimation.TreadDustEmitter[i]);
        }
        else
        {
            te = TransformEffect::AddEffect("treaddust", Vec3f(0, 0, 0), 0, v->StateAnimation.TreadDustEmitter[i]);

            if(te)
                te->setupdatecallback(RainCheck);
        }

        if(te)
        {
            te->setstate(&v->StateAnimation.TreadDustOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: ApplyDebris
Description:
-----------------------------------------------------------------------*/
void ApplyDebris(struct vehVehicle* v)
{
    TransformEffect* te;

    for(int i = 0; i < VEH_MAX_DEBRIS_POINTS; i++)
    {
        if(v->StateAnimation.DebrisEmitter[i] == nullptr)
            continue;

        te = TransformEffect::AddEffect("manydebris", Vec3f(0, 0, 0), 0, v->StateAnimation.DebrisEmitter[i]);

        if(te)
        {
            te->setstate(&v->StateAnimation.DebrisOn[i]);
            te->setvehicle(v);
        }
    }
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::Apply
Description:
-----------------------------------------------------------------------*/
void TransformEffect::Apply(struct vehVehicle* vehicle)
{
    ApplyFuelFire(vehicle);
    ApplyWingVortex(vehicle);
    ApplyGlycolLeak(vehicle);
    ApplyOilLeak(vehicle);
    ApplyExhaust(vehicle);
    ApplyFuelLeak(vehicle);
    ApplyEngineFire(vehicle);
    ApplyTankFire(vehicle);
    ApplyTreadDust(vehicle);
    ApplyDebris(vehicle);
    ApplySpecial(vehicle);
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::Apply
Description:
-----------------------------------------------------------------------*/
void TransformEffect::Apply(struct weapAmmo* weapammo)
{
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::Apply
Description:
-----------------------------------------------------------------------*/
void TransformEffect::Apply(struct trnAutoGun* autogun)
{
    if(autogun->FirePnt)
    {
        if(autogun->Weapon->WeaponType->muzzleflash[0])
        {
            TransformEffect* te = nullptr;

            te = TransformEffect::AddEffect(autogun->Weapon->WeaponType->muzzleflash, Vec3f(0, 0, 0), 0, autogun->FirePnt);

            if(te)
            {
                te->setstate(&autogun->GunIsFiring);
            }
        }
    }
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::Apply
Description:
-----------------------------------------------------------------------*/
void TransformEffect::Apply(struct gr2AnimTrooper* trooper, struct OBJECT* o)
{
    if(!trooper)
    {
        return;
    }

    TransformEffect* te = TransformEffect::AddEffect("trooperwatertrail", Vec3f(0, 0, 0), 0, o);
    if(te)
    {
        te->setstate((unsigned int*)&trooper->currentPose);
        te->setenablestatevalue(TPS_POSETYPE_SWIM);
    }
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::Apply
Description:
-----------------------------------------------------------------------*/
void TransformEffect::Apply(struct trnCDTerrainObject* to)
{
#define WANT_ALL_EMITTERS   1

#if WANT_ALL_EMITTERS

    for(int i = 0; i < TRN_MAX_LOD; i++)
    {
        if(to->LOD[i] == nullptr)
        {
            continue;
        }

        if(!(to->LOD[i]->flag2 & OBJECT_HAS_FX))
        {
            continue;
        }

        OBJECT* o = to->LOD[i];

        while(o->child)
        {
            o = o->child;
        }

        // Add transform for this TO object for a specific effect
        while(o)
        {
            o->flag2 |= OBJECT_HIDDEN;

            if(o->name[0] != 'd' && o->name[0] != 'i')
            {
                o = o->sibling;
                continue;
            }

            if(!gCustomTerrain && !strncmp(o->name, "d_smoke", 7))
            {
                TransformEffect* te = TransformEffect::AddEffect("buildingdamagesmoke", Vec3f(0, 0, -1), 0, o);

                if(te)
                {
                    te->settoobj(to);
                    te->setdamagelevel(1);
					te->setstate(&WantCombatSmoke);
                }
            }

            if(!strncmp(o->name, "i_smoke", 7))
            {
                if(o->comment)
                {
                    char* p = strstr(o->comment, "effect=");

                    if(p)
                    {
                        p += 7;

						TransformEffect* te = TransformEffect::AddEffect(p, Vec3f(0, 0, -1), 0, o);
                        if(te)
                        {
                            te->settoobj(to);
                            te->setdamagelevel(0);
                        }
                    }
                }
            }

            o = o->sibling;
        }
    }
#else

    for(int i = 0; i < TRN_MAX_LOD; i++)
    {
        if(to->LOD[i] == nullptr)
        {
            continue;
        }

        if(!(to->LOD[i]->flag2 & OBJECT_HAS_FX))
        {
            continue;
        }

        OBJECT* o = to->LOD[i];

        while(o->child)
        {
            o = o->child;
        }

        OBJECT* o2 = o;

        int immersioncount = 0;
        int damagecount = 0;

        while(o2)
        {
            o2->flag2 |= OBJECT_HIDDEN;

            if(o2->name[0] == 'd')
                damagecount++;

            o2 = o2->sibling;
        }

        o2 = o;

        piVECTOR pos;
        math3dVectorMultiply(&pos, &o2->pos, &o2->mat);

        unsigned int dseed = 0;

        if(damagecount)
        {
	        dseed = (unsigned int)(9133 * (pos.x - (int)pos.x) + 15137 * (pos.y - (int)pos.y)) % damagecount;
        }

        int donecount = 0;

        if(damagecount == 0)
            donecount++;

        damagecount = 0;

        // Add transform for this TO object for a specific effect
        // Only choose one damage effect
        while(o2 && donecount < 1)
        {
            o2->flag2 |= OBJECT_HIDDEN;

            if(o2->name[0] != 'd')
            {
                o2 = o2->sibling;
                continue;
            }

            if(!gCustomTerrain && !strncmp(o2->name, "d_smoke", 7))
            {
                if(dseed == damagecount)
                {
                    donecount++;

                    TransformEffect* te = TransformEffect::AddEffect("buildingdamagesmoke", Vec3f(0, 0, -1), 0, o2);

                    if(te)
                    {
                        te->settoobj(to);
                        te->setdamagelevel(1);
                    }
                }

                damagecount++;
            }
        }

        o2 = o;
        // Need all immersion effects
        while(o2)
        {
            if(o2->name[0] != 'i')
            {
                o2 = o2->sibling;
                continue;
            }

            if(!strncmp(o2->name, "i_smoke", 7))
            {
                if(o2->comment)
                {
                    char* p = strstr(o->comment, "effect=");

                    if(p)
                    {
                        p += 7;
                    }

                    if(p)
                    {
                        TransformEffect* te = nullptr;
                        te = TransformEffect::AddEffect(p, Vec3f(0, 0, -1), 0, o2);

                        if(te)
                        {
                            te->settoobj(to);
                            te->setdamagelevel(0);
                        }
                    }
                }
            }

            o2 = o2->sibling;
        }
    }
#endif
}

/*----------------------------------------------------------------------
Function: TrasnsformEffect::AddTracer
Description:
-----------------------------------------------------------------------*/
TransformEffect* TransformEffect::AddTracer(const weapAmmo* ammo, const char* effect)
{
    TransformEffect* te = TransformEffect::AddEffect(effect, Vec3f(float(ammo->Velocity.x), float(ammo->Velocity.y), float(-ammo->Velocity.z)), 0, nullptr);
	if(te)
    {
        te->setweapammo(ammo);
        te->enableemitters(true);;
    }

    return te;
}

/*----------------------------------------------------------------------
Function: TransformEffect::AddEffect
Description:
-----------------------------------------------------------------------*/
TransformEffect* TransformEffect::AddEffect(const char* effect,
                                            const Vec3f& n,
                                            int lifetime,
                                            struct OBJECT* state)
{
    TransformFx* fx = FxGet().gettransformeffect(effect);

    if(!fx)
    {
        return nullptr;
    }

    TransformEffect* te = nullptr;

    if(fx->flags & transform_is_standalone)
    {
        te = AddStandAloneEffect(effect, n, lifetime);
        te->enable(true);

        return te;
    }

    te = GetFreeTransform();

    if(!te)
    {
        return nullptr;
    }

    if(te->addemittereffect(effect, n, lifetime, state))
    {
        te->enable(true);
        return te;
    }
    else
    {
        FreeTransform(te);
    }

    return nullptr;
}

/*----------------------------------------------------------------------
Function: TransformEffect::AddStandAloneEffect
Description: Transform not attached to any object/ammo etc...
-----------------------------------------------------------------------*/
TransformEffect* TransformEffect::AddStandAloneEffect(const char* effect,
                                                      const Vec3f& normal,
                                                      int lifetime)
{
    TransformEffect* te = GetFreeTransform();

    if(!te)
    {
        return nullptr;
    }

    if(te->addemittereffect(effect, Vec3f(0, 0, 0), lifetime, nullptr))
    {
        TransformFx* fx = FxGet().gettransformeffect(effect);

        te->setflag(TransformEffect::STAND_ALONE);
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->_dragcoef = fx->drag;
        te->_gravcoef = fx->gravity;

        Vec3f n = normal;
        Vec3f velocity = normal;

        if(fx->flags & transform_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomdirection) * 0.5f;

                float dist = GetRandom(0.0f, 1.0f); // Distance between base and tip
                float theta = GetRandom(0.0f, 1.0f) * 2.0f * PI2; // Angle around axis
                // Distance from axis
                float r = 0.0f + GetRandom(0.0f, 1.0f) * radius;

                // Another way to do this is to choose a random point in a square and keep it if it's in the circle.
                float x = r * cosf(theta);
                float y = r * sinf(theta);

                // Scale radius along axis for cones
                x *= dist;
                y *= dist;

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                if(fabsf(basis * normal) > 0.999f)
                {
                    basis.x() = 0.0f;
                    basis.y() = 1.0f;
                    basis.z() = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - n * dot;
                u.fastnormalize();

                Vec3f v = normal ^ u;

                n = normal * dist + u * x + v * y;
            }
            else
            {
                n.x() = GetRandom(-1.0f, 1.0f);
                n.y() = GetRandom(-1.0f, 1.0f);
                n.z() = GetRandom(-1.0f, 1.0f);
            }

            n.fastnormalize();
            velocity = n;
        }

        te->_velocity = velocity;

        if(fx->speed > 0)
            te->_velocity *= fx->speed;

        te->enable(true);

        return te;
    }
    else
    {
        FreeTransform(te);
    }

    return nullptr;
}

/*----------------------------------------------------------------------
Function: TransformEffect::CreateEffect
Description:
-----------------------------------------------------------------------*/
TransformEffect* TransformEffect::CreateEffect(const char* effect,
                                               const Vec3f& n,
                                               int lifetime,
                                               struct OBJECT* state,
                                               int numemitters)
{
    TransformEffect* te = GetFreeTransform();

    if(!te)
    {
        return nullptr;
    }

    if(te->createemittereffect(effect, n, lifetime, state, numemitters))
    {
        te->setflag(TransformEffect::USER_CREATED_EFFECT);
        te->enable(true);
        return te;
    }
    else
    {
        FreeTransform(te);
    }

    return nullptr;
}

/*----------------------------------------------------------------------
Function: TransformEffect::createemittereffect
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::createemittereffect(const char* effect,
                                          const Vec3f& n,
                                          int lifetime,
                                          struct OBJECT* state,
                                          int numemitters)
{
    _effectname = effect;
    _txobject = state;

    for(int i = 0; i < numemitters; i++)
    {
        Emitter* emitter = EmitterMgrGetEmitter();
        emitter->reset();
        emitter->link(_particlemgr);

        emitter->setupemitter(n, lifetime);

        _emitters.push_back(emitter);
    }

    _lastupdate = frametime + 1;
    updatelist.insert(this);

    return true;
}

/*----------------------------------------------------------------------
Function: TransformEffect::allocateparticles
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::allocateparticles(const char* effect, int numparticles)
{
    ParticleEffect* fx = FxGet().getparticleeffect(effect);

    if(!fx)
    {
        return nullptr;
    }

    Particle2* part = nullptr;

    unsigned int q;

    for(int i = 0; i < numparticles; i++)
    {
        part = _particlemgr->getparticle();

        if(!part)
        {
            return nullptr;
        }

        link(part);

        part->_textureindex = fx->texture[0];

        for(q = 0; q < fx->alpha.size(); q++)
            part->_alpha[q] = fx->alpha[q];

        for(q = 0; q < fx->size.size(); q++)
            part->_size[q] = fx->size[q];

        for(q = 0; q < fx->time.size(); q++)
            part->_time[q] = fx->time[q];

        part->_lifetime = fx->lifetime;

        part->_material = fx->matid;
        part->_stretchscale = fx->stretchscale;
        part->_stretchlength = fx->stretchscale;
        part->_flags = fx->particleflags;
    }

    return true;
}

/*----------------------------------------------------------------------
Function: TransformEffect::addtracer
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::addtracer(const weapAmmo* ammo)
{
    ParticleEffect* fx = FxGet().getparticleeffect("tracerparticle");

    if(!fx)
    {
        return nullptr;
    }

    if(fx->number.empty())
    {
        return nullptr;
    }

    Particle2* part = nullptr;

    unsigned int size = fx->number.size();

    int num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    unsigned int q;

    for(int i = 0; i < num; i++)
    {
        part = _particlemgr->getparticle();

        if(!part)
        {
            return nullptr;
        }

        _particles.push_back(part);

        _weapammo = ammo;

        Matrixd m;

        Vec3d p;
        p.set(_weapammo->Position);
        p.z() = -p.z();

        m = Matrixd::translate(p);

        part->_lastposition = Vec3d(m(3, 0), m(3, 1), m(3, 2));
        part->_position = Vec3d(m(3, 0), m(3, 1), m(3, 2));
        part->_originalposition = Vec3d(m(3, 0), m(3, 1), m(3, 2));
        part->_textureindex = fx->texture[0];

        for(q = 0; q < fx->alpha.size(); q++)
            part->_alpha[q] = fx->alpha[q];

        for(q = 0; q < fx->size.size(); q++)
            part->_size[q] = fx->size[q];//ammo->AmmoType->Diameter * 10.0f;

        for(q = 0; q < fx->time.size(); q++)
            part->_time[q] = fx->time[q];

        part->_lifetime = fx->lifetime;

        part->_material = fx->matid;
        part->_stretchscale = fx->stretchscale;
        part->_stretchlength = fx->stretchscale;
        part->_flags = fx->particleflags | part_is_beam;
        part->_velocity.set(float(ammo->Velocity.x), float(ammo->Velocity.y), float(-ammo->Velocity.z));
        part->_velocity.fastnormalize();
        part->_velocity *= fx->speed;
    }

    _weapammo = ammo;
    _effectname = "tracer";

    _lastupdate = frametime + 1;
    updatelist.insert(this);

    return true;
}

/*----------------------------------------------------------------------
Function: TransformEffect::addemittereffect
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::addemittereffect(const char* effect,
                                       const Vec3f& n,
                                       int lifetime,
                                       struct OBJECT* state)
{
    TransformFx* ee = FxGet().gettransformeffect(effect);

    if(!ee)
    {
        return false;
    }

    if(ee->_emitters.empty())
    {
        return false;
    }

    _effectname = effect;
    _txobject = state;

    for(unsigned int i = 0; i < ee->_emitters.size(); i++)
    {
        Emitterfx* e = ee->_emitters[i];

        if(e->_particleeffectnames.empty())
        {
            continue;
        }

        // Each particle effect has an emitter
        for(unsigned int j = 0; j < e->_particleeffectnames.size(); j++)
        {
            ParticleEffect* pe = FxGet().getparticleeffect(e->_particleeffectnames[j].c_str());

            if(!pe)
                continue;

            Emitter* emitter = EmitterMgrGetEmitter();
            emitter->reset(e);
            emitter->link(_particlemgr);

            if(!emitter->setupemitter(pe, n, lifetime ? lifetime : e->lifetime))
            {
                continue;
            }

            _emitters.push_back(emitter);
        }
    }

    if(_emitters.empty())
        return false;

    _lastupdate = frametime + 1;
    updatelist.insert(this);

    return true;
}

/*----------------------------------------------------------------------
Function: ApplyDrag
Description:
-----------------------------------------------------------------------*/
void ApplyDrag(Vec3f& vel, float coef, float time)
{
    if(coef == 0.0f)
    {
        return;
    }

    float magnitude;
    float speed;

    magnitude = math3dFastSqrt(vel.length2());

    // Fast exponent
    speed = magnitude * (float)exp(-coef * time);

    if(magnitude > 0.00001f || magnitude < -0.00001f)
    {
        vel = (vel / magnitude) * speed;
    }
    else
    {
        vel.set(0, 0, 0);
    }
}

/*----------------------------------------------------------------------
Function: ApplyGravity
Description:
-----------------------------------------------------------------------*/
void ApplyGravity(Vec3f& vel, float coef, float time)
{
    if(coef != 0.0f)
    {
        vel.z() += coef * time;
    }
}

/*----------------------------------------------------------------------
Function: TransformEffect::validate
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::validate()
{
    bool success = true;

    if(_vehicle)
    {
        if(_vehicle->InUse == false)
        {
            removelist.insert(this);
            success = false;
        }
    }

    if(success && _txobject)
    {
        if(!obmainValidateObject(_txobject))
        {
            removelist.insert(this);
            success = false;
        }
    }

    if(success && _to)
    {
        if(_to->InUse == 0)
        {
            removelist.insert(this);
            success = false;
        }
    }

    if(success && _weapammo)
    {
        if(_weapammo->inuse == false)
        {
            removelist.insert(this);
            success = false;
        }
    }

    return success;
}

/*----------------------------------------------------------------------
Function: TransformEffect::updatetransform
Description:
-----------------------------------------------------------------------*/
void TransformEffect::updatetransform()
{
    if(_txobject)
    {
        if(_txobject->flags & OBJECT_DOF)
        {
            _transform.set(_txobject->mat);

            _transform = Matrixf(_txobject->dof->dofmatrix2) * _transform;

            _transform(3, 0) += _lastwooffset.x();
            _transform(3, 1) += _lastwooffset.y();
            _transform(3, 2) += _lastwooffset.z();
        }
        else
        {
            piVECTOR p = _txobject->pos;

            _transform.set(_txobject->mat);

            _transform(3, 0) = _transform(0, 0) * p.x + _transform(1, 0) * p.y + _transform(2, 0) * p.z + _transform(3, 0);
            _transform(3, 1) = _transform(0, 1) * p.x + _transform(1, 1) * p.y + _transform(2, 1) * p.z + _transform(3, 1);
            _transform(3, 2) = _transform(0, 2) * p.x + _transform(1, 2) * p.y + _transform(2, 2) * p.z + _transform(3, 2);

            _transform(3, 0) += _lastwooffset.x();
            _transform(3, 1) += _lastwooffset.y();
            _transform(3, 2) += _lastwooffset.z();
        }
    }
    else if(_to)
    {
        Vec3d p;
        p.set(_to->Center);

        p += _lastwooffset;

        _transform = Matrixd::translate(p);
    }
    else if(_weapammo)
    {
        Vec3d p;
        p.set(_weapammo->Position);
        p.z() = -p.z();

        _transform = Matrixd::translate(p);
    }
    else if(_flags & STAND_ALONE)
    {
        Vec3d pos;

        pos.x() = _transform(3, 0);
        pos.y() = _transform(3, 1);
        pos.z() = _transform(3, 2);

        float time = float(frametime - _lastupdate) * 0.001f;

        pos += (_velocity * time);

        ApplyDrag(_velocity, _dragcoef, time);
        ApplyGravity(_velocity, _gravcoef, time);

        _transform(3, 0) = pos.x();
        _transform(3, 1) = pos.y();
        _transform(3, 2) = pos.z();
    }

    // Eliminates problems when world origin offset moves
    _lastwooffset = Vec3d(WorldOriginOffset);
}

/*----------------------------------------------------------------------
Function: TransformEffect::checkstates
Description:
-----------------------------------------------------------------------*/
void TransformEffect::checkstates()
{
    if(_to)
    {
        if(_to->DamageLevel == _damagelevel)
        {
            bool enableemitter = true;

            for(unsigned int i = 0; i < _emitters.size(); i++)
            {
                if(!_emitters[i]->isenabled())
                {
                    if(_lastdamagelevel != _to->DamageLevel)
                    {
					    if(_state)
					    {
						    if(!(*_state))
						    {
							    enableemitter = false;
						    }
					    }

					    if(enableemitter)
					    {
                            _emitters[i]->setfirstrun(true);
                            _emitters[i]->setenable(true);
					    }
					    else
					    {
                            _emitters[i]->setenable(false);
					    }
                    }
                }
			    else
			    {
				    if(_state)
				    {
					    if(!(*_state))
					    {
                            enableemitter = false;
                            _emitters[i]->setenable(false);
					    }
				    }
                }
			}

            if(enableemitter)
                _lastdamagelevel = _to->DamageLevel;
            else
                _lastdamagelevel = -1;
        }
        else
        {
            if(_lastdamagelevel != -1)
            {
                for(unsigned int i = 0; i < _emitters.size(); i++)
                {
                    _emitters[i]->setenable(false);
                }
            }

            _lastdamagelevel = -1;
        }

        return;
    }

    if(_state)
    {
        if((*_state))
        {
            for(unsigned int i = 0; i < _emitters.size(); i++)
            {
                if(!_emitters[i]->isenabled())
                {
                    if(_enablestatevalue)
                    {
                        if(*_state == _enablestatevalue)
                        {
                            _emitters[i]->setenable(true);
                            _emitters[i]->setfirstrun(true);
                        }
                    }
                    else
                    {
                        _emitters[i]->setenable(true);
                        _emitters[i]->setfirstrun(true);
                    }
                }
                else if(_enablestatevalue)
                {
                    if(*_state != _enablestatevalue)
                        _emitters[i]->setenable(false);
                }
            }
        }
        else
        {
            for(unsigned int i = 0; i < _emitters.size(); i++)
            {
                _emitters[i]->setenable(false);
            }
        }
    }
}

/*----------------------------------------------------------------------
Function: TransformEffect::update
Description:
-----------------------------------------------------------------------*/
void TransformEffect::update()
{
    // Update date once per frame (avoid multiple updates from physics loop)
    if(_lastupdate == frametime)
    {
        return;
    }

    if(!validate())
        return;

    if(_updatecallback)
    {
        _updatecallback(this);
    }

    if(isEnabled())
    {
        checkstates();

        int enablecount = 0;

        updatetransform();

        if(!_emitters.empty())
        {
            std::vector<Emitter*>::iterator itr = _emitters.begin();

            for(; itr != _emitters.end(); ++itr)
            {
                Emitter* e = *itr;

                if(e->isenabled())
                {
                    enablecount++;
                }

                e->update(_transform);
                e->_lastupdate = frametime;
            }
        }

        if(((_flags & REMOVE_ME_WHEN_DONE) && (enablecount == 0)) || _dead)
        {
            removelist.insert(this);
        }
    }

    _lastupdate = frametime + 1;
}

/*----------------------------------------------------------------------
Function: TransformEffect::Init
Description:
-----------------------------------------------------------------------*/
bool TransformEffect::Init(vehVehicle* vehicle)
{
    /*
    Vehicle::VehicleStateList states = vehicle->GetVehicleStates();

	Vehicle::VehicleStateList::iterator it = states.begin();

	for(; it != states.end(); ++it)
	{
        VehicleEffectState* state = dynamic_cast<VehicleEffectState*>(*it);

        if(!state)
        {
            continue;
        }

        const std::string& obj = state->EmitterObject();

        if(obj == _emittername)
        {
            addemittereffect(state->getName().c_str(), osg::Vec3d(0, 0, 0), 0, state);
        }
    }
*/
    return true;
}
