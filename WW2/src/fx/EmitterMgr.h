#pragma once

#include <vector>
#include "fx/DataFx.h"

namespace StratDB { class StratObject; }

class Emitter
{
public:
    Emitter();
    ~Emitter();

    void reset();
    void reset(Emitterfx* e);
    void link(ParticleMgr* pm) {_pm = pm;}
    void update(const Matrixd& mat);
    void applydrag(float time);
    void setparticleeffect(ParticleEffect* pe);
    void releaseparticleeffect() {FxGet().releaseparticleeffect(_particleeffect);}
    void initemittertoeffect();
    void setenable(bool enable) {_enable = enable;}
    void setfirstrun(bool firstrun)
    {
        _firstrun = firstrun;
        _createtime = frametime;
    }
    const std::string& getname() {return _effect;}
    const ParticleEffect* getparticleeffect() {return _particleeffect;}

    bool setupemitter(const Vec3d& normal, int lifetime);
    bool setupemitter(ParticleEffect* pe, const Vec3d& normal, int lifetime);
    bool hasgravity() {return _emitterflags & emitter_has_gravity;}
    bool hasdrag() {return _emitterflags & emitter_has_drag;}
    bool isfountain() {return _emitterflags & emitter_is_fountain;}
    bool hasalphadegradation() {return _emitterflags & emitter_has_alphadegradation;}
    bool hasdof() {return _emitterflags & emitter_has_dof;}
    bool isenabled() {return _enable;}
    bool isparticleeffectonly() {return _emitterflags & emitter_particle_effect_only;}
    bool isparticlebeam() {return _emitterflags & emitter_particle_beam;}
    bool isanimation() {return _emitterflags & emitter_is_animation;}
    bool iswatersurfaceeffect() {return _emitterflags & emitter_is_watersurfaceeffect;}

    bool expired()
    {
        return _lifetime != -1 && age() > _lifetime;
    }

    int age()
    {
        return frametime - _createtime;
    }

    int timesincelastupdate()
    {
        return frametime - _lastupdate;
    }

    int timesincelastparticle()
    {
        return frametime - _lastparticle;
    }

public:
    int _id;
    int _lifetime;
    int _lastparticle;
    int _lastupdate;

    unsigned int _particleflags;
    unsigned int _emitterflags;
    unsigned int _fxflags;

    const StratDB::StratObject* _stratobj;
    Vec3d _position;
    Vec3d _lastposition;
    Vec3d _lastparticleposition;
    Vec3d _wooffset;

    float _gravitycoef;
    float _dragcoef;
    float _separation;
    float _numpersecond;

    Vec3f _velocity;

private:
    ParticleMgr* _pm;
    std::string _effect;
    int _animindex;
    int _createtime;

    bool _firstrun;
    bool _enable;

    Particle2* _particle;
    ParticleEffect* _particleeffect;

private:
    void _checkStateIfNecessary();
    void _setFirstRunParams(Vec3d& eye);
};

extern void EmitterMgrInit();
extern void EmitterMgrKill();
extern void EmitterMgrFree(Emitter*);

extern Emitter* EmitterMgrGetEmitter();
