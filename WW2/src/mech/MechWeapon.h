#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechWeapon class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "ml/mltypes.h"
#include "veh/vehmain.h"
#include "weap/weapons.h"
#include "mech/MechModule.h"

class gr2Vehicle;
struct weapWeapon;
struct gr2ModuleSpec;

enum MECH_WEAPON_CONTROL_FLAG
{
	MW_CONTROL_FIRE				= 1<<0,
	MW_CONTROL_RELOAD			= 1<<1
};

enum MECH_WEAPON_ACTION_FLAG
{
	MW_ACTION_FIRE				= 1<<0,
	MW_ACTION_RECYCLING			= 1<<1,
	MW_ACTION_RELOAD			= 1<<2
};

enum MECH_WEAPON_CONDITION_FLAG
{
	MW_CONDITION_EMPTY			= 1<<0,
	MW_CONDITION_DAMAGED		= 1<<1,
	MW_CONDITION_COOLDOWN		= 1<<2,
};

class MechWeapon : public MechModule
{
public:

	//typedef std::map<std::string, GR2_WEAPON_CREATOR> CreatorMap;

	// ui properties
	enum
	{
		WEAPON_PROP,
		AMMOTYPE_PROP,
		MAXAMMO_PROP,
		AMMO_PROP,
		RELOADTIME_PROP,
		WEAPGROUP_PROP,
		TOTAL_PROPS
	};

	MechWeapon(gr2ModuleSpec* spec);
	virtual ~MechWeapon();

	virtual bool Init();
	virtual void Update(FLOAT64 delta);
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	void SetAmmo(int ammo);

	// gets the absolute position
	virtual bool GetAbsoluteWorldPosition(FLOAT64VECTOR* position);

	// recursively calls up the chain until
	// it reaches NULL or a vehicle
	weapWeapon* GetGameWeapon() const { return _gameWeapon; }

	bool GetFirePoint(piMATRIX* mat);
	bool GetEjectPoint(piMATRIX* mat);

	//virtual bool StartFiring();
	virtual bool Fire();
	//virtual bool StopFiring();
	virtual bool SetActive(bool a);

	// state handling
	virtual void HandleStateEvent(GR2_STATE_EVENT e, gr2State* s);

	virtual bool IsReady();
	virtual bool IsReloading();
	virtual bool IsRecycling();
	virtual bool HasAmmo();

	virtual void SetConvergence(piMATRIX* fireMat);

	// ui interface
	virtual void InitJSProps(jsGR2Object* ui, int index);
	virtual JSBool GetJSProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetJSProperty(JSContext* ctx, int propid, jsval* val);

	//static void RegisterCreator(const char* creatorName, GR2_WEAPON_CREATOR creator);
	//static MechWeapon* Create(std::string& creatorName, gr2ModuleSpec* spec);
	static gr2GameObject* Create(gr2ModuleSpec* spec);

protected:

	// cached bones
	int _fireBone;
	int _ejectBone;

	int _weaponAmmo;
	int _weaponAmmoType;

	int _fireDelay;
	int _startFireTime;
	int _lastFireCheckTime;

	// name of the firepoint to use
	const std::string* _firePoint;

	// ref to underlying weapon
	weapWeapon* _gameWeapon;
};

