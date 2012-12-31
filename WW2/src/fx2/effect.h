#pragma once

#define USE_NEW_EFFECT
//#undef USE_NEW_EFFECT

#include <vector>
#include "fx2/fxdata.h"
#include "weap/weapammo.h"

struct EMITTER
{
    std::vector<FX *> _fx;

    SINT32 id;
    SINT32 createtime;
    SINT32 lifetime;
    SINT32 lastupdate;
    SINT32 numpersecond;
    SINT32 lastparticle;

    OBJECT* o;
    const weapAmmo* ammo;

    UINT32 particleflags;
    UINT32 emitterflags;

    FLOAT64VECTOR position;
    FLOAT64VECTOR lastposition;

    FLOAT32 gravitycoef;
    FLOAT32 dragcoef;
    FLOAT32 separation;

    piVECTOR velocity;

    bool dead;

    void (*update)(EMITTER*);

    bool hasgravity() {return emitterflags & emitter_has_gravity;}
    bool hasdrag() {return emitterflags & emitter_has_drag;}
    bool isfountain() {return emitterflags & emitter_is_fountain;}
    bool hasalphadegradation() {return emitterflags & emitter_has_alphadegradation;}
    bool hasdof() {return emitterflags & emitter_has_dof;}

    SINT32 age() {return frametime - createtime;}
    SINT32 updatetime() {return frametime - lastupdate;}
    SINT32 lastparticletime() {return frametime - lastparticle;}

    bool expired() {return lifetime != -1 && age() > lifetime;}
} ;

void fxinit();
void fxdraw(float distance2, int& fxindex);
void fxdrawRemainingParticles(int fxindex);
void fxkill();
void fxremove(SINT32 id);
void fxupdate();
void fxsetcolor(piCOLORVALUE& color);

SINT32 fxaddeffect(const char* effect, const FLOAT64VECTOR& p, const piVECTOR& n, int lifetime);
SINT32 fxaddeffectobject(const char* effect, const FLOAT64VECTOR& p, OBJECT* o, const piVECTOR& n, int lifetime);
SINT32 fxaddeffectdof(const char* effect, OBJECT* o, const piVECTOR& n, int lifetime);
SINT32 fxaddeffectammo(const char* effect, const weapAmmo* ammo, const piVECTOR& n, int lifetime);
SINT32 fxadddecal(OBJECT* o, const piVECTOR& pos, SINT32 polygon);
SINT32 fxaddfire(FLOAT64VECTOR& position, SINT32 lifetime, FLOAT32 size);

SINT32 fxaddbillowsmoke(FLOAT64VECTOR& position,
                        piVECTOR& velocity,
                        SINT32 lifetime,
                        SINT32 particletime[4],
                        FLOAT32 alpha[4],
                        FLOAT32 sizes[4],
                        OBJECT* o);

SINT32 fxaddbillowfire(FLOAT64VECTOR& position,
                       SINT32 lifetime,
                       SINT32 particletime,
                       FLOAT32 alphastart,
                       FLOAT32 alphaend,
                       FLOAT32 sizestart,
                       FLOAT32 sizeend);
