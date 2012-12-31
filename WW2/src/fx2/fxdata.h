#pragma once

#include <vector>
#include <string>

#define emitter_has_gravity             0x00000001
#define emitter_has_drag                0x00000002
#define emitter_is_fountain             0x00000004
#define emitter_has_alphadegradation    0x00000008
#define emitter_has_dof                 0x00000010

#define part_has_gravity             0x00000001
#define part_has_drag                0x00000002
#define part_has_rotate              0x00000004
#define part_has_stretch             0x00000008
#define part_has_glow                0x00000010
#define part_has_fixstretch          0x00000020
#define part_no_flip                 0x00000040
#define part_has_time_stages         0x00000080
#define part_has_alpha_stages        0x00000100
#define part_has_size_stages         0x00000200
#define part_has_velvariance         0x00000400
#define part_always_stretches        0x00000800

#define fx_has_velvariance           0x00000001
#define fx_has_velreduction          0x00000002
#define fx_has_posvariance           0x00000004
#define fx_out_dofdirection          0x00000008
#define fx_random_direction          0x00000010
#define fx_overridedirection         0x00000020

struct FX
{
    std::vector<SINT32> texture;
    std::vector<SINT32> number;
    std::vector<SINT32> time;
    std::vector<FLOAT32> alpha;
    std::vector<FLOAT32> size;
    std::vector<std::string> use;
    std::vector<std::string> attach;

    FLOAT32 speed;
    FLOAT32 dragcoef;
    FLOAT32 gravitycoef;
    FLOAT32 velreduction;
    FLOAT32 rotate;
    FLOAT32 separation;
    FLOAT32 stretchscale;
    FLOAT32 posvariance[6];
    FLOAT32 velvariance[6];

    piVECTOR overridedir;

    SINT32 matid;
    SINT32 lifetime;
    SINT32 delaystart;
    SINT32 delayeachparticle;
    SINT32 numpersec;
    SINT32 glowtime;
    SINT32 stopstretch;
    SINT32 randomdirection;

    UINT32 particleflags;
    UINT32 fxflags;
    UINT32 emitterflags;

    bool isemitter;

    FX()
		: speed(1.0f)
		, dragcoef(0.0f)
		, gravitycoef(-9.8f)
		, velreduction(0.0f)
		, rotate(0.0f)
		, separation(0.1f)
		, stretchscale(0.1f)

		, matid(0)
		, lifetime(0)
		, delaystart(0)
		, delayeachparticle(0)
		, numpersec(0)
		, glowtime(-1)
		, stopstretch(0)
		, randomdirection(0)
		, particleflags(0)
		, fxflags(0)
		, emitterflags(0)
		, isemitter(false)
    {
    }

    ~FX()
    {
        texture.clear();
        number.clear();
        time.clear();
        alpha.clear();
        size.clear();
        use.clear();
        attach.clear();
    }

};

struct EFFECT
{
    std::string name;
    std::vector<FX *> fx;

};

class FxData
{
public:
    FxData();
    virtual ~FxData();

    void init();
    void kill();
    void killeffects();
    void getuv(FLOAT32& u1, FLOAT32& v1, FLOAT32& u2, FLOAT32& v2, SINT32 i);
    void update();
    void bind() {piSetActiveTexture(_particletex, 0);}

    EFFECT* geteffect(const SCHAR* name);

    std::vector<piQUAT> _uvcoords;

    piVECTOR _wind;

    SINT32 _smokematerial;
    SINT32 _firematerial;
    SINT32 _watermaterial;
    SINT32 _particletex;
    SINT32 _bullettex;
};

extern FxData _fxdata;
