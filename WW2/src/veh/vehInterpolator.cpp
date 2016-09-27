#include <syscfg.h> // Path is project specific.

#include <math.h>

#include "gr2/gr2Anim.h"
#include "veh/vehInterpolator.h"
#include "veh/vehmain.h"
#include "math/mathvect.h"
#include "fp/fpTPSdata.h"

bool DualInterpolator::use_new = false;

void DualInterpolator::Copy(vehVehicle* v, UINT32 flags) {
	DualInterpolator* other = dynamic_cast<DualInterpolator*>(&v->interpolator);
	if ( ! other ) return;
	_new.CopyImpl(v, &other->_new, flags);
	_old.CopyImpl(v, &other->_old, flags);
}

namespace {
	// Copies position-related data.
	void CopyPositionData(DualInterpolator::Update& lhs, const DualInterpolator::Update& rhs) {
		lhs.pos = rhs.pos;
		lhs.dir = rhs.dir;
		lhs.vel = rhs.vel;
		lhs.speed = rhs.speed;
	}
}

/************************************************************************************************
	vehInterpolator
*************************************************************************************************/

InterpolatorBase::InterpolatorBase(vehVehicle* v) :
	_vehicle(v),
	_interpolatetime(0),
	_lastUpdateYaw(0)
{
}

//==============================================================================

InfantryPredictor::InfantryPredictor(vehVehicle* v)
: InterpolatorBase(v) {
	prev_msg.frametime = curr_msg.frametime = -1;
}

//------------------------------------------------------------------------------

void InfantryPredictor::NewPosDataUpdate(Timestamp timestamp, FrameTime frametime) {
	prev_msg = curr_msg;

	curr_msg.timestamp = timestamp;
	curr_msg.frametime = frametime;
	curr_msg.pos = _vehicle->PosData.Pos;
	curr_msg.vel = _vehicle->PosData.Vel;
	curr_msg.vel.x = -_vehicle->PosData.Vel.x;        // Weird math stuff is due to some
	curr_msg.dir.x = -sin(_vehicle->PosData.Att.Yaw); // sort of hack in how infantry
	curr_msg.dir.y = -cos(_vehicle->PosData.Att.Yaw); // position/direction is handled.
	curr_msg.dir.z = 0;
	curr_msg.speed = mathExactVectorLength(&curr_msg.vel);
	curr_msg.animationFlags = _vehicle->PosData.animationFlags;

	// Our prediction is always based on the latest information.
	CopyPositionData(pred_pos, curr_msg);

	if ( prev_msg.frametime == -1 ) {
		// This is the first update we've received
		pred_pos = real_pos = prev_msg = curr_msg;
		real_pos.frametime = -1; // HACK: To guarantee Interpolate() is called on our first update.
	} else if ( curr_msg.timestamp < prev_msg.timestamp ) {
		// Because the timestamp is sent over the network as,
		// a uint16, it will rollover every ~65 seconds.
		typedef decltype(SHARED_VEHICLE_INFO::posTimeStamp) RolloverType;
		prev_msg.timestamp -= std::numeric_limits<RolloverType>::max();
		real_pos.timestamp -= std::numeric_limits<RolloverType>::max();
	}

	// Correct the timestamp if our earlier messages were lagging behind.
	if ( curr_msg.timestamp < real_pos.timestamp )
		real_pos.timestamp = curr_msg.timestamp;

	UpdateInterpolation();
}

//------------------------------------------------------------------------------

const InterpolatorBase::Update& InfantryPredictor::Interpolate(FrameTime frametime) {
	if ( frametime == real_pos.frametime )
		return real_pos;

	// Update our predicted position before doing anything else.
	{
		pred_pos.pos += pred_pos.vel * ((frametime - pred_pos.frametime) / 1000.0);

		// TODO: Collision.

		pred_pos.frametime = frametime;
		pred_pos.timestamp += (frametime - pred_pos.frametime);
	}

	// Update the real position.
	// TODO: A lot of this is still mostly from the old interpolator and could use some cleanup.
	if ( frametime < interp_end ) {
		double t = (double)(frametime - interp_beg) / (double)(interp_end - interp_beg);
		assert ( 0.0 <= t && t <= 1.0 );
		if ( t < 0.0 ) t = 0.0;
		if ( t > 1.0 ) t = 1.0;

		real_pos.pos = spline.InterpolatePoint(t);

		FLOAT64VECTOR vel = spline.InterpolateDt(t);
		mathScaleVector(&vel, &vel, 1000.0/(interp_end - interp_beg));

		FLOAT64VECTOR dir = vel;
		mathNormalize(&dir);

		double speed = mathExactVectorLength2(&vel);
		if ( speed < 0.1 ) { // HACK: To avoid twitchiness
			vel = { 0.0, 0.0, 0.0 };
			speed = 0.0;
			dir = curr_msg.dir;
		}

		real_pos.vel = vel;
		real_pos.speed = speed;
		// If the last message has us not going somewhat forward, respect that orientation.
		// TODO: We should use a proper constant to determine what counts as strafing.
		if ( mathVectDotProduct(&curr_msg.dir, &curr_msg.vel) < .42 )
			real_pos.dir = curr_msg.dir;
		else
			real_pos.dir = dir;
	} else {
		// When we're done with interpolation, keep following the prediction.
		CopyPositionData(real_pos, pred_pos);
	}

	// TODO: Collision.
	// TODO: Gravity.

	// Override motion flags to match what the player's seeing.
	gr2AnimBitPacker flags(curr_msg.animationFlags);
	if ( flags.movement <= TPS_MOVETYPE_IDLE && real_pos.speed >= .1 ) {
		if ( flags.pose == TPS_POSETYPE_STAND ) {
			if ( real_pos.speed > 6.0 ) // TODO: Match this with an actual constant.
				flags.movement = TPS_MOVETYPE_SPRINT;
			else if ( real_pos.speed > 0.1 )
				flags.movement = TPS_MOVETYPE_FORWARD;
			else
				flags.movement = TPS_MOVETYPE_STILL;
		}
		if ( flags.pose == TPS_POSETYPE_PRONE && real_pos.speed >= 1.0f ) {
			// Don't allow fast crawling
			flags.pose = TPS_POSETYPE_CROUCH;
			flags.movement = TPS_MOVETYPE_FORWARD;
		} else {
			flags.movement = TPS_MOVETYPE_FORWARD;

			if ( flags.pose == TPS_POSETYPE_PRONE )
				flags.point = TPS_POINTTYPE_RELAXED;
		}
	}
	real_pos.animationFlags = flags.pack();

	real_pos.frametime = frametime;
	real_pos.timestamp += (frametime - real_pos.frametime);

	return real_pos;
}

//------------------------------------------------------------------------------

void InfantryPredictor::Copy(vehVehicle* v, UINT32 flags) {
	InfantryPredictor* from = dynamic_cast<InfantryPredictor*>(&v->interpolator);

	if ( ! from )
		return;

	CopyImpl(v, from, flags);
}

void InfantryPredictor::CopyImpl(vehVehicle* v, InfantryPredictor* from, UINT32 flags) {
	prev_msg = from->prev_msg;
	curr_msg = from->curr_msg;
	pred_pos = from->pred_pos;
	real_pos = from->real_pos;

	prev_msg.animationFlags = curr_msg.animationFlags =
	pred_pos.animationFlags = real_pos.animationFlags = flags;

	prev_msg.pos = curr_msg.pos =
	pred_pos.pos = real_pos.pos = v->PosData.Pos;

	FLOAT64VECTOR dir = { -sin(v->PosData.Att.Yaw), -cos(v->PosData.Att.Yaw), 0.0 };
	prev_msg.dir = curr_msg.dir =
	pred_pos.dir = real_pos.dir = dir;

	_vehicle->GetAnim()->twist.predictedYaw = v->GetAnim()->twist.predictedYaw;
}

//------------------------------------------------------------------------------

void InfantryPredictor::UpdateInterpolation(void) {
	// TODO: Consider skipping interpolation if extreme lag is going on.

	// Make the real and predicted positions match in this many frames:
	const FrameTime interp_time = 200;

	const Update& from = real_pos;
	Update to = curr_msg;
	// We're smoothing from the on-client position, so adjust the time accordingly.
	to.pos += to.vel * ((interp_time - (to.frametime - from.frametime)) / 1000.0);

	const double dt = interp_time / 1000.0;
	FLOAT64VECTOR v1, v2;
	if ( mathExactVectorLength2(&from.vel) > .001) {
		mathScaleVector(&v1, &from.vel, dt);
	} else {
		mathScaleVector(&v1, &from.dir, .001);
	}
	if(mathExactVectorLength2(&to.vel) > .001) {
		mathScaleVector(&v2, &to.vel, dt);
	} else {
		mathScaleVector(&v2, &to.dir, .001);
	}

	spline.SetControlPointsWithDt(from.pos, v1, to.pos, v2);

	interp_beg = from.frametime;
	interp_end = interp_beg + interp_time;
}

//==============================================================================

InfantryInterpolator::InfantryInterpolator(vehVehicle* v) : InterpolatorBase(v),
	_dead(false),
	_rollover(false)
{
}

void InfantryInterpolator::NewPosDataUpdate(int timestamp, int frametime)
{

	//printf("%s %lf\n", _vehicle->Handle, (frametime - _to.frametime) / (double)(_to.timestamp - _from.timestamp));

	//if(_dead) return;

	Interpolate(frametime);

	if(_cur.frametime && _next.timestamp > _cur.timestamp && _next.timestamp - _cur.timestamp < 2000)
	{
		_cur.animationFlags = _to.animationFlags;
		_from = _cur;
	}
	else
	{
		_to.vel.x = _to.vel.y = _to.vel.z = 0;
		_from = _to;
	}

	//	_to = _next;
	//	_to.frametime = frametime;

	_next.timestamp = timestamp;
	_next.pos = _vehicle->PosData.Pos;
	_next.vel = _vehicle->PosData.Vel;
	_next.vel.x = -_vehicle->PosData.Vel.x; // usual bs (for infantry)
	_next.dir.x = -sin(_vehicle->PosData.Att.Yaw); // more bs (inf specific as well i think)
	_next.dir.y = -cos(_vehicle->PosData.Att.Yaw);
	_next.dir.z = 0;
	_next.speed = mathExactVectorLength(&_next.vel);
	_lastUpdateYaw = _vehicle->PosData.Att.Yaw;

	_next.animationFlags = _vehicle->PosData.animationFlags;

	// check for a short rollover
	if(_next.timestamp < _to.timestamp)
	{
		//printf("SHORT ROLLOVER\n");

		if(_from.frametime)
			_from.timestamp -= USHRT_MAX;
		if(_to.frametime)
			_to.timestamp -= USHRT_MAX;
		if(_cur.frametime)
			_cur.timestamp -= USHRT_MAX;

		_rollover = true;
	}
	else
	{
		_rollover = false;
	}

	// not buffering for now, people don't like the delay when fighting in buildings
	_to = _next;
	_to.frametime = frametime;

	UpdateSpline(_from.pos, _to.pos, _from.vel, _to.vel, _from.dir, _to.dir, (_to.timestamp - _from.timestamp)/1000.0);
}

void InfantryInterpolator::UpdateSpline(FLOAT64VECTOR const& frompos, FLOAT64VECTOR const& topos
								,  FLOAT64VECTOR const& fromvel, FLOAT64VECTOR const& tovel
								,  FLOAT64VECTOR const& fromdir, FLOAT64VECTOR const& todir, double dt)
{
	FLOAT64VECTOR v1, v2;

	if(mathExactVectorLength2(&fromvel) > .001)
	{
		mathScaleVector(&v1, &fromvel, dt);
	}
	else
	{
		mathScaleVector(&v1, &fromdir, .001);
	}

	if(mathExactVectorLength2(&tovel) > .001)
	{
		mathScaleVector(&v2, &tovel, dt);
	}
	else
	{
		mathScaleVector(&v2, &todir, .001);
	}

	_spline.SetControlPointsWithDt(frompos, v1, topos, v2);

	/*if(mathExactVectorLength(&tovel) > .1 || mathExactVectorLength(&fromvel) > .1)
	{
		printf("%s\n", _vehicle->Handle);
		printf("%lf m/s %lf deg %lf deg -> %lf m/s %lf deg %lf deg\n",  mathExactVectorLength(&fromvel), atan2(v1.y, v1.x)*180/PI,  atan2(fromdir.y, fromdir.x)*180/PI
																	,	mathExactVectorLength(&tovel), atan2(v2.y, v2.x)*180/PI, atan2(todir.y, todir.x)*180/PI );
		FLOAT64VECTOR diff;
		mathSubVectors(&diff, &topos, &frompos);
		printf("%lf m %lf sec %lf m/s\n", mathExactVectorLength(&diff), dt, mathExactVectorLength(&diff) / dt);

		for(double t = 0; t<1.09; t+=.1)
		{
			FLOAT64VECTOR vel = _spline.InterpolateDt(t);
			mathScaleVector(&vel, &vel, 1/dt);

			//printf("%lf %lf\n", t, mathExactVectorLength(&vel));
			printf("%lf %lf m/s %lf deg\n", t, mathExactVectorLength(&vel), atan2(vel.y, vel.x)*180/PI);
		}
	}
	*/
}

InterpolatorBase::Update const& InfantryInterpolator::Interpolate(int frametime)
{
	return _to;

	if(_from.frametime == 0 || _to.frametime == 0) return _cur; // _dead
	if(_interpolatetime == frametime) return _cur;

	double dt,t;
	dt = (_to.timestamp - _from.timestamp) / 1000.0;
	t = (dt > .001) ? (frametime - _to.frametime) / (double)(_to.timestamp - _from.timestamp) : 0;

	//printf("t: %lf\n", t);

	// haven't gotten the next update yet so use the one that's buffered in _next
	/*
	if(t > 1 && _next.frametime)
	{
		//printf("%s _next\n", _vehicle->Handle);
		_from = _to;
		_to = _next;
		_to.frametime = frametime;
		_next.frametime = 0;

		dt = (_to.timestamp - _from.timestamp) / 1000.0;
		t = (dt > .001) ? (frametime - _to.frametime) / (double)(_to.timestamp - _from.timestamp) : 0;

		UpdateSpline(_from.pos, _to.pos, _from.vel, _to.vel, _from.dir, _to.dir, dt);
	}
	*/

	if(t < 0) t = 0;
	if(t > 1) t = 1;

	_cur.frametime = frametime;
	_cur.timestamp = _from.timestamp + (frametime - _to.frametime);

	//	if(_cur.timestamp > _to.timestamp)
	//		_cur.timestamp = _to.timestamp;

	//printf("%f %f %f\n", (float)t, (float)(_from.timestamp + (frametime - _from.frametime)), (float)(_from.timestamp * (1-t) + _to.timestamp * t));

	_cur.pos = _spline.InterpolatePoint(t);

	FLOAT64VECTOR predictVel = _spline.InterpolateDt(t);

	mathScaleVector(&predictVel, &predictVel, 1/dt);

	FLOAT64VECTOR veldir = predictVel;
	mathNormalize(&veldir);

	gr2AnimBitPacker toFlags(_to.animationFlags);
	gr2AnimBitPacker fromFlags(_from.animationFlags);
	gr2AnimBitPacker curFlags = toFlags; //fromFlags;

	double speed = mathExactVectorLength2(&predictVel);

	if(speed <= (.05f * .05f))  // was .1
	{
		_cur.vel.x = _cur.vel.y = _cur.vel.z = 0;
	}
	else
	{
		_cur.dir = veldir;
		_cur.vel = predictVel;
	}

	FLOAT64VECTOR delta;
	mathSubVectors(&delta, &_to.pos, &_from.pos);

	if(mathExactVectorLength2(&delta) < .01) // no movement just rotation
	{
		_cur.dir = _to.dir;
		_cur.vel.x = _cur.vel.y = _cur.vel.z = 0;
	}
	else if(mathVectDotProduct(&_to.dir, &_to.vel) < .42) // strafing
	{
		//printf("strafing\n");
		_cur.dir = _to.dir;
	}

	_cur.speed = mathExactVectorLength(&_cur.vel);

	// override sent speed with what comes from our predicted speed
	if(curFlags.movement <= TPS_MOVETYPE_IDLE && speed >= .1)
	{
		if(curFlags.pose == TPS_POSETYPE_STAND)
		{
			if (_cur.speed >= 6.0f)
				curFlags.movement = TPS_MOVETYPE_SPRINT;
			else if (_cur.speed > .1f)
				curFlags.movement = TPS_MOVETYPE_FORWARD;
			else
				curFlags.movement = TPS_MOVETYPE_STILL;
		}
		if(curFlags.pose == TPS_POSETYPE_PRONE && _cur.speed >= 1.0f)
		{
			// don't allow fast crawling
			curFlags.pose = TPS_POSETYPE_CROUCH;
			curFlags.movement = TPS_MOVETYPE_FORWARD;
		}
		else
		{
			curFlags.movement = TPS_MOVETYPE_FORWARD;

			if(curFlags.pose == TPS_POSETYPE_PRONE)
				curFlags.point = TPS_POINTTYPE_RELAXED;
		}
	}

	curFlags.deathState = toFlags.deathState;

	_dead = curFlags.deathState;
	_cur.animationFlags = curFlags.pack(); // toFlags.pack();
	_interpolatetime = frametime;

	//if(mathExactVectorLength(&_cur.vel) > .1)
	//	printf("%lf %lf m/s %lf deg %lf deg\n", t, mathExactVectorLength(&_cur.vel), atan2(_cur.vel.y, _cur.vel.x)*180/PI, atan2(_cur.dir.y, _cur.dir.x)*180/PI);

	return _cur;
}

void InfantryInterpolator::Copy(vehVehicle* v, UINT32 flags) {
	InfantryInterpolator* from = dynamic_cast<InfantryInterpolator*>(&v->interpolator);

	if ( ! from ) return;

	CopyImpl(v, from, flags);
}

void InfantryInterpolator::CopyImpl(vehVehicle* v, InfantryInterpolator* from, UINT32 flags) {
	_from					= from->GetFrom();
	_cur					= from->GetCur();
	_to						= from->GetTo();
	_next					= from->GetNext();
	//_lastUpdateYaw			= v->PosData.Att.Yaw;

	_from.animationFlags	= flags;
	_cur.animationFlags		= flags;
	_to.animationFlags		= flags;
	_next.animationFlags	= flags;

	_from.pos				= v->PosData.Pos;
	_cur.pos				= v->PosData.Pos;
	_to.pos					= v->PosData.Pos;
	_next.pos				= v->PosData.Pos;

	_from.dir.x				= -sin(v->PosData.Att.Yaw);
	_from.dir.y				= -cos(v->PosData.Att.Yaw);

	_cur.dir				= _from.dir;
	_to.dir					= _from.dir;
	_next.dir				= _from.dir;

	_vehicle->GetAnim()->twist.predictedYaw = v->GetAnim()->twist.predictedYaw;
}

void PassThruInterpolator::NewPosDataUpdate(int timestamp, int frametime)
{
	//if(_dead) return; // causes para troops to freeze in midair

	gr2AnimBitPacker curFlags(_vehicle->PosData.animationFlags);

	_dead = curFlags.deathState;
	_cur.animationFlags = curFlags.pack();
	_interpolatetime = frametime;

	_cur.timestamp = timestamp;
	_cur.frametime = frametime;
	_cur.pos = _vehicle->PosData.Pos;
	_cur.vel = _vehicle->PosData.Vel;
	_cur.vel.x = -_vehicle->PosData.Vel.x; // usual bs (for infantry)
	_cur.dir.x = -sin(_vehicle->PosData.Att.Yaw); // more bs (inf specific as well i think)
	_cur.dir.y = -cos(_vehicle->PosData.Att.Yaw);
	_cur.dir.z = 0;
	_cur.speed = mathExactVectorLength(&_cur.vel);

	if(_cur.speed <= .05)  // was .1
		_cur.vel.x = _cur.vel.y = _cur.vel.z = 0;

	FLOAT64VECTOR delta;
	mathSubVectors(&delta, &_last.pos, &_cur.pos);

	if(mathExactVectorLength2(&delta) < .01) // no movement just rotation
	{
		_cur.dir = _last.dir;
		_cur.vel.x = _cur.vel.y = _cur.vel.z = 0;
	}
	else if(mathVectDotProduct(&_cur.dir, &_cur.vel) < .42) // strafing
	{
		_cur.dir = _last.dir;
	}

	_last = _cur;
	_lastUpdateYaw = _vehicle->PosData.Att.Yaw;
}

InterpolatorBase::Update const& PassThruInterpolator::Interpolate(int frametime)
{
	return _cur;
}

void PassThruInterpolator::Copy(vehVehicle* v, UINT32 flags)
{
	// do nothing
}

void CubicSpline::SetControlPoints(FLOAT64VECTOR const& v0, FLOAT64VECTOR const& v1, FLOAT64VECTOR const& v2, FLOAT64VECTOR const& v3)
{
	_xc[3] = 1 * v3.x -	3 * v2.x +	3 * v1.x -	1 * v0.x;
	_xc[2] =			3 * v2.x -	6 * v1.x +	3 * v0.x;
	_xc[1] =						3 * v1.x -	3 * v0.x;
	_xc[0] =									1 * v0.x;

	_yc[3] = 1 * v3.y -	3 * v2.y +	3 * v1.y -	1 * v0.y;
	_yc[2] =			3 * v2.y -	6 * v1.y +	3 * v0.y;
	_yc[1] =						3 * v1.y -	3 * v0.y;
	_yc[0] =									1 * v0.y;

	_zc[3] = 1 * v3.z -	3 * v2.z +	3 * v1.z -	1 * v0.z;
	_zc[2] =			3 * v2.z -	6 * v1.z +	3 * v0.z;
	_zc[1] =						3 * v1.z -	3 * v0.z;
	_zc[0] =									1 * v0.z;
}

void CubicSpline::SetControlPointsWithDt(FLOAT64VECTOR const& x0, FLOAT64VECTOR const& v0, FLOAT64VECTOR const& x1, FLOAT64VECTOR const& v1)
{
	_xc[3] =  1 * v1.x	- 2 * x1.x + 1 * v0.x + 2 * x0.x;
	_xc[2] = -1 * v1.x	+ 3 * x1.x - 2 * v0.x - 3 * x0.x;
	_xc[1] =						 v0.x;
	_xc[0] =										x0.x;

	_yc[3] =  1 * v1.y	- 2 * x1.y + 1 * v0.y + 2 * x0.y;
	_yc[2] = -1 * v1.y	+ 3 * x1.y - 2 * v0.y - 3 * x0.y;
	_yc[1] =						 v0.y;
	_yc[0] =										x0.y;

	_zc[3] =  1 * v1.z	- 2 * x1.z + 1 * v0.z + 2 * x0.z;
	_zc[2] = -1 * v1.z	+ 3 * x1.z - 2 * v0.z - 3 * x0.z;
	_zc[1] =						 v0.z;
	_zc[0] =										x0.z;
}

FLOAT64VECTOR CubicSpline::InterpolatePoint(double t)
{
	FLOAT64VECTOR pos;

	pos.x = calcpos(_xc, t);
	pos.y = calcpos(_yc, t);
	pos.z = calcpos(_zc, t);

	return pos;
}

FLOAT64VECTOR CubicSpline::InterpolateDt(double t)
{
	FLOAT64VECTOR dt;

	dt.x = calcdt(_xc, t);
	dt.y = calcdt(_yc, t);
	dt.z = calcdt(_zc, t);

	return dt;
}

double CubicSpline::calcpos(double c[4], double t)
{
	return c[3]*t*t*t + c[2]*t*t + c[1]*t + c[0];
}

double CubicSpline::calcdt(double c[4], double t)
{
	return 3*c[3]*t*t + 2*c[2]*t + c[1];
}
