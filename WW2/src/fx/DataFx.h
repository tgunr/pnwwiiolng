#pragma once

#include <vector>
#include <string>
#include "math/Vec4d.h"
#include "math/Vec3f.h"
#include "osgImport/Texture2D.h"

#define transform_is_standalone         0x00000001
#define transform_random_direction      0x00000002
#define transform_is_waterprojection    0x00000004

#define emitter_has_gravity             0x00000001
#define emitter_has_drag                0x00000002
#define emitter_is_fountain             0x00000004
#define emitter_has_alphadegradation    0x00000008
#define emitter_has_dof                 0x00000010
#define emitter_particle_effect_only    0x00000020
#define emitter_particle_beam           0x00000040
#define emitter_is_animation            0x00000080
#define emitter_is_watersurfaceeffect   0x00000100
#define emitter_is_landsurfaceeffect    0x00000200
#define emitter_is_water_projection     0x00000400

#define part_has_gravity             0x00000001
#define part_has_drag                0x00000002
#define part_is_watersurfaceeffect   0x00000004
#define part_has_stretch             0x00000008
#define part_has_glow                0x00000010
#define part_has_fixstretch          0x00000020
#define part_no_flip                 0x00000040
#define part_has_time_stages         0x00000080
#define part_has_alpha_stages        0x00000100
#define part_has_size_stages         0x00000200
#define part_has_velvariance         0x00000400
#define part_always_stretches        0x00000800
#define part_dont_update_position    0x00001000
#define part_is_beam                 0x00002000
#define part_has_color_stages        0x00004000
#define part_follows_player_xy       0x00008000
#define part_has_legacy_stretch      0x00010000
#define part_has_random_size         0x00020000
#define part_is_landsurfaceeffect    0x00040000
#define part_no_wind                 0x00080000

#define fx_has_velvariance           0x00000001
#define fx_has_velreduction          0x00000002
#define fx_has_posvariance           0x00000004
#define fx_out_dofdirection          0x00000008
#define fx_random_direction          0x00000010
#define fx_clamp_emitter             0x00000020
#define fx_clamp_beam                0x00000040
#define fx_has_randomglow            0x00000080
#define fx_rotate_particle_to_dir    0x00000100

struct ParticleEffect
{
    std::string name;

    std::vector<int> texture;
    std::vector<int> number;
    std::vector<int> time;
    std::vector<float> alpha;
    std::vector<Vec3d> color;
    std::vector<float> size;
    std::vector<std::string> attachedtransforms;

    float speed;
    float dragcoef;
    float gravitycoef;
    float velreduction;
    float rotate;
    float posvariance[6];
    float velvariance[6];
    float stretchscale;
    float separation;
    float clampbeam;

    int lifetime;
    int delaystart;
    int delayeachparticle;
    int glowtime;
    int stopstretch;
    int randomdirection;
    int randomxydirection;
    int matid;
    int clampemitter;
    int startrotation;

    unsigned int particleflags;
    unsigned int fxflags;

    bool isclone;

    void setsize(float s1, float s2, float s3, float s4)
    {
        size.clear();

        size.push_back(s1);
        size.push_back(s2);
        size.push_back(s3);
        size.push_back(s4);

        particleflags |= part_has_size_stages;
    }

    void setsize(float s1, float s2)
    {
        size.clear();

        size.push_back(s1);
        size.push_back(s2);

        particleflags &= ~part_has_size_stages;
    }

    void settime(int t1, int t2, int t3, int t4)
    {
        time.clear();

        time.push_back(t1);
        time.push_back(t2);
        time.push_back(t3);
        time.push_back(t4);

        particleflags |= part_has_time_stages;
    }

    void setalpha(float a1, float a2, float a3, float a4)
    {
        alpha.clear();

        alpha.push_back(a1);
        alpha.push_back(a2);
        alpha.push_back(a3);
        alpha.push_back(a4);

        particleflags |= part_has_alpha_stages;
    }

    void setposvariance(float variance)
    {
        posvariance[0] = -variance;
        posvariance[1] = -variance;
        posvariance[2] = -variance;
        posvariance[3] = variance;
        posvariance[4] = variance;
        posvariance[5] = variance;

        fxflags |= fx_has_posvariance;
    }

    void reset()
    {
        texture.clear();
        number.clear();
        time.clear();
        alpha.clear();
        size.clear();
        name.clear();

        stretchscale = 0.1f;
        speed = 1.0f;
        clampbeam = 0.0f;
        dragcoef = 0.0f;
        gravitycoef = -9.8f;
        rotate = 0.0f;
        velreduction = 0.0f;
        fxflags = 0;
        particleflags = 0;
        lifetime = 0;
        randomdirection = 0;
        delaystart = 0;
        delayeachparticle = 0;
        separation = 0.1f;
        isclone = false;
        clampemitter = 0;
        glowtime = -1;
        stopstretch = 0;
        matid = -1;
        startrotation = 0;
    }

    ParticleEffect()
    {
        reset();
    }

    ~ParticleEffect()
    {
    }
};

struct Emitterfx
{
    std::vector<std::string> _particleeffectnames;

    int lifetime;
    float numpersecond;

    unsigned int emitterflags;

    Emitterfx() :
        lifetime(-1),
        numpersecond(0),
        emitterflags(0)
    {
    }

    void reset()
    {
        lifetime = -1;
        numpersecond = 0;
        emitterflags = 0;
        _particleeffectnames.clear();
    }
};

struct TransformFx
{
    unsigned int flags;
    int randomdirection;
    float speed;
    float drag;
    float gravity;

    std::string name;
    std::vector<Emitterfx*> _emitters;

    TransformFx() :
        flags(0),
        speed(0),
        drag(0),
        gravity(0)
    {
    }

    void reset()
    {
        flags = 0;
        speed = 0;
        drag = 0;
        gravity = 0;
    }
};

class DataFx
{
public:
    DataFx();
    virtual ~DataFx();

    void init();
    void killeffects();
    void getuv(Vec4f& uv, int i);
    void update();
    void releaseparticleeffect(ParticleEffect* pe);
    void releaseemittereffect(Emitterfx* e);
    void releasetransformeffect(TransformFx* e);

    bool getuv(Vec4f& uv, const char* str);

    TransformFx* gettransformeffect(const char* name);
    ParticleEffect* getparticleeffect(const char* name);
    ParticleEffect* getfreeparticleeffect();
    ParticleEffect* cloneparticleeffect(const char* effect);
    Emitterfx* getfreeemittereffect();
    TransformFx* getfreetransformeffect();

    int getTexture() {return _fxtexture;}
    int getTextureId(const char* name);
    int getMaterialId(const char* name);

    std::vector<Vec4f> _uvcoords;
    int _fxtexture;
    Vec3f _wind;
};

extern void DataFxInit();
extern void DataFxKill();

extern DataFx& FxGet();
