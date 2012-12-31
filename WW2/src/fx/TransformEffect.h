#pragma once

#include <vector>

#include "datafx.h"
#include "ParticleMgr.h"
#include "EmitterMgr.h"
#include "math/Matrixd.h"
#include "trn/trncd.h"
#include "math/Quat.h"

struct weapAmmo;

class TransformEffect
{
public:
    TransformEffect();

    virtual ~TransformEffect();

	virtual bool Init(struct vehVehicle* vehicle);

    void checkstates();
    void setemittername(const char* name) {_emittername = name;}
    void setdamagelevel(int damagelevel);
    void update();
    void enable(bool flag);
    void enableemitters(bool flag);
    void setdirection(Vec3f dir) {_dir = dir;}
    void seteffect(const char* effect) {_effectname = effect;}
    void setlifetime(int lifetime) {_lifetime = lifetime;}
    void setflag(int flag = 0) {_flags |= flag;}
    void setstratobj(const StratDB::StratObject* so);
    void settoobj(const trnCDTerrainObject* to);
    void setweapammo(const weapAmmo* weapammo);
    void setstate(unsigned int* state);
    void setvehicle(const vehVehicle* v);
    void setmatrix(const Matrixd& m) {_transform = m;}
    void reset();
    void kill() {_dead = true;}
    void releaseemitters();
    void releaseparticles();
    void setenablestatevalue(unsigned int i);
    void link(Particle2* p)
    {
        _particles.push_back(p);
    }

    void setupdatecallback(void (*fn)(TransformEffect* te))
    {
        _updatecallback = fn;
    }

    int age() {return frametime - _createtime;}
    int updatetime() {return frametime - _lastupdate;}

    bool isEnabled() {return _isenabled;}
    bool expired()
    {
        return _lifetime != -1 && age() > _lifetime;
    }
    bool isEffectNamedThis(const char* name)
    {
        return _effectname.find(name) != std::string::npos;
    }
    bool isEmitterNamedThis(const char* name)
    {
        return _emittername.find(name) != std::string::npos;
    }

    enum
    {
        REMOVE_ME_WHEN_DONE = 0x1,
        USER_CREATED_EFFECT = 0x2,
        STAND_ALONE         = 0x4,
        PARTICLE_LINK       = 0x8,
        TARGET_AIM          = 0x10
    };

    Emitter* getemitter(unsigned int i)
    {
        if(i >= _emitters.size())
        {
            return NULL;
        }

        return _emitters[i];
    }

    std::vector<Particle2*>& GetParticles() {return _particles;}

    static TransformEffect* AddStandAloneEffect(const char* effect, const Vec3f& vel, int lifetime);
    static TransformEffect* AddEffect(const char* effect, const Vec3f& n, int lifetime, struct OBJECT* o);
    static TransformEffect* CreateEffect(const char* effect, const Vec3f& n, int lifetime, struct OBJECT* o, int numemitters);
    static TransformEffect* AddTracer(const weapAmmo* ammo, const char* effect);
    static TransformEffect* AddTargetAim(const char* effect);
    static ParticleMgr* GetParticleMgr();

    static void Apply(struct trnCDTerrainObject* to);
    static void Apply(struct vehVehicle* vehicle);
    static void Apply(struct trnAutoGun* autogun);
    static void Apply(struct weapAmmo* weapammo);
    static void Apply(struct gr2AnimTrooper* trooper, struct OBJECT* o);

    Vec3f _velocity;
    float _gravcoef;
    float _dragcoef;

 protected:

    int _lifetime;
    int _lastupdate;
    int _createtime;
    int _damagelevel;

    std::string _emittername;
    std::string _effectname;

    Vec3f _dir;

    Matrixd _transform;

    bool addemittereffect(const char* effect, const Vec3f& n, int lifetime, struct OBJECT* o);
    bool createemittereffect(const char* effect, const Vec3f& n, int lifetime, struct OBJECT* o, int numemitters);
    bool addtracer(const weapAmmo* ammo);
    bool allocateparticles(const char* effect, int numparticles);
    bool validate();

    void updatetransform();

private:
    std::vector<Emitter*> _emitters;
    std::vector<Particle2*> _particles;
    OBJECT* _txobject;
    const weapAmmo* _weapammo;
    const trnCDTerrainObject* _to;
    const StratDB::StratObject* _stratobj;
    const vehVehicle* _vehicle;
    bool _isenabled;
    bool _dead;

    void (*_updatecallback)(TransformEffect* te);

    int _flags;
    int _lastdamagelevel;

    unsigned int* _state;
    unsigned int _enablestatevalue;

    Vec3d _lastwooffset;
};

extern void InitializeFxSystem();
extern void UpdateFxSystem();
extern void KillFxSystem();
extern void CullFxSystem();
extern void DrawSurfaceFx();
extern void DrawSurfaceFxDeprecated();

extern bool DrawFxSystemDeprecated(float dist2);
extern bool DrawFxSystem(float dist2);
