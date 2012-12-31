#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechVehicle class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "gr2/gr2Vehicle.h"
#include "gr2/gr2Data.h"

class TargetView;
class MechWeapon;

typedef std::vector<MechWeapon*> MechWeaponList;

//  ----------------------------------------  Classes

class MechVehicle : public gr2Vehicle
{
public:

	typedef void (MechVehicle::*TARGET_DESTROYED_CALLBACK)();

	// ui properties
	enum
	{
		SPEED_PROP,
		THROTTLE_PROP,
		POWER_PROP,
		MAXPOWER_PROP,
		ACCURACY_PROP,
		GRACE_PROP,
		RANGE_PROP,
		CALLSIGN_PROP,
		SAMESIDE_PROP,
		LIFT_PROP,
		TOTAL_PROPS
	};

	struct MechCamera
	{
		bool freelook;
		bool sync;
		float traverse;
		float elevate;
		float wantedTraverse;
		float wantedElevate;
		float savedTraverse;	// set when freelook is enabled
		float savedElevate;		// set when freelook is enabled
		float traverseRate;
		float elevateRate;
		float maxLeft;
		float maxRight;
		float maxUp;
		float maxDown;
	};

	struct MechTarget
	{
		MechTarget() : vehicle(NULL), x(0), y(0), z(0), width(0), height(0), view(NULL)
		{
		}

		void Reset()
		{
			vehicle = NULL;
			view = NULL;
			x = y = z = width = height = 0;
		}

		gr2Vehicle* vehicle;
		TargetView* view; // phui target view

		// screen dimensions
		float x;
		float y;
		float z;
		float width;
		float height;
	};

	MechVehicle(vehVehicle* vehicle, OBJECT* pObject);
	virtual ~MechVehicle();

	// handle inputs; used by 1st person mechs
	virtual UINT32 ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve);

	// init vehicle resources if we find a spec
	virtual bool Init();

	// update this mech
	virtual void Update(FLOAT64 delta);

	// set we can draw the aim line
	virtual void Draw();

	// override so we can calculate the targeting box
	virtual void PreDrawCallback(gr2ModelRef model);

	// mech turret handling
	virtual void UpdateTurrets();

	// handle state changes
	virtual void HandleStateEvent(GR2_STATE_EVENT e, gr2State* s);

	// update our current eyepoint position and rotation
	virtual void UpdateEyeMatrix(FLOAT64 delta);

	// mech config stuff
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	// handle mech anim events
	virtual void HandleAnimEvent(gr2ModelRef model, SINT32 type, SINT32 event);

	// intercept attached weapons so we can do stuff
	virtual void AttachObject(gr2Object* object, const char* from, const char* to, bool owner);
	virtual void RemoveObject(gr2Object* object, bool freeOwned);

	// where is our cross hairs at?
	virtual const FLOAT64VECTOR* GetAimPosition() const;
	virtual FLOAT32 GetAimRange();
	virtual void HandleAimContact(dGeomID ray, dGeomID hit);
	virtual void CalculateAimPosition();

	virtual UINT32 GetSharedMask();

	virtual void SetFalling(bool yesno);
	virtual bool IsFalling();

	virtual void SetMaxTurn(float mt);
	virtual float GetMaxTurn(bool fromSpec = false) const;

	virtual void CalculateHUDTargetInfo();
	void ClearTargeter();
	void SetTargeter(MechVehicle* by, TARGET_DESTROYED_CALLBACK callback);
	void OnTargetDestroyed();
	void ClearTarget();

	UINT32 GetCollisionTimeLeft() const;
	virtual FLOAT32 GetAccuracy() const;
	const MechVehicle::MechTarget& GetMechTarget() const;
	void FindMechTarget(int side = 0);

	// power functions
	UINT32 GetPower();
	UINT32 GetMaxPower();
	bool ProcessPower(SINT32 amount, UINT32 minpower = 0, UINT32 maxpower = 0);
	bool HasPower(UINT32 amount);

	// ui interface
	virtual void InitJSProps(jsGR2Object* ui, int index);
	virtual JSBool GetJSProperty(JSContext* ctx, int propid, jsval* val);

	static gr2Vehicle* CreateMech(vehVehicle* vehicle, OBJECT* pObject);

private:

	// mech's animation walking speed
	float _animSpeed;

	// our current power
	UINT32 _power;
	UINT32 _maxPower;

	// boost in max speed
	float _turbo;

	// camera position controls
	float _eyeLookAt; // how far out in front to look
	float _eyeAdjust; // how much to adjust each "click"
	float _eyeVOffset; // max vertical offset
	float _eyeVPad; // amount of vertical padding
	float _eyeHOffset; // max horizontal offset
	float _eyeHPad; // amount of horizontal padding
	float _eyeOffsetWanted;
	float _eyeOffset; // current eye offset (0 to 1)
	float _aimOffset; // how far out in front will our crosshairs be?
	int _eCamTurret; // which turret to set the camera elevation from
	int _tCamTurret; // which turret to set the camera traverse from

	float _absMaxTurn; // absolute max turn;

	float _camTraverse;
	float _camElevate;

	FLOAT64VECTOR _aimPosition;
	piVECTOR _aimDirection;
	bool _aimChanged;
	FLOAT32 _aimDistance;
	FLOAT32 _hitDistance;

	MechCamera _camera;

	float _traverseAccuracy; // how accurate in traverse are we
	float _elevateAccuracy;  // how accurate in elevattion are we

	// targeting stuff
	MechVehicle* _targeter;
	TARGET_DESTROYED_CALLBACK _targeterCallback;
	MechTarget _target;

	// our current list of weapons
	MechWeaponList _weapons;
};

