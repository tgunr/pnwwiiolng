#pragma once

struct vehVehicle;

/************************************************************************************************
	vehInterpolator
*************************************************************************************************/

class CubicSpline
{
public:
	void SetControlPoints(FLOAT64VECTOR const& r0, FLOAT64VECTOR const& r1, FLOAT64VECTOR const& r2, FLOAT64VECTOR const& r3);
	void SetControlPointsWithDt(FLOAT64VECTOR const& r0, FLOAT64VECTOR const& v0, FLOAT64VECTOR const& r3, FLOAT64VECTOR const& v3);

	FLOAT64VECTOR InterpolatePoint(double t);
	FLOAT64VECTOR InterpolateDt(double t);

protected:
	double calcpos(double c[4], double t);
	double calcdt(double c[4], double t);
	double _xc[4], _yc[4], _zc[4];
};

class InfantryPredictor {
public:
	struct Update {
		SINT32 timestamp;
		SINT32 frametime;
		FLOAT64VECTOR pos;
		FLOAT64VECTOR vel;
		FLOAT64VECTOR dir;
		unsigned int animationFlags;
		FLOAT64 speed;

		Update(void) { memset(this, 0x0, sizeof(Update)); pos.z = -10000; }
	};

	typedef int Timestamp; // The time on the other client.
	typedef int FrameTime; // The time on this end.

	InfantryPredictor(vehVehicle* v = nullptr);

	void NewPosDataUpdate(Timestamp timestamp, FrameTime frametime);
	const Update& Interpolate(FrameTime frametime);
	void SetFlags(UINT32 flags_) { pred_pos.animationFlags = flags_; }

	void Copy(vehVehicle* v, UINT32 flags);

	FLOAT64 LastUpdateYaw(void) const { return _lastUpdateYaw; }
private:
	void UpdateTimestamps(Timestamp latest);
	void NewMessageReceived(Timestamp timestamp, FrameTime frametime);
	void UpdateInterpolation(void);

	vehVehicle* _vehicle;
	FLOAT64 _lastUpdateYaw = 0.0;
	// We're waiting for a message that corresponds to at least this frame on our end.
	FrameTime waiting_for = -1;
	bool dirty = false;

	bool is_dead = false;

	FrameTime in_air_for = 0;

	// Updates based on the last info received
	Update prev_msg, curr_msg;
	// The predicted and real positions. Real may lag behind slightly to keep motion smooth.
	Update pred_pos;

	// Used to smooth the transition when updates are received.
	FrameTime interp_beg, interp_end;
	CubicSpline spline;
};

typedef InfantryPredictor vehInterpolator;
