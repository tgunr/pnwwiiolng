#include "syscfg.h" // Path is project specific.

#include "ez/EZVehicles.h"
#include "imaplib/imap.h"
#include "imap/ww2func.h"
#include "pers/persmain.h"
#include "math/math3d.h"
#include "math/Matrixd.h"
#include "math/Vec3d.h"
#include "math/matheul.h"
#include "fp/fpFPS.h"
#include "ml/mlutility.h"
#include "trn/trnvars.h"
#include "pers/pershud.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "uimap/uiMap.h"
#include "ui/uiMain.h"
#include "ml/mlEngine.h"
#include "fp/fpFPSdata.h"
#include "phui/CSSViewDrawer.h"
#include "cfg/cfgPrefs.h"

EZGameVehicle::EZGameVehicle(vehVehicle* v) :
	EZManager(v),
	_maxRPMs(0),
	_currentRPMs(0),
	_lastRPMs(0),
	_speed(0),
	_lastSpeed(0),
	_currentGear(0),
	_forwardGears(0),
	_reverseGears(0),
	_lastShift(0),
	_nextShift(0)
{
}

bool EZGameVehicle::Init(const char* path)
{
	if(EZManager::Init(path))
	{
		// look up vehicle specific data
		mlEngineComponentData* engineComponent = mlGetEngine(_vehicle->ModelData);

		if(engineComponent)
		{
			_forwardGears = engineComponent->NumForwardGears;
			_reverseGears = engineComponent->NumReverseGears;

			mlEngineData* engineData = engineComponent->EngineData;
			if(engineData)
				_maxRPMs = (FLOAT32)engineData->WepRpm;
		}

		return true;
	}

	return false;
}

void EZGameVehicle::FrameUpdate()
{
	// do normal update
	EZManager::FrameUpdate();

	// update some vehicle data
	_currentRPMs = GetGaugeData(ML_DOF_GAUGE_RPM);

	// current speed
	Vec3d velocity = Vec3d(_vehicle->ModelData->VehicleVel);
	_speed = velocity.length();

	// save our current gear
	_currentGear = GetGear();

	bool deadDriver = IsDriverDead();

	// if our driver died, clear these
	if(deadDriver)
	{
		_move = EZ_MOVE_IDLE;
		_turn = EZ_TURN_NONE;
	}

	if(HasFlag(EZManager::START_ENGINES))
	{
		if(deadDriver)
		{
			SetFlag(EZManager::START_ENGINES, false);
		}
		else if(mlGetEnginesOn(_vehicle->ModelData))
		{
			SetFlag(EZManager::ENGINES_ON, true);
			SetFlag(EZManager::START_ENGINES, false);
		}
		else if(!_vehicle->ModelData->MasterStarterOn)
		{
			mlStarterIsDepressed(1, 0, 0);
		}
	}
	else if(!deadDriver && (_move || _turn))
	{
		if(HasFlag(EZManager::ENGINES_ON))
		{
			UpdateEngine();
		}
		else
		{
			SetFlag(EZManager::START_ENGINES, true);
		}
	}
}

void EZGameVehicle::UpdateEngine()
{
	_lastRPMs = _currentRPMs;
	_lastSpeed = _speed;
}

void EZGameVehicle::UpdateGears()
{
}

void EZGameVehicle::OnMove(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	if(param > 0)
	{
		_move = EZ_MOVE_FORWARD;
	}
	else if(param < 0)
	{
		_move = EZ_MOVE_BACKWARD;
	}
	else
	{
		_move = EZ_MOVE_IDLE;
	}

	/*
	static SINT32 m = 0;

	if(m != _move)
	{
		uiChatPrintLine("Move is %d", _move);
		m = _move;
	}
	*/
}

void EZGameVehicle::OnTurn(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	if(param < 0)
	{
		_turn = EZ_TURN_LEFT;
		mlSteeringFunction(-1000, 0, 0);
	}
	else if(param > 0)
	{
		_turn = EZ_TURN_RIGHT;
		mlSteeringFunction(1000, 0, 0);
	}
	else
	{
		_turn = EZ_TURN_NONE;
		mlSteeringFunction(0, 0, 0);
	}

	/*
	static SINT32 t = 0;

	if(false && t != _turn)
	{
		uiChatPrintLine("Turn is %d", _turn);
		t = _turn;
	}
	*/
}

void EZGameVehicle::OnEngine(SINT32 param, UINT32 id, UINT32 extra)
{
	// no engine management if driver is dead
	if(!param || IsDriverDead()) return;

	if(HasFlag(EZManager::START_ENGINES|EZManager::ENGINES_ON))
	{
		if(mlGetEnginesOn(_vehicle->ModelData))
			mlStarterIsDepressed(1, 0, 0);

		SetFlag(EZManager::ENGINES_ON, false);
		SetFlag(EZManager::START_ENGINES, false);
	}
	else
	{
		SetFlag(EZManager::START_ENGINES, true);
	}
}

void EZGameVehicle::OnAction1(SINT32 param, UINT32 id,UINT32 extra)
{
	if(!(param && GetPosition().IsDead()))
		mlPrimaryTriggerDown(param, id, extra);
}

void EZGameVehicle::OnAction2(SINT32 param, UINT32 id, UINT32 extra)
{
	if(!(param && GetPosition().IsDead()))
		mlSecondaryTriggerDown(param, id, extra);
}

FLOAT32 EZGameVehicle::GetGaugeData(UINT32 gauge, UINT32 subGauge)
{
	FLOAT32 value = 0;
	mlGetGaugeData(&value, gauge, subGauge);
	return value;
}

SINT32 EZGameVehicle::GetGear()
{
	FLOAT32 gear = GetGaugeData(ML_DOF_ELEMENT_GEARSHIFT);

	if(_vehicle->ModelData->ReverseToggle || gear < 0)
		return -1;

	if(gear < .05f)
		return 0;

	return (SINT32)(gear + 0.5f);
}

bool EZGameVehicle::SetForward()
{
	if(frametime - _lastShift < ShiftTime())
		return false;

	if(frametime < _nextShift)
		return false;

	mlComponentVector list;
	mlengineGetComponents(_vehicle->ModelData, list, ML_COMPONENT_ENGINE, ML_ENGINE_SUBTYPE_GEARBOX);
	mlEngineComponentData* gearBox = nullptr;

	if(!list.empty())
		gearBox = (mlEngineComponentData*)(list.at(0)->SpecialData);

	if(gearBox->InReverse)
	{
		if(!HasFlag(EZManager::TOGGLING_GEARBOX))
			mlReverse(0, 0 ,0);

		return false;
	}

	if(HasFlag(EZManager::TOGGLING_GEARBOX))
	{
		// update our flag
		SetFlag(EZManager::TOGGLING_GEARBOX, false);

		_lastShift = frametime;
		_nextShift = 0;
	}

	return true;
}

bool EZGameVehicle::SetReverse()
{
	if(frametime - _lastShift < ShiftTime())
		return false;

	if(frametime < _nextShift)
		return false;

	mlComponentVector list;
	mlengineGetComponents(_vehicle->ModelData, list, ML_COMPONENT_ENGINE, ML_ENGINE_SUBTYPE_GEARBOX);
	mlEngineComponentData* gearBox = nullptr;

	if(!list.empty())
		gearBox = (mlEngineComponentData*)(list.at(0)->SpecialData);

	if(!gearBox->InReverse)
	{
		if(!HasFlag(EZManager::TOGGLING_GEARBOX))
			mlReverse(0, 0 ,0);

		return false;
	}

	if(HasFlag(EZManager::TOGGLING_GEARBOX))
	{
		// update our flag
		SetFlag(EZManager::TOGGLING_GEARBOX, false);

		_lastShift = frametime;
		_nextShift = 0;
	}

	return true;
}

void EZGameVehicle::ShiftUp()
{
	if(frametime - _lastShift < ShiftTime())
		return;

	if(frametime < _nextShift)
		return;

	mlShiftUp(0, 0, 0);

	_lastShift = frametime;
	_nextShift = 0;

	//uiChatPrintLine("Shifting UP - RPMS: %.2f", GetGaugeData(ML_DOF_GAUGE_RPM));
}

void EZGameVehicle::ShiftDown()
{
	if(frametime - _lastShift < ShiftTime())
		return;

	if(frametime < _nextShift)
		return;

	mlShiftDown(0, 0, 0);

	_lastShift = frametime;
	_nextShift = 0;

	//uiChatPrintLine("Shifting DOWN - RPMS: %.2f", GetGaugeData(ML_DOF_GAUGE_RPM));
}

SINT32 EZGameVehicle::ShiftTime()
{
	return 500;
}


/*
void EZGameVehicle::UpdateHUD()
{
	if(!IsCommandView())
		return;

	phui::View* reticle = uiFindView("reticle");
	if(!reticle) return;

	TargetView* target = dynamic_cast<TargetView*>(reticle);
	if(!target) return;

	mlWeaponMount* mount = nullptr;
	mlComponent* component = nullptr;

	mlComponentListIterator it;
	mlComponentListBegin(_vehicle->ModelData->ComponentList, &it);

	for(; it.comp; mlComponentListNext(&it))
	{
		component = it.comp;

		if(component->Type == ML_COMPONENT_WEAPONMOUNT)
		{
			if(!component->SpecialData)
				continue;

			mount = (mlWeaponMount*)(component->SpecialData);

			// found a valid mount
			if(mount->PositionNum == _positions[_position].position)
			{
				FLOAT64VECTOR firePoint, fireVelocity, offset;
				piMATRIX attitude;

				// get our firepoint's position and attitude
				mlWeaponGetFireAttPosVel(mount, 0, _vehicle->ModelData, &attitude, &offset, &firePoint, &fireVelocity);

				Matrixd rotate = attitude;
				Vec3d start = firePoint;
				start += offset;

				Vec3d forward = Vec3d(0, 1.0f, 0);

				forward = forward * rotate;
				forward *= 200.0f;

				Vec3d finalPos = start + forward;

				// position our reticle to be in align with our barrel
				target->SetTarget(finalPos.x(), finalPos.y(), finalPos.z());
				target->Update();

				return;
			}
		}
	}
}
*/

/************************************************************************************************
	EZPlane
*************************************************************************************************/

EZPlane::EZPlane(vehVehicle* v) :
	EZGameVehicle(v)
{
}

void EZPlane::InstallInputCallbacks()
{
	// install ez mode callbacks
	//imapInstallControlCallback(IMAP_EZ_MOVE, ezInputControls);
	//imapInstallControlCallback(IMAP_EZ_TURN, ezInputControls);

	if(IsCommandView())
	{
		imapInstallControlCallback(IMAP_EZ_ACTION1, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_ACTION2, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_FREELOOK, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_CAMERA_X, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_CAMERA_Y, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_DOWN, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_LEFT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_LEFT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK_LEFT, ezInputControls);
	}

	imapInstallControlCallback(IMAP_MOM_POSITION_1, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_2, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_3, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_4, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_5, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_6, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_7, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_8, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_9, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_10, ezInputControls);

	imapInstallControlCallback(IMAP_EZ_NEXTPOS, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_PREVPOS, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_ENGINE, ezInputControls);
}

/************************************************************************************************
	EZTruck
*************************************************************************************************/

EZTruck::EZTruck(vehVehicle* v) :
	EZGameVehicle(v),
	_hiGear(1),
	_hiShiftDownRPMs(.65f),
	_hiShiftUpRPMs(.97f),
	_hiAccelTime(3000),
    _hiShiftTime(500),
	_loShiftDownRPMs(.65f),
	_loShiftUpRPMs(.97f),
	_loAccelTime(500),
    _loShiftTime(100)
{
}

	/*
	if(IsCommandView() && _mainTurret >= 0)
	{
		EZCamera& camera = GetCamera();

		if(!HasFlag(EZManager::FREELOOK))
		{
			// handle traverse
			FLOAT64 traverse = (-camera.traverse / 180.0f) * .5f;

			_vehicle->ModelData->WantedTurretTraverse[_mainTurret] = traverse;

			FLOAT64 move = traverse - _vehicle->ModelData->TurretTraverse[_mainTurret];

			if(move < 0)
				_vehicle->ModelData->TurretTraverseRate[_mainTurret] = -1.0f;
			else if(move > 0)
				_vehicle->ModelData->TurretTraverseRate[_mainTurret] = 1.0f;
			else
				_vehicle->ModelData->TurretTraverseRate[_mainTurret] = 0;


			// handle elevation
			FLOAT64 elevate = (-_lookElev / 180.0f) * .5f;

			_vehicle->ModelData->WantedTurretElevate[_mainTurret] = elevate;

			move = elevate - _vehicle->ModelData->TurretElevate[_mainTurret];

			if(move < 0)
				_vehicle->ModelData->TurretElevateRate[_mainTurret] = -1.0f;
			else if(move > 0)
				_vehicle->ModelData->TurretElevateRate[_mainTurret] = 1.0f;
			else
				_vehicle->ModelData->TurretElevateRate[_mainTurret] = 0;
		}
		else
		{
			_vehicle->ModelData->TurretTraverseRate[_mainTurret] = 0;
			_vehicle->ModelData->TurretElevateRate[_mainTurret] = 0;
		}
	}
	*/

void EZTruck::SetConfig(const char* name, const char* value)
{
	if(strcasecmp(name, "higear") == 0)
		_hiGear = atoi(value);
	else if(strcasecmp(name, "hishiftuprpms") == 0 || strcasecmp(name, "shiftuprpms") == 0)
		_hiShiftUpRPMs = (FLOAT32)atof(value);
	else if(strcasecmp(name, "hishiftdownrpms") == 0 || strcasecmp(name, "shiftdownrpms") == 0)
		_hiShiftDownRPMs = (FLOAT32)atof(value);
	else if(strcasecmp(name, "hishifttime") == 0 || strcasecmp(name, "shifttime") == 0)
		_hiShiftTime = atoi(value);
	else if(strcasecmp(name, "hiacceltime") == 0 || strcasecmp(name, "acceltime") == 0)
		_hiAccelTime = atoi(value);
	else if(strcasecmp(name, "loshiftuprpms") == 0)
		_loShiftUpRPMs = (FLOAT32)atof(value);
	else if(strcasecmp(name, "loshiftdownrpms") == 0)
		_loShiftDownRPMs = (FLOAT32)atof(value);
	else if(strcasecmp(name, "loshifttime") == 0)
		_loShiftTime = atoi(value);
	else if(strcasecmp(name, "loacceltime") == 0)
		_loAccelTime = atoi(value);
	else
		EZGameVehicle::SetConfig(name, value);
}


SINT32 EZTruck::ShiftTime()
{
	return (HighGear()) ? _loShiftTime : _hiShiftTime;
}

bool EZTruck::HighGear()
{
	return (_currentGear > 0 && _currentGear >= _hiGear);
}

void EZTruck::UpdateEngine()
{
	if(IsDriverDead())
		return;

	SINT32 agear = abs(_currentGear);

	if(agear == 0)
		agear = 1;

	const FLOAT64 switchSpeed = .1f;

	if(_move == EZ_MOVE_FORWARD || (_move == EZ_MOVE_IDLE && _turn))
	{
		if(_currentGear < 0)
		{
			mlAdjustGas(-1000, 0, 0);
			mlCenterBrakeFunction(1000 / agear, 0, 0);
			ShiftDown();
		}
		else if(_currentGear == 0)
		{
			mlAdjustGas(-1000, 0, 0);
			mlCenterBrakeFunction(1000 / agear, 0, 0);

			if(_speed <= switchSpeed && SetForward())
				ShiftUp();
		}
		else
		{
			mlAdjustGas(1000, 0, 0);
			mlCenterBrakeFunction(-1000, 0, 0);
			UpdateGears();
		}
	}
	else if(_move == EZ_MOVE_BACKWARD)
	{
		if(_currentGear > 0)
		{
			mlAdjustGas(-1000, 0, 0);
			mlCenterBrakeFunction(1000 / agear, 0, 0);
			ShiftDown();
		}
		else if(_currentGear == 0)
		{
			mlAdjustGas(-1000, 0, 0);
			mlCenterBrakeFunction(1000 / agear, 0, 0);

			if(_speed <= switchSpeed && SetReverse())
				ShiftUp();
		}
		else
		{
			mlAdjustGas(1000, 0, 0);
			mlCenterBrakeFunction(-1000, 0, 0);
			UpdateGears();
		}
	}
	else
	{
		UpdateGears();
	}

	/*
	static SINT32 g = 0;

	if(g != _currentGear)
	{
		uiChatPrintLine("Gear is %d", _currentGear);
		g = _currentGear;
	}
	*/

	EZGameVehicle::UpdateEngine();
}

void EZTruck::UpdateGears()
{
	SINT32 agear = abs(_currentGear);

	mlEngineComponentData* engineComponent = mlGetEngine(_vehicle->ModelData);
	if(!engineComponent) return;

	mlEngineData* engineData = engineComponent->EngineData;
	if(!engineData) return;

	bool hiGear = HighGear();

	// max engine rpms
	FLOAT64 rpmPercent = (_maxRPMs) ? (_currentRPMs / _maxRPMs) : 0;
	FLOAT32& shiftUpRPMs = (hiGear) ? _hiShiftUpRPMs : _loShiftUpRPMs;
	FLOAT32& shiftDownRPMs = (hiGear) ? _hiShiftDownRPMs : _loShiftDownRPMs;
	SINT32& accelTime = (hiGear) ? _hiAccelTime : _loAccelTime;

	if(rpmPercent >= shiftUpRPMs && !_nextShift)
	{
		SINT32 scaler = 1;

		if(_currentGear >= 0)
		{
			scaler = _forwardGears - _currentGear;
			if(scaler <= 0) scaler = 1;
		}

		_nextShift = frametime + (accelTime / scaler);
	}

	if(_speed < _lastSpeed && rpmPercent <= shiftDownRPMs && agear > 1)
	{
		// we need a lower gear
		ShiftDown();
	}
	else if(_speed >= _lastSpeed && rpmPercent >= shiftUpRPMs)
	{
		// we need a higher gear
		ShiftUp();
	}

	EZGameVehicle::UpdateGears();
}


void EZTruck::OnTurn(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	SINT32 agear = abs(_currentGear);

	if(agear == 0)
		agear = 1;
	if(agear > 2)
		agear = 2;

	if(param < 0)
	{
		_turn = EZ_TURN_LEFT;
		mlSteeringFunction(-(1000 / agear), 0, 0);
	}
	else if(param > 0)
	{
		_turn = EZ_TURN_RIGHT;
		mlSteeringFunction((1000 / agear), 0, 0);
	}
	else
	{
		_turn = EZ_TURN_NONE;
		mlSteeringFunction(0, 0, 0);
	}

	/*
	static SINT32 t = 0;

	if(false && t != _turn)
	{
		uiChatPrintLine("Turn is %d", _turn);
		t = _turn;
	}
	*/
}

void EZTank::OnTurn(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	SINT32 agear = abs(_currentGear);

	if(agear == 0)
		agear = 1;
	if(agear > 2)
		agear = 2;

	if(param < 0)
	{
		_turn = EZ_TURN_LEFT;
		mlLeftBrakeFunction(1000 / agear, 0, 0);
		mlLeftTrackClutchFunction(1000 / agear, 0, 0);
		mlRightBrakeFunction(-1000, 0, 0);
		mlRightTrackClutchFunction(-1000, 0, 0);
	}
	else if(param > 0)
	{
		_turn = EZ_TURN_RIGHT;
		mlLeftBrakeFunction(-1000, 0, 0);
		mlLeftTrackClutchFunction(-1000, 0, 0);
		mlRightBrakeFunction((1000 / agear), 0, 0);
		mlRightTrackClutchFunction((1000 / agear), 0, 0);
	}
	else
	{
		_turn = EZ_TURN_NONE;
		mlLeftBrakeFunction(-1000, 0, 0);
		mlLeftTrackClutchFunction(-1000, 0, 0);
		mlRightBrakeFunction(-1000, 0, 0);
		mlRightTrackClutchFunction(-1000, 0, 0);
	}

	/*
	static SINT32 t = 0;

	if(false && t != _turn)
	{
		uiChatPrintLine("Turn is %d", _turn);
		t = _turn;
	}
	*/
}

void EZStug::SetConfig(const char* name, const char* value)
{
	if(strcasecmp(name, "quickpos") == 0)
		_quickPos = atoi(value);
	else
		EZTank::SetConfig(name, value);
}

void EZStug::OnSpawn()
{
	EZTank::OnSpawn();

	// go ahead and deploy the stug's gun
	mlToggleBombBayDoor(1, 0, 0);
}

void EZStug::OnAction2(SINT32 param, UINT32 id,UINT32 extra)
{
	if(param)
	{
		_lastPosition = _position;
		SetPosition(_quickPos);
	}
	else
	{
		SetPosition(_lastPosition);
	}
}

void EZATGun::FrameUpdate()
{
	// do normal update
	EZManager::FrameUpdate();

	// current speed
	Vec3d velocity = Vec3d(_vehicle->ModelData->VehicleVel);

	// save our current speed
	_speed = velocity.length();
}

bool EZATGun::IsDriverDead()
{
	return (IsCommanderDead() && IsGunnerDead());
}

bool EZATGun::IsCommanderDead()
{
	return (persGetCrewIsDead(0) > 0);
}

bool EZATGun::IsGunnerDead()
{
	return (persGetCrewIsDead(1) > 0);
}

bool EZATGun::IsLoaderDead()
{
	return IsDriverDead();
}

void EZATGun::OnTurn(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	if(param < 0)
	{
		_turn = EZ_TURN_LEFT;
		mlATGunLeftRight(-1000, 0, 0);
	}
	else if(param > 0)
	{
		_turn = EZ_TURN_RIGHT;
		mlATGunLeftRight(1000, 0, 0);
	}
	else
	{
		_turn = EZ_TURN_NONE;
		mlATGunLeftRight(0, 0, 0);
	}

	/*
	static SINT32 t = 0;

	if(false && t != _turn)
	{
		uiChatPrintLine("Turn is %d", _turn);
		t = _turn;
	}
	*/
}

void EZATGun::OnMove(SINT32 param, UINT32 id, UINT32 extra)
{
	if(PN_UNLIKELY(IsDriverDead()))
		return;

	if(param > 0)
	{
		_move = EZ_MOVE_FORWARD;
		mlATGunForwardBackwards(1000, 0, 0);
	}
	else if(param < 0)
	{
		_move = EZ_MOVE_BACKWARD;
		mlATGunForwardBackwards(-1000, 0, 0);
	}
	else
	{
		_move = EZ_MOVE_IDLE;
		mlATGunForwardBackwards(0, 0, 0);
	}

	/*
	static SINT32 m = 0;

	if(m != _move)
	{
		uiChatPrintLine("Move is %d", _move);
		m = _move;
	}
	*/
}

void EZATGun::OnAction2(SINT32 param, UINT32 id, UINT32 extra)
{
	if(!param || IsGunnerDead())
		return;
	mlToggleBombBayDoor(param, id, extra);
}

void EZBoat::UpdateEngine()
{
	if(_move == EZ_MOVE_FORWARD)
	{
		if(_currentGear < 0)
		{
			mlAdjustGas(-1000, 0, 0);
			ShiftDown();
		}
		else if(_currentGear == 0)
		{
			mlAdjustGas(-1000, 0, 0);

			if(SetForward())
				ShiftUp();
		}
		else
		{
			mlAdjustGas(1000, 0, 0);
		}
	}
	else if(_move == EZ_MOVE_BACKWARD)
	{
		if(_currentGear > 0)
		{
			mlAdjustGas(-1000, 0, 0);
			ShiftDown();
		}
		else if(_currentGear == 0)
		{
			mlAdjustGas(-1000, 0, 0);

			if(SetReverse())
				ShiftUp();
		}
		else
		{
			mlAdjustGas(1000, 0, 0);
		}
	}

	/*
	static SINT32 g = 0;

	if(g != _currentGear)
	{
		uiChatPrintLine("Gear is %d", _currentGear);
		g = _currentGear;
	}
	*/

	EZGameVehicle::UpdateEngine();
}

EZTrooper::~EZTrooper()
{
	fpFPSSetAutoReload(FALSE);
}

void EZTrooper::SetConfig(const char* name, const char* value)
{
	if(strcasecmp(name, "dispersion") == 0)
	{
		_dispersion = (FLOAT32)atof(value);
	}
	else if(strcasecmp(name, "autoreload") == 0)
	{
		if(strcasecmp(value, "true") == 0)
			fpFPSSetAutoReload(TRUE);
		else
			fpFPSSetAutoReload(FALSE);
	}
	else
	{
		for(SINT32 i = 0; i < FPS_WEAPONTYPE_NUM_TYPES; i++)
		{
			if(strcasecmp(name, fpFPSWeaponTypeStrings[i]) == 0)
			{
				_reticles[name] = value;
				return;
			}
		}

		EZManager::SetConfig(name, value);
	}
}

void EZTrooper::FrameUpdate()
{
	if(!uiInGame() || !_vehicle || !_vehicle->InUse)
		return;

	//FLOAT64 delta = FLOAT64(frametime - _lastUpdate) / 1000.0f;

	phui::View* reticle = uiFindView("camera_reticle");
	SINT32 weapType = fpFPSGetWeaponType();

	LOGICAL showReticle = !fpFPSIsAim();

	const char* typeString = fpFPSWeaponTypeStrings[weapType];

	if(fpFPSIsProne() || fpFPSIsDeployed() || fpFPSIsSwimming() || fpFPSIsFast() || fpFPSIsReloading())
		showReticle = FALSE;
	else if(persIsCameraExternalMode())
		showReticle = FALSE;
	else if(_reticles.find(typeString) == _reticles.end())
		showReticle = FALSE;

	if(_lastWeapon != weapType)
	{
		if(reticle)
		{
			reticle->Close();
			reticle = nullptr;
		}

		_lastWeapon = weapType;
		_showReticle = !showReticle; // force it to reload the reticle
	}

	if(_showReticle != showReticle)
	{
		if(!showReticle)
		{
			if(reticle)
			{
				reticle->Close();
				reticle = nullptr;
			}
		}
		else
		{
			_curReticle = 0;
			_tgtReticle = 0;
			_minReticle = 0;
			_maxReticle = 0;
			_returnRate = .1f;
			_fireBump = .3f;

			if(!reticle)
			{
				reticle = uiLoadView(_reticles[typeString].c_str());

				if(reticle)
				{
					phui::View* child = reticle->ChildAt(0);

					if(child)
					{
						_minReticle = child->Width();

						phui::CSSViewDrawer* css = dynamic_cast<phui::CSSViewDrawer*>(child->GetDrawer());

						if(css)
						{
							const char* travel = css->GetStyleProperty("travel");
							if(travel) _maxReticle = _minReticle + (_minReticle * (FLOAT32)atof(travel));

							const char* rate = css->GetStyleProperty("returnrate");
							if(rate) _returnRate = (FLOAT32)atof(rate);

							const char* bump = css->GetStyleProperty("firebump");
							if(rate) _fireBump = (FLOAT32)atof(bump);
						}
					}
				}
			}
		}

		_showReticle = showReticle;
	}

	if(reticle)
		UpdateReticle(reticle);

	_lastUpdate = frametime;
}

void EZTrooper::InstallInputCallbacks()
{
	// don't install any callbacks for troops
}

bool EZTrooper::IsDriverDead()
{
	return (fpFPSIsDead() == TRUE);
}

bool EZTrooper::IsCommanderDead()
{
	return (fpFPSIsDead() == TRUE);
}

bool EZTrooper::IsGunnerDead()
{
	return (fpFPSIsDead() == TRUE);
}

bool EZTrooper::IsLoaderDead()
{
	return (fpFPSIsDead() == TRUE);
}

FLOAT32 EZTrooper::GetDispersionMod()
{
	return _dispersion;
}

void EZTrooper::SetDispersion(FLOAT32 d)
{
	_tgtReticle = d;
}

void EZTrooper::AddDispersion(FLOAT32 d)
{
	_curReticle += _fireBump;

	if(_curReticle > 1.0f)
		_curReticle = 1.0f;
}


void EZTrooper::UpdateReticle(phui::View* r)
{
	if(!r) return;

	TargetView* tv = dynamic_cast<TargetView*>(r);
	if(!tv) return;

	vehVehicle* me = persGetPersonaVehiclePtr();
	if(!me) return;

	phui::View* child = tv->ChildAt(0);
	if(!child) return;

	piMATRIX fireAtt;
	FLOAT64VECTOR firePos;

	// Determine Point at which bullets should come out
	fpFPSGetFireDirection(me->VehicleObjectData->PlaneObject, &fireAtt);

	firePos.x = fireAtt._41;
	firePos.y = fireAtt._42;
	firePos.z = fireAtt._43;

	fireAtt._41 = 0;
	fireAtt._42 = 0;
	fireAtt._43 = 0;

	math3dMatrixMultiply(&fireAtt, &fireAtt, &me->ModelData->VehicleAttMat);

	mathVectorMultiply64(&firePos, &firePos, &me->ModelData->VehicleAttMat);

	mathAddVectors(&firePos, &firePos, &(me->ModelData->VehiclePos));

	Matrixd rotate	= Matrixd(fireAtt);
	Vec3d dir		= Vec3d(0, 50.0f, 0);

	dir = dir * rotate;

	firePos.x += dir.x();
	firePos.y += dir.y();
	firePos.z += dir.z();

	// now we need to resize the reticle
	// based on movements, firing, etc...
	FLOAT64 deltaTime	= FLOAT64(frametime - _lastUpdate) / 1000.0f;
	FLOAT64 diff		= _tgtReticle - _curReticle;
	FLOAT64 move		= _returnRate * deltaTime;

	if(move * move >= diff * diff)
		_curReticle = _tgtReticle;
	else
		_curReticle += FLOAT32((diff < 0) ? -move : move);

	if(_curReticle > 1.0f)
		_curReticle = 1.0f;
	else if(_curReticle < 0)
		_curReticle = 0;

	FLOAT32 width = FLOAT32(_minReticle + ((_maxReticle - _minReticle) * _curReticle));

	/*
	static FLOAT32 cw = 0;

	if(width != cw)
	{
		uiChatPrintLine("Reticle width is now %.5f", width);
		cw = width;
	}
	*/

	child->ResizeTo(width, width);

	// set our target reticle position
	tv->SetTarget(firePos.x, firePos.y, firePos.z);
	tv->Update();
}
