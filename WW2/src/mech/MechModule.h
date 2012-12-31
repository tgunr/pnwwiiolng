#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechModule class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "gr2/gr2GameObject.h"
#include "gr2/gr2DamageModel.h"

// pre declares

class MechVehicle;
struct gr2ModuleSpec;

class MechModule : public gr2GameObject, public gr2DamageListener
{
public:

	// ui properties
	enum
	{
		NAME_PROP,
		UI_PROP,
		ENABLED_PROP,
		POWERED_PROP,
		ACTIVE_PROP,
		USEPOWER_PROP,
		IDLEPOWER_PROP,
		MINPOWER_PROP,
		MOUNTX_PROP,
		MOUNTY_PROP,
		USETYPE_PROP,
		USETIME_PROP,
		RECOVERYTIME_PROP,
		MODIFIED_PROP,
		USEPERIOD_PROP,
		RECOVERYPERIOD_PROP,
		USES_PROP,
		CYCLETIME_PROP,
		EFFICIENCY_PROP,
		MAXPOWER_PROP,
		TOTAL_PROPS
	};

	enum USE_TYPE
	{
		UT_TOGGLE,
		UT_CONTINUOUS,
		UT_INSTANT
	};

	enum EFF_EFFECT
	{
		EE_IDLEPOWER	= 1<<0,
		EE_USEPOWER		= 1<<1,
		EE_USETIME		= 1<<2,
		EE_RECOVERYTIME = 1<<3
	};

	MechModule(gr2ModuleSpec* spec = NULL);
	virtual ~MechModule();

	virtual bool Init();
	virtual int Use(SINT32 on, UINT32 data = 0);
	virtual int Enable(SINT32 on, UINT32 data = 0);
	virtual bool EnableOnUse() const;
	virtual UINT32 ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve);
	virtual void Update(FLOAT64 delta);
	virtual void UpdatePower(FLOAT64 delta);
	virtual void AddConfigParam(const std::string& name, const std::string& value);

	void SetAttachBone(const char* bone);
	void SetAttachBone(int bone);
	void SetMount(int mx, int my = 0);

	int GetMountX() const;
	int GetMountY() const;
	float GetCycleTime() const;
	float GetUseTime() const;
	float GetRecoveryTime() const;

	// toggles this weapon's active flag
	virtual bool Toggle();

	// is this module enabled?
	virtual bool IsEnabled() const;

	virtual bool IsBlocked() const;
	virtual bool SetBlocked(bool b);

	// is this module currently active?
	virtual bool IsActive() const;

	// is this module powered?
	virtual bool IsPowered() const;

	// are we this use type?
	virtual bool IsUseType(USE_TYPE type) const;

	// use/disuse this module; this returns true if there was a change
	virtual bool SetActive(bool a);

	// enable/disable this module; this returns true if there was a change
	virtual bool SetEnabled(bool e);

	// module efficiency management
	virtual void SetEfficiency(float e);
	virtual float GetEfficiency() const;
	virtual float CalculateEfficiency(float value) const;

	// get our parent mech
	MechVehicle* GetParentVehicle();

	// ui interface
	virtual void InitJSProps(jsGR2Object* ui, int index);
	virtual JSBool GetJSProperty(JSContext* ctx, int propid, jsval* val);

	// do something if the component we're tied to has been damaged
	virtual void HandleDamage(gr2DamageComponent* component);

	// create a new module
	static gr2GameObject* Create(gr2ModuleSpec* spec);

protected:

	gr2ModuleSpec* _spec;

	// our parent mech, duh
	MechVehicle* _parentMech;

	// is this module on or off; users can turn modules on and off
	bool _enabled;

	// whether another module is blocking this module's operation
	bool _blocked;

	// are we active; modules are active when they are used
	bool _active;

	// do we have anough power to be active and/or enabled
	bool _powered;

	// how much power we use or give each second
	// -1 or less = we take power, 0 = neutral power, 1+ = we add power
	int _idlePower;

	// negative = infinite, 1+ = limited, 0 = exhausted
	int _uses;

	// how much power we use or give each use
	// -1 or less = we take power, 0 = neutral power, 1+ = we add power
	int _usePower;

	// if _usePeriod is 1+, then this will be used to track how much power has been used
	int _usedPower;

	// how long to limit a module's use in any one user
	int _usePeriod;

	// when was the last time we activated this module
	int _useTime;

	// how long before this module can be used again
	int _recoveryPeriod;

	// if _rechargePeriod is 1+, this is used to track how much has been
	int _recoveryTime;

	// how long module takes to run one cycle
	int _cyclePeriod;
	int _cycleTime;

	// min and max power required to use the module
	// must be greater than _minPower and smaller than _maxPower
	UINT32 _minPower;
	UINT32 _maxPower;

	// input keys
	int _useInput;
	int _enableInput;

	// if this module shares when its active over the network; it needs to
	// specify a mount and index to use
	int _mountX;
	int _mountY;

	// if we have a visual component; we need
	// to specify how we connect to our parent
	int _attachBone; // parent attach bone
	int _connectBone; // this object's connect bone

	// how this module behaves when used
	USE_TYPE _useType;

	int _modified;

	// how efficient is the module
	// as modules are damaged, they
	// become less efficient
	float _efficiency; // 0 to 1

	// what level do we start decreasing efficiency
	// defaults to right away
	UINT32 _effMinDamageLevel; // 1 to max level - 1

	// how much efficiency to drop per damage level
	// defaults to 1 / num levels
	float _effDropPerLevel; // 0 to 1

	// minimum amount of efficiency allowed
	// defaults to 0 or completely inefficient
	float _effMinimum; // 0 to 1

	// what efficiency effects this
	// module has
	UINT32 _effFlags;

	// what modules to block while we're enabled
	std::vector<std::string> _blockModules;
};

