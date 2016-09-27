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

class InterpolatorBase
{
public:
	struct Update
	{
		SINT32 timestamp;
		SINT32 frametime;
		FLOAT64VECTOR pos;
		FLOAT64VECTOR vel;
		FLOAT64VECTOR dir;
		unsigned int animationFlags;
		FLOAT64 speed;

		Update() { memset(this, 0x0, sizeof(Update)); pos.z = -10000; }
	};

	InterpolatorBase(vehVehicle* v=0);

	virtual void NewPosDataUpdate(int timestamp_, int frametime_) = 0;
	virtual Update const& Interpolate(int frametime_) = 0;
	virtual void Copy(vehVehicle* v_, UINT32 flags_) = 0;
	virtual FLOAT64 LastUpdateYaw() const { return _lastUpdateYaw; }
	virtual void SetFlags(UINT32 flags_){}

protected:
	vehVehicle* _vehicle;
	int _interpolatetime;
	FLOAT64 _lastUpdateYaw;
};

class InfantryPredictor : public InterpolatorBase {
public:
	typedef int Timestamp; // The time on the other client.
	typedef int FrameTime; // The time on this end.

	InfantryPredictor(vehVehicle* v = nullptr);

	void NewPosDataUpdate(Timestamp timestamp, FrameTime frametime) override;
	const Update& Interpolate(FrameTime frametime) override;
	void SetFlags(UINT32 flags_) override { real_pos.animationFlags = flags_; }

	void Copy(vehVehicle* v, UINT32 flags) override;
	void CopyImpl(vehVehicle* v, InfantryPredictor* from, UINT32 flags);
private:
	void UpdateInterpolation(void);

	// Updates based on the last info received
	Update prev_msg, curr_msg;
	// The predicted and real positions.
	// Real may lag behind slightly to keep motion smooth.
	Update pred_pos, real_pos;

	// Used to smooth the transition when updates are received.
	FrameTime interp_beg, interp_end;
	CubicSpline spline;
};

class InfantryInterpolator : public InterpolatorBase
{
public:

	InfantryInterpolator(vehVehicle* v=0);

	void NewPosDataUpdate(int timestamp_, int frametime_) override;
	Update const& Interpolate(int frametime_) override;
	void Copy(vehVehicle* v_, UINT32 flags_) override;
	void CopyImpl(vehVehicle* v, InfantryInterpolator* from, UINT32 flags);
	void SetFlags(UINT32 flags_) override { _cur.animationFlags = flags_; }

	bool Rollover() const { return _rollover; }

	const Update& GetFrom(){ return _from; }
	const Update& GetCur(){ return _cur; }
	const Update& GetTo(){ return _to; }
	const Update& GetNext(){ return _next; }

protected:
	void UpdateSpline(FLOAT64VECTOR const& frompos, FLOAT64VECTOR const& topos
					, FLOAT64VECTOR const& fromvel, FLOAT64VECTOR const& tovel
					, FLOAT64VECTOR const& fromdir, FLOAT64VECTOR const& todir, double dt);

	Update _from, _cur, _to, _next;
	CubicSpline _spline;
	bool _dead;
	bool _rollover;
};

class PassThruInterpolator : public InterpolatorBase
{
public:

	PassThruInterpolator(vehVehicle* v=0) : InterpolatorBase(v),
		_dead(false)
	{
	}
	virtual ~PassThruInterpolator(){}

	void NewPosDataUpdate(int timestamp_, int frametime_) override;
	Update const& Interpolate(int frametime_) override;
	void Copy(vehVehicle* v_, UINT32 flags_) override;

protected:

	Update _cur, _last;
	bool _dead;
};

//==============

class DualInterpolator : public InterpolatorBase {
public:
	DualInterpolator(vehVehicle* v = nullptr) : InterpolatorBase(v), _new(v), _old(v) {}

	void NewPosDataUpdate(int timestamp_, int frametime_) override {
		_new.NewPosDataUpdate(timestamp_, frametime_);
		_old.NewPosDataUpdate(timestamp_, frametime_);
	}
	Update const& Interpolate(int f) override {
		const Update& unew = _new.Interpolate(f);
		const Update& uold = _old.Interpolate(f);

		if ( use_new ) return unew;
		else return uold;
	}
	void Copy(vehVehicle* v, UINT32 flags) override ;

	static bool use_new;
private:
	InfantryPredictor _new;
	InfantryInterpolator _old;
};

//==============

// type used by vehicle member variables
typedef DualInterpolator vehInterpolator;
