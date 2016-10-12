#include <syscfg.h> // Path is project specific.
#include "veh/vehInterpolator.h"

#include <cmath>

#include <fp/fpTPSdata.h>
#include <fp/fpFPS.h>
#include <gr2/gr2Anim.h>
#include <math/mathvect.h>

#include "veh/vehmain.h"

namespace {
	// Copies position-related data.
	void CopyPositionData(vehInterpolator::Update& lhs, const vehInterpolator::Update& rhs) {
		lhs.pos = rhs.pos;
		lhs.dir = rhs.dir;
		lhs.vel = rhs.vel;
		lhs.speed = rhs.speed;
	}
}

//==============================================================================

namespace {
	// Make the real and predicted positions match in this many milliseconds.
	const InfantryPredictor::FrameTime interp_time = 300;
}

InfantryPredictor::InfantryPredictor(vehVehicle* v)
: _vehicle(v) {
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
	_lastUpdateYaw = _vehicle->PosData.Att.Yaw;
	curr_msg.speed = mathExactVectorLength(&curr_msg.vel);
	curr_msg.animationFlags = _vehicle->PosData.animationFlags;

	// Our prediction is always based on the latest information.
	CopyPositionData(pred_pos, curr_msg);
	pred_pos.animationFlags = curr_msg.animationFlags;

	if ( prev_msg.frametime == -1 ) {
		// This is the first update we've received
		pred_pos = real_pos = prev_msg = curr_msg;
		real_pos.frametime = -1; // HACK: To guarantee Interpolate() is called on our first update.
	} else if ( curr_msg.timestamp < prev_msg.timestamp ) {
		// Because the timestamp is sent over the network as
		// a uint16, it will rollover every ~65 seconds.
		typedef decltype(SHARED_VEHICLE_INFO::posTimeStamp) RolloverType;
		prev_msg.timestamp -= std::numeric_limits<RolloverType>::max();
		real_pos.timestamp -= std::numeric_limits<RolloverType>::max();
	}

	// Correct the timestamps if our earlier messages were lagging behind.
	if ( curr_msg.timestamp < real_pos.timestamp )
		real_pos.timestamp = curr_msg.timestamp;
	if ( curr_msg.timestamp < pred_pos.timestamp )
		pred_pos.timestamp = curr_msg.timestamp;

	UpdateInterpolation();
}

//------------------------------------------------------------------------------

// !!!HACK!!!
// This is all really horrible and I'm not proud to write it, but the logic for
// this stuff is spread over so many places and makes so many assumptions that
// it would take ages to try to decouple it from the player-controlled soldier,
// and writing something new just isn't on the table right now.
// NOTE: Largely taken from fpFPSAreaChecks().
namespace {
	void CheckCollisions(InfantryPredictor::Update& update, fpTPSPoseType pose) {
		// HACK: Guarantee the FPS colliders have been set up.
		InitColliderGeometry();

		{
			switch ( pose ) {
			case TPS_POSETYPE_PRONE:
				HACK_forced_pose = POSE_PRONE; break;
			case TPS_POSETYPE_CROUCH:
				HACK_forced_pose = POSE_CROUCH; break;
			case TPS_POSETYPE_STAND:
			default:
				HACK_forced_pose = POSE_STAND; break;
			}

			_updateFPSColliders();

			HACK_forced_pose = POSE_NUM_POSES;
		}

		FPSColliderData& coldata = fpFPSGetCollider();
		assert ( coldata.collider );
		FPSCollider& col = *coldata.collider;

		{
			Vec3d pos = update.pos;
			HACK_forced_pos = &pos;
			col.Update(0);
			HACK_forced_pos = nullptr;
		}

		const Vec3d up = Vec3d(0.0,0.0,1.0);
		Vec3d dir = update.vel;
		dir.z() = 0.0; // Vertical motion is much more simple.

		if ( col.findCollisions(true) ) {
			const colliderQuery::Collision* best_floor = nullptr;
			Vec3d push(0.0, 0.0, 0.0);

			const colliderQuery::CollisionList& hits = col.getCollisions();
			for ( auto it = hits.begin() ; it != hits.end() ; ++it ) {
				const colliderQuery::Collision& hit = *it;
				UINT32 collide_bits = dGeomGetCollideBits(hit.g1);
				UINT32 category_bits = dGeomGetCategoryBits(hit.g2);

				switch ( collide_bits ) {
				case WADE_COLLIDER:
					// We don't simulate gravity right now, so this doesn't matter on our end.
					continue;
				case FALL_COLLIDER:
					if ( (up * hit.normal) < 0 )
						continue;
					if ( colliderIsType(category_bits, COLLIDER_VEHICLE_MASK) ||
					     colliderIsType(category_bits, COLLIDER_WALK) )
						continue; // Don't let other entities mess with our prediction.

					if ( (best_floor == nullptr || hit.pos.z() > best_floor->pos.z()) &&
					     // Only count if it forces us out of the ground.
					     hit.pos.z() > update.pos.z ) {
						best_floor = &hit;
					}
					break;
				case CAGE_COLLIDER: {
					// Scenario-only colliders.
					if ( colliderIsType(category_bits, COLLIDER_BLOCK) ||
					     colliderIsType(category_bits, COLLIDER_VOLUME) )
						continue;

					// If we're not really moving, don't bother to check collisions.
					if ( dir.length2() < 0.001 )
						continue;

					double angle = dir * hit.normal;

					// Lower is more forgiving, 1 means must be flat.
					const double flat_threshold = 0.6;

					// Ignore normals facing the wrong direction.
					if ( angle > 0.1 )
						continue;
					// Ignore relatively flat slopes.
					if ( up * hit.normal >= flat_threshold )
						continue;
					// Just ignoring downward-facing normals for the predcitor.
					if ( hit.normal.z() < 0 )
						continue;

					dGeomID geom = dGeomTransformGetGeom(hit.g1);
					int type = dGeomGetClass(geom);

					Vec3d this_push = hit.normal;
					this_push.z() = 0; // Again, height is handled differently.
					this_push.normalize();
					this_push *= hit.depth;

					// We don't bother to check for conflicting push directions since we're relying
					// on the other client somewhat to handle the heavy lifting until we can get to
					// generalizing the collision/gravity code.

					if ( abs(this_push.x()) > abs(push.x()) )
						push.x() = this_push.x();
					if ( abs(this_push.y() > abs(push.y())) )
						push.y() = this_push.y();

					break;
				}
				default:
					continue;
				}
			}

			if ( best_floor )
				update.pos.z = best_floor->pos.z();

			if ( push.length2() != 0.0 ) {
				Vec3d pos = update.pos;
				pos += push;
				update.pos = {pos.x(), pos.y(), pos.z()};
			}
		}

		// Reset the collider just in case code elsewhere doesn't.
		_updateFPSColliders();
	}
}

//--------------

const InfantryPredictor::Update& InfantryPredictor::Interpolate(FrameTime frametime) {
	if ( frametime == real_pos.frametime )
		return real_pos;

	gr2AnimBitPacker flags(pred_pos.animationFlags);

	pred_pos.pos += pred_pos.vel * ((frametime - pred_pos.frametime) / 1000.0);
	CheckCollisions(pred_pos, (fpTPSPoseType)flags.pose);
	pred_pos.frametime = frametime;
	pred_pos.timestamp += (frametime - pred_pos.frametime);

	if ( frametime < interp_end ) {
		// TODO: A lot of this is still mostly from the old interpolator and could use some cleanup.
		double t = (double)(frametime - interp_beg) / (double)(interp_end - interp_beg);
		assert ( 0.0 <= t && t <= 1.0 );
		if ( t < 0.0 ) t = 0.0;
		if ( t > 1.0 ) t = 1.0;

		real_pos.pos = spline.InterpolatePoint(t);

		FLOAT64VECTOR vel = spline.InterpolateDt(t);
		mathScaleVector(&vel, &vel, 1000.0/interp_time);

		FLOAT64VECTOR dir = vel;
		mathNormalize(&dir);

		double speed = mathExactVectorLength2(&vel);
		if ( speed < 0.1 ) { // HACK: To avoid twitchiness
			vel = { 0.0, 0.0, 0.0 };
			speed = 0.0;
			dir = pred_pos.dir;
		}
		// HACK: Something isn't being calculated correctly.
		else {
			speed = pred_pos.speed;
		}

		real_pos.vel = vel;
		real_pos.speed = speed;
		// If the last message has us not going somewhat forward, respect that orientation.
		// TODO: We should use a proper constant to determine where "strafing" begins.
		if ( mathVectDotProduct(&pred_pos.dir, &pred_pos.vel) < .42 )
			real_pos.dir = pred_pos.dir;
		else
			real_pos.dir = dir;
	} else {
		CopyPositionData(real_pos, pred_pos);
	}

	// Collisions are best addressed through the predictions, but our height isn't
	// actually handled with any of that. The bit of lag isn't actually that big of
	// a deal except when going up slopes since we'll visibly clip into the floor
	// before the up-to-date height gets sent to us.
	CheckCollisions(real_pos, (fpTPSPoseType)flags.pose);
	// TODO: Gravity prediction.

	// Override motion flags to match what the player's seeing.
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

	// We should now match the predicted position, although it
	// may actually be slightly different due to smoothing.
	real_pos.frametime = pred_pos.frametime;
	real_pos.timestamp = pred_pos.timestamp;

	return real_pos;
}

//------------------------------------------------------------------------------

void InfantryPredictor::Copy(vehVehicle* v, UINT32 flags) {
	InfantryPredictor* from = dynamic_cast<InfantryPredictor*>(&v->interpolator);
	if ( ! from )
		return;

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

	// If the message was sent just as they started moving, the soldier will
	// slowly walk based on the slower speed used in the prediction, then
	// rubber-band to the proper position once the next message is received.
	//
	// To help with this, we give our prediction a little boost if the
	// difference when they suddenly accelerate.
	const double diff_threshhold = 1.25;
	if ( prev_msg.speed == 0.0 || curr_msg.speed / prev_msg.speed > diff_threshhold ) {
		double new_speed = curr_msg.speed * 1.15;
		// TODO: Cap so we don't go above sprint speed.
		pred_pos.vel = *mathNormalize(&pred_pos.vel) * new_speed;
		pred_pos.speed = new_speed;
	}

	const Update& from = real_pos;
	Update to = pred_pos;
	
	double pred_time_passed = ((interp_time - (to.frametime - from.frametime)) / 1000.0);
	to.pos += to.vel * pred_time_passed;

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

	// TODO: It's very unlikely, but we may end up on the other
	// side of a wall/floor after the interp_time has passed.
	gr2AnimBitPacker flags(to.animationFlags);
	CheckCollisions(to, (fpTPSPoseType)flags.pose);

	spline.SetControlPointsWithDt(from.pos, v1, to.pos, v2);

	interp_beg = from.frametime;
	interp_end = interp_beg + interp_time;
}

//==============================================================================

void CubicSpline::SetControlPoints(FLOAT64VECTOR const& v0, FLOAT64VECTOR const& v1,
                                   FLOAT64VECTOR const& v2, FLOAT64VECTOR const& v3) {
	_xc[3] = 1 * v3.x - 3 * v2.x + 3 * v1.x - 1 * v0.x;
	_xc[2] = 3 * v2.x - 6 * v1.x + 3 * v0.x           ;
	_xc[1] =                       3 * v1.x - 3 * v0.x;
	_xc[0] =                                  1 * v0.x;

	_yc[3] = 1 * v3.y - 3 * v2.y + 3 * v1.y - 1 * v0.y;
	_yc[2] =            3 * v2.y - 6 * v1.y + 3 * v0.y;
	_yc[1] =                       3 * v1.y - 3 * v0.y;
	_yc[0] =                                  1 * v0.y;

	_zc[3] = 1 * v3.z - 3 * v2.z + 3 * v1.z - 1 * v0.z;
	_zc[2] =            3 * v2.z - 6 * v1.z + 3 * v0.z;
	_zc[1] =                       3 * v1.z - 3 * v0.z;
	_zc[0] =                                  1 * v0.z;
}

void CubicSpline::SetControlPointsWithDt(FLOAT64VECTOR const& x0, FLOAT64VECTOR const& v0,
                                         FLOAT64VECTOR const& x1, FLOAT64VECTOR const& v1) {
	_xc[3] =  1 * v1.x - 2 * x1.x + 1 * v0.x + 2 * x0.x;
	_xc[2] = -1 * v1.x + 3 * x1.x - 2 * v0.x - 3 * x0.x;
	_xc[1] =                            v0.x           ;
	_xc[0] =                                       x0.x;

	_yc[3] =  1 * v1.y - 2 * x1.y + 1 * v0.y + 2 * x0.y;
	_yc[2] = -1 * v1.y + 3 * x1.y - 2 * v0.y - 3 * x0.y;
	_yc[1] =                            v0.y           ;
	_yc[0] =                                       x0.y;

	_zc[3] =  1 * v1.z - 2 * x1.z + 1 * v0.z + 2 * x0.z;
	_zc[2] = -1 * v1.z + 3 * x1.z - 2 * v0.z - 3 * x0.z;
	_zc[1] =                            v0.z           ;
	_zc[0] =                                       x0.z;
}

FLOAT64VECTOR CubicSpline::InterpolatePoint(double t) {
	FLOAT64VECTOR pos;

	pos.x = calcpos(_xc, t);
	pos.y = calcpos(_yc, t);
	pos.z = calcpos(_zc, t);

	return pos;
}

FLOAT64VECTOR CubicSpline::InterpolateDt(double t) {
	FLOAT64VECTOR dt;

	dt.x = calcdt(_xc, t);
	dt.y = calcdt(_yc, t);
	dt.z = calcdt(_zc, t);

	return dt;
}

double CubicSpline::calcpos(double c[4], double t) {
	return c[3]*t*t*t + c[2]*t*t + c[1]*t + c[0];
}

double CubicSpline::calcdt(double c[4], double t) {
	return 3*c[3]*t*t + 2*c[2]*t + c[1];
}
