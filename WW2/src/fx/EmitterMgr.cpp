#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------
Module: emittermgr.cpp
Description:
-------------------------------------------------------------------*/
#include "include/vars.h"

#include "ParticleMgr.h"
#include "EmitterMgr.h"
#include "math/math3d.h"
#include "math/Matrixd.h"
#include "trn/trnvars.h"
#include "trn/trncd.h"
#include "physics/colliderMain.h"

#define MAX_EFFECTS 4096

extern int FloatToInt(float);

#include <deque>

static std::deque<Emitter*> _freeemitters;
static Emitter* EmitterBlock = NULL;

/*------------------------------------------------------------------
Function: EmitterMgrInit
Description:
-------------------------------------------------------------------*/
void EmitterMgrInit()
{
    EmitterBlock = new Emitter[MAX_EFFECTS];

    for(int i = 0; i < MAX_EFFECTS; i++)
    {
        _freeemitters.push_back(&EmitterBlock[i]);
    }
}

/*------------------------------------------------------------------
Function: EmitterMgrKill
Description:
-------------------------------------------------------------------*/
void EmitterMgrKill()
{
    if(EmitterBlock)
    {
        delete[] EmitterBlock;
        EmitterBlock = NULL;
    }

    _freeemitters.clear();
}

/*------------------------------------------------------------------
Function: EmitterMgrFree
Description:
-------------------------------------------------------------------*/
void EmitterMgrFree(Emitter* e)
{
    e->reset();
    _freeemitters.push_back(e);
}

/*------------------------------------------------------------------
Function: EmitterMgrGetEmitter
Description:
-------------------------------------------------------------------*/
Emitter* EmitterMgrGetEmitter()
{
    Emitter* e = NULL;

    if(_freeemitters.empty())
    {
        e = new Emitter;
    }
    else
    {
        e = _freeemitters.front();
        _freeemitters.pop_front();
    }

    return e;
}

/*------------------------------------------------------------------
Function: Emitter::reset
Description:
-------------------------------------------------------------------*/
void Emitter::reset()
{
    if(_particleeffect)
    {
        if(_particleeffect->isclone)
        {
            FxGet().releaseparticleeffect(_particleeffect);
        }
    }

    if(_particle)
    {
        _particle->_dead = true;
    }

    _enable = false;
    _firstrun = true;
    _emitterflags = 0;
    _animindex = -1;
    _separation = 0.1f;
    _particleflags = 0;
    _pm = NULL;
    _particleeffect = NULL;
    _stratobj = NULL;
    _particle = NULL;
}

/*------------------------------------------------------------------
Function: Emitter::reset
Description:
-------------------------------------------------------------------*/
void Emitter::reset(Emitterfx* e)
{
    if(_particle)
    {
        _particle->_dead = true;
    }

    _enable = false;
    _firstrun = true;
    _emitterflags = e->emitterflags;
    _numpersecond = e->numpersecond;
    _lifetime = e->lifetime;
    _separation = 0.1f;
    _particleflags = 0;
    _animindex = -1;
    _pm = NULL;
    _stratobj = NULL;
    _particleeffect = NULL;
    _particle = NULL;
}

/*------------------------------------------------------------------
Function: Emitter::Emitter
Description:
-------------------------------------------------------------------*/
Emitter::Emitter()
{
    _id = 0;
    _createtime = 0;
    _lifetime = 0;
    _lastupdate = 0;
    _numpersecond = 0;
    _lastparticle = 0;

    _particleflags = 0;
    _emitterflags = 0;

    _stratobj = NULL;
    _particleeffect = NULL;

    _gravitycoef = 0;
    _dragcoef = 0;
    _separation = 0;
    _animindex = -1;
    _particle = NULL;
    _enable = false;
    _firstrun = true;
}

/*----------------------------------------------------------------------
Function: Emitter:~Emitter
Description:
----------------------------------------------------------------------*/
Emitter::~Emitter()
{
}

/*----------------------------------------------------------------------
Function: Emitter::applydrag
Description:
----------------------------------------------------------------------*/
void Emitter::applydrag(float time)
{
    // Apply drag
    float magnitude;
    float speed;

    magnitude = (float)sqrt(_velocity * _velocity);
    // Fast exponent
    speed = magnitude * (float)exp(-_dragcoef * time);

    if(magnitude > 0.00001f || magnitude < -0.00001f)
    {
        _velocity = (_velocity / magnitude) * speed;
    }
    else
    {
        _velocity.x() = 0.0f;
        _velocity.y() = 0.0f;
        _velocity.z() = 0.0f;
    }
}

/*----------------------------------------------------------------------
Function: Emitter::setupemitter
Description: Predefined effect
-----------------------------------------------------------------------*/
bool Emitter::setupemitter(ParticleEffect* pe, const Vec3d& normal, int lifetime)
{
    _effect = pe->name;

    _particleeffect = pe;

    if(!_particleeffect)
        return false;

    _particleflags = _particleeffect->particleflags;
    _fxflags = _particleeffect->fxflags;

    if(lifetime)
    {
        _lifetime = lifetime;
    }
    else
    {
        _lifetime = _particleeffect->lifetime;
    }

    _gravitycoef = _particleeffect->gravitycoef;
    _dragcoef = _particleeffect->dragcoef;
    _createtime = frametime;
    _lastupdate = frametime;
    _position = Vec3d(0, 0, 0);
    _lastposition = Vec3d(0, 0, 0);
    _lastparticle = frametime;
    _velocity = normal;
    _separation = _particleeffect->separation;

    return true;
}

/*----------------------------------------------------------------------
Function: Emitter::setparticleeffect
Description: User created effect
-----------------------------------------------------------------------*/
void Emitter::setparticleeffect(ParticleEffect* pe)
{
    _particleeffect = pe;

    _particleflags = _particleeffect->particleflags;
    _fxflags = _particleeffect->fxflags;

    if(_lifetime == 0)
    {
        _lifetime = _particleeffect->lifetime;
    }

    _gravitycoef = _particleeffect->gravitycoef;
    _dragcoef = _particleeffect->dragcoef;
    _createtime = frametime;
    _lastupdate = frametime;
    _position = Vec3d(0, 0, 0);
    _lastposition = Vec3d(0, 0, 0);
    _lastparticle = frametime;
    _separation = _particleeffect->separation;
}

/*----------------------------------------------------------------------
Function: Emitter::setupemitter
Description:
-----------------------------------------------------------------------*/
bool Emitter::setupemitter(const Vec3d& normal, int lifetime)
{
    Vec3f velocity = normal;

    if(lifetime)
    {
        _lifetime = lifetime;
    }

    _createtime = frametime;
    _lastupdate = frametime;
    _position = Vec3d(0, 0, 0);
    _lastposition = Vec3d(0, 0, 0);
    _lastparticle = frametime;
    _velocity = velocity;

    return true;
}

/*----------------------------------------------------------------------
Function: Emitter::_setFirstRunParams
Description:
----------------------------------------------------------------------*/
void Emitter::_setFirstRunParams(Vec3d& eye)
{
    _lastposition = eye;
    _lastparticleposition = _lastposition;
    _createtime = frametime + 1; // Force lifetime = 1 to expire after next update
    _lastparticle = frametime - 50;
}

/*----------------------------------------------------------------------
Function: Emitter::update
Description:
----------------------------------------------------------------------*/
void Emitter::update(const Matrixd& mat)
{
    // Particle manager not set
    if(!_pm)
    {
        return;
    }

    float lastupdatetime = (float)timesincelastupdate() * 0.001f;

    if(expired())
    {
        _enable = false;
    }

    if(isenabled())
    {
        if(!_particleeffect)
        {
            return;
        }

        Vec3d transpos = Vec3d(mat(3, 0), mat(3, 1), mat(3, 2));

        if(_firstrun)
        {
            _setFirstRunParams(transpos);
            _firstrun = false;
        }

        float lastparticletime = (float)timesincelastparticle() * 0.001f;

        _lastposition = _position;
        _position = transpos;

        int numparticles = 0;

        Vec3f posvector = _position - _lastparticleposition;
        float poslength = math3dFastSqrt(posvector.length2());

        if(!isparticleeffectonly() && poslength > 0.0001f && !isfountain())
        {
            Vec3f dir = _position - _lastposition;
            float magnitude = 1.0f;

            if(lastupdatetime > 0.0001f)
            {
                magnitude = math3dFastSqrt(dir * dir) / lastupdatetime;
            }

            dir.fastnormalize();
            dir *= magnitude;

            _velocity = dir;
        }

        if(isfountain() && !isparticleeffectonly())
        {
            numparticles = FloatToInt(lastparticletime * _numpersecond);

            if(numparticles <= 0)
            {
                return;
            }
        }

        if(!isfountain() && !isparticleeffectonly() && !isanimation())
        {
            numparticles = (int)(poslength / _particleeffect->separation);

            if(numparticles == 0)
            {
                return;
            }
        }

        if(isanimation())
        {
            // delayeachparticle used to switch frames
            if(timesincelastparticle() < _particleeffect->delayeachparticle)
            {
                return;
            }
        }

        // Clamp to reduce number of particles
        if(_fxflags & fx_clamp_emitter)
        {
            if(numparticles > _particleeffect->clampemitter)
            {
                numparticles = _particleeffect->clampemitter;
            }
        }

        Vec3f dir;

        if(!isparticleeffectonly() && poslength > 0.0001f)
        {
            dir = posvector;
        }
        else
        {
            dir = _velocity;
        }

        if(_fxflags & fx_out_dofdirection)
        {
            float speed = dir * dir;
            speed = math3dFastSqrt(speed);

            speed = speed <= 1.0f ? 1.0f : speed;

            Vec3f dofdir;
            dofdir = Vec3f((float)mat(1, 0), (float)mat(1, 1), (float)mat(1, 2));

            dir = dofdir * speed;
        }

        float alphadegradation = 1.0f;

        if(hasalphadegradation())
        {
            alphadegradation = 1.0f - ((float)age() / (float)_lifetime);
        }

        if(numparticles > 128)
        {
            numparticles = 128;
        }

        if(isparticleeffectonly())
        {
            // Bombs, bullet hits, etc...
            if(iswatersurfaceeffect())
            {
                _pm->setupsurfaceparticle(_particleeffect, dir, _position, frametime, alphadegradation);
            }
            else
            {
                _pm->setupparticle(_particleeffect, dir, _position, frametime, 1.0);
            }
        }
        else if(isparticlebeam())
        {
            Vec3d p1 = _position;
            Vec3d p2;

            Vec3f d(dir);
            d.fastnormalize();

            if(_fxflags & fx_clamp_beam)
            {
                Vec3f d(dir);
                d.fastnormalize();

                p2 = p1 + d * _particleeffect->clampbeam;
            }
            else
            {
                p2 = _lastparticleposition;
            }

            Vec3f step = (p1 - p2) / numparticles;

            for(int j = 0; j < numparticles; j++)
            {
                p2 = p1 - step;
                _pm->setupparticlebeam(_particleeffect, dir, p1, p2, frametime, alphadegradation);
                p1 = p2;
            }
        }
        else if(isanimation())
        {
            _pm->setupparticleanim(_particleeffect, dir, _position, frametime, _animindex, _particle);
        }
        else
        {
            for(int j = 0; j < numparticles; j++)
            {
                float t = (float)j / (float)numparticles;
                Vec3d p = _position - dir * t;

                if(iswatersurfaceeffect())
                {
                    _pm->setupsurfaceparticle(_particleeffect, dir, p, frametime, alphadegradation);
                }
                else
                {
                    _pm->setupparticle(_particleeffect, dir, p, frametime, alphadegradation);
                }
            }
        }

        _lastparticleposition = _position;
        _lastparticle = frametime;
    }
}
