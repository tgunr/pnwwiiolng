#pragma once

#include "datafx.h"
#include "math/Matrixf.h"

extern int frametime;

class Particle2
{
public:
    Particle2();
    virtual ~Particle2() {};

    virtual void update(const piVECTOR& delta);
    virtual void setdelay(int delay) {}

    void setid(int i) {_id = i;}
    void settextureindex(int i) {_textureindex = i;}
    void applygravity(float time);
    void applydrag(float time);
    void applywind(float time);
    void updateposition(float time);
    void updatevelocity(float time);
    void getrelativeposition(Vec3f& pos);
    void getrelativeposition(Vec3f& outpos, const Vec3d& inpos);
    void rotateextents(float time);
    void setextents(int i, float x, float y, float z);
    void popflag(unsigned int flag) {_flags &= ~flag;};
    void setname(const char* name) {_name = name;}
    void reset();

    const char* getname() {return _name.c_str();}

    int lifetime() {return _time[3] - _time[0];}
    int age() {return frametime - _time[0];}
    int updatetime() {return frametime - _lastupdate;}
    int drawtime() {return frametime - _lastdrawtime;}
    int getid() {return _id;}
    virtual int gettextureindex() {return _textureindex;}

    bool canstretch();
    bool expired()
    {
        if(_lifetime == -1)
            return false;

        if(age() > lifetime())
            return true;

        return false;
    }
    bool hasstretch()
    {
        if(_stretchlength <= 1.0f)
            return false;

        return _flags & part_has_stretch || hasfixedstretch();
    }
    bool hasfixedstretch() {return _flags & part_has_fixstretch;}
    bool haslegacystretch() {return _flags & part_has_legacy_stretch;}
    bool hasdrag() {return _flags & part_has_drag;}
    bool hasgravity() {return _flags & part_has_gravity;}
    bool hasrandomsize() {return _flags & part_has_random_size;}
    bool hastimestages() {return _flags & part_has_time_stages;}
    bool hassizestages() {return _flags & part_has_size_stages;}
    bool hasalphastages() {return _flags & part_has_alpha_stages;}
    bool hascolorstages() {return _flags & part_has_color_stages;}
    bool hasnoflip() {return _flags & part_no_flip;}
    bool hasvelvariance() {return _flags & part_has_velvariance;}
    bool stretchalways() {return _flags & part_always_stretches;}
    bool dontupdateposition() {return _flags & part_dont_update_position;}
    bool isparticlebeam() {return _flags & part_is_beam;}
    bool iswatersurfaceeffect() {return _flags & part_is_watersurfaceeffect;}

    bool istype(unsigned int type) {return _flags & type;}

    bool hasglow()
    {
        return (_flags & part_has_glow) && (_glowtime == -1 || age() < _glowtime);
    }

    bool stretchParticle(Vec3f corner[4]);
    bool LegacyStretchParticle(Vec3f corner[4]);
    bool fixstretchParticle(Vec3f corner[4]);
    bool particleBeam(Vec3f corner[4]);
    bool particleTracer(Vec3f corner[4]);

    virtual bool setupshape();

    unsigned int _flags;

    short _stopstretch;
    short _material;
    short _textureindex;

    int _glowtime;
    int _lifetime;
    int _time[4];
    int _lastupdate;
    int _lastdrawtime;

    float _distance2;
    float _alpha[4];
    float _size[4];
    float _stretchscale;
    float _currentalpha;
    float _currentsize;
    float _dragcoeff;
    float _rotate;
    float _gravcoef;
    float _stretchlength;

    void (*_killcallback)(Particle2* p);
    bool (*_updatecallback)(Particle2* p);

    Vec3d _originalposition;
    Vec3d _position;
    Vec3d _lastposition;
    Vec3f _color[4];
    Vec3f _corners[4];

    Vec3f _velocity;
    Vec3f _velvariance;
    Vec3f _extents[4];
    Vec3f _endpos;
    Vec4f _currentcolor;

    bool _dead;
    bool _particleeffectonly;
    bool _hasendpos;
    bool _ready;
    bool _enabled;

private:
    std::string _name;

    int _id;
};
