#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: effect.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"
#include "include/vars.h"

#include "ob/obmain.h"
#include "random.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "math/mathUnitConversion.h"
#include "fx/fxMgr.h"
#include "fio/fio.h"
#include "trn/trnmain.h"
#include "fx/fxUtl.h"
#include "weap/weapammo.h"

#include "fx2/effect.h"
#include "fx2/particle.h"

#define MAX_EFFECTS 512

#define PRINT_EFFECTS 0

#if PRINT_EFFECTS
#include "rn/rnmain.h"
#endif

static std::multimap<SINT32, EMITTER *> _effects;
static std::deque<EMITTER *> _freeemitters;

//static Fire _fire;
static ParticleManager _particle;

static SINT32 _emitterid = 1;

#ifdef BETA_RELEASE
#include "ui/uiChat.h"
static bool dontdrawparticles = false;
#endif

static void _emitterupdate(EMITTER* e);
static void _doparticleeffect(const FX* fx, const FLOAT64VECTOR& p, const piVECTOR& normal, SINT32 starttime, FLOAT32 alphadegradation);
static void _doemittereeffect(const FX* fx, const FLOAT64VECTOR& p, const piVECTOR& normal);

//////////////////////////////////////////////////////////////////////
// Function: fxtoggledrawparticles
// Description:
//////////////////////////////////////////////////////////////////////
void fxtoggledrawparticles()
{
#ifdef BETA_RELEASE
    dontdrawparticles = !dontdrawparticles;
    uiChatPrintLine(L"Draw particles %s", dontdrawparticles ? L"DISABLED" : L"ENABLED");
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: fxinit
// Description:
//////////////////////////////////////////////////////////////////////
void fxinit()
{
    _fxdata.init();
    _particle.init();

    for(SINT32 i = 0; i < MAX_EFFECTS; i++)
    {
        EMITTER* e = new EMITTER;
        _freeemitters.push_back(e);
    }
}

/*----------------------------------------------------------------------
Function: fxsetcolor
Description:
----------------------------------------------------------------------*/
void fxsetcolor(piCOLORVALUE& color)
{
    _particle.setcolor(color);
}

//////////////////////////////////////////////////////////////////////
// Function: _getemitter
// Description:
//////////////////////////////////////////////////////////////////////
static EMITTER* _getemitter()
{
    EMITTER* e = NULL;

    if(_freeemitters.empty())
    {
        e = new EMITTER;
    }
    else
    {
        e = _freeemitters.front();
        _freeemitters.pop_front();
    }

    e->o = NULL;
    e->ammo = NULL;
    e->dead = 0;
    e->emitterflags = 0;
    e->separation = 0.1f;
    e->particleflags = 0;
    e->_fx.clear();

    return e;
}

//////////////////////////////////////////////////////////////////////
// Function: fxdraw
// Description:
//////////////////////////////////////////////////////////////////////
void fxdraw(float distance2, int& fxindex)
{
#ifdef BETA_RELEASE
    if(dontdrawparticles)
    {
        return;
    }
#endif

    _particle.draw(distance2, fxindex);
}

//////////////////////////////////////////////////////////////////////
// Function: _killattached
// Description:
//////////////////////////////////////////////////////////////////////
void fxdrawRemainingParticles(int fxindex)
{
#ifdef BETA_RELEASE
    if(dontdrawparticles)
    {
        return;
    }
#endif

    _particle.draw(0.0f, fxindex);
}

//////////////////////////////////////////////////////////////////////
// Function: _killattached
// Description:
//////////////////////////////////////////////////////////////////////
static void _killattached(EMITTER* emitter)
{
    /*
    if(emitter->attached.empty())
    {
        return;
    }

    // Kill attached emitters
    for(UINT32 i = 0; i < emitter->attached.size(); i++)
    {
        EMITTER* e = emitter->attached[i];

        // Centralize kill on next update
        e->dead = true;
    }

    emitter->attached.clear();
    */
}

//////////////////////////////////////////////////////////////////////
// Function: fxupdate
// Description:
//////////////////////////////////////////////////////////////////////
void fxupdate()
{
    logTimerBeginMacro();

    _fxdata.update();

    static std::vector<int> remove;

    std::multimap<SINT32, EMITTER *>::iterator itr = _effects.begin();

    for(; itr != _effects.end(); ++itr)
    {
        EMITTER* e = itr->second;

        if(e->update && !itr->second->dead)
        {
            e->update(e);
        }

        itr->second->lastupdate = frametime;

        if(itr->second->dead)
        {
            remove.push_back(itr->first);
        }
    }

    for(unsigned int i = 0; i < remove.size(); i++)
    {
        fxremove(remove[i]);
    }

    remove.erase(remove.begin(), remove.end());

    _particle.update();

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: fxkill
// Description:
//////////////////////////////////////////////////////////////////////
void fxkill()
{
    _fxdata.kill();

    std::multimap<SINT32, EMITTER *>::iterator begin = _effects.begin();
    std::multimap<SINT32, EMITTER *>::iterator free;

    while(!_effects.empty())
    {
        free = begin;
        ++begin;

        fxremove(free->first);
    }

    while(!_freeemitters.empty())
    {
        EMITTER* e = *_freeemitters.begin();
        _freeemitters.pop_front();

        delete e;
    }

    _particle.kill();

    Write_Log(LOG_OLD, LOG_INFO, "[effectKill]");
}

//////////////////////////////////////////////////////////////////////
// Function: fxremove
// Description:
//////////////////////////////////////////////////////////////////////
void fxremove(SINT32 key)
{
    std::multimap<SINT32, EMITTER *>::iterator find;
    std::multimap<SINT32, EMITTER *>::iterator free;

    find = _effects.find(key);

    while(find != _effects.end())
    {
        if(find->first != key)
        {
            break;
        }

        free = find;
        ++find;

        _freeemitters.push_back(free->second);
        _effects.erase(free);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _applydrag
// Description:
//////////////////////////////////////////////////////////////////////
void _applydrag(EMITTER* e, float time)
{
    logTimerBeginMacro();

    // Apply drag
    FLOAT32 magnitude;
    FLOAT32 speed;

    magnitude = (FLOAT32)math3dFastSqrt(math3dDotProduct(&e->velocity, &e->velocity));
    // Fast exponent
    speed = magnitude * (FLOAT32)fexp(-e->dragcoef * time);

    if(magnitude > 0.00001f || magnitude < -0.00001f)
    {
        e->velocity.x = speed * e->velocity.x / magnitude;
        e->velocity.y = speed * e->velocity.y / magnitude;
        e->velocity.z = speed * e->velocity.z / magnitude;
    }
    else
    {
        e->velocity.x = 0.0f;
        e->velocity.y = 0.0f;
        e->velocity.z = 0.0f;
    }

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: _emitterupdate
// Description:
//////////////////////////////////////////////////////////////////////
void _emitterupdate(EMITTER* e)
{
    if(!e) return;

    logTimerBeginMacro();

    if(e->expired())
    {
        e->dead = true;
    }
    else if(e->o && !obmainValidateObject(e->o))
    {
        e->dead = true;
    }
    else if(e->ammo && e->ammo->KillAmmo)
    {
        e->dead = true;
    }
    else
    {
        if(e->o && e->o->flags & OBJECT_SKIPDRAWING)
        {
            e->lastparticle = frametime;
            return;
        }

        SINT32 timediff = 0;

        if(e->isfountain())
        {
            timediff = e->lastparticletime();
        }
        else
        {
            timediff = e->updatetime();
        }

        FLOAT32 time = 0.001f * (FLOAT32)timediff;
		if(time < FLT_MIN)	time = FLT_MIN;

        if(e->hasgravity())
        {
            // Apply gravity
            e->velocity.z -= e->gravitycoef * time;
        }

        if(e->hasdrag())
        {
            _applydrag(e, time);
        }

        if(!e->isfountain())
        {
            if(e->ammo)
            {
                e->position = e->ammo->Position;
            }
            else if(e->o)
            {
                FLOAT64VECTOR pos;

                if(e->hasdof())
                {
                    if(!e->o->dof)
                    {
                        e->dead = true;
                        return;
                    }

                    pos.x = e->o->dof->dofmatrix2._41;
                    pos.y = e->o->dof->dofmatrix2._42;
                    pos.z = e->o->dof->dofmatrix2._43;
                }
                else
                {
                    pos.x = e->o->pos.x;
                    pos.y = e->o->pos.y;
                    pos.z = e->o->pos.z;
                }

                mathVectorMultiply64(&e->position, &pos, &e->o->mat);

                e->position.x += trnGetWorldOriginOffsetPtr()->x;
                e->position.y += trnGetWorldOriginOffsetPtr()->y;
                e->position.z += trnGetWorldOriginOffsetPtr()->z;

                e->position.z = -e->position.z;

                e->velocity.x = (FLOAT32)(e->lastposition.x - e->position.x) / time;
                e->velocity.y = (FLOAT32)(e->lastposition.y - e->position.y) / time;
                e->velocity.z = (FLOAT32)(e->lastposition.z - e->position.z) / time;
            }
            else
            {
                // Update position
                FLOAT32 x = e->velocity.x * time;
                FLOAT32 y = e->velocity.y * time;
                FLOAT32 z = e->velocity.z * time;

                e->position.x += x;
                e->position.y += y;
                e->position.z -= z;
            }
        }
        else
        {
            if(e->ammo)
            {
                e->position = e->ammo->Position;
            }
            else if(e->hasdof())
            {
                fxUtlGetDOFPos(e->o->dof, &e->o->mat, &e->position);
            }
        }

        SINT32 numparticles = 0;

        FLOAT64VECTOR posvector = {0, 0, 0};
        mathSubVectors(&posvector, &e->position, &e->lastposition);

        FLOAT32 poslength = (FLOAT32)mathExactVectorLength(&posvector);

        FLOAT64VECTOR unitpos = {0, 0, 0};

        if(poslength > 0.0f)
        {
            unitpos.x = posvector.x / poslength;
            unitpos.y = posvector.y / poslength;
            unitpos.z = posvector.z / poslength;
        }

        if(e->isfountain())
        {
            numparticles = FloatToInt(time * e->numpersecond);

            if(numparticles <= 0)
            {
                return;
            }
        }

//		SINT32 starttime = frametime;

        for(UINT32 i = 0; i < e->_fx.size(); i++)
        {
            FX* fx = e->_fx[i];

            if(!e->isfountain())
            {
                numparticles = FloatToInt(poslength / fx->separation);

                if(!numparticles)
                {
                    continue;
                }
            }

            piVECTOR dir = e->velocity;
            FLOAT64VECTOR position = e->lastposition;

            float speed2 = mathVectDotProduct(&dir, &dir);

            if(e->isfountain() && speed2 >= 0.5)
            {
                // Input velocity / normal to be used
                float speed = math3dFastSqrt(speed2);

                if(speed >= 1.0)
                {
                    dir.x = dir.x * speed;
                    dir.y = dir.y * speed;
                    dir.z = dir.z * speed;
                }
            }
            else
            {
                dir.x = (float) posvector.x;
                dir.y = (float) posvector.y;
                dir.z = (float)-posvector.z;
            }

            if(e->hasdof() && (fx->fxflags & fx_out_dofdirection))
            {
                float speed = mathVectDotProduct(&dir, &dir);
                speed = math3dFastSqrt(speed);

                speed = speed <= 1.0f ? 1.0f : speed;

                piVECTOR dofdir;
                fxUtlGetDOFDirection(e->o->dof, &e->o->mat, &dofdir);

                dir.x = dofdir.x * speed;
                dir.y = dofdir.y * speed;
                dir.z = dofdir.z * speed;
            }

            if(fx->fxflags & fx_overridedirection)
            {
                dir = fx->overridedir;
            }

            FLOAT32 alphadegradation = 1.0f;

            if(e->hasalphadegradation())
            {
                alphadegradation = 1.0f - ((FLOAT32)e->age() / (FLOAT32)e->lifetime);
            }

            if(numparticles > 128)
            {
                numparticles = 128;
            }

            for(SINT32 j = 0; j < numparticles; j++)
            {
                _doparticleeffect(fx, position, dir, frametime, alphadegradation);

                position.x += unitpos.x * fx->separation;
                position.y += unitpos.y * fx->separation;
                position.z += unitpos.z * fx->separation;
            }
        }

        e->lastparticle = frametime;

        if(numparticles > 0)
        {
            e->lastposition = e->position;
        }
    }

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: _doemittereffect
// Description:
//////////////////////////////////////////////////////////////////////
void _doemittereffect(const FX* fx,
                      const FLOAT64VECTOR& p,
                      const piVECTOR& normal,
                      OBJECT* o,
                      const weapAmmo* ammo,
                      int lifetime,
                      int emitterid)
{
    FLOAT64VECTOR pos = p;

    piVECTOR n =
    {
         normal.x,
         normal.y,
         normal.z
    };

    if(o)
    {
        o->flag2 |= OBJECT_HAS_FX;
    }

    SINT32 num;
    UINT32 size = fx->number.size();

    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    piVECTOR velocity = {n.x * fx->speed, n.y * fx->speed, n.z * fx->speed};

    for(SINT32 i = 0; i < num; i++)
    {
        EMITTER* e = _getemitter();

        e->o = o;
        e->ammo = ammo;

        if(!fx->attach.empty())
        {
            for(UINT32 j = 0; j < fx->attach.size(); j++)
            {
                EFFECT* effect = _fxdata.geteffect(fx->attach[j].c_str());

                if(effect)
                {
                    for(UINT32 q = 0; q < effect->fx.size(); q++)
                    {
                        e->_fx.push_back(effect->fx[q]);
                    }
                }
            }
        }

        e->particleflags = fx->particleflags;
        e->emitterflags = fx->emitterflags;

        if(lifetime)
        {
            e->lifetime = lifetime;
        }
        else
        {
            e->lifetime = fx->lifetime;
        }

        e->numpersecond = fx->numpersec;
        e->position = p;
        e->lastposition = p;
        e->gravitycoef = fx->gravitycoef;
        e->dragcoef = fx->dragcoef;
        e->createtime = frametime;
        e->lastupdate = frametime;
        e->lastparticle = frametime;
        e->velocity = velocity;
        e->separation = fx->separation;

        if(fx->fxflags & fx_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD(fx->randomdirection) * 0.5f;

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

                piVECTOR basis = {1.0f, 0.0f, 0.0f};

                if(fabsf(math3dDotProduct(&basis, &normal)) > 0.999f)
                {
                    basis.x = 0.0f;
                    basis.y = 1.0f;
                    basis.z = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                piVECTOR u;
                float dot = math3dDotProduct(&basis, &normal);

                u.x = basis.x - n.x * dot;
                u.y = basis.y - n.y * dot;
                u.z = basis.z - n.z * dot;

                mathReallyFastNormalize(u);

                piVECTOR v;
                math3dCrossProduct(&v, (piVECTOR*)&normal, &u);

                n.x = normal.x * dist + u.x * x + v.x * y;
                n.y = normal.y * dist + u.y * x + v.y * y;
                n.z = normal.z * dist + u.z * x + v.z * y;
            }
            else
            {
                n.x = GetRandom(-1.0f, 1.0f);
                n.y = GetRandom(-1.0f, 1.0f);
                n.z = GetRandom(-1.0f, 1.0f);
            }

            mathReallyFastNormalize(n);

            velocity.x = n.x * fx->speed;
            velocity.y = n.y * fx->speed;
            velocity.z = n.z * fx->speed;
        }

        if(fx->fxflags & fx_has_velvariance)
        {
            e->velocity.x += GetRandom(fx->velvariance[0], fx->velvariance[3]);
            e->velocity.y += GetRandom(fx->velvariance[1], fx->velvariance[4]);
            e->velocity.z += GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            velocity.x *= fx->velreduction;
            velocity.y *= fx->velreduction;
            velocity.z *= fx->velreduction;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            e->position.x += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            e->position.y += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            e->position.z += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        e->update = _emitterupdate;
        // All emitters for this effect have the same id (for removal upon request)
        e->id = emitterid;

        // Multi-map it
        _effects.insert(std::make_pair(e->id, e));
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _doparticleeffect
// Description:
//////////////////////////////////////////////////////////////////////
void _doparticleeffect(const FX* fx,
                       const FLOAT64VECTOR& pt,
                       const piVECTOR& normal,
                       SINT32 starttime,
                       FLOAT32 alphadegradation)
{
    FLOAT64VECTOR pos = pt;

    piVECTOR n =
    {
         normal.x,
         normal.y,
         normal.z
    };

    FLOAT32 speed = fx->speed;

    if(fx->number.empty())
    {
        return;
    }

    logTimerBeginMacro();

    UINT32 size = fx->number.size();

    SINT32 num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    piVECTOR velocity = {n.x * speed, n.y * speed, n.z * speed};

    int delayoffset = 0;

    for(SINT32 i = 0; i < num; i++)
    {
        Particle* p = _particle.getparticle();

        if(!p)
        {
            break;
        }

        if(fx->fxflags & fx_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD(fx->randomdirection) * 0.5f;

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

                piVECTOR basis = {1.0f, 0.0f, 0.0f};

                if(fabsf(math3dDotProduct(&basis, &normal)) > 0.999f)
                {
                    basis.x = 0.0f;
                    basis.y = 1.0f;
                    basis.z = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                piVECTOR u;
                float dot = math3dDotProduct(&basis, &normal);

                u.x = basis.x - n.x * dot;
                u.y = basis.y - n.y * dot;
                u.z = basis.z - n.z * dot;

                mathReallyFastNormalize(u);

                piVECTOR v;
                math3dCrossProduct(&v, (piVECTOR*)&normal, &u);

                n.x = normal.x * dist + u.x * x + v.x * y;
                n.y = normal.y * dist + u.y * x + v.y * y;
                n.z = normal.z * dist + u.z * x + v.z * y;
            }
            else
            {
                n.x = GetRandom(-1.0f, 1.0f);
                n.y = GetRandom(-1.0f, 1.0f);
                n.z = GetRandom(-1.0f, 1.0f);
            }

            mathReallyFastNormalize(n);

            velocity.x = n.x * speed;
            velocity.y = n.y * speed;
            velocity.z = n.z * speed;
        }

        p->_flags = fx->particleflags;

        if(fx->texture.size() > 1)
        {
            p->_textureindex = fx->texture[GetRandom(0, fx->texture.size() - 1)];;
        }
        else
        {
            p->_textureindex = fx->texture[0];
        }

        p->_alpha[0] = fx->alpha[0] * alphadegradation;
        p->_alpha[1] = fx->alpha[1] * alphadegradation;

        if(p->hasalphastages())
        {
            p->_alpha[2] = fx->alpha[2] * alphadegradation;
            p->_alpha[3] = fx->alpha[3] * alphadegradation;
        }

        p->_size[0] = fx->size[0];
        p->_size[1] = fx->size[1];

        if(p->hassizestages())
        {
            p->_size[2] = fx->size[2];
            p->_size[3] = fx->size[3];
        }

        if(p->hastimestages())
        {
            p->_time[0] = starttime + fx->time[0] + fx->delaystart + delayoffset;
            p->_time[1] = p->_time[0] + fx->time[1];
            p->_time[2] = p->_time[1] + fx->time[2];
            p->_time[3] = p->_time[2] + fx->time[3];
        }
        else
        {
            p->_time[0] = starttime + fx->delaystart + delayoffset;
            p->_time[3] = p->_time[0] + fx->lifetime;
        }

        p->_dragcoeff = fx->dragcoef;
        p->_gravcoef = fx->gravitycoef;
        p->_stretchscale = fx->stretchscale;
        p->_glowtime = fx->glowtime;
        p->_stopstretch = fx->stopstretch;

        if(p->hasrotate())
        {
            p->_rotate = GetRandom(-fx->rotate, fx->rotate);
        }

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_windvelocity = _fxdata._wind;

        p->_velocity = velocity;
        p->_material = fx->matid;
        p->_position = pos;

        if(fx->fxflags & fx_has_velvariance)
        {
            p->_velvariance.x = GetRandom(fx->velvariance[0], fx->velvariance[3]);
            p->_velvariance.y = GetRandom(fx->velvariance[1], fx->velvariance[4]);
            p->_velvariance.z = GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            velocity.x *= fx->velreduction;
            velocity.y *= fx->velreduction;
            velocity.z *= fx->velreduction;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            p->_position.x += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            p->_position.y += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            p->_position.z += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        delayoffset += fx->delayeachparticle;
    }

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: fxaddeffectobject
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxaddeffectobject(const char* effect, const FLOAT64VECTOR& p, OBJECT* o, const piVECTOR& n, int lifetime)
{
    EFFECT* e = _fxdata.geteffect(effect);

    if(!e)
    {
        return 0;
    }

#if PRINT_EFFECTS
    rnmainListText("fxaddeffectobject %s", effect);
#endif

    int id = 0;

    logTimerBeginMacro();

    bool emitterIdIncremented = false;

    for(UINT32 i = 0; i < e->fx.size(); i++)
    {
        FX* fx = e->fx[i];

        if(fx->isemitter)
        {
            // Can have multiple emitters on one effect...multi map them
            if(!emitterIdIncremented)
            {
                id = ++_emitterid;
                emitterIdIncremented = true;
            }

            _doemittereffect(fx, p, n, o, NULL, lifetime, id);
            continue;
        }

        if(!fx->use.empty())
        {
            for(UINT32 j = 0; j < fx->use.size(); j++)
            {
                fxaddeffect(fx->use[j].c_str(), p, n, lifetime);
            }

            continue;
        }

        _doparticleeffect(fx, p, n, frametime, 1.0f);
    }

    logTimerEndMacro();

    return id;
}

//////////////////////////////////////////////////////////////////////
// Function: fxaddeffectammo
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxaddeffectammo(const char* effect, const weapAmmo* ammo, const piVECTOR& n, int lifetime)
{
    EFFECT* e = _fxdata.geteffect(effect);

    if(!e)
    {
        return 0;
    }

    if(!ammo)
    {
        return 0;
    }

#if PRINT_EFFECTS
    rnmainListText("fxaddeffectammo %s", effect);
#endif

    int id = 0;

    logTimerBeginMacro();

    FLOAT64VECTOR p = ammo->Position;

    bool emitterIdIncremented = false;

    for(UINT32 i = 0; i < e->fx.size(); i++)
    {
        FX* fx = e->fx[i];

        if(fx->isemitter)
        {
            // Can have multiple emitters on one effect...multi map them
            if(!emitterIdIncremented)
            {
                id = ++_emitterid;
                emitterIdIncremented = true;
            }

            _doemittereffect(fx, p, n, ammo->AmmoType->Object, ammo, lifetime, id);
            continue;
        }

        if(!fx->use.empty())
        {
            for(UINT32 j = 0; j < fx->use.size(); j++)
            {
                fxaddeffect(fx->use[j].c_str(), p, n, lifetime);
            }

            continue;
        }

        _doparticleeffect(fx, p, n, frametime, 1.0f);
    }

    logTimerEndMacro();

    return id;
}

//////////////////////////////////////////////////////////////////////
// Function: fxaddeffectdof
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxaddeffectdof(const char* effect, OBJECT* o, const piVECTOR& n, int lifetime)
{
    EFFECT* e = _fxdata.geteffect(effect);

    if(!e)
    {
        return 0;
    }

    if(!o)
    {
        return 0;
    }

    if(!o->dof)
    {
        return 0;
    }

#if PRINT_EFFECTS
    rnmainListText("fxaddeffectdof %s", effect);
#endif

    int id = 0;

    logTimerBeginMacro();

    FLOAT64VECTOR p;
    fxUtlGetDOFPos(o->dof, &o->mat, &p);

    bool emitterIdIncremented = false;

    for(UINT32 i = 0; i < e->fx.size(); i++)
    {
        FX* fx = e->fx[i];

        fx->emitterflags |= emitter_has_dof;

        if(fx->isemitter)
        {
            // Can have multiple emitters on one effect...multi map them
            if(!emitterIdIncremented)
            {
                id = ++_emitterid;
                emitterIdIncremented = true;
            }

            _doemittereffect(fx, p, n, o, NULL, lifetime, id);
            continue;
        }

        if(!fx->use.empty())
        {
            for(UINT32 j = 0; j < fx->use.size(); j++)
            {
                fxaddeffect(fx->use[j].c_str(), p, n, lifetime);
            }

            continue;
        }

        _doparticleeffect(fx, p, n, frametime, 1.0f);
    }

    logTimerEndMacro();

    return id;
}

//////////////////////////////////////////////////////////////////////
// Function: fxaddeffect
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxaddeffect(const char* effect, const FLOAT64VECTOR& p, const piVECTOR& n, int lifetime)
{
    EFFECT* e = _fxdata.geteffect(effect);

    if(!e)
    {
        return 0;
    }

#if PRINT_EFFECTS
    rnmainListText("fxaddeffect %s", effect);
#endif

    int id = 0;

    logTimerBeginMacro();

    bool emitterIdIncremented = false;

    for(UINT32 i = 0; i < e->fx.size(); i++)
    {
        FX* fx = e->fx[i];

        if(fx->isemitter)
        {
            // Can have multiple emitters on one effect...multi map them
            if(!emitterIdIncremented)
            {
                id = ++_emitterid;
                emitterIdIncremented = true;
            }

            _doemittereffect(fx, p, n, NULL, NULL, lifetime, id);
            continue;
        }

        if(!fx->use.empty())
        {
            for(UINT32 j = 0; j < fx->use.size(); j++)
            {
                fxaddeffect(fx->use[j].c_str(), p, n, lifetime);
            }

            continue;
        }

        _doparticleeffect(fx, p, n, frametime, 1.0f);
    }

    logTimerEndMacro();

    return id;
}
