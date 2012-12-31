#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2Weapon class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "ml/mltypes.h"
#include "veh/vehmain.h"
#include "weap/weapons.h"
#include "gr2/gr2GameObject.h"

class gr2Weapon;
class gr2Vehicle;
struct weapWeapon;
struct gr2ModuleSpec;

typedef gr2Weapon* (*GR2_WEAPON_CREATOR)(gr2ModuleSpec*);

enum GR2_WEAPON_CONTROL_FLAG
{
	GW_CONTROL_FIRE				= 1<<0,
	GW_CONTROL_RELOAD			= 1<<1
};

enum GR2_WEAPON_LOGIC_FLAG
{
	GW_LOGIC_FIRE				= 1<<0,
	GW_LOGIC_RECYCLING			= 1<<1,
	GW_LOGIC_RELOAD				= 1<<2
};

enum GR2_WEAPON_CONDITION_FLAG
{
	GW_CONDITION_EMPTY			= 1<<0,
	GW_CONDITION_DAMAGED		= 1<<1,
	GW_CONDITION_COOLDOWN		= 1<<2,
};

class gr2Weapon : public gr2GameObject
{
public:

	typedef std::map<std::string, GR2_WEAPON_CREATOR> CreatorMap;

	gr2Weapon(gr2ModuleSpec* spec = NULL);
	gr2Weapon(const char* skeletonFile, bool skeletonIsLOD = true);
	gr2Weapon(gr2ModelRef skeleton, bool skeletonIsLOD = true);
	virtual ~gr2Weapon();

	virtual bool Init();
	virtual void Update(FLOAT64 delta);

	void SetAttachBone(const char* bone);
	void SetAttachBone(int bone);
	void SetMount(int mount, int index);
	void SetAmmo(int ammo);

	// toggles this weapon's active flag
	virtual void Toggle();

	// is this weapon active?
	virtual bool IsActive() const;

	// gets the absolute position
	virtual bool GetAbsoluteWorldPosition(FLOAT64VECTOR* position);

	// recursively calls up the chain until
	// it reaches NULL or a vehicle
	gr2Vehicle* GetParentVehicle() const;
	weapWeapon* GetGameWeapon() const { return _gameWeapon; }

	bool GetFirePoint(piMATRIX* mat);
	bool GetEjectPoint(piMATRIX* mat);

	int GetWeaponMount() const;

	virtual bool StartFiring();
	virtual bool Fire();
	virtual bool StopFiring();

	// state handling
	virtual void HandleStateEvent(GR2_STATE_EVENT e, gr2State* s);

	virtual bool IsReady();
	virtual bool IsReloading();
	virtual bool IsRecycling();
	virtual bool HasAmmo();

	virtual void SetConvergence(piMATRIX* fireMat);

	static void RegisterCreator(const char* creatorName, GR2_WEAPON_CREATOR creator);
	static gr2Weapon* Create(std::string& creatorName, gr2ModuleSpec* spec);

protected:

	// is this weapon active?
	bool _active;

	gr2ModuleSpec* _spec;

	// cached bones
	int _fireBone;
	int _ejectBone;
	int _attachBone; // parent attach bone
	int _connectBone; // this object's connect bone

	int _weaponAmmo;
	int _weaponAmmoType;

	// which vehicle weapon are we mounted to?
	int _weaponMount;
	int _weaponIndex;

	int _fireDelay;
	int _startFireTime;
	int _lastFireCheckTime;
	int _coolDown;

	// ref to underlying weapon
	weapWeapon* _gameWeapon;
};

