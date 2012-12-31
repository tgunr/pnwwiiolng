#pragma once

#include <vector>
#include <map>
#include <deque>

class Particle
{
public:
    Particle();
    virtual ~Particle() {};

    virtual void update();
    virtual void setdelay(SINT32 delay) {}

    void setid(SINT32 i) {_id = i;}
    void settextureindex(SINT32 i) {_textureindex = i;}
    void applygravity(FLOAT32 time) {_velocity.z -= _gravcoef * time;}
    void applydrag(FLOAT32 time);
    void applywind(FLOAT32 time);
    void updateposition(FLOAT32 time);
    void updatevelocity(FLOAT32 time);
    void getrelativeposition(piVECTOR& pos);
    void rotateextents(FLOAT32 time);
    void setextents(SINT32 i, FLOAT32 x, FLOAT32 y, FLOAT32 z);
    void setworld(piMATRIX& matrix) {world = matrix;}
    void popflag(unsigned int flag) {_flags &= ~flag;};

    SINT32 lifetime() {return _time[3] - _time[0];}
    SINT32 age() {return frametime - _time[0];}
    SINT32 updatetime() {return frametime - _lastupdate;}
    SINT32 getid() {return _id;}
    virtual SINT32 gettextureindex() {return _textureindex;}

    bool expired() {return age() > lifetime();}
    bool hasstretch() {return _flags & part_has_stretch || hasfixedstretch();}
    bool hasdrag() {return _flags & part_has_drag;}
    bool hasgravity() {return _flags & part_has_gravity;}
    bool hasrotate() {return _flags & part_has_rotate;}
    bool hasfixedstretch() {return _flags & part_has_fixstretch;}
    bool hastimestages() {return _flags & part_has_time_stages;}
    bool hassizestages() {return _flags & part_has_size_stages;}
    bool hasalphastages() {return _flags & part_has_alpha_stages;}
    bool hasnoflip() {return _flags & part_no_flip;}
    bool hasvelvariance() {return _flags & part_has_velvariance;}
    bool stretchalways() {return _flags & part_always_stretches;}

    bool hasglow()
    {
        return (_flags & part_has_glow) && (_glowtime == -1 || age() < _glowtime);
    }

    bool stretchParticle(piVECTOR corner[4], FLOAT32 time, FLOAT32 len);

    virtual bool setupshape(piVECTOR corner[4], FLOAT32 time);

    UINT32 _flags;

    SINT32 _lifetime;
    SINT32 _time[4];
    SINT32 _lastupdate;
    SINT32 _material;
    SINT32 _textureindex;
    SINT32 _glowtime;
    SINT32 _stopstretch;

    FLOAT32 _distance2;
    FLOAT32 _sizestart;
    FLOAT32 _sizeend;
    FLOAT32 _alphastart;
    FLOAT32 _alphaend;
    FLOAT32 _alpha[4];
    FLOAT32 _size[4];
    FLOAT32 _stretchscale;
    FLOAT32 _currentalpha;
    FLOAT32 _currentsize;
    FLOAT32 _dragcoeff;
    FLOAT32 _rotate;
    FLOAT32 _gravcoef;

    FLOAT64VECTOR _position;

    piVECTOR _velocity;
    piVECTOR _velvariance;
    piVECTOR _windvelocity;
    piVECTOR _extents[4];

    piMATRIX world;

    bool _dead;

private:

    SINT32 _id;
};

class ParticleManager
{
public:
    ParticleManager();
    virtual ~ParticleManager();

    void kill();
    void remove(SINT32 id);
    void update();
    void draw(float distance2, int& fxindex);
    void init();
    void computebillboard();
    void setcolor(piCOLORVALUE& color) {_color = color;}

    Particle* getparticle();

private:
    UINT32 _getColor(piVECTOR& pos, Particle* p);

private:
    std::vector<Particle *> _draw;
    std::deque<Particle *> _freeparticle;
    std::map<SINT32, Particle *> _particle;

    piCOLORVALUE _color;
};
