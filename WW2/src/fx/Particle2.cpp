#include <syscfg.h> // Path is project specific.
/*----------------------------------------------------------------------
Module: particle.cpp
Description:
----------------------------------------------------------------------*/
#include "spt/Random.h"
#include "trn/trnmain.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "math/mathUnitConversion.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "trn/trnvars.h"

#include <string>
#include <algorithm>

#include "fx/TransformEffect.h"
#include "fx/Particle2.h"

/*----------------------------------------------------------------------
Function: Particle2::Particle2
Description:
----------------------------------------------------------------------*/
Particle2::Particle2()
{
    _flags = 0;

    _lastupdate = 0;
    _id = 0;
    _textureindex = 0;
    _gravcoef = -9.8f;

    _distance2 = 0.0f;
    _dragcoeff = 0.0f;
    _rotate = 0.0f;
}

/*----------------------------------------------------------------------
Function: Particle2::rotateextents
Description:
----------------------------------------------------------------------*/
void Particle2::reset()
{
    _lastupdate = frametime;
    _lastdrawtime = frametime;
    _flags = 0;
    _dead = 0;
    _rotate = 0;
    _hasendpos = false;
    _velocity.set(0, 0, 0);
    _velvariance.set(0, 0, 0);
    _ready = false;
    _enabled = true;
    _killcallback = NULL;
    _updatecallback = NULL;
    _distance2 = 0.0f;
    _stretchscale = 0.0f;
    _currentalpha = 0.0f;
    _currentsize = 0.0f;
    _dragcoeff = 0.0f;
    _rotate = 0.0f;
    _gravcoef = 0.0f;
    _stretchlength = 0.0f;
    _name.clear();
    _lifetime = 0;
    _textureindex = 0;
    _glowtime = 0;
    _stopstretch = 0;
    _material = 0;

    memset(_alpha, 0, sizeof(float) * 4);
    memset(_size, 0, sizeof(float) * 4);
    memset(_time, 0, sizeof(int) * 4);

    _color[0] = Vec3f(1, 1, 1);
    _color[1] = Vec3f(1, 1, 1);
    _color[2] = Vec3f(1, 1, 1);
    _color[3] = Vec3f(1, 1, 1);

    setextents(0,  1, -1, 0);
    setextents(1, -1, -1, 0);
    setextents(2, -1,  1, 0);
    setextents(3,  1,  1, 0);
}

/*----------------------------------------------------------------------
Function: Particle2::rotateextents
Description:
----------------------------------------------------------------------*/
void Particle2::rotateextents(float time)
{
    float radians = DEG_TO_RAD(_rotate) * time;

    Vec2f rot;

    rot.x() = cosf(radians);
    rot.y() = sinf(radians);

    _extents[0] = ROTATE_VECTOR_Z(rot, _extents[0]);
    _extents[1] = ROTATE_VECTOR_Z(rot, _extents[1]);
    _extents[2] = ROTATE_VECTOR_Z(rot, _extents[2]);
    _extents[3] = ROTATE_VECTOR_Z(rot, _extents[3]);
}

/*----------------------------------------------------------------------
Function: Particle2::setextents
Description:
----------------------------------------------------------------------*/
void Particle2::setextents(int i, float x, float y, float z)
{
    _extents[i].x() = x;
    _extents[i].y() = y;
    _extents[i].z() = z;
}

/*----------------------------------------------------------------------
Function: Particle2::applygravity
Description:
----------------------------------------------------------------------*/
void Particle2::applygravity(float time)
{
    _velocity.z() += _gravcoef * time;
}

/*----------------------------------------------------------------------
Function: Particle2::applydrag
Description:
----------------------------------------------------------------------*/
void Particle2::applydrag(float time)
{
    float magnitude;
    float speed;

    magnitude = math3dFastSqrt(_velocity.length2());

    // Fast exponent
    speed = magnitude * (float)exp(-_dragcoeff * time);

    if(magnitude > 0.00001f || magnitude < -0.00001f)
    {
        _velocity = (_velocity / magnitude) * speed;

        if(hasvelvariance())
        {
            _velvariance = (_velvariance / magnitude) * speed;
        }

        if(hasfixedstretch())
        {
            _stretchlength = _velocity.length();
        }
    }
    else
    {
        _velocity.set(0, 0, 0);

        if(hasstretch())
        {
            popflag(part_has_stretch | part_has_fixstretch);
        }
    }
}

/*----------------------------------------------------------------------
Function: Particle2::applywind
Description:
----------------------------------------------------------------------*/
void Particle2::applywind(float time)
{
    if(!haslegacystretch() && !hasstretch() && !istype(part_is_beam))
    {
        _velocity += (FxGet()._wind * time);
    }
}

/*----------------------------------------------------------------------
Function: Particle2::updateposition
Description:
----------------------------------------------------------------------*/
void Particle2::updateposition(float time)
{
    if(dontupdateposition())
    {
        return;
    }

    _lastposition = _position;
     _position += (_velocity * time);

     Vec3f relpos;
     getrelativeposition(relpos);

     if(_hasendpos)
     {
        if(relpos.z() >= _endpos.z())
        {
            _dead = true;
        }
     }
}

/*----------------------------------------------------------------------
Function: Particle2::getrelativeposition
Description:
----------------------------------------------------------------------*/
void Particle2::getrelativeposition(Vec3f& pos)
{
    pos.x() = (float)(_position.x() - WorldOriginOffset.x);
    pos.y() = (float)(_position.y() - WorldOriginOffset.y);
    pos.z() = (float)(_position.z() - WorldOriginOffset.z);
}

/*----------------------------------------------------------------------
Function: Particle2::getrelativeposition
Description:
----------------------------------------------------------------------*/
void Particle2::getrelativeposition(Vec3f& outpos, const Vec3d& inpos)
{
    outpos.x() = (float)(inpos.x() - WorldOriginOffset.x);
    outpos.y() = (float)(inpos.y() - WorldOriginOffset.y);
    outpos.z() = (float)(inpos.z() - WorldOriginOffset.z);
}

/*----------------------------------------------------------------------
Function: Particle2::particleTracer
Description: Between two points
----------------------------------------------------------------------*/
bool Particle2::particleTracer(Vec3f corner[])
{
    Vec3f p1 = _lastposition;
    Vec3f p2 = _position;

    Vec3f forward = p1 - p2;
    Vec3f right = Vec3f(-playpos.x, -playpos.y, -playpos.z) - p1;

    Vec3f up = right ^ forward;
    up *= (_currentsize / math3dFastSqrt(up * up));

    corner[0] = p2 + up;
    corner[1] = p2 - up;
    corner[2] = p1 - up;
    corner[3] = p1 + up;

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::particleBeam
Description: Between two points
----------------------------------------------------------------------*/
bool Particle2::particleBeam(Vec3f corner[])
{
    Vec3f pos;
    getrelativeposition(pos);

    Vec3f orig;
    getrelativeposition(orig, _originalposition);

    Vec3f p1 = orig;
    Vec3f p2 = pos;

    Vec3f forward = p1 - p2;
    Vec3f right = Vec3f(-playpos.x, -playpos.y, -playpos.z) - p1;

    Vec3f up = right ^ forward;
    up *= (_currentsize / math3dFastSqrt(up * up));

    corner[0] = p2 + up;
    corner[1] = p2 - up;
    corner[2] = p1 - up;
    corner[3] = p1 + up;

#if(0)
    piVECTOR pa, pb;
    corner[0].get(pa);
    corner[1].get(pb);

    draw3dLine(&pa, &pb, colorBLUEALPHA);
    corner[1].get(pa);
    corner[2].get(pb);

    draw3dLine(&pa, &pb, colorYELLOWALPHA);
    corner[2].get(pa);
    corner[3].get(pb);

    draw3dLine(&pa, &pb, colorREDALPHA);
    corner[3].get(pa);
    corner[0].get(pb);

    draw3dLine(&pa, &pb, colorWHITEALPHA);
#endif

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::fixstretchParticle
Description:
----------------------------------------------------------------------*/
bool Particle2::fixstretchParticle(Vec3f corner[4])
{
    static Vec3f forward;
    Vec3f up;
    Vec3f offset;

    Vec3f pos;
    getrelativeposition(pos);

    Vec3f dir = _velocity;
    dir.fastnormalize();

    Vec3f p2 = dir * _stretchlength;

    forward = p2 * _currentsize;

    Vec3f right;

    right = Vec3f(-playpos.x, -playpos.y, -playpos.z) - pos;

    // Prevent texture from 'flipping' in mid-air
    if(hasnoflip())
    {
        if(forward.z() > 0)
        {
            forward.z() = -forward.z();
        }
    }

    up = forward ^ right;

    float up2 = up.length2();

    up2 = mathFastInvSqrt(up2) * _currentsize;
    up *= up2;

    float fw2 = forward.length2();

    fw2 = mathFastInvSqrt(fw2) * _currentsize;

    offset = forward * fw2;

    Vec3f x, y;

    x = forward + offset;
    y = up;

    corner[0] = pos - x - y;
    corner[1] = pos - x + y;
    corner[2] = pos + x + y;
    corner[3] = pos + x - y;

#if(0)
    piVECTOR pa, pb;
    corner[0].get(pa);
    corner[1].get(pb);

    draw3dLine(&pa, &pb, colorBLUEALPHA);
    corner[1].get(pa);
    corner[2].get(pb);

    draw3dLine(&pa, &pb, colorYELLOWALPHA);
    corner[2].get(pa);
    corner[3].get(pb);

    draw3dLine(&pa, &pb, colorREDALPHA);
    corner[3].get(pa);
    corner[0].get(pb);

    draw3dLine(&pa, &pb, colorWHITEALPHA);
#endif

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::stretchParticle
Description:
----------------------------------------------------------------------*/
bool Particle2::stretchParticle(Vec3f corner[4])
{
    static Vec3f forward;
    Vec3f up;
    Vec3f offset;

    forward = _velocity * _stretchlength;

    Vec3f pos;
    getrelativeposition(pos);

    Vec3f right;
    right = Vec3f(-playpos.x, -playpos.y, -playpos.z) - pos;

    // Prevent texture from 'flipping' in mid-air
    if(hasnoflip())
    {
        if(forward.z() < 0)
        {
            forward.z() = -forward.z();
        }
    }

    up = forward ^ right;
    float up2 = up.length2();

    up2 = mathFastInvSqrt(up2) * _currentsize;
    up = up * up2;

    float fw2 = forward.length2();

    fw2 = mathFastInvSqrt(fw2) * _currentsize;
    offset = forward * fw2;

    Vec3f p1 = pos;
    Vec3f p2 = pos - _velocity * _stretchlength;

	corner[0] = p2 + (forward - offset) + up;
	corner[1] = p2 + (forward - offset) - up;
	corner[2] = p1 - (forward + offset) - up;
	corner[3] = p1 - (forward + offset) + up;

#if(0)
    piVECTOR pa, pb;
    corner[0].get(pa);
    corner[1].get(pb);

    draw3dLine(&pa, &pb, colorBLUEALPHA);
    corner[1].get(pa);
    corner[2].get(pb);

    draw3dLine(&pa, &pb, colorYELLOWALPHA);
    corner[2].get(pa);
    corner[3].get(pb);

    draw3dLine(&pa, &pb, colorREDALPHA);
    corner[3].get(pa);
    corner[0].get(pb);

    draw3dLine(&pa, &pb, colorWHITEALPHA);
#endif

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::LegacyStretchParticle
Description: Legacy stretch from old code.
----------------------------------------------------------------------*/
bool Particle2::LegacyStretchParticle(Vec3f corner[4])
{
    if(_currentsize < 0.0001f)
        return false;

    float Ratio = _stretchlength / _currentsize;

    piVECTOR TempVec;

    TempVec.x = _velocity.x() * Ratio;
    TempVec.y = _velocity.y() * Ratio;
    TempVec.z = _velocity.z() * Ratio;

    piVECTOR TempVec1;

    TempVec1.x = _velocity.x() * _stretchlength;
    TempVec1.y = _velocity.y() * _stretchlength;
    TempVec1.z = _velocity.z() * _stretchlength;

    piVECTOR vecPos;
    Vec3f pos;
    getrelativeposition(pos);
    pos.get(vecPos);

    mathAddVectors(&TempVec1,&vecPos,&TempVec1);
    mathAddVectors(&TempVec1,&TempVec1,&playpos);

	float Len = mathVectDotProduct(&playdir,&TempVec1);
	if (Len <= 0.0)//size * 2.0f)
	{
		return false;
	}

    TempVec1.x = _velocity.x() * _stretchlength;
    TempVec1.y = _velocity.y() * _stretchlength;
    TempVec1.z = _velocity.z() * _stretchlength;

    mathSubVectors(&TempVec1,&vecPos,&TempVec1);
    mathAddVectors(&TempVec1,&TempVec1,&playpos);

	Len = mathVectDotProduct(&playdir,&TempVec1);
	if (Len <= 0.0f)//size * 2.0f)
	{
		return false;
	}

    // we must project this point onto the view plane
    // and then compensate for perspective

    piVECTOR V1, V2;

    // determine direction to point
    mathAddVectors(&V1,&vecPos,&playpos);
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

	if (Cos > 0.001f || Cos < -0.001f)
	{
		Len = Len / Cos;

		// move point
		TempVec.x -= V1.x * Len;
		TempVec.y -= V1.y * Len;
		TempVec.z -= V1.z * Len;
	}

	piMATRIX ViewAttMatrix = *piGetUnscaledViewMatrix();
	ViewAttMatrix.m[3][0] = ViewAttMatrix.m[3][1] = ViewAttMatrix.m[3][2] = 0.0f;

	math3dVectorMultiply(&TempVec,&TempVec,&ViewAttMatrix);

    float StretchLength;

	Len = StretchLength = (FLOAT32)sqrt(TempVec.x * TempVec.x + TempVec.y * TempVec.y);

    if (StretchLength > 1.0f)
    {
		float OneOverHypotenuse = 1.0f / Len;

		// set up displacement and conversion to view space
		TempVec1.x = -vecPos.x;
		TempVec1.y = -vecPos.y;
		TempVec1.z = -vecPos.z;
		math3dVectorMultiply(&TempVec1,&TempVec1,&ViewAttMatrix);
		piMATRIX Mat2 = ViewAttMatrix;
		Mat2.m[3][0]/*_41*/ = TempVec1.x;
		Mat2.m[3][1]/*_42*/ = TempVec1.y;
		Mat2.m[3][2]/*_43*/ = TempVec1.z;

		// now rotate so that we're aligned with the y axis for stretching
		piMATRIX Mat1 = IdentityMatrix;

		Mat1.m[0][0] = Mat1.m[1][1] = TempVec.y * OneOverHypotenuse; // cos
		Mat1.m[0][1] = TempVec.x * OneOverHypotenuse; // sin
		Mat1.m[1][0] = -Mat1.m[0][1];
		math3dMatrixMultiply(&Mat1,&Mat2,&Mat1);

		// now STREEEEEEEEETTTTTCCCCCHHHHHH!!!!!! :)
		piMATRIX Mat3 = IdentityMatrix;
		Mat3.m[1][1]/*_22*/ = StretchLength;
		math3dMatrixMultiply(&Mat2,&Mat1,&Mat3);

		// invert the setup matrix
		math3dInvertMatrix(&Mat1,&Mat1);

		// and reverse everything so we're back in render space
		math3dMatrixMultiply(&Mat2,&Mat2,&Mat1);

        piVECTOR v1, v2, v3, v4;
        corner[0].get(v1);
        corner[1].get(v2);
        corner[2].get(v3);
        corner[3].get(v4);

		math3dVectorMultiply(&v1, &v1, &Mat2);
		math3dVectorMultiply(&v2, &v2, &Mat2);
		math3dVectorMultiply(&v3, &v3, &Mat2);
		math3dVectorMultiply(&v4, &v4, &Mat2);

        corner[0].set(v1);
        corner[1].set(v2);
        corner[2].set(v3);
        corner[3].set(v4);
    }

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::canstretch
Description:
----------------------------------------------------------------------*/
bool Particle2::canstretch()
{
    if(stretchalways())
    {
        return true;
    }

    if(_currentsize <= 0.0001f)
    {
        return false;
    }

    float Ratio = _stretchlength / _currentsize;

    Vec3f TempVec = _velocity * Ratio;

    if(TempVec.length() <= 1.0f)
    {
        return false;
    }

    Vec3f pos;
    getrelativeposition(pos);

    Vec3f player(playpos);
    player = -player;

    float theta1;

    Vec3f playertoparticle = pos - player;
    Vec3f dir = playertoparticle;
    dir.fastnormalize();

    float len = _velocity * Vec3f(playdir);

    Vec3f playertovelocity;

    if(len < 0)
        playertovelocity = pos - TempVec - player;
    else
        playertovelocity = pos + TempVec - player;

    theta1 = (playertoparticle * playertovelocity) / (playertoparticle.length() * playertovelocity.length());
    theta1 = theta1 > 1.0f ? 1.0f : theta1 < -1.0f ? -1.0f : theta1;
    theta1 = acos(theta1);

    float theta2 = atan(_currentsize * 2.0f / playertoparticle.length());

    if(theta1 <= theta2)
    {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::setupshape
Description:
----------------------------------------------------------------------*/
bool Particle2::setupshape()
{
    logTimerBeginMacro();

    bool calculated = false;

    Vec3f pos;
    getrelativeposition(pos);

    bool stretchok = false;

    if(haslegacystretch() || hasstretch())
    {
        stretchok = canstretch();

        if(!stretchok)
        {
            popflag(part_has_legacy_stretch | part_has_stretch | part_has_fixstretch);
        }
    }

    if(!haslegacystretch() && hasstretch())
    {
        if(hasfixedstretch())
        {
            calculated = fixstretchParticle(_corners);
        }
        else
        {
            calculated = stretchParticle(_corners);
        }
    }
    else if(istype(part_is_beam))
    {
        calculated = particleBeam(_corners);
    }

#if(0)
    if(hasstretch() && !stretchalways() && !calculated)
    {
        // Move position to reduce visual anomalies from stretch to rotate to face
       // _position -= (_velocity * _stretchlength) * 0.5f;
        popflag(part_has_stretch | part_has_fixstretch);
    }
#endif

    if(!calculated)
    {
        if(iswatersurfaceeffect())
        {
            _corners[0] = _extents[0] * _currentsize + pos;
            _corners[1] = _extents[1] * _currentsize + pos;
            _corners[2] = _extents[2] * _currentsize + pos;
            _corners[3] = _extents[3] * _currentsize + pos;
        }
        else
        {
            // Do normal rotate to face, unless special cased.
            Vec3f x, y;

            x = Vec3f(playright) * _currentsize;
            y = Vec3f(playup) * _currentsize;

            _corners[0].x() = pos.x() + _extents[0].x() * x.x() + _extents[0].y() * y.x();
            _corners[0].y() = pos.y() + _extents[0].x() * x.y() + _extents[0].y() * y.y();
            _corners[0].z() = pos.z() + _extents[0].x() * x.z() + _extents[0].y() * y.z();

            _corners[1].x() = pos.x() + _extents[1].x() * x.x() + _extents[1].y() * y.x();
            _corners[1].y() = pos.y() + _extents[1].x() * x.y() + _extents[1].y() * y.y();
            _corners[1].z() = pos.z() + _extents[1].x() * x.z() + _extents[1].y() * y.z();

            _corners[2].x() = pos.x() + _extents[2].x() * x.x() + _extents[2].y() * y.x();
            _corners[2].y() = pos.y() + _extents[2].x() * x.y() + _extents[2].y() * y.y();
            _corners[2].z() = pos.z() + _extents[2].x() * x.z() + _extents[2].y() * y.z();

            _corners[3].x() = pos.x() + _extents[3].x() * x.x() + _extents[3].y() * y.x();
            _corners[3].y() = pos.y() + _extents[3].x() * x.y() + _extents[3].y() * y.y();
            _corners[3].z() = pos.z() + _extents[3].x() * x.z() + _extents[3].y() * y.z();

            if(haslegacystretch())
            {
                if(!LegacyStretchParticle(_corners))
                {
                    return false;
                }
            }
        }
    }

    _distance2 =  math3dFastSqrt((pos + Vec3f(playpos)).length2());

    logTimerEndMacro();

    return true;
}

/*----------------------------------------------------------------------
Function: Particle2::updatevelocity
Description:
----------------------------------------------------------------------*/
void Particle2::updatevelocity(float time)
{
    _velocity += (_velvariance * time);
}

/*----------------------------------------------------------------------
Function: _timeindex
Description:
-----------------------------------------------------------------------*/
inline int _timeindex(Particle2* p)
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

/*----------------------------------------------------------------------
Function: _timet
Description:
-----------------------------------------------------------------------*/
inline float _timet(Particle2* p, int i)
{
    int t1, t2;

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

    float t = (float)(frametime - t1) / (float)(t2 - t1);

    return t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
}

/*----------------------------------------------------------------------
Function: _lerpsize
Description:
-----------------------------------------------------------------------*/
inline float _lerpsize(Particle2* p, float t, int i)
{
    float s1, s2;

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

    float size = s1 + t * (s2 - s1);
    return size < 0.0f ? 0.0f : size;
}

/*----------------------------------------------------------------------
Function: _lerpalpha
Description:
-----------------------------------------------------------------------*/
inline float _lerpalpha(Particle2* p, float t, int i)
{
    float a1, a2;

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

    float alpha = a1 + t * (a2 - a1);
    return alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
}

/*----------------------------------------------------------------------
Function: _lerpcolor
Description:
-----------------------------------------------------------------------*/
inline void _lerpcolor(Particle2* p, float t, int i)
{
    Vec3f c1, c2;

    if(p->hascolorstages())
    {
        c1 = p->_color[i - 1];
        c2 = p->_color[i];
    }
    else
    {
        c1 = p->_color[0];
        c2 = p->_color[1];
    }

    Vec3f color = c1 + (c2 - c1) * t;
    p->_currentcolor = Vec4f(color.x(), color.y(), color.z(), p->_currentalpha);
}

/*----------------------------------------------------------------------
Function: Particle2::update
Description:
----------------------------------------------------------------------*/
void Particle2::update(const piVECTOR& delta)
{
    logTimerBeginMacro();

    if(_updatecallback)
    {
        _updatecallback(this);
    }

    int index = _timeindex(this);

    float t = _timet(this, index);

    _currentsize = _lerpsize(this, t, index);

    if(_alpha[0] || _alpha[1] || _alpha[2] || _alpha[3])
    {
        _currentalpha = _lerpalpha(this, t, index);
    }

    _lerpcolor(this, t, index);

    if(_flags & part_follows_player_xy)
    {
        _position.x() += delta.x;
        _position.y() += delta.y;
        _position.z() += delta.z;
    }

    int timediff = updatetime();

    float time = 0.001f * (float)timediff;

    if(hasgravity())
    {
        applygravity(time);
    }

    if(hasdrag())
    {
        applydrag(time);
    }

    if(hasstretch() || haslegacystretch())
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
        if(_rotate != 0)
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
