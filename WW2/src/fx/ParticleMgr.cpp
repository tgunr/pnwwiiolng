#include "syscfg.h" // Path is project specific.
/*----------------------------------------------------------------------
Module: particlemgr.cpp
Description:
-----------------------------------------------------------------------*/
#include "include/vars.h"

#include <algorithm>

#include "fx/ParticleMgr.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/mathUnitConversion.h"
#include "rn/rnvis.h"
#include "trn/trnvars.h"
#include "ogl/oglGlobalStates.h"
#include "ui/uiChat.h"
#include "fx/TransformEffect.h"
#include "spt/Random.h"
#include "ob/obdlite.h"
#include "ob/obdraw.h"

#define MAX_PARTICLES   40960

static int _particleid = 1;

#if(0)
static Particle2* chunk = nullptr;
#endif

/*----------------------------------------------------------------------
Function: ParticleMgr::ParticleMgr
Description:
-----------------------------------------------------------------------*/
ParticleMgr::ParticleMgr() :
    _killMyself(false),
    UsingSoftParticles(g_postRenderFilter),
    _particleindex(0),
    _pgm(nullptr),
    _particlemgrInit(false)
{
}

/*----------------------------------------------------------------------
Function: ~ParticleMgr
Description:
-----------------------------------------------------------------------*/
ParticleMgr::~ParticleMgr()
{
    if(_particlemgrInit)
    {
        kill();
    }
}

/*----------------------------------------------------------------------
Function: ParticleMgr::setupparticleanim
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::setupparticleanim(const ParticleEffect* fx,
                                    const Vec3f& normal,
                                    const Vec3d& pos,
                                    int starttime,
                                    int& animindex,
                                    Particle2*& p)
{
    if(!fx)
    {
        return;
    }

    float alphadegradation = 1.0f;

    Vec3f n = Vec3f(normal.x(), normal.y(), normal.z());

    bool init = false;

    if(animindex == -1)
    {
        init = true;
        p = getparticle();
        animindex = GetRandom(fx->texture[0], fx->texture[1]);
    }

    if(animindex >= fx->texture[1])
    {
        animindex = fx->texture[0];
    }

    Vec3f velocity = n;

    int delayoffset = 0;

    if(init)
    {
        if(!p)
        {
            return;
        }

        p->_material = fx->matid;
        p->_lifetime = fx->lifetime;

        if(fx->fxflags & fx_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomdirection) * 0.5f;

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

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                if(fabsf(basis * normal) > 0.999f)
                {
                    basis.x() = 0.0f;
                    basis.y() = 1.0f;
                    basis.z() = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - n * dot;
                u.fastnormalize();

                Vec3f v = normal ^ u;

                n = normal * dist + u * x + v * y;
            }
            else if(fx->randomxydirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomxydirection) * 0.5f;

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

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - Vec3f(n.x(), n.y(), 0.0f) * dot;
                u.fastnormalize();

                Vec3f v = Vec3f(normal.x(), normal.y(), 0.0f) ^ u;

                n = normal * dist + u * x + v * y;
            }
            else
            {
                n.x() = GetRandom(-1.0f, 1.0f);
                n.y() = GetRandom(-1.0f, 1.0f);
                n.z() = GetRandom(-1.0f, 1.0f);
            }

            n.fastnormalize();
            velocity = n;
        }

        p->_flags = fx->particleflags;

        p->_alpha[0] = fx->alpha[0] * alphadegradation;
        p->_alpha[1] = fx->alpha[1] * alphadegradation;

        if(p->hasalphastages())
        {
            p->_alpha[2] = fx->alpha[2] * alphadegradation;
            p->_alpha[3] = fx->alpha[3] * alphadegradation;
        }

        if(!fx->color.empty())
        {
            p->_color[0] = fx->color[0];
            p->_color[1] = fx->color[1];

            if(p->hascolorstages())
            {
                p->_color[2] = fx->color[2];
                p->_color[3] = fx->color[3];
            }
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
        else if(p->_lifetime != -1)
        {
            p->_time[0] = starttime + fx->delaystart + delayoffset;
            p->_time[3] = p->_time[0] + fx->lifetime;
        }

        p->_dragcoeff = fx->dragcoef;
        p->_gravcoef = fx->gravitycoef;
        p->_stretchscale = fx->stretchscale;
        p->_glowtime = fx->glowtime;
        p->_stopstretch = fx->stopstretch;

        if(fx->rotate != 0)
        {
            p->_rotate = GetRandom(-fx->rotate, fx->rotate);
        }

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_velocity = velocity;
        p->_position = pos;
        p->_lastposition = pos;
        p->_originalposition = pos;

        if(fx->fxflags & fx_has_velvariance)
        {
            p->_velvariance.x() = GetRandom(fx->velvariance[0], fx->velvariance[3]);
            p->_velvariance.y() = GetRandom(fx->velvariance[1], fx->velvariance[4]);
            p->_velvariance.z() = GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            velocity *= fx->velreduction;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            p->_position.x() += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            p->_position.y() += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            p->_position.z() += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        if(fx->fxflags & fx_has_randomglow)
        {
            bool glow = GetRandom(0, 1);

            if(glow)
                p->_flags |= part_has_glow;
        }

        float length = math3dFastSqrt(velocity * velocity);

        if(p->_flags & part_has_stretch && length < 1.0)
        {
            p->popflag(part_has_stretch | part_has_fixstretch);
        }

        delayoffset += fx->delayeachparticle;
    }

    p->_textureindex = animindex;

    animindex++;
}

/*----------------------------------------------------------------------
Function: ParticleMgr::setupparticle
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::setupparticle(const ParticleEffect* fx,
                                const Vec3f& normal,
                                const Vec3d& pos,
                                int starttime,
                                Particle2* p)
{
    if(!fx)
    {
        return;
    }

    Vec3f n = Vec3f(normal.x(), normal.y(), normal.z());

    float speed = fx->speed;

    if(fx->number.empty())
    {
        return;
    }

    unsigned int size = fx->number.size();

    int num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    Vec3f velocity = n * speed;

    int delayoffset = 0;

    // Some particles can emit other particles
    if(!fx->attachedtransforms.empty())
    {
        for(unsigned int q = 0; q < fx->attachedtransforms.size(); q++)
        {
            TransformEffect* te = TransformEffect::AddEffect(fx->attachedtransforms[q].c_str(), n, 0, nullptr);

            if(te)
            {
                te->enableemitters(true);;
                te->setmatrix(Matrixf::translate(pos));
                te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
            }

            te->link(p);
        }
    }

    p->_material = fx->matid;
    p->_lifetime = fx->lifetime;

    if(fx->fxflags & fx_random_direction)
    {
        if(fx->randomdirection > 0)
        {
            float radius = DEG_TO_RAD((float)fx->randomdirection) * 0.5f;

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

            Vec3f basis = Vec3f(1.0, 0.0, 0.0);

            if(fabsf(basis * normal) > 0.999f)
            {
                basis.x() = 0.0f;
                basis.y() = 1.0f;
                basis.z() = 0.0f;
            }

            // Project away N component, normalize and cross to get
            // second orthonormal vector.
            Vec3f u;
            float dot = basis * normal;

            u = basis - n * dot;
            u.fastnormalize();

            if(u.length2() < 0.00001)
                u = Vec3f(0, 0, -1);

            Vec3f v = normal ^ u;

            if(v.length2() < 0.00001)
                v = Vec3f(0, 0, -1);

            n = normal * dist + u * x + v * y;
        }
        else if(fx->randomxydirection > 0)
        {
            float radius = DEG_TO_RAD((float)fx->randomxydirection) * 0.5f;

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

            Vec3f basis = Vec3f(1.0, 0.0, 0.0);

            if(fabsf(basis * normal) > 0.999f)
            {
                basis.x() = 0.0f;
                basis.y() = 1.0f;
                basis.z() = 0.0f;
            }

            // Project away N component, normalize and cross to get
            // second orthonormal vector.
            Vec3f u;
            float dot = basis * normal;

            u = basis - n * dot;
            u.fastnormalize();

            if(u.length2() < 0.00001)
                u = Vec3f(0, 0, -1);

            Vec3f v = normal ^ u;

            if(v.length2() < 0.00001)
                v = Vec3f(0, 0, -1);

            n = Vec3f(normal.x(), normal.y(), 0.0f) * dist + Vec3f(u.x(), u.y(), 0.0f) * x + v * y;
        }
        else
        {
            n.x() = GetRandom(-1.0f, 1.0f);
            n.y() = GetRandom(-1.0f, 1.0f);
            n.z() = GetRandom(-1.0f, 1.0f);
        }

        n.fastnormalize();
        velocity = n * speed;
    }

    p->_flags = fx->particleflags;

    if(fx->texture.size() > 1)
    {
        p->_textureindex = fx->texture[GetRandom(0, fx->texture.size() - 1)];
    }
    else
    {
        p->_textureindex = fx->texture[0];
    }

    p->_alpha[0] = fx->alpha[0];
    p->_alpha[1] = fx->alpha[1];

    if(p->hasalphastages())
    {
        p->_alpha[2] = fx->alpha[2];
        p->_alpha[3] = fx->alpha[3];
    }

    if(!fx->color.empty())
    {
        p->_color[0] = fx->color[0];
        p->_color[1] = fx->color[1];

        if(p->hascolorstages())
        {
            p->_color[2] = fx->color[2];
            p->_color[3] = fx->color[3];
        }
    }

    if(p->hasrandomsize())
    {
        float size = PNRandom::random(fx->size[0], fx->size[1]);

        p->_size[0] = size;
        p->_size[1] = size;
    }
    else
    {
        p->_size[0] = fx->size[0];
        p->_size[1] = fx->size[1];

        if(p->hassizestages())
        {
            p->_size[2] = fx->size[2];
            p->_size[3] = fx->size[3];
        }
    }

    if(p->hastimestages())
    {
        p->_time[0] = starttime + fx->time[0] + fx->delaystart + delayoffset;
        p->_time[1] = p->_time[0] + fx->time[1];
        p->_time[2] = p->_time[1] + fx->time[2];
        p->_time[3] = p->_time[2] + fx->time[3];
    }
    else if(p->_lifetime != -1)
    {
        p->_time[0] = starttime + fx->delaystart + delayoffset;
        p->_time[3] = p->_time[0] + fx->lifetime;
    }

    p->_dragcoeff = fx->dragcoef;
    p->_gravcoef = fx->gravitycoef;
    p->_stretchscale = fx->stretchscale;
    p->_glowtime = fx->glowtime;
    p->_stopstretch = fx->stopstretch;

    // May want to start particle at certain orientation.
    // Must be done before setting p->_rotate below
    if(fx->startrotation != 0)
    {
        p->_rotate = (float)fx->startrotation;
        p->rotateextents(1);
        p->_rotate = 0;
    }

    if(fx->rotate != 0)
    {
        p->_rotate = GetRandom(-fx->rotate, fx->rotate);
    }

    p->_currentsize = p->_size[0];
    p->_currentalpha = p->_alpha[0];

    p->_velocity = velocity;
    p->_position = pos;
    p->_lastposition = pos;
    p->_originalposition = pos;

    if(fx->fxflags & fx_has_velvariance)
    {
        p->_velvariance.x() = GetRandom(fx->velvariance[0], fx->velvariance[3]);
        p->_velvariance.y() = GetRandom(fx->velvariance[1], fx->velvariance[4]);
        p->_velvariance.z() = GetRandom(fx->velvariance[2], fx->velvariance[5]);
    }

    if(fx->fxflags & fx_has_velreduction)
    {
        speed *= fx->velreduction;

        p->_velocity.fastnormalize();
        p->_velocity *= speed;
    }

    if(fx->fxflags & fx_has_posvariance)
    {
        p->_position.x() += GetRandom(fx->posvariance[0], fx->posvariance[3]);
        p->_position.y() += GetRandom(fx->posvariance[1], fx->posvariance[4]);
        p->_position.z() += GetRandom(fx->posvariance[2], fx->posvariance[5]);
    }

    if(fx->fxflags & fx_has_randomglow)
    {
        bool glow = GetRandom(0, 1);

        if(glow)
            p->_flags |= part_has_glow;
    }

    float length = math3dFastSqrt(velocity * velocity);

    if(p->_flags & part_has_stretch && length < 1.0)
    {
        p->popflag(part_has_stretch | part_has_fixstretch);
    }
    else if(p->_flags & part_has_legacy_stretch && length < 1.0)
    {
        p->popflag(part_has_legacy_stretch);
    }
    else if(p->_flags & part_has_stretch || p->_flags & part_has_fixstretch || p->_flags & part_has_legacy_stretch)
    {
        if(p->_flags & part_has_fixstretch)
        {
            p->_stretchlength = p->_stretchscale;
        }
        else
        {
            p->_stretchlength = length * p->_stretchscale;
        }

        // Only reduce if necessary
        if(!(fx->fxflags & fx_has_velreduction))
        {
            p->_velocity.fastnormalize();
            p->_velocity *= speed;
        }
    }

    p->setname(fx->name.c_str());

    delayoffset += fx->delayeachparticle;
}

/*----------------------------------------------------------------------
Function: ParticleMgr::setupparticle
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::setupparticle(const ParticleEffect* fx,
                                const Vec3f& normal,
                                const Vec3d& pos,
                                int starttime,
                                float alphadegradation)
{
    if(!fx)
    {
        return;
    }

    Vec3f n = Vec3f(normal.x(), normal.y(), normal.z());

    float speed = fx->speed;

    if(fx->number.empty())
    {
        return;
    }

    unsigned int size = fx->number.size();

    int num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    Vec3f velocity = n * speed;

    int delayoffset = 0;

    for(int i = 0; i < num; i++)
    {
        Particle2* p = getparticle();

        if(!p)
        {
            break;
        }

        // Some particles can emit other particles
        if(!fx->attachedtransforms.empty())
        {
            for(unsigned int q = 0; q < fx->attachedtransforms.size(); q++)
            {
                TransformEffect* te = TransformEffect::AddEffect(fx->attachedtransforms[q].c_str(), n, 0, nullptr);

                if(te)
                {
                    te->enableemitters(true);;
                    te->setmatrix(Matrixf::translate(pos));
                    te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                }

                te->link(p);
            }
        }

        p->_material = fx->matid;
        p->_lifetime = fx->lifetime;

        if(fx->fxflags & fx_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomdirection) * 0.5f;

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

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                if(fabsf(basis * normal) > 0.999f)
                {
                    basis.x() = 0.0f;
                    basis.y() = 1.0f;
                    basis.z() = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - n * dot;
                u.fastnormalize();

                Vec3f v = normal ^ u;

                n = normal * dist + u * x + v * y;
            }
            else if(fx->randomxydirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomxydirection) * 0.5f;

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

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                if(fabsf(basis * normal) > 0.999f)
                {
                    basis.x() = 0.0f;
                    basis.y() = 1.0f;
                    basis.z() = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - n * dot;
                u.fastnormalize();

                Vec3f v = normal ^ u;

                n = Vec3f(normal.x(), normal.y(), 0.0f) * dist + Vec3f(u.x(), u.y(), 0.0f) * x + v * y;
            }
            else
            {
                n.x() = GetRandom(-1.0f, 1.0f);
                n.y() = GetRandom(-1.0f, 1.0f);
                n.z() = GetRandom(-1.0f, 1.0f);
            }

            n.fastnormalize();
            velocity = n * speed;
        }

        p->_flags = fx->particleflags;

        if(fx->texture.size() > 1)
        {
            p->_textureindex = fx->texture[GetRandom(0, fx->texture.size() - 1)];
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

        if(!fx->color.empty())
        {
            p->_color[0] = fx->color[0];
            p->_color[1] = fx->color[1];

            if(p->hascolorstages())
            {
                p->_color[2] = fx->color[2];
                p->_color[3] = fx->color[3];
            }
        }

        if(p->hasrandomsize())
        {
            float size = PNRandom::random(fx->size[0], fx->size[1]);

            p->_size[0] = size;
            p->_size[1] = size;
        }
        else
        {
            p->_size[0] = fx->size[0];
            p->_size[1] = fx->size[1];

            if(p->hassizestages())
            {
                p->_size[2] = fx->size[2];
                p->_size[3] = fx->size[3];
            }
        }

        if(p->hastimestages())
        {
            p->_time[0] = starttime + fx->time[0] + fx->delaystart + delayoffset;
            p->_time[1] = p->_time[0] + fx->time[1];
            p->_time[2] = p->_time[1] + fx->time[2];
            p->_time[3] = p->_time[2] + fx->time[3];
        }
        else if(p->_lifetime != -1)
        {
            p->_time[0] = starttime + fx->delaystart + delayoffset;
            p->_time[3] = p->_time[0] + fx->lifetime;
        }

        p->_dragcoeff = fx->dragcoef;
        p->_gravcoef = fx->gravitycoef;
        p->_stretchscale = fx->stretchscale;
        p->_glowtime = fx->glowtime;
        p->_stopstretch = fx->stopstretch;

        if(fx->rotate != 0)
        {
            p->_rotate = GetRandom(-fx->rotate, fx->rotate);
        }

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_velocity = velocity;
        p->_position = pos;
        p->_lastposition = pos;
        p->_originalposition = pos;

        if(fx->fxflags & fx_has_velvariance)
        {
            p->_velvariance.x() = GetRandom(fx->velvariance[0], fx->velvariance[3]);
            p->_velvariance.y() = GetRandom(fx->velvariance[1], fx->velvariance[4]);
            p->_velvariance.z() = GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            speed *= fx->velreduction;

            p->_velocity.fastnormalize();
            p->_velocity *= speed;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            p->_position.x() += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            p->_position.y() += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            p->_position.z() += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        if(fx->fxflags & fx_has_randomglow)
        {
            bool glow = GetRandom(0, 1);

            if(glow)
                p->_flags |= part_has_glow;
        }

        float length = math3dFastSqrt(velocity * velocity);

        if(p->_flags & part_has_stretch && length < 1.0)
        {
            p->popflag(part_has_stretch | part_has_fixstretch);
        }
        else if(p->_flags & part_has_legacy_stretch && length < 1.0)
        {
            p->popflag(part_has_legacy_stretch);
        }
        else if(p->_flags & part_has_stretch || p->_flags & part_has_fixstretch || p->_flags & part_has_legacy_stretch)
        {
            if(p->_flags & part_has_fixstretch)
            {
                p->_stretchlength = p->_stretchscale;
            }
            else
            {
                p->_stretchlength = length * p->_stretchscale;
            }

            // Only reduce if necessary
            if(!(fx->fxflags & fx_has_velreduction))
            {
                p->_velocity.fastnormalize();
                p->_velocity *= speed;
            }
        }

        p->setname(fx->name.c_str());

        delayoffset += fx->delayeachparticle;
    }
}

/*----------------------------------------------------------------------
Function: ParticleMgr::setupsurfaceparticle
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::setupsurfaceparticle(const ParticleEffect* fx,
                                       const Vec3f& normal,
                                       const Vec3d& pos,
                                       int starttime,
                                       float alphadegradation)
{
    if(!gWaterShadersEnabled)
    {
        return;
    }

    if(!fx)
    {
        return;
    }

    Vec3f n = Vec3f(normal.x(), normal.y(), normal.z());
    Vec3f dir(n);

    dir.normalize();

    float speed = fx->speed;

    if(fx->number.empty())
    {
        return;
    }

    unsigned int size = fx->number.size();

    int num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    Vec3f velocity = n * speed;

    int delayoffset = 0;

    for(int i = 0; i < num; i++)
    {
        Particle2* p = getparticle();

        if(!p)
        {
            break;
        }

        // Some particles can emit other particles
        if(!fx->attachedtransforms.empty())
        {
            for(unsigned int q = 0; q < fx->attachedtransforms.size(); q++)
            {
                TransformEffect* te = TransformEffect::AddEffect(fx->attachedtransforms[q].c_str(), n, 0, nullptr);

                if(te)
                {
                    te->enableemitters(true);;
                    te->setmatrix(Matrixf::translate(pos));
                    te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                }

                te->link(p);
            }
        }

        p->_material = fx->matid;
        p->_lifetime = fx->lifetime;

        if(fx->fxflags & fx_random_direction)
        {
            if(fx->randomdirection > 0)
            {
                float radius = DEG_TO_RAD((float)fx->randomdirection) * 0.5f;

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

                Vec3f basis = Vec3f(1.0, 0.0, 0.0);

                if(fabsf(basis * normal) > 0.999f)
                {
                    basis.x() = 0.0f;
                    basis.y() = 1.0f;
                    basis.z() = 0.0f;
                }

                // Project away N component, normalize and cross to get
                // second orthonormal vector.
                Vec3f u;
                float dot = basis * normal;

                u = basis - n * dot;
                u.fastnormalize();

                Vec3f v = normal ^ u;

                n = normal * dist + u * x + v * y;
            }
            else
            {
                n.x() = GetRandom(-1.0f, 1.0f);
                n.y() = GetRandom(-1.0f, 1.0f);
                n.z() = GetRandom(-1.0f, 1.0f);
            }

            n.fastnormalize();
            velocity = n * speed;
        }

        p->_flags = fx->particleflags;
        p->_flags |= part_is_watersurfaceeffect;

        if(fx->texture.size() > 1)
        {
            p->_textureindex = fx->texture[GetRandom(0, fx->texture.size() - 1)];
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

        if(!fx->color.empty())
        {
            p->_color[0] = fx->color[0];
            p->_color[1] = fx->color[1];

            if(p->hascolorstages())
            {
                p->_color[2] = fx->color[2];
                p->_color[3] = fx->color[3];
            }
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
        else if(p->_lifetime != -1)
        {
            p->_time[0] = starttime + fx->delaystart + delayoffset;
            p->_time[3] = p->_time[0] + fx->lifetime;
        }

        p->_dragcoeff = fx->dragcoef;
        p->_gravcoef = fx->gravitycoef;
        p->_stretchscale = fx->stretchscale;
        p->_glowtime = fx->glowtime;
        p->_stopstretch = fx->stopstretch;

        if(fx->rotate != 0)
        {
            p->_rotate = GetRandom(-fx->rotate, fx->rotate);
        }

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_velocity = velocity;
        p->_position = pos;
        p->_lastposition = pos;
        p->_originalposition = pos;

        if(fx->fxflags & fx_has_velvariance)
        {
            p->_velvariance.x() = GetRandom(fx->velvariance[0], fx->velvariance[3]);
            p->_velvariance.y() = GetRandom(fx->velvariance[1], fx->velvariance[4]);
            p->_velvariance.z() = GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_rotate_particle_to_dir)
        {
            Matrixf rot;
            rot.makeRotate(Vec3f(0, 1, 0), dir);

            p->_extents[0] = p->_extents[0] * rot;
            p->_extents[1] = p->_extents[1] * rot;
            p->_extents[2] = p->_extents[2] * rot;
            p->_extents[3] = p->_extents[3] * rot;
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            velocity *= fx->velreduction;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            p->_position.x() += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            p->_position.y() += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            p->_position.z() += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        if(fx->fxflags & fx_has_randomglow)
        {
            bool glow = GetRandom(0, 1);

            if(glow)
                p->_flags |= part_has_glow;
        }

        float length = math3dFastSqrt(velocity * velocity);

        if(p->_flags & part_has_stretch && length < 1.0)
        {
            p->popflag(part_has_stretch | part_has_fixstretch);
        }
        else if((p->_flags & part_has_stretch) || (p->_flags & part_has_fixstretch))
        {
            if(p->_flags & part_has_fixstretch)
            {
                p->_stretchlength = p->_stretchscale;
            }
            else
            {
                p->_stretchlength = length * p->_stretchscale;
            }

            p->_velocity.fastnormalize();
            p->_velocity *= speed;
        }

        delayoffset += fx->delayeachparticle;
    }
}

/*----------------------------------------------------------------------
Function: ParticleMgr::createparticle
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::createparticle(Vec3d& pos,
                                 Vec3f& vel,
                                 float size[4],
                                 float alpha[4],
                                 int time[4],
                                 const char* tex,
                                 float rotation,
                                 unsigned int flags)
{
    int texid = FxGet().getTextureId(tex);

    if(texid == -1)
        return;

    Particle2* p = getparticle();

    if(!p)
        return;

    p->_flags = flags;
    p->_textureindex = texid;

    for(int i = 0; i < 4; i++)
    {
        p->_alpha[i] = alpha[i];
        p->_size[i] = size[i];
    }

    p->_time[0] = frametime + time[0];
    p->_time[1] = p->_time[0] + time[1];
    p->_time[2] = p->_time[1] + time[2];
    p->_time[3] = p->_time[2] + time[3];

    p->_flags |= part_has_time_stages;
    p->_flags |= part_has_alpha_stages;

    p->_rotate = rotation;

    p->_currentsize = p->_size[0];
    p->_currentalpha = p->_alpha[0];

    p->_velocity = vel;
    p->_position = pos;
    p->_lastposition = pos;
    p->_originalposition = pos;
}

/*----------------------------------------------------------------------
Function: ParticleMgr::createparticle
Description:
-----------------------------------------------------------------------*/
Particle2* ParticleMgr::createparticle(const char* tex)
{
    int texid = FxGet().getTextureId(tex);

    if(texid == -1)
        return nullptr;

    Particle2* p = getparticle();

    if(!p)
        return nullptr;

    p->_textureindex = texid;

    return p;
}

/*----------------------------------------------------------------------
Function: ParticleMgr::setupparticlebeam
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::setupparticlebeam(const ParticleEffect* fx,
                                    const Vec3f& normal,
                                    const Vec3d& pos1,
                                    const Vec3d& pos2,
                                    int starttime,
                                    float alphadegradation)
{
    if(!fx)
    {
        return;
    }

    Vec3f n = Vec3d(normal.x(), normal.y(), normal.z());

    float speed = fx->speed;

    if(fx->number.empty())
    {
        return;
    }

    unsigned int size = fx->number.size();

    int num;
    num = size == 1 ? fx->number[0] : GetRandom(fx->number[0], fx->number[1]);

    Vec3f velocity = n * speed;

    int delayoffset = 0;

    for(int i = 0; i < num; i++)
    {
        Particle2* p = getparticle();

        if(!p)
        {
            break;
        }

        p->_flags = fx->particleflags;
        p->_flags |= part_is_beam;

        if(fx->texture.size() > 1)
        {
            p->_textureindex = fx->texture[GetRandom(0, fx->texture.size() - 1)];
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

        p->_lifetime = fx->lifetime;

        if(p->hastimestages() && p->_lifetime != -1)
        {
            p->_time[0] = starttime + fx->time[0] + fx->delaystart + delayoffset;
            p->_time[1] = p->_time[0] + fx->time[1];
            p->_time[2] = p->_time[1] + fx->time[2];
            p->_time[3] = p->_time[2] + fx->time[3];
        }
        else if(p->_lifetime != -1)
        {
            p->_time[0] = starttime + fx->delaystart + delayoffset;
            p->_time[3] = p->_time[0] + fx->lifetime;
        }

        if(!fx->color.empty())
        {
            p->_color[0] = fx->color[0];
            p->_color[1] = fx->color[1];

            if(p->hascolorstages())
            {
                p->_color[2] = fx->color[2];
                p->_color[3] = fx->color[3];
            }
        }

        p->_dragcoeff = fx->dragcoef;
        p->_gravcoef = fx->gravitycoef;
        p->_stretchscale = fx->stretchscale;
        p->_glowtime = fx->glowtime;
        p->_stopstretch = fx->stopstretch;

        if(fx->rotate != 0)
        {
            p->_rotate = GetRandom(-fx->rotate, fx->rotate);
        }

        p->_currentsize = p->_size[0];
        p->_currentalpha = p->_alpha[0];

        p->_velocity = velocity;
        p->_position = pos2;
        p->_lastposition = pos1;
        p->_originalposition = pos1;

        if(fx->fxflags & fx_has_velvariance)
        {
            p->_velvariance.x() = GetRandom(fx->velvariance[0], fx->velvariance[3]);
            p->_velvariance.y() = GetRandom(fx->velvariance[1], fx->velvariance[4]);
            p->_velvariance.z() = GetRandom(fx->velvariance[2], fx->velvariance[5]);
        }

        if(fx->fxflags & fx_has_velreduction)
        {
            velocity *= fx->velreduction;
        }

        if(fx->fxflags & fx_has_posvariance)
        {
            p->_position.x() += GetRandom(fx->posvariance[0], fx->posvariance[3]);
            p->_position.y() += GetRandom(fx->posvariance[1], fx->posvariance[4]);
            p->_position.z() += GetRandom(fx->posvariance[2], fx->posvariance[5]);
        }

        if(fx->fxflags & fx_has_randomglow)
        {
            bool glow = GetRandom(0, 1);

            if(glow)
                p->_flags |= part_has_glow;
        }

        delayoffset += fx->delayeachparticle;
    }
}

/*----------------------------------------------------------------------
Function: init
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::init(bool global)
{
#if USE_STATESET
    extern Texture* obdrawGetSceneDepthTexture();
#endif

#if(0) // Let smartheap manage this
    chunk = new Particle2[MAX_PARTICLES];

    for(int i = 0; i < 40960; i++)
    {
        _freeparticle.push_back(&chunk[i]);
    }
#endif

    _raintexture = FxGet().getTextureId("rain");

    if(!ogl::UseDeprecatedPath())
    {
#if USE_STATESET
        material = new RenderStateSet;

        softparticlepgm = new Program("softparticle");
        particlepgm = new Program("particle");

        if(g_postRenderFilter)
            material->SetRenderState(softparticlepgm.get());
        else
            material->SetRenderState(particlepgm.get());

        material->SetRenderState(new TextureUnit(0, FxGet().getTexture()));
        material->SetRenderState(new TextureUnit(1, obdrawGetSceneDepthTexture()));
        material->SetRenderState(new DepthMask(false));
        material->SetRenderState(new BlendFunc);
        material->SetMode(GL_BLEND, 1);
        material->SetMode(GL_LIGHTING, 0);

        surfacematerial = new RenderStateSet;
        surfacematerial->SetRenderState(new TextureUnit(0, FxGet().getTexture()));
        surfacematerial->SetRenderState(new DepthMask(false));
        surfacematerial->SetRenderState(new BlendFunc);
        surfacematerial->SetMode(GL_BLEND, 1);
        surfacematerial->SetMode(GL_LIGHTING, 0);
#else
        _pgm = GLSL.getProgram("particle");

        material = new oglMaterial;
        material->SetShader("particle");
        material->AddTexture(0, FxGet().getTexture());
#endif
    }
    else
    {
#if USE_STATESET
        material = new RenderStateSet;

        softparticlepgm = new Program("softparticle");
        particlepgm = new Program("particle");

        if(g_postRenderFilter)
            material->SetRenderState(softparticlepgm.get());
        else
            material->SetRenderState(particlepgm.get());

        material->SetRenderState(new TextureUnit(0, FxGet().getTexture()));
        material->SetRenderState(new TextureUnit(1, obdrawGetSceneDepthTexture()));
        material->SetRenderState(new DepthMask(false));
        material->SetRenderState(new BlendFunc);
        material->SetRenderState(new Material);
        material->SetMode(GL_BLEND, 1);
        material->SetMode(GL_LIGHTING, 0);

        surfacematerial = new RenderStateSet;
        surfacematerial->SetRenderState(new TextureUnit(0, FxGet().getTexture()));
        surfacematerial->SetRenderState(new DepthMask(false));
        surfacematerial->SetRenderState(new BlendFunc);
        surfacematerial->SetRenderState(new Material);
        surfacematerial->SetMode(GL_BLEND, 1);
        surfacematerial->SetMode(GL_LIGHTING, 0);
#endif
    }

    _particlemgrInit = true;
}

/*----------------------------------------------------------------------
Function: getparticle
Description:
-----------------------------------------------------------------------*/
Particle2 *ParticleMgr::getparticle()
{
    Particle2 *p = nullptr;

    if(_freeparticle.empty())
    {
        p = new Particle2;
    }
    else
    {
        p = _freeparticle.front();
        _freeparticle.pop_front();
    }

    p->setid(_particleid++);
    p->reset();

    _particle.insert(p);

    return p;
}

/*----------------------------------------------------------------------
Function: kill
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::kill()
{
    if(_particlemgrInit)
    {
        std::set<Particle2*>::iterator itr = _particle.begin();

        for(; itr != _particle.end(); ++itr)
        {
            _freeparticle.push_back(*itr);
        }

        for(size_t i = 0; i < _freeparticle.size(); i++)
            delete _freeparticle[i];

        _particle.clear();
        _freeparticle.clear();
    }

    WriteLog(LOG_OLD, LOG_INFO, "[ParticleMgr::kill]");
}

/*----------------------------------------------------------------------
Function: GetLightColor
Description:
-----------------------------------------------------------------------*/
static void GetLightColor(Vec4f& c)
{
    static LIGHT *s = nullptr;
    static LIGHT *m = nullptr;

    if(!s)
    {
        s = obdliteFindDynaLight("sun");
    }

    if(!m)
    {
        m = obdliteFindDynaLight("moon");
    }

    if(!s || !m)
    {
        WriteLog(LOG_OLD, LOG_INFO, "ERROR: Cannot find sun/moon object");
        return;
    }

    static piCOLORVALUE color = {0, 0, 0, 1};

    piVECTOR dir;
    piLIGHT& light = s->flags & LITE_ON ? s->light : m->light;

    FLOAT32 daylight;

    dir.z = -1.0f;
    dir.x = 0.0f;
    dir.y = 0.0f;

    daylight = -math3dDotProduct(&dir, &light.dvDirection);

    if(daylight < 0.0f)
    {
        daylight = 0.0f;
    }

    color.r = light.dcvColor.r * daylight;
    color.r += light.dcvAmbient.r;

    color.g = light.dcvColor.g * daylight;
    color.g += light.dcvAmbient.g;

    color.b = light.dcvColor.b * daylight;
    color.b += light.dcvAmbient.b;

    // Clip for some visiblity.
    color.r = color.r > 1 ? 1 : color.r;
    color.g = color.g > 1 ? 1 : color.g;
    color.b = color.b > 1 ? 1 : color.b;

    color.r = color.r < 0 ? 0 : color.r;
    color.g = color.g < 0 ? 0 : color.g;
    color.b = color.b < 0 ? 0 : color.b;

    c.r() = color.r;
    c.g() = color.g;
    c.b() = color.b;
}

/*----------------------------------------------------------------------
Function: update
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::update()
{
    if(_particle.empty())
    {
        return;
    }

    static bool first = true;

    if(first)
    {
        first = false;
        _oldplayer.set(-playpos.x, -playpos.y, -playpos.z);
    }

    if(!_pgm)
    {
        GetLightColor(_color);
    }

    piVECTOR delta, player;
    player.set(-playpos.x, -playpos.y, -playpos.z);

    mathSubVectors(&delta, &player, &_oldplayer);
    _oldplayer.set(-playpos.x, -playpos.y, -playpos.z);

    std::set<Particle2*>::iterator itr = _particle.begin();
    std::set<Particle2*>::iterator free;

    while(itr != _particle.end())
    {
        Particle2* p = *itr;

        // Some particles have a delayed start
        if(frametime < p->_time[0])
        {
            p->_lastupdate = frametime;
            ++itr;
            continue;
        }

        if(p->_dead)
        {
            if(p->_killcallback)
            {
                p->_killcallback(p);
            }

            free = itr;
            ++itr;

            _freeparticle.push_back(p);
            _particle.erase(free);

            continue;
        }

        if(p->expired() && p->_ready)
        {
            // Delete next update
            p->_dead = true;
        }

        // Don't add to draw list or kill unless particle has first been updated (necessary for 'run for one frame'
        // particles)
        p->_ready = true;
        p->update(delta);
        p->_lastupdate = frametime;

        ++itr;
    }
}

static int _backtofront(Particle2 *l, Particle2 *r)
{
	return l->_distance2 > r->_distance2;
}

/*----------------------------------------------------------------------
Function: ParticleMgr::cull
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::cull()
{
    if(_particle.empty())
    {
        return;
    }

    _draw.clear();

    std::set<Particle2*>::iterator itr = _particle.begin();

    Vec3f eye(-playpos.x, -playpos.y, -playpos.z);
    Vec3f dir(playdir);

    Vec3f relpos;
    piVECTOR pos;

    for(; itr != _particle.end(); ++itr)
    {
        Particle2* p = *itr;

        if(p->_dead || !p->_ready || !p->_enabled)
        {
            continue;
        }

         // Particle2 has delayed start time
        if(frametime < p->_time[0])
        {
            continue;
        }

        static float radius = p->_currentsize * 2.0f;

        p->getrelativeposition(relpos);
        relpos.get(pos);

        if(rnvisVectorFOVCheck(&pos, radius))
        {
            if(p->setupshape())
            {
                if(p->iswatersurfaceeffect())
                {
                    _drawwatersurface.push_back(p);
                }
                else
                {
                    _draw.push_back(p);
                }
            }
        }
    }

    if(!_draw.empty())
    {
#if USE_STATESET
        if(g_postRenderFilter)
        {
            if(!UsingSoftParticles)
            {
                material->SetRenderState(softparticlepgm.get());
                UsingSoftParticles = true;
            }

            // Set shader options
            Vec4f cam;

            cam.x() = FrontClip;
            cam.y() = oglGetFieldOfViewZMax();
            cam.z() = cam.y() * cam.x();
            cam.w() = cam.y() - cam.x();

            Program* pgm = dynamic_cast<Program*>(material->GetRenderState(RS_GLSLPROGRAM));

            if(pgm)
            {
                glslProgram* program = pgm->GetProgram();

                if(program)
                {
                    glslUniform* u = program->GetUniform("camera");

                    if(u)
                    {
                        u->Set(cam);
                    }
                }
            }
        }
        else
        {
            if(UsingSoftParticles)
            {
                material->SetRenderState(particlepgm.get());
                UsingSoftParticles = false;
            }
        }
#endif

        sort(_draw.begin(), _draw.end(), _backtofront);
    }
}

#if USE_STATESET
/*----------------------------------------------------------------------
Function: ParticleMgr::drawsurfacedeprecated
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::drawsurfacedeprecated()
{
    if(_drawwatersurface.empty())
    {
        return;
    }

    logTimerBeginMacro();

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(_drawwatersurface.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;
    int material = -1;

    oglSetWorldMatrix(&IdentityMatrix);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    Material* mat = dynamic_cast<Material*>(surfacematerial->GetRenderState(RS_MATERIAL));
    BlendFunc* blend = dynamic_cast<BlendFunc*>(surfacematerial->GetRenderState(RS_BLENDFUNC));

    // Bucket particles
    for(unsigned int i = 0; i < _drawwatersurface.size(); i++)
    {
        Particle2* p = _drawwatersurface[i];

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            surfacematerial->Apply();
        }

        glow = p->hasglow();

        if(oldmat == -1)
        {
            oldmat = p->_material;
            mat->SetMaterialID(p->_material);
            mat->apply();
        }

        if(oldmat != p->_material)
        {
            mat->SetMaterialID(oldmat);
            mat->apply();
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
            ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            indice = 0;
            oldmat = -1;

            blend->SetDstAlpha(DestBlend);
            blend->SetSrcAlpha(SrcBlend);
            blend->apply();

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            blend->SetDstAlpha(GL_ONE);
            blend->SetSrcAlpha(GL_SRC_ALPHA);
            blend->apply();
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        if(!_pgm && !glow)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
        ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

        mat->SetMaterialID(oldmat);
        mat->apply();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    logTimerEndMacro();
}
#else
/*----------------------------------------------------------------------
Function: ParticleMgr::drawsurfacedeprecated
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::drawsurfacedeprecated()
{
    if(_drawwatersurface.empty())
    {
        return;
    }

    logTimerBeginMacro();

    ProgramState::Bind(0);

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(_drawwatersurface.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    // Bucket particles
    for(unsigned int i = 0; i < _drawwatersurface.size(); i++)
    {
        Particle2* p = _drawwatersurface[i];

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;

            oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
            oglSetActiveTexture(0, FxGet().getTexture());

            if(_pgm)
            {
                _pgm->Apply();
                _pgm->ApplyUniformMap();
            }
        }

        glow = p->hasglow();

        if(oldmat == -1)
        {
            oldmat = p->_material;
            oglSetActiveMaterial(p->_material);
        }

        if(oldmat != p->_material)
        {
            oglSetActiveMaterial(oldmat);
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
            ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            indice = 0;
            oldmat = -1;

            oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_SRC_ALPHA);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ONE);
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        if(!_pgm && !glow)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
        ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

        oglSetActiveMaterial(oldmat);

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    if(_pgm)
    {
        ProgramState::Bind(0);
    }

    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);

    logTimerEndMacro();
}
#endif

#if USE_STATESET
/*----------------------------------------------------------------------
Function: ParticleMgr::drawsurface
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::drawsurface()
{
    if(_drawwatersurface.empty())
    {
        return;
    }

    logTimerBeginMacro();

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(_drawwatersurface.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    oglSetWorldMatrix(&IdentityMatrix);
    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    Material* mat = dynamic_cast<Material*>(surfacematerial->GetRenderState(RS_MATERIAL));
    BlendFunc* blend = dynamic_cast<BlendFunc*>(surfacematerial->GetRenderState(RS_BLENDFUNC));

    // Bucket particles
    for(unsigned int i = 0; i < _drawwatersurface.size(); i++)
    {
        Particle2* p = _drawwatersurface[i];

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            material->Apply();
        }

        glow = p->hasglow();

        if(oldmat == -1)
        {
            mat->SetMaterialID(p->_material);
            mat->apply();

            oldmat = p->_material;
        }

        if(oldmat != p->_material)
        {
            mat->SetMaterialID(oldmat);
            mat->apply();
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
            ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            indice = 0;
            oldmat = -1;

            blend->SetDstAlpha(DestBlend);
            blend->SetSrcAlpha(SrcBlend);
            blend->apply();

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            blend->SetDstAlpha(GL_ONE);
            blend->SetSrcAlpha(GL_SRC_ALPHA);
            blend->apply();
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        if(!_pgm && !glow)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
        ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

        mat->SetMaterialID(oldmat);
        mat->apply();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    logTimerEndMacro();
}
#else
/*----------------------------------------------------------------------
Function: ParticleMgr::drawsurface
Description:
-----------------------------------------------------------------------*/
void ParticleMgr::drawsurface()
{
    if(_drawwatersurface.empty())
    {
        return;
    }

    logTimerBeginMacro();

    ProgramState::Bind(0);

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(_drawwatersurface.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    // Bucket particles
    for(unsigned int i = 0; i < _drawwatersurface.size(); i++)
    {
        Particle2* p = _drawwatersurface[i];

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            material->Apply();
        }

        glow = p->hasglow();

        if(oldmat == -1)
        {
            material->SetMaterial(p->_material);
            material->ApplyMaterial();

            oldmat = p->_material;
        }

        if(oldmat != p->_material)
        {
            material->SetMaterial(oldmat);
            material->ApplyMaterial();
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
            ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            indice = 0;
            oldmat = -1;

            oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_SRC_ALPHA);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ONE);
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        if(!_pgm && !glow)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
        ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

        material->SetMaterial(oldmat);
        material->ApplyMaterial();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    if(_pgm)
    {
        ProgramState::Bind(0);
    }

    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);

    logTimerEndMacro();
}
#endif

#if USE_STATESET
/*----------------------------------------------------------------------
Function: ParticleMgr::drawdeprecated
Description:
-----------------------------------------------------------------------*/
bool ParticleMgr::drawdeprecated(double dist2)
{
    if(_draw.empty())
    {
        return false;
    }

    bool drawn = false;

    logTimerBeginMacro();

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(data.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    unsigned int i = 0;

    oglSetWorldMatrix(&IdentityMatrix);

    Vec3f corners[4];

#define IND_PER_QUAD    6
#define STRIDE          40

    int indindex = 0;
    int datindex = 0;

    Material* mat = dynamic_cast<Material*>(material->GetRenderState(RS_MATERIAL));
    BlendFunc* blend = dynamic_cast<BlendFunc*>(material->GetRenderState(RS_BLENDFUNC));

    blend->SetDstRGB(DestBlend);
    blend->SetSrcRGB(SrcBlend);
    blend->SetDstAlpha(DestBlend);
    blend->SetSrcAlpha(SrcBlend);

    // Bucket particles
    while(_particleindex < _draw.size())
    {
        i = _particleindex;

        Particle2* p = _draw[i];

        // Draw all particles at greater distance than the current transparent drawable
        if(p->_distance2 < dist2)
        {
            // Break if we have reached the current drawable distance
            break;
        }

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            material->Apply();
        }

        glow = p->hasglow();

        _particleindex++;

        if(oldmat == -1)
        {
            oldmat = p->_material;
            mat->SetMaterialID(p->_material);
            drawn = true;
        }

        if(oldmat != p->_material)
        {
            mat->SetMaterialID(oldmat);
            drawn = true;
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, STRIDE, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, STRIDE, (void*)&data[4]);
            ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, (void*)&data[6]);

            mat->apply();
            blend->apply();

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            blend->SetDstRGB(DestBlend);
            blend->SetDstAlpha(DestBlend);

            indice = 0;
            oldmat = -1;

            datindex = 0;
            indindex = 0;
        }

        if(glow)
        {
            blend->SetDstRGB(GL_ONE);
            blend->SetDstAlpha(GL_ONE);
        }

        oldmat = p->_material;

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + STRIDE > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        // Don't light rain so we can see it at night.
        if(!_pgm && !p->hasglow() && p->_textureindex != _raintexture)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();
        data[datindex++] = 1.0f;

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();
        data[datindex++] = 1.0f;

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();
        data[datindex++] = 1.0f;

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();
        data[datindex++] = 1.0f;

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, STRIDE, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, STRIDE, (void*)&data[4]);
        ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, (void*)&data[6]);

        mat->SetMaterialID(oldmat);

        mat->apply();
        blend->apply();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

        drawn = true;
    }

    logTimerEndMacro();

    return drawn;
}
#else
/*----------------------------------------------------------------------
Function: ParticleMgr::drawdeprecated
Description:
-----------------------------------------------------------------------*/
bool ParticleMgr::drawdeprecated(double dist2)
{
    if(_draw.empty())
    {
        return false;
    }

    logTimerBeginMacro();
    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_FOGENABLE, FALSE);

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(data.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    bool statesSet = false;

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    unsigned int i = 0;

    oglSetWorldMatrix(&IdentityMatrix);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);

    // Bucket particles
    while(_particleindex < _draw.size())
    {
        i = _particleindex;

        Particle2* p = _draw[i];

        // Draw all particles at greater distance than the current transparent drawable
        if(p->_distance2 < dist2)
        {
            // Break if we have reached the current drawable distance
            break;
        }

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;

            oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
            oglSetActiveTexture(0, FxGet().getTexture());

            if(_pgm)
            {
                _pgm->Apply();
                _pgm->ApplyUniformMap();
            }
        }

        _particleindex++;

        glow = p->hasglow();

        if(oldmat == -1)
        {
            oldmat = p->_material;
            oglSetActiveMaterial(p->_material);
        }

        if(oldmat != p->_material)
        {
            oglSetActiveMaterial(oldmat);
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            // Have to send
            ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
            ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            indice = 0;
            oldmat = -1;

            oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_SRC_ALPHA);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ONE);
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        // Don't light rain so we can see it at night.
        if(!_pgm && !glow && p->_textureindex != _raintexture)
        {
            p->_currentcolor.r() *= _color.r();
            p->_currentcolor.g() *= _color.g();
            p->_currentcolor.b() *= _color.b();
        }

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();

        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetColorPointer(4, GL_FLOAT, 36, (void*)&data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 36, (void*)&data[4]);
        ogl::SetVertexPointer(3, GL_FLOAT, 36, (void*)&data[6]);

        oglSetActiveMaterial(oldmat);

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);

    if(_pgm)
    {
        ProgramState::Bind(0);
    }

    oglSetRenderState(oglRENDERSTATE_FOGENABLE, TRUE);

    logTimerEndMacro();

    return true;
}
#endif

#if USE_STATESET
/*----------------------------------------------------------------------
Function: ParticleMgr::draw
Description:
-----------------------------------------------------------------------*/
bool ParticleMgr::draw(double dist2)
{
    if(_draw.empty())
    {
        return false;
    }

    bool statesSet = false;

    logTimerBeginMacro();

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(data.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    unsigned int i = 0;
    const int stride = 36;

    oglSetWorldMatrix(&IdentityMatrix);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    Material* mat = dynamic_cast<Material*>(material->GetRenderState(RS_MATERIAL));
    BlendFunc* blend = dynamic_cast<BlendFunc*>(material->GetRenderState(RS_BLENDFUNC));

    // Bucket particles
    while(_particleindex < _draw.size())
    {
        i = _particleindex;

        Particle2* p = _draw[i];

        // Draw all particles at greater distance than the current transparent drawable
        if(p->_distance2 < dist2)
        {
            // Break if we have reached the current drawable distance
            break;
        }

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            material->Apply();
        }

        _particleindex++;

        glow = p->hasglow();

        // Initialize
        if(oldmat == -1)
        {
            mat->SetMaterialID(p->_material);
            mat->apply();

            oldmat = p->_material;
        }

        if(oldmat != p->_material)
        {
            mat->SetMaterialID(oldmat);
            mat->apply();
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, stride, &data[0]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &data[3]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_FLOAT, 0, stride, &data[5]);

            mat->SetMaterialID(oldmat);
            mat->apply();

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            blend->SetDstAlpha(DestBlend);
            blend->SetSrcAlpha(SrcBlend);
            blend->apply();

            indice = 0;
            oldmat = -1;

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            blend->SetDstAlpha(GL_ONE);
            blend->SetSrcAlpha(GL_SRC_ALPHA);
            blend->apply();
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, stride, &data[0]);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &data[3]);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_FLOAT, 0, stride, &data[5]);

        mat->SetMaterialID(oldmat);
        mat->apply();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    logTimerEndMacro();

    return statesSet;
}
#else
/*----------------------------------------------------------------------
Function: ParticleMgr::draw
Description:
-----------------------------------------------------------------------*/
bool ParticleMgr::draw(double dist2)
{
    if(_draw.empty())
    {
        return false;
    }

    bool statesSet = false;

    logTimerBeginMacro();

    static std::vector<float> data;
    static std::vector<short> _indices;

    if(data.empty())
        data.resize(4096 / sizeof(float));

    if(_indices.empty())
        _indices.resize(4096 / sizeof(short));

    short indice = 0;

    bool glow = false;
    bool oldglow = false;

    int oldmat = -1;

    unsigned int i = 0;
    const int stride = 36;

    oglSetWorldMatrix(&IdentityMatrix);

    Vec3f corners[4];

#define DATA_PER_QUAD   36
#define IND_PER_QUAD    6

    int indindex = 0;
    int datindex = 0;

    // Bucket particles
    while(_particleindex < _draw.size())
    {
        i = _particleindex;

        Particle2* p = _draw[i];

        // Draw all particles at greater distance than the current transparent drawable
        if(p->_distance2 < dist2)
        {
            // Break if we have reached the current drawable distance
            break;
        }

        // Set only if necessary
        if(!statesSet)
        {
            statesSet = true;
            material->Apply();
       }

        _particleindex++;

        glow = p->hasglow();

        // Initialize
        if(oldmat == -1)
        {
            material->SetMaterial(p->_material);
            material->ApplyMaterial();

            oldmat = p->_material;
        }

        if(oldmat != p->_material)
        {
            material->SetMaterial(oldmat);
            material->ApplyMaterial();
        }

        if((oldglow != glow || oldmat != p->_material) && indindex)
        {
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, stride, &data[0]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &data[3]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_FLOAT, 0, stride, &data[5]);

            material->SetMaterial(oldmat);
            material->ApplyMaterial();

            ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);

            oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);

            indice = 0;
            oldmat = -1;

            datindex = 0;
            indindex = 0;
        }

        oldmat = p->_material;

        if(glow)
        {
            oglSetRenderState(oglRENDERSTATE_SRCBLEND, GL_SRC_ALPHA);
            oglSetRenderState(oglRENDERSTATE_DESTBLEND, GL_ONE);
        }

        int texindex = p->gettextureindex();
        Vec4f uv;

        FxGet().getuv(uv, texindex);

        float u1 = uv.x();
        float v1 = uv.y();
        float u2 = uv.z();
        float v2 = uv.w();

        corners[0] = p->_corners[0];
        corners[1] = p->_corners[1];
        corners[2] = p->_corners[2];
        corners[3] = p->_corners[3];

        if(datindex + DATA_PER_QUAD > (int)data.size())
            data.resize(data.size() + (4096 / sizeof(float)));

        data[datindex++] = corners[0].x();
        data[datindex++] = corners[0].y();
        data[datindex++] = corners[0].z();
        data[datindex++] = u2;
        data[datindex++] = v2;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[1].x();
        data[datindex++] = corners[1].y();
        data[datindex++] = corners[1].z();
        data[datindex++] = u1;
        data[datindex++] = v2;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[2].x();
        data[datindex++] = corners[2].y();
        data[datindex++] = corners[2].z();
        data[datindex++] = u1;
        data[datindex++] = v1;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        data[datindex++] = corners[3].x();
        data[datindex++] = corners[3].y();
        data[datindex++] = corners[3].z();
        data[datindex++] = u2;
        data[datindex++] = v1;
        data[datindex++] = p->_currentcolor.r();
        data[datindex++] = p->_currentcolor.g();
        data[datindex++] = p->_currentcolor.b();
        data[datindex++] = p->_currentcolor.a();

        if(indindex + IND_PER_QUAD > (int)_indices.size())
            _indices.resize(_indices.size() + (4096 / sizeof(short)));

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 1;
        _indices[indindex++] = indice + 2;

        _indices[indindex++] = indice + 0;
        _indices[indindex++] = indice + 2;
        _indices[indindex++] = indice + 3;

        indice += 4;

        oldglow = glow;

        p->_lastdrawtime = frametime;
    }

    if(indindex)
    {
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, stride, &data[0]);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &data[3]);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_FLOAT, 0, stride, &data[5]);

        material->SetMaterial(oldmat);
        material->ApplyMaterial();

        ogl::DrawElements(GL_TRIANGLES, indindex, GL_UNSIGNED_SHORT, (void*)&_indices[0]);
    }

    oglSetRenderState(oglRENDERSTATE_DESTBLEND, DestBlend);
    oglSetRenderState(oglRENDERSTATE_SRCBLEND, SrcBlend);

    logTimerEndMacro();

    return statesSet;
}
#endif
