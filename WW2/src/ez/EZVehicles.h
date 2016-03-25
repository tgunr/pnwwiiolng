#pragma once

#include "ez/EZManager.h"

// ----------- defines

// ----------- predeclares

// ----------- types
namespace phui
{
	class View;
}

/************************************************************************************************
	EZVehicle
*************************************************************************************************/

class EZGameVehicle : public EZManager
{
public:

	EZGameVehicle(vehVehicle* v);
	virtual ~EZGameVehicle(){}

	// virtual overrides
	virtual bool Init(const char* path) override;
	virtual void FrameUpdate() override;
	virtual void UpdateEngine();
	virtual void UpdateGears();

	// transmission control
	virtual bool SetForward();
	virtual bool SetReverse();
	virtual void ShiftUp();
	virtual void ShiftDown();
	virtual SINT32 ShiftTime();

	virtual void OnMove(SINT32 param, UINT32 id,UINT32 extra) override;
	virtual void OnTurn(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnEngine(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnAction1(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnAction2(SINT32 param, UINT32 id, UINT32 extra) override;

	FLOAT32 GetGaugeData(UINT32 gauge, UINT32 subGauge = 0);
	SINT32 GetGear();

protected:

	// vehicle data
	FLOAT32 _maxRPMs;
	FLOAT32 _currentRPMs;
	FLOAT32 _lastRPMs;

	FLOAT64 _speed;
	FLOAT64 _lastSpeed;

	SINT32 _currentGear;
	SINT32 _forwardGears;
	SINT32 _reverseGears;

	// shift timers
	SINT32 _lastShift;
	SINT32 _nextShift;
};

/************************************************************************************************
	EZPlane - duh
*************************************************************************************************/

class EZPlane : public EZGameVehicle
{
public:

	EZPlane(vehVehicle* v);
	virtual ~EZPlane(){}
	virtual void InstallInputCallbacks() override;

	//virtual void UpdateEngine();
	//virtual SINT32 ShiftTime();

	// handle vehicle type specific data
	//virtual void SetConfig(const char* name, const char* value);
};

/************************************************************************************************
	EZTruck - wheel based default
*************************************************************************************************/

class EZTruck : public EZGameVehicle
{
public:

	EZTruck(vehVehicle* v);
	virtual ~EZTruck(){}

	// our current gearshift setting
	// for engine handling
	bool HighGear();

	// frame update
	virtual void UpdateEngine() override;
	virtual void UpdateGears() override;
	virtual SINT32 ShiftTime() override;

	// handle vehicle type specific data
	virtual void SetConfig(const char* name, const char* value) override;
	virtual void OnTurn(SINT32 param, UINT32 id,UINT32 extra) override;

protected:

	////////// hi gear data
	SINT32 _hiGear; // first hi speed gear

	// optimal rpm range
	FLOAT32 _hiShiftDownRPMs;
	FLOAT32 _hiShiftUpRPMs;

	// controls how long to allow max rpms before shifting
	SINT32	_hiAccelTime;

	// controls how often gear shifts can occur
	SINT32 _hiShiftTime;

	// some vehicles have pulling gears that are very low
	// optimal rpm range
	FLOAT32 _loShiftDownRPMs;
	FLOAT32 _loShiftUpRPMs;

	// controls how long to allow max rpms before shifting
	SINT32	_loAccelTime;

	// controls how often gear shifts can occur
	SINT32 _loShiftTime;

};

/************************************************************************************************
	EZTank
*************************************************************************************************/

class EZTank : public EZTruck
{
public:

	EZTank(vehVehicle* v) : EZTruck(v) {}
	virtual ~EZTank(){}

	// input callbacks
	virtual void OnTurn(SINT32 param, UINT32 id,UINT32 extra) override;
};

class EZStug : public EZTank
{
public:

	EZStug(vehVehicle* v) : EZTank(v), _lastPosition(0), _quickPos(0) {}
	virtual ~EZStug(){}

	virtual void SetConfig(const char* name, const char* value) override;
	virtual void OnAction2(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnSpawn() override;

protected:

	SINT32 _lastPosition;
	SINT32 _quickPos;
};


/************************************************************************************************
	EZATGun
*************************************************************************************************/

class EZATGun : public EZGameVehicle
{
public:

	EZATGun(vehVehicle* v) : EZGameVehicle(v) {}
	virtual ~EZATGun(){}

	virtual void FrameUpdate() override;
	virtual bool IsDriverDead() override;
	virtual bool IsCommanderDead() override;
	virtual bool IsGunnerDead() override;
	virtual bool IsLoaderDead() override;

	// input callbacks
	virtual void OnMove(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnTurn(SINT32 param, UINT32 id, UINT32 extra) override;
	virtual void OnAction2(SINT32 param, UINT32 id, UINT32 extra) override;
};

/************************************************************************************************
	EZBoat
*************************************************************************************************/

class EZBoat : public EZGameVehicle
{
public:

	EZBoat(vehVehicle* v) : EZGameVehicle(v) {}
	virtual ~EZBoat(){}

	virtual void UpdateEngine() override;

};


/************************************************************************************************
	EZTrooper
*************************************************************************************************/

class EZTrooper : public EZManager
{
public:

	EZTrooper(vehVehicle* v) : EZManager(v),
		_dispersion(1.0f),
        _curReticle(0),
        _tgtReticle(0),
        _minReticle(0),
        _maxReticle(0),
        _returnRate(0),
        _fireBump(0),
        _showReticle(FALSE),
        _lastWeapon(-1)
	{
	}

	virtual ~EZTrooper();

	// virtual overrides
	virtual void SetConfig(const char* name, const char* value) override;
	virtual void FrameUpdate() override;
	virtual void InstallInputCallbacks() override;

	// override input callbacks
	virtual void OnMove(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnTurn(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnAction1(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnAction2(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnFreeLook(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnPosition(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnEngine(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnView(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual void OnCamera(SINT32 param, UINT32 id, UINT32 extra) override {}
	virtual bool OnCamera(piVECTOR* pos, piVECTOR* aim) override { return false; }
	virtual void OnDespawn()  override {}
	virtual void OnSpawn() override {}
	virtual bool IsDriverDead() override;
	virtual bool IsCommanderDead() override;
	virtual bool IsGunnerDead() override;
	virtual bool IsLoaderDead() override;

	FLOAT32 GetDispersionMod();

	// these adjust the reticle size
	void SetDispersion(FLOAT32 d);
	void AddDispersion(FLOAT32 d);

	// update our reticle size and position
	void UpdateReticle(phui::View* r);

private:

	typedef std::map<std::string, std::string> RETICLE_MAP;

	FLOAT32 _dispersion;
	FLOAT32 _curReticle; // current reticle size
	FLOAT32 _tgtReticle; // target reticle size
	FLOAT32 _minReticle; // min reticle size
	FLOAT32 _maxReticle; // max reticle size
	FLOAT32 _returnRate; // rate of return
	FLOAT32 _fireBump; // bump in reticle size for each bullet fired
	LOGICAL _showReticle;
	SINT32 _lastWeapon;
	RETICLE_MAP _reticles;
};