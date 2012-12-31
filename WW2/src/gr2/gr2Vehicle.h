#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2Vehicle class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "ml/mltypes.h"
#include "veh/vehmain.h"
#include "physicsMain.h"
#include "gr2/gr2GameObject.h"
#include "gr2/gr2Data.h"

class gr2Vehicle;
struct gr2TurretSpec;
struct VEHICLE_UPDATE_HEADER;

typedef gr2Vehicle* (*GR2_VEHICLE_CREATOR)(vehVehicle* v, OBJECT* o);

class gr2Vehicle : public gr2GameObject
{
public:

	typedef std::map<std::string, GR2_VEHICLE_CREATOR> CreatorMap;

	enum GR2_VEHICLE_TURRET_AXIS
	{
		GVTA_VERTICAL_AXIS,
		GVTA_HORIZONTAL_AXIS
	};

	struct gr2Turret
	{
		int bone; // bone index
		bool disabled;
		float efficiency;
		float traverse; // current traverse
		float elevate; // current elevation
		float wantedTraverse; // duh
		float wantedElevate; // duh
		float manualTraverse; // if this turret isn't synced, we can manually set a traverse angle
		float manualElevate; // if this turret isn't synced, we can manually set a elevate angle
		gr2TurretSpec* spec; // holds traverse and elevation specs
	};

	typedef std::vector<gr2Turret> gr2TurretList;

	gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject);
	gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject, const char* skeletonFile, bool skeletonIsLOD = true);
	gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject, gr2ModelRef skeleton, bool skeletonIsLOD = true);
	virtual ~gr2Vehicle();

	// network animation flags
	virtual void PackFlags(VEHICLE_UPDATE_HEADER* header, SHARED_VEHICLE_INFO* info);
	virtual void UnpackFlags(const VEHICLE_UPDATE_HEADER* header);

	// for colision detection; takes vehicle orientation into account
	virtual bool CollideWithRay(piVECTOR* start,
								piVECTOR* end,
								SINT32* location,
								piVECTOR* impact,
								piMATRIX* transform = NULL, // transform to apply before checking
								bool recursive = true, // check children?
								bool update = false);

	virtual void CollideWithGeom(dGeomID hit);
	virtual void CollideWithGround(dGeomID ray, dGeomID hit);

	virtual dGeomID GetCollider(){ return _collider; }

	// init vehicle resources if we find a spec
	virtual bool Init();

	// we need to draw the vehicle
	virtual void Draw();

	// override so we can pack up animation flags
	virtual void Update(FLOAT64 delta);

	// gets the absolute position
	virtual bool GetAbsoluteWorldPosition(FLOAT64VECTOR* position);

	// get the vehicle's attitude matrix
	virtual bool GetAttitudeMatrix(piMATRIX* mat);

	// add a vehicle parameter; mostly used by subclasses
	virtual void AddConfigParam(const std::string& name, const std::string& value){}

	virtual bool IsFirstPerson() const;
	virtual bool IsThirdPerson() const;

	// eye point calculation
	virtual bool GetEyeMatrix(piMATRIX* pMatrix);
	virtual void SetEyePosition(piVECTOR* pVector);
	virtual void UpdateEyeMatrix(FLOAT64 delta);
	virtual void LookAt(piVECTOR* pVector);
	virtual const FLOAT64VECTOR* GetAimPosition() const { return NULL; }
	virtual FLOAT32 GetAimRange() { return 0; }

	virtual bool GetWorldToScreen(const FLOAT64VECTOR* position, float* sx, float* sy, float* sz = NULL);
	virtual bool GetWorldToScreen(const piVECTOR* position, float* sx, float* sy, float* sz = NULL);

	// movement stuff
	virtual void CalculateMoves(FLOAT64 delta);
	virtual void GetVelocity(FLOAT64VECTOR* velocity);
	virtual void UpdateMoves(FLOAT64 delta, FLOAT64VECTOR* currentVel);

	// do vehicle specific skeletal updates
	virtual void UpdateModel(FLOAT64 delta);
	virtual void UpdateCallback(gr2ModelRef model);

	virtual int PlaySound(const char* sound,
							FLOAT32 vol = 1.0f,
							FLOAT32 fMin = 1.0f,
							FLOAT32 fMax = 2.0f,
							bool infinite = true,
							int priority = IDE_FP_PRIORITY_HIGH,
							int soundType = TYPE_FINITE_SOUND,
							int playType = DEF_PLAYTYPE_HEAD,
							OBJECT* emitter = NULL);

	// update turret positions
	virtual void UpdateTurrets();

	// is this my vehicle?
	bool IsPersonaVehicle() const;

	// handle falling
	virtual void SetFalling(bool yesno){}
	virtual bool IsFalling(){ return false; }

	virtual void SetMaxSpeed(float ms);
	virtual void SetMaxTurn(float mt);
	virtual void SetMaxAccel(float ma);

	virtual float GetMaxSpeed(bool fromSpec = false) const;
	virtual float GetMaxTurn(bool fromSpec = false) const;
	virtual float GetMaxAccel(bool fromSpec = false) const;

	// get HUD data, duh!
	bool GetHUDData(int type, SINT32* data) const;
	virtual FLOAT32 GetAccuracy() const { return 1.0f; }
	virtual FLOAT32 GetRangeToPlayer() const;
	virtual FLOAT32 GetRangeToPosition(FLOAT64VECTOR& position) const;

	// gets this gr2Object's game vehicle
	vehVehicle* GetGameVehicle() const {  return _vehicle; }
	mlVehicleData* GetGameVehicleData() const {  return (_vehicle) ? _vehicle->ModelData: NULL; }
	const char* GetGameVehicleShape() const { return (_vehicle) ? (const char*)_vehicle->VehicleType->Shape: ""; }
	void UpdateMount(UINT32 mount, UINT32 state);
	bool GetMountState(UINT32 mount);
	SINT32 GetLastNetUpdate() const;
	gr2Vehicle::gr2Turret* GetTurret(UINT32 index);

	static void RegisterCreator(const char* creatorName, GR2_VEHICLE_CREATOR creator);
	static gr2Vehicle* Create(std::string& creatorName, vehVehicle* vehicle, OBJECT* pObject);

protected:

	UINT32 _lastFlags;

	// vehicle's characteristics
	gr2VehicleSpec* _spec;

	// game vehicle
	vehVehicle* _vehicle;

	float _throttle;

	float _wantedSpeed;
	float _currentSpeed;
	float _currentLift;

	float _wantedTurn;
	float _currentTurn;

	float _cruise; // best cruising speed (ideal animation speed)
	float _maxSpeed;
	float _maxAccel;
	float _maxTurn;

	float _aglAltitude; // above ground level altitude
	FLOAT64VECTOR _aglPosition; // position on the ground we're above

	// vehicle skeleton transform
	piMATRIX _rotation;
	piVECTOR _position;

	// vehicle relative eye matrix
	piMATRIX _eyeMatrix;

	// vehicle relative lookat position
	piVECTOR _lookAt;

	// turret to bone map
	gr2TurretList _turrets;

	// collision detection
	bool _collided;
	dVector3 _lastFreePos;
	dGeomID _collider;
	dSpaceID _collisionSpace;
	FLOAT64VECTOR _lastCollision;

	SINT32 _spawnTime;
	SINT32 _lastNetUpdate; // last time we sent a network update
};

