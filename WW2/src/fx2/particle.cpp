#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: particle.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"
#include "ld/ldpix.h"
#include "include/vars.h"
#include "random.h"
#include "trn/trnmain.h"
#include "rn/rnvis.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "fx/fxMgr.h"
#include "ob/obdlite.h"
#include "math/mathUnitConversion.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "ld/nv_dds.h"
#include "weap/weapammo.h"
#include "ogl/oglGlobalStates.h"


#include <string>
#include <algorithm>

#include "fx2/fxdata.h"
#include "fx2/effect.h"
#include "fx2/particle.h"

#define MAX_SMOKE_TEXTURES  4
#define MAX_PARTICLES       4096
#define MAX_FIRES           256

static int _particleid = 1;

//////////////////////////////////////////////////////////////////////
// Function: Particle
// Description:
//////////////////////////////////////////////////////////////////////
Particle::Particle()
{
    _flags = 0;

    _lastupdate = 0;
    _id = 0;
    _textureindex = 0;
    _material = -1;
    _gravcoef = -9.8f;

    memset(&_position, 0, sizeof(FLOAT64VECTOR));

    memset(&_velocity, 0, sizeof(piVECTOR));
    memset(&_windvelocity, 0, sizeof(piVECTOR));

    _distance2 = 0.0f;
    _dragcoeff = 0.0f;
    _rotate = 0.0f;
}

//////////////////////////////////////////////////////////////////////
// Function: _rotateZ
// Description:
//////////////////////////////////////////////////////////////////////
inline void _rotateZ(piMATRIX& m, piVECTOR& v)
{
    piVECTOR retvector;

    retvector.x = v.x * m._11 + v.y * m._12;
    retvector.y = v.x * m._21 + v.y * m._22;
    retvector.z = 0.0f;

    v = retvector;
}

//////////////////////////////////////////////////////////////////////
// Function: rotateextents
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::rotateextents(FLOAT32 time)
{
    FLOAT32 radians = DEG_TO_RAD(_rotate) * time;

    piMATRIX rot = identity;

    rot._11 = cosf(radians);
    rot._12 = sinf(radians);
    rot._21 = -rot._12;
    rot._22 =  rot._11;

    _rotateZ(rot, _extents[0]);
    _rotateZ(rot, _extents[1]);
    _rotateZ(rot, _extents[2]);
    _rotateZ(rot, _extents[3]);
}

//////////////////////////////////////////////////////////////////////
// Function: setextents
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::setextents(SINT32 i, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    _extents[i].x = x;
    _extents[i].y = y;
    _extents[i].z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: applydrag
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::applydrag(FLOAT32 time)
{
#if(0)
    FLOAT32 min2 = _minspeed * _minspeed;
    FLOAT32 max2 = _maxspeed * _maxspeed;

    FLOAT32 speed2 = math3dDotProduct(&_velocity, &_velocity);

    if(speed2 < min2 && speed2 > 0.00001f)
    {
        FLOAT32 s = math3dFastSqrt(speed2);

        _velocity.x *= _minspeed / s;
        _velocity.y *= _minspeed / s;
        _velocity.z *= _minspeed / s;
    }
    else if(speed2 > _maxspeed)
    {
        FLOAT32 s = math3dFastSqrt(speed2);

        _velocity.x *= _maxspeed / s;
        _velocity.y *= _maxspeed / s;
        _velocity.z *= _maxspeed / s;
    }
#else
    // Apply drag
    FLOAT32 magnitude;
    FLOAT32 speed;

    magnitude = (FLOAT32)math3dFastSqrt(math3dDotProduct(&_velocity, &_velocity));

    // Fast exponent
    speed = magnitude * (FLOAT32)fexp(-_dragcoeff * time);

    if(magnitude > 0.00001f || magnitude < -0.00001f)
    {
        _velocity.x = speed * _velocity.x / magnitude;
        _velocity.y = speed * _velocity.y / magnitude;
        _velocity.z = speed * _velocity.z / magnitude;

        if(hasvelvariance())
        {
            _velvariance.x = speed * _velvariance.x / magnitude;
            _velvariance.y = speed * _velvariance.y / magnitude;
            _velvariance.z = speed * _velvariance.z / magnitude;
        }
    }
    else
    {
        _velocity.x = 0.0f;
        _velocity.y = 0.0f;
        _velocity.z = 0.0f;

        if(hasstretch())
        {
            popflag(part_has_stretch | part_has_fixstretch);
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: applywind
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::applywind(FLOAT32 time)
{
    if(!hasstretch())
    {
        _velocity.x += _windvelocity.x * time;
        _velocity.y += _windvelocity.y * time;
        _velocity.z += _windvelocity.z * time;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: updateposition
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::updateposition(FLOAT32 time)
{
    _position.x += _velocity.x * time;
    _position.y += _velocity.y * time;
    _position.z -= _velocity.z * time;
}

//////////////////////////////////////////////////////////////////////
// Function: getrelativeposition
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::getrelativeposition(piVECTOR& pos)
{
    pos.x = (FLOAT32)(_position.x - trnGetWorldOriginOffsetPtr()->x);
    pos.y = (FLOAT32)(_position.y - trnGetWorldOriginOffsetPtr()->y);
    pos.z = (FLOAT32)(_position.z - trnGetWorldOriginOffsetPtr()->z);

    pos.z = -pos.z;
}

//////////////////////////////////////////////////////////////////////
// Function: stretchParticle
// Description:
//////////////////////////////////////////////////////////////////////
bool Particle::stretchParticle(piVECTOR corner[4], FLOAT32 time, FLOAT32 len)
{
    logTimerBeginMacro();

    static piVECTOR forward;
    piVECTOR up;
    piVECTOR offset = {0, 0, 0};

    FLOAT32 vel2 = math3dDotProduct(&_velocity, &_velocity);
//	FLOAT32 length = math3dFastSqrt(vel2);

    piVECTOR pos;
    getrelativeposition(pos);

    if(!hasfixedstretch() && vel2 != 0.0f)
    {
        forward.x = _velocity.x * _stretchscale;
        forward.y = _velocity.y * _stretchscale;
        forward.z = _velocity.z * _stretchscale;
    }
    else
    {
        static FLOAT32 invvel2 = 0;

        if(vel2 > 0.0001f)
        {
            invvel2 = mathFastInvSqrt(vel2);

            forward.x = _velocity.x * invvel2 * _stretchscale * _currentsize;
            forward.y = _velocity.y * invvel2 * _stretchscale * _currentsize;
            forward.z = _velocity.z * invvel2 * _stretchscale * _currentsize;
        }
    }

    piVECTOR right;

    right.x = -playpos.x - pos.x;
    right.y = -playpos.y - pos.y;
    right.z = -playpos.z - pos.z;

    // Prevent texture from 'flipping' in mid-air
    if(hasnoflip())
    {
        if(forward.z > 0)
        {
            forward.z = -forward.z;
        }
    }

    math3dCrossProduct(&up, &forward, &right);

    FLOAT32 up2 = math3dDotProduct(&up, &up);

    up2 = mathFastInvSqrt(up2) * _currentsize;
    math3dScaleVector(&up, &up, up2);

    FLOAT32 fw2 = math3dDotProduct(&forward, &forward);

    fw2 = mathFastInvSqrt(fw2) * _currentsize;

    math3dScaleVector(&offset, &forward, fw2);

    piVECTOR x, y;

    x.x = forward.x + offset.x;
    x.y = forward.y + offset.y;
    x.z = forward.z + offset.z;

    y = up;

    corner[0].x = -x.x - y.x;
    corner[0].y = -x.y - y.y;
    corner[0].z = -x.z - y.z;

    corner[1].x = -x.x + y.x;
    corner[1].y = -x.y + y.y;
    corner[1].z = -x.z + y.z;

    corner[2].x = x.x + y.x;
    corner[2].y = x.y + y.y;
    corner[2].z = x.z + y.z;

    corner[3].x = x.x - y.x;
    corner[3].y = x.y - y.y;
    corner[3].z = x.z - y.z;

    logTimerEndMacro();

    return true;
}

//////////////////////////////////////////////////////////////////////
// Function: setupshape
// Description:
//////////////////////////////////////////////////////////////////////
bool Particle::setupshape(piVECTOR corner[4], FLOAT32 time)
{
    piVECTOR x, y;

    x.x = _currentsize * playright.x;
    x.y = _currentsize * playright.y;
    x.z = _currentsize * playright.z;

    y.x = _currentsize * playup.x;
    y.y = _currentsize * playup.y;
    y.z = _currentsize * playup.z;

    corner[0].x = _extents[0].x * x.x + _extents[0].y * y.x;
    corner[0].y = _extents[0].x * x.y + _extents[0].y * y.y;
    corner[0].z = _extents[0].x * x.z + _extents[0].y * y.z;

    corner[1].x = _extents[1].x * x.x + _extents[1].y * y.x;
    corner[1].y = _extents[1].x * x.y + _extents[1].y * y.y;
    corner[1].z = _extents[1].x * x.z + _extents[1].y * y.z;

    corner[2].x = _extents[2].x * x.x + _extents[2].y * y.x;
    corner[2].y = _extents[2].x * x.y + _extents[2].y * y.y;
    corner[2].z = _extents[2].x * x.z + _extents[2].y * y.z;

    corner[3].x = _extents[3].x * x.x + _extents[3].y * y.x;
    corner[3].y = _extents[3].x * x.y + _extents[3].y * y.y;
    corner[3].z = _extents[3].x * x.z + _extents[3].y * y.z;

    FLOAT32 vel2 = math3dDotProduct(&_velocity, &_velocity);
    FLOAT32 length = mathExactVectorLength(&_velocity);

    if(hasstretch() && vel2 > 0.0001f && _currentsize > 0.0001f)
    {
        piVECTOR pos;
        getrelativeposition(pos);

		float Ratio = length / _currentsize;

        /*--- BEGIN FROM LEGACY STRETCH CODE --*/
        piVECTOR TempVec;

        piVECTOR normvel = _velocity;
        mathReallyFastNormalize(normvel);

		TempVec.x = normvel.x * Ratio;
		TempVec.y = normvel.y * Ratio;
		TempVec.z = normvel.z * Ratio;

        piVECTOR v;
        mathAddVectors(&v, &pos, &_velocity);
        mathAddVectors(&v, &v, &playpos);

		float Len = mathVectDotProduct(&playdir, &v);

		if(Len <= 0.0)
		{
			return true;
		}

        mathSubVectors(&v, &pos, &_velocity);
        mathAddVectors(&v, &v, &playpos);

        Len = mathVectDotProduct(&playdir, &v);

		if(Len <= 0.0f)
		{
			return true;
		}

	    // we must project this point onto the view plane
	    // and then compensate for perspective
        piVECTOR V1, V2;

	    // determine direction to point
	    mathAddVectors(&V1,&pos,&playpos);
	    Len = mathVectDotProduct(&playdir,&TempVec);

		if (Len < 0.0f)
		{
			mathSubVectors(&V2,&V1,&TempVec);
		}
		else
		{
			mathAddVectors(&V2,&V1,&TempVec);
		}

		float Len1 = (FLOAT32)mathExactVectorLength(&V1);
		float Len2 = (FLOAT32)mathExactVectorLength(&V2);

		if (Len2 < Len1)
		{
			Len1 = Len2;
			V1 = V2;
		}

		float TempNum = 1.0f / Len1;
		V1.x *= TempNum;
		V1.y *= TempNum;
		V1.z *= TempNum;

		// now determine amount to move point
		float Cos = mathVectDotProduct(&playdir,&V1);

		if(Cos > 0.001f || Cos < -0.001f)
		{
			Len = Len / Cos;

			// move point
			TempVec.x -= V1.x * Len;
			TempVec.y -= V1.y * Len;
			TempVec.z -= V1.z * Len;
		}

        piMATRIX ViewAttMatrix = *piGetViewMatrix();
        ViewAttMatrix._41 = ViewAttMatrix._42 = ViewAttMatrix._43 = 0.0f;

		math3dVectorMultiply(&TempVec,&TempVec,&ViewAttMatrix);

        float StretchLength;

        Len = StretchLength = (FLOAT32)sqrt(TempVec.x * TempVec.x + TempVec.y * TempVec.y);
        /*--- END FROM LEGACY STRETCH CODE --*/

		if(stretchalways() || StretchLength > 1.0)
		{
            // My stretch code
            stretchParticle(corner, time, Len);
		}
        else
        {
            popflag(part_has_stretch | part_has_fixstretch);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// Function: updatevelocity
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::updatevelocity(FLOAT32 time)
{
    _velocity.x += _velvariance.x * time;
    _velocity.y += _velvariance.y * time;
    _velocity.z += _velvariance.z * time;
}

//////////////////////////////////////////////////////////////////////
// Function: update
// Description:
//////////////////////////////////////////////////////////////////////
void Particle::update()
{
    logTimerBeginMacro();

    SINT32 timediff = updatetime();

    FLOAT32 time = 0.001f * (FLOAT32)timediff;

    if(hasgravity())
    {
        applygravity(time);
    }

    if(hasdrag())
    {
        applydrag(time);
    }

    if(hasstretch())
    {
        if(_stopstretch)
        {
            if(age() > _stopstretch)
            {
                popflag(part_has_stretch | part_has_fixstretch);
            }
        }
    }
    else
    {
        if(hasrotate())
        {
            rotateextents(time);
        }

        if(hasvelvariance())
        {
            updatevelocity(time);
        }
    }

    applywind(time);

    updateposition(time);

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: ParticleManager
// Description:
//////////////////////////////////////////////////////////////////////
ParticleManager::ParticleManager()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~ParticleManager
// Description:
//////////////////////////////////////////////////////////////////////
ParticleManager::~ParticleManager()
{
}

//////////////////////////////////////////////////////////////////////
// Function: init
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::init()
{
    for(SINT32 i = 0; i < MAX_PARTICLES; i++)
    {
        Particle* p = new Particle;
        _freeparticle.push_back(p);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: getparticle
// Description:
//////////////////////////////////////////////////////////////////////
Particle* ParticleManager::getparticle()
{
    Particle* p = NULL;

    if(_freeparticle.empty())
    {
        p = new Particle;
    }
    else
    {
        p = _freeparticle.front();
        _freeparticle.pop_front();
    }

    p->setextents(0,  1, -1, 0);
    p->setextents(1, -1, -1, 0);
    p->setextents(2, -1,  1, 0);
    p->setextents(3,  1,  1, 0);

    p->setid(_particleid++);

    p->_lastupdate = frametime;
    p->_flags = 0;
    p->_dead = 0;
    p->_velocity.x = 0.0f;
    p->_velocity.y = 0.0f;
    p->_velocity.z = 0.0f;
    p->_velvariance.x = 0.0f;
    p->_velvariance.y = 0.0f;
    p->_velvariance.z = 0.0f;

    _particle.insert(std::make_pair(p->getid(), p));

    return p;
}

//////////////////////////////////////////////////////////////////////
// Function: kill
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::kill()
{
    std::map<SINT32, Particle *>::iterator free;

    while(!_particle.empty())
    {
        free = _particle.begin();
        delete free->second;

        _particle.erase(free);
    };

    while(!_freeparticle.empty())
    {
        Particle* p = _freeparticle.front();
        _freeparticle.pop_front();

        delete p;
    }

    Write_Log(LOG_OLD, LOG_INFO, "[Particle::kill]");
}

//////////////////////////////////////////////////////////////////////
// Function: particleRemove
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::remove(SINT32 id)
{
    std::map<SINT32, Particle *>::iterator find;

    find = _particle.find(id);

    if(find == _particle.end())
    {
        return;
    }

    _freeparticle.push_back(find->second);

    _particle.erase(find);
}

//////////////////////////////////////////////////////////////////////
// Function: computebillboard
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::computebillboard()
{
    /*
#if(1)
    piMATRIX m = identity;

    piVECTOR c1, c2, c3, c4;

    piVECTOR X;
    piVECTOR Y;

    X.x = playright.x;
    X.y = playright.y;
    X.z = playright.z;

    Y.x = playup.x;
    Y.y = playup.y;
    Y.z = playup.z;

    c1.x = X.x + Y.x;
    c1.y = X.y + Y.y;
    c1.z = X.z + Y.z;

    c2.x = X.x - Y.x;
    c2.y = X.y - Y.y;
    c2.z = X.z - Y.z;

    c3.x = -X.x - Y.x;
    c3.y = -X.y - Y.y;
    c3.z = -X.z - Y.z;

    c4.x = -X.x + Y.x;
    c4.y = -X.y + Y.y;
    c4.z = -X.z + Y.z;

    m._11 = c1.x;
    m._12 = c1.y;
    m._13 = c1.z;
    m._14 = 0.0f;

    m._21 = c2.x;
    m._22 = c2.y;
    m._23 = c2.z;
    m._24 = 0.0f;

    m._31 = c3.x;
    m._32 = c3.y;
    m._33 = c3.z;
    m._34 = 0.0f;

    m._41 = c4.x;
    m._42 = c4.y;
    m._43 = c4.z;
    m._44 = 0.0f;

    _fxdata._particlev.setMatrix("billboard", m);
#else
    piMATRIX m = identity;

    piVECTOR c1, c2, c3, c4;

    piVECTOR X;

    FLOAT32 z = -2.0f;

    X.x = playright.x;
    X.y = playright.y;
    X.z = 0.0f;

    c1.x = X.x;
    c1.y = X.y;
    c1.z = z;

    c2.x = X.x;
    c2.y = X.y;
    c2.z = 0.0f;

    c3.x = -X.x;
    c3.y = -X.y;
    c3.z =  0.0f;

    c4.x = -X.x;
    c4.y = -X.y;
    c4.z = z;

    m._11 = c1.x;
    m._12 = c1.y;
    m._13 = c1.z;
    m._14 = 0.0f;

    m._21 = c2.x;
    m._22 = c2.y;
    m._23 = c2.z;
    m._24 = 0.0f;

    m._31 = c3.x;
    m._32 = c3.y;
    m._33 = c3.z;
    m._34 = 0.0f;

    m._41 = c4.x;
    m._42 = c4.y;
    m._43 = c4.z;
    m._44 = 0.0f;

    _fxdata._particlev.setMatrix("billboard", m);
#endif
    */
}

//////////////////////////////////////////////////////////////////////
// Function: _backtofront
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 _backtofront(Particle* l, Particle* r)
{
    if ( l == NULL )
    {
        return 0;
    }
    if ( r == NULL )
    {
        return 1;
    }

    return l->_distance2 > r->_distance2;
}

//////////////////////////////////////////////////////////////////////
// Function: update
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::update()
{
    if(_particle.empty())
    {
        return;
    }

    logTimerBeginMacro();

    std::map<SINT32, Particle *>::iterator begin = _particle.begin();
    std::map<SINT32, Particle *>::iterator free;

    piVECTOR pos;

    _draw.clear();

    while(begin != _particle.end())
    {
        Particle* p = begin->second;

        // Some particles have a delayed start
        if(frametime < p->_time[0])
        {
            p->_lastupdate = frametime;
            ++begin;
            continue;
        }

        if(p->_dead)
        {
            free = begin;
            ++begin;

            _freeparticle.push_back(p);
            _particle.erase(free);

            continue;
        }

        if(p->expired())
        {
            // Delete next update
            p->_dead = true;
        }

        p->update();
        p->_lastupdate = frametime;
        p->getrelativeposition(pos);

        if(rnvisVectorFOVCheck(&pos, p->_currentsize * 2.0f))
        {
            p->_distance2 = (math3dDotProduct(&pos, &playdir) + playtrans.z);

            _draw.push_back(p);
        }

        ++begin;
    }

    sort(_draw.begin(), _draw.end(), _backtofront);

    logTimerEndMacro();
}

//////////////////////////////////////////////////////////////////////
// Function: _timeindex
// Description:
//////////////////////////////////////////////////////////////////////
inline SINT32 _timeindex(Particle* p)
{
    if(!p->hastimestages())
    {
        return 0;
    }

    if(frametime >= p->_time[0] && frametime < p->_time[1])
    {
        return 1;
    }

    if(frametime >= p->_time[1] && frametime < p->_time[2])
    {
        return 2;
    }

    if(frametime >= p->_time[2] && frametime <= p->_time[3])
    {
        return 3;
    }

    return 3;
}

//////////////////////////////////////////////////////////////////////
// Function: _lerptime
// Description:
//////////////////////////////////////////////////////////////////////
inline FLOAT32 _time(Particle* p, SINT32 i)
{
    SINT32 t1, t2;

    if(p->hastimestages())
    {
        t1 = p->_time[i - 1];
        t2 = p->_time[i];
    }
    else
    {
        t1 = p->_time[0];
        t2 = p->_time[3];
    }

    FLOAT32 t = (FLOAT32)(frametime - t1) / (FLOAT32)(t2 - t1);

    return t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
}

//////////////////////////////////////////////////////////////////////
// Function: _lerpsize
// Description:
//////////////////////////////////////////////////////////////////////
inline FLOAT32 _lerpsize(Particle* p, FLOAT32 t, SINT32 i)
{
    FLOAT32 s1, s2;

    if(p->hassizestages())
    {
        s1 = p->_size[i - 1];
        s2 = p->_size[i];
    }
    else
    {
        s1 = p->_size[0];
        s2 = p->_size[1];
    }

    FLOAT32 size = s1 + t * (s2 - s1);
    return size < 0.0f ? 0.0f : size;
}

//////////////////////////////////////////////////////////////////////
// Function: _lerpalpha
// Description:
//////////////////////////////////////////////////////////////////////
inline FLOAT32 _lerpalpha(Particle* p, FLOAT32 t, SINT32 i)
{
    FLOAT32 a1, a2;

    if(p->hasalphastages())
    {
        a1 = p->_alpha[i - 1];
        a2 = p->_alpha[i];
    }
    else
    {
        a1 = p->_alpha[0];
        a2 = p->_alpha[1];
    }

    FLOAT32 alpha = a1 + t * (a2 - a1);
    return alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
}

//////////////////////////////////////////////////////////////////////
// Function: _sendtocard
// Description:
//////////////////////////////////////////////////////////////////////
inline void _sendtocard(std::vector<piLVERTEX>& data, std::vector<piINDICES>& indices)
{
    piRenderIndexScene(piPT_TRIANGLELIST,
                       piVT_LVERTEX,
                       &data[0],
                       data.size(),
                       (UINT16*)&indices[0],
                       indices.size() * 3);
/*
    SINT32 stride = 24;

    // Position x y z
    ogl::SetVertexPointer(3, GL_FLOAT, stride, (GLvoid*)&data[0]);

    // UV
    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, (GLvoid*)&data[3]);

    // Color
    ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, stride, (GLvoid*)&data[5]);

	ogl::DrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, &indices[0]);
    */
}

//////////////////////////////////////////////////////////////////////
// Function: _getColor
// Description:
//////////////////////////////////////////////////////////////////////
UINT32 ParticleManager::_getColor(piVECTOR& pos, Particle* p)
{
    DYNALITE* lites = NULL;

    int num = obdliteGetLights(&lites);

    piVECTOR diff;

    float r = 0, g = 0, b = 0;

    // Just for guns/bombs. Moon and sun already done in _color;
    for(int i = 2; i < num; i++)
    {
        if(lites[i].light.flags & LITE_ON)
        {
            piCOLORVALUE* c = &lites[i].light.light.dcvColor;

            mathSubVectors(&diff, &pos, &lites[i].light.light.dvPosition);

            float dist2 = mathVectDotProduct(&diff, &diff);
            float attn = dist2 > 1.0f ? 1.0f / dist2 : 1.0f;

            r += c->r * attn;
            g += c->g * attn;
            b += c->b * attn;
        }
    }

    r += _color.r;
    g += _color.g;
    b += _color.b;

    r = r > 1.0f ? 1.0f : r;
    g = g > 1.0f ? 1.0f : g;
    b = b > 1.0f ? 1.0f : b;

    return piRGBA(r, g, b, p->_currentalpha * 0.5f);
}

//////////////////////////////////////////////////////////////////////
// Function: particleDraw
// Description:
//////////////////////////////////////////////////////////////////////
void ParticleManager::draw(float distance2, int& fxindex)
{
    if(_draw.empty())
    {
        return;
    }

    logTimerBeginMacro();

    static std::vector<piLVERTEX> _data;
    static std::vector<piINDICES> _indices;

    piLVERTEX v;
    piINDICES indices;

    static LPMATERIAL mat = NULL;

    if(!mat)
    {
        piGetMaterials(&mat);
    }

    _fxdata.bind();

    UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
    UINT32 blendon = piGetEngineRenderState(piRENDERSTATE_ALPHABLENDENABLE);
    UINT32 dest = piGetEngineRenderState(piRENDERSTATE_DESTBLEND);
    UINT32 src = piGetEngineRenderState(piRENDERSTATE_SRCBLEND);
    UINT32 zon = piGetEngineRenderState(piRENDERSTATE_ZENABLE);
    UINT32 alpha = piGetEngineRenderState(piRENDERSTATE_ALPHATESTENABLE);
    UINT32 zwrite = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);

    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);
    piSetRenderState(piRENDERSTATE_ZENABLE, TRUE);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, FALSE);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

    FLOAT32 u1, u2, v1, v2;

    SINT16 indice = 0;

    SINT32 oldmatid = -1;
    SINT32 matid = -1;

    bool glow = false;
    bool oldglow = false;

    piPushMatrix(WORLD_MATRIX);
    piSetWorldMatrix(&identity);

//	unsigned int start = fxindex;
    Particle* p = NULL;

    // Bucket particles
    while(fxindex < (int)_draw.size())
    {
        p = _draw[fxindex];

        if ( p == NULL ) // hack for mac particle null ctd
        {
          fxindex++;
          continue;
        }

        // Only draw selected particles so it sorts correctly among transparent objects.
        // Particles sorted back to front
        if(p->_distance2 < distance2)
        {
            // We can break here
            break;
        }

        glow = p->hasglow();
        matid = p->_material;

        if(fxindex == 0)
        {
            piSetActiveMaterial(matid);
            oldmatid = matid;
            oldglow = glow;
        }

        fxindex++;

        if(!_data.empty() && (matid != oldmatid || oldglow != glow))
        {
            piSetActiveMaterial(matid);

            // Have to send
            _sendtocard(_data, _indices);

            _data.clear();
            _indices.clear();

            indice = 0;
            piSetRenderState(piRENDERSTATE_DESTBLEND, dest);
        }

        if(glow)
        {
            piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
        }

        piVECTOR pos = {0, 0, 0};

        p->getrelativeposition(pos);

        SINT32 index = _timeindex(p);

        FLOAT32 t = _time(p, index);

        p->_currentsize = _lerpsize(p, t, index);
        p->_currentalpha = _lerpalpha(p, t, index);

        piVECTOR corners[4];

        if(!p->setupshape(corners, t))
        {
            continue;
        }

        UINT32 color;

        if(glow)
        {
            color = piRGBA(1, 1, 1, p->_currentalpha);
        }
        else
        {
            color = _getColor(pos, p);
        }

        SINT32 texindex = p->gettextureindex();

        _fxdata.getuv(u1, v1, u2, v2, texindex);

        v.color = color;
        v.tu = u2;
        v.tv = v2;
        v.x = pos.x + corners[0].x;
        v.y = pos.y + corners[0].y;
        v.z = pos.z + corners[0].z;

        _data.push_back(v);

        v.color = color;
        v.tu = u1;
        v.tv = v2;
        v.x = pos.x + corners[1].x;
        v.y = pos.y + corners[1].y;
        v.z = pos.z + corners[1].z;

        _data.push_back(v);

        v.color = color;
        v.tu = u1;
        v.tv = v1;
        v.x = pos.x + corners[2].x;
        v.y = pos.y + corners[2].y;
        v.z = pos.z + corners[2].z;

        _data.push_back(v);

        v.color = color;
        v.tu = u2;
        v.tv = v1;
        v.x = pos.x + corners[3].x;
        v.y = pos.y + corners[3].y;
        v.z = pos.z + corners[3].z;

        _data.push_back(v);

        indices.v1 = indice + 0;
        indices.v2 = indice + 1;
        indices.v3 = indice + 2;

        _indices.push_back(indices);

        indices.v1 = indice + 0;
        indices.v2 = indice + 2;
        indices.v3 = indice + 3;

        _indices.push_back(indices);

        indice += 4;

        oldmatid = matid;
        oldglow = glow;
    }

    if(!_indices.empty())
    {
        _sendtocard(_data, _indices);
    }

    piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, blendon);
    piSetRenderState(piRENDERSTATE_ZENABLE, zon);
    piSetRenderState(piRENDERSTATE_SRCBLEND, src);
    piSetRenderState(piRENDERSTATE_DESTBLEND, dest);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, alpha);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);

    _data.clear();
    _indices.clear();

    piPopMatrix(WORLD_MATRIX);

    logTimerEndMacro();
}
