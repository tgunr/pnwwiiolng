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
#include "mech/MechWeapon.h"

typedef std::vector<int> BoneList;

class MechRocketPack : public MechWeapon
{
public:

	struct RocketTube
	{
		std::string name;
		int firebone;
		bool loaded;
	};

	typedef std::vector<RocketTube> RocketTubeList;

	// ui properties
	enum
	{
		SALVO_PROP,
		CURRENTTUBE_PROP,
		SHORTRANGE_PROP,
		FIRESTAGE_PROP,
		NUMLOADED_PROP,
		TOTAL_PROPS
	};

	enum FIRE_STAGE
	{
		FIRE_STAGE_READY,
		FIRE_STAGE_INIT,
		FIRE_STAGE_SETUP,
		FIRE_STAGE_FIRE,
		FIRE_STAGE_RELOAD
		// back to ready after this
	};

	MechRocketPack(gr2ModuleSpec* spec = NULL);
	virtual ~MechRocketPack();

	// collect config members
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	// init vehicle resources if we find a spec
	virtual bool Init();

	// update this mech module
	virtual void Update(FLOAT64 delta);

	//virtual bool StartFiring();
	virtual bool Fire();
	//virtual bool StopFiring();

	// state handling
	virtual void HandleStateEvent(GR2_STATE_EVENT e, gr2State* s);

	// tests whether the _currentTube member is valid or not
	virtual bool IsValidTube() const;

	// set it back to ready
	virtual void ResetFireStage();

	// set the next fire stage
	virtual void SetFireStage(FIRE_STAGE s, int duration = 0);

	// is the current stage complete
	virtual bool IsStageComplete() const;

	// rocket the pack
	virtual void DoSetupStage(FLOAT64 delta);

	// fire the salvo
	virtual void DoFireStage(FLOAT64 delta);

	// rotate and reload the pack
	virtual void DoReloadStage(FLOAT64 delta);

	// rotate the pack to the _wantedAngle, if needed
	virtual void RotatePack(FLOAT64 delta);

	// helper call to reload all tubes; only reloads if we have ammo
	void ReloadAllTubes();

	// ui interface
	virtual void InitJSProps(jsGR2Object* ui, int index);
	virtual JSBool GetJSProperty(JSContext* ctx, int propid, jsval* val);

	static gr2GameObject* Create(gr2ModuleSpec* spec);

private:

	// what stage we are in
	FIRE_STAGE _fireStage;

	// how many rockets in the salvo
	int _salvo;

	// how long before the firing sequence starts
	int _setupDelay;

	// the current stage timer
	int _stageTime;

	// is the curent stage complete
	bool _stageComplete;

	// what angle the pack is currently angled at
	float _fireAngle;

	// what angle we want the pack to be rotated at
	float _wantedAngle;

	// range's below this use the short range ammo
	float _shortRange;

	// are we firing short range?
	bool _srFire;

	// short range ammo mount
	int _srMountX;

	// the current tube to fire
	int _currentTube;

	// which turret am I attached to?
	int _turretIndex;

	// when to reload the next tube
	int _tubeReloadTime;

	// how long between tube reloads
	int _tubeReloadPeriod;

	// our rocket tube list
	RocketTubeList _tubes;

};

