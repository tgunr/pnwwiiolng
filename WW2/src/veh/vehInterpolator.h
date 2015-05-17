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
	virtual ~InterpolatorBase(){}

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

class InfantryInterpolator : public InterpolatorBase
{
public:

	InfantryInterpolator(vehVehicle* v=0);
	virtual ~InfantryInterpolator(){}

	void NewPosDataUpdate(int timestamp_, int frametime_) override;
	Update const& Interpolate(int frametime_) override;
	void Copy(vehVehicle* v_, UINT32 flags_) override;
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

// type used by vehicle member variables
typedef InfantryInterpolator vehInterpolator;
