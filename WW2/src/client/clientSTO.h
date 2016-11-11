#pragma once
// Created: Fri Mar 24 16:27:26 CST 2006
//
// Server tracked objects are described in three layers:
//  1. "ServerTrackedObject" - a stub class with enough
//     information to look up any STO
//  2. "STO_*" - transport classes which contain only
//     as much information as is common to both the
//     client and server
//  3. Implementations - these extend one of the STO_*
//     classes with client or host specific members
//     that are not shared. Typically these should
//     have STO at the end of the name, e.g. SoAndSoSTO
//
// This file is for ClientSTO implementation classes.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiServerTrackedObject.h"		// Stub class
#include "include/vars.h" // frametime
#include "veh/vehtype.h"
#include "math/Vec3f.h"

#define AVERAGE_STO_LAG 350

class OrdnanceSTO : public ServerTrackedObject
{
public:
	OrdnanceSTO (UINT16 clientAmmoIndex, SINT32 seed, const WorldPosition_m& position, const AttitudeVector& attitude, UINT32 msLifetime)
		: ServerTrackedObject(0, _ORDNANCE, clientAmmoIndex,position,attitude,(UINT32)seed,msLifetime)
		, launched((frametime - age() - AVERAGE_STO_LAG) / 1000.0f)
		{}

	OrdnanceSTO(ACTION_OBJECT* const a)
		: ServerTrackedObject(a)
		, launched((frametime - age() - AVERAGE_STO_LAG)/1000.0f)
		{}

	UINT16 clientAmmoIndex() const { return subType(); }
	SINT32 randomSeed() const { return (SINT32)flags(); }

public:
protected:
	virtual bool activate();
	virtual void deactivate();

private:
	FLOAT64 launched;
};

class VehicleSTO : public ServerTrackedObject
{
public:
	// Constructor for building a request instance
	VehicleSTO (UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, UINT16 type = _VEHICLE)
		: ServerTrackedObject(id, type, subType, position, attitude)
		, m_uniqueSTOvehicleID(0)
		, icon("UMS")
		, persDecal(0)
		, unitDecal(0)
		{}

	VehicleSTO (UINT32 id, const struct vehType* vtype, const WorldPosition_m& position, const AttitudeVector& attitude, UINT16 type = _VEHICLE)
		: ServerTrackedObject(id, type, 0, position, attitude)
		, m_uniqueSTOvehicleID(0)
		, icon("UMS")
		, persDecal(0)
		, unitDecal(0)
		{
			if(vtype)
				m_subType = vtype->WeaponSpec.asUINT16();
		}

	// Build an actual effect instance from a network stream
	VehicleSTO(ACTION_OBJECT* const a)
		: ServerTrackedObject(a)
		, m_uniqueSTOvehicleID(0)
		, icon("UMS")
		, persDecal(0)
		, unitDecal(0)
		{}

public:
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr);

	int getSTOVehicleID() const { return m_uniqueSTOvehicleID; }
	UINT32 getVehicleTypeNum();

	virtual void load();

	void setIcon(const char* i)
	{
		icon = i;
	}

	void setDecals(SINT32 p, SINT32 u)
	{
		persDecal = p;
		unitDecal = u;
	}

	struct vehVehicle* getVehicle();

protected:
	// Interface for activating and deactivating
	virtual bool activate();
	virtual void deactivate();

	SINT32 m_uniqueSTOvehicleID;
	std::string icon;
	SINT32 persDecal;
	SINT32 unitDecal;
};

class HulkSTO : public VehicleSTO
{
public:
	HulkSTO(UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude)
		: VehicleSTO(id, subType, position, attitude, _HULK)
		{}

	HulkSTO(UINT32 id, const struct vehType* vtype, const WorldPosition_m& position, const AttitudeVector& attitude)
		: VehicleSTO(id, vtype, position, attitude, _HULK)
		{}

	HulkSTO(ACTION_OBJECT* const a) : VehicleSTO(a) {}

protected:
	virtual bool activate();
	virtual void deactivate();
};

class TerrainSTO : public ServerTrackedObject
{
public:

	//! Constructor for building a request instance
	TerrainSTO(UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, UINT16 type = _TERRAIN)
		: ServerTrackedObject(id, type, subType, position, attitude), _modelFile(nullptr), _collisionGrace(0), _to(nullptr)
	{
	}

	// Constructor for building offline arbitrary terrain models
	TerrainSTO(const char* model, const WorldPosition_m& position, const AttitudeVector& attitude, UINT16 type = _TERRAIN)
		: ServerTrackedObject(0, type, 0, position, attitude), _modelFile(nullptr), _collisionGrace(20000), _to(nullptr)
	{
		_modelFile = new char[strlen(model)+1];
		strcpy(_modelFile, model);
	}


	//! Build an ums instance from a network stream
	TerrainSTO(ACTION_OBJECT* const a)
		: ServerTrackedObject(a), _modelFile(nullptr), _collisionGrace(0), _to(nullptr)
	{
	}

	virtual ~TerrainSTO()
	{
		if(_modelFile)
			delete [] _modelFile;

		_modelFile = nullptr;
	}

	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr);
	virtual void enter();
	virtual void leave();

	virtual bool load();						//!< load the terrain object
	virtual void preProcess(){}
	virtual void postProcess();
	virtual bool isCollidable();

	// toggle our damage state
	void ToggleDamage();

	// set our model's scale
	void SetScale(float sx, float sy, float sz);

	// set our visible country flag if we have one
	void SetCountry(UINT32 country);

	struct OBJECT* GetObject() {return _to;}

	static const char* ModelName(int type);

protected:
	virtual bool activate();					//!< Interface for activating.
	virtual void deactivate();					//!< Interface for deactivating.

	virtual const char* model() const
	{
		return (_modelFile) ? _modelFile: "";
	}

	Vec3f _scale;
	char* _modelFile;
	UINT32 _collisionGrace;
	struct OBJECT* _to;
};

class PlayerPlacedSTO : public TerrainSTO
{
public:
	//! Constructor for building a request instance
	PlayerPlacedSTO(UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, UINT16 type = _PPO, UINT32 country = 0)
		: TerrainSTO(id, subType, position, attitude, type), _lastUpdate(0)
		{
            m_flags = country;
        }

	//! Build an ums instance from a network stream
	PlayerPlacedSTO(ACTION_OBJECT* const a)
		: TerrainSTO(a), _lastUpdate(0)
		{}

	virtual ~PlayerPlacedSTO();
	virtual bool applyJoules(const SINT64& joules, class Player* player = nullptr);
	virtual bool load();

    virtual void preProcess();
    virtual void postProcess();

	void Sink();
	void Update();

protected:
	virtual const char* model() const;

private:
	SINT32 _lastUpdate;
	FLOAT64VECTOR _pos;
};

class MobileSpawnSTO : public PlayerPlacedSTO
{
public:
	//! Constructor for building a request instance
	MobileSpawnSTO(UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, UINT32 country = 0)
		: PlayerPlacedSTO(id, subType, position, attitude, _UMS, country)
	{
		//flagTextures[0] = flagTextures[1] = flagTextures[2] = flagTextures[3] = -1;
		_collisionGrace = 20000;
	}

	//! Build an ums instance from a network stream
	MobileSpawnSTO(ACTION_OBJECT* const a)
		: PlayerPlacedSTO(a)
	{
		//flagTextures[0] = flagTextures[1] = flagTextures[2] = flagTextures[3] = -1;
		_collisionGrace = 20000;
	}

	virtual void preProcess();
	virtual void postProcess();

protected:
	// which model to use
    virtual const char* model() const;

//private:
	//int flagTextures[4];
};

class BailoutSTO : public VehicleSTO
{
public:
	BailoutSTO(UINT32 id, UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude)
		: VehicleSTO(id, subType, position, attitude, _BAILOUT)
	{
	}

	BailoutSTO(UINT32 id, const struct vehType* vtype, const WorldPosition_m& position, const AttitudeVector& attitude)
		: VehicleSTO(id, vtype, position, attitude, _BAILOUT)
	{
	}

	BailoutSTO(ACTION_OBJECT* const a) : VehicleSTO(a) {}

	virtual ~BailoutSTO();
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr) { return false; }  // do nothing

	void setLifeSpan(UINT32 life){ m_lifespan = life; }
	bool isMyPilot() const;

	//void expire(){ expired = frametime; }

	static FLOAT32 SAFE_DISTANCE(){ return 180.0f; } //!< meters per second
	static FLOAT32 FALL_RATE(){ return 10.0f; } //!< meters per second
	static FLOAT32 MIN_ALT(){ return 1.0f; } //!< min allowed height above ground before expiring
	static const struct vehType* VTYPE(vehVehicle*); //!< which type of vehicle to use based on the original vehicle type

protected:
	virtual bool activate();
};
