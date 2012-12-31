#pragma once

#include <vector>
#include <map>
#include <deque>

#include "fx/Particle2.h"
#include "shader/glsl.h"
#include "ogl/oglMaterial.h"
#include "ogl/States.h"

class ParticleMgr
{
public:
    ParticleMgr();

    virtual ~ParticleMgr();

    void drawsurface();
    void drawsurfacedeprecated();
    void kill();
    void cleardraws()
    {
        _particleindex = 0;
        _draw.clear();
        _drawwatersurface.clear();
    }
    void remove(int id);
    void killWhenDone() {_killMyself = true;}
    void update();
    void init(bool global);
    void computebillboard();
    void setcolor(Vec4f& color) {_color = color;}
    void cull();

    void createparticle(Vec3d& pos,
                        Vec3f& vel,
                        float size[4],
                        float alpha[4],
                        int time[4],
                        const char* tex,
                        float rotation,
                        unsigned int flags);

    Particle2* createparticle(const char* tex);

    void setupparticle(const ParticleEffect* fx,
                       const Vec3f& normal,
                       const Vec3d& pos,
                       int starttime,
                       float alphadegradation);

    void setupsurfaceparticle(const ParticleEffect* fx,
                              const Vec3f& normal,
                              const Vec3d& pos,
                              int starttime,
                              float alphadegradation);

    void setupparticleanim(const ParticleEffect* fx,
                           const Vec3f& normal,
                           const Vec3d& pos,
                           int starttime,
                           int& animindex,
                           Particle2*& p);

    void setupparticlebeam(const ParticleEffect* fx,
                           const Vec3f& normal,
                           const Vec3d& pos1,
                           const Vec3d& pos2,
                           int starttime,
                           float alphadegradation);

    void setupparticle(const ParticleEffect* fx,
                    const Vec3f& normal,
                    const Vec3d& pos,
                    int starttime,
                    Particle2* p);

    bool readyToRemove() {return _killMyself && _particle.empty();}
    bool drawdeprecated(double dist2);
    bool draw(double dist2);

    Particle2* getparticle();

protected:
    mutable std::vector<Particle2*> _draw;
    mutable std::vector<Particle2*> _drawwatersurface;
    std::set<Particle2*> _particle;

    Vec4f _color;

    piVECTOR _oldplayer;

    bool _reset_bounds_flag;
    bool _killMyself;
    bool UsingSoftParticles;

    unsigned int _particleindex;

    glslProgram* _pgm;
#if USE_STATESET
    ref_ptr<Program> softparticlepgm;
    ref_ptr<Program> particlepgm;
    ref_ptr<RenderStateSet> material;
    ref_ptr<RenderStateSet> surfacematerial;
#else
    ref_ptr<oglMaterial> material;
#endif

private:
    std::deque<Particle2*> _freeparticle;
    bool _particlemgrInit;
    int _raintexture;
};
