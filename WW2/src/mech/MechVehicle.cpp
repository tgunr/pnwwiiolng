#include <syscfg.h> // Path is project specific.
#include "mech/MechVehicle.h"

#include <string>
#include "fio/fio.h"
#include "str/stringLib.h"
#include "gr2/gr2Data.h"
#include "imap/ww2func.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/mathvect.h"
#include "math/mathUnitConversion.h"
#include "pi/piogl.h"
#include "gr2/gr2Main.h"
#include "gr2/gr2DamageModel.h"
#include "gr2/gr2Weapon.h"
#include "imap/imaptypes.h"
#include "draw/draw3d.h"
#include "ui/uiChat.h"
#include "trn/trnmain.h"
#include "ui/uiMain.h"
#include "mech/MechTarget.h"
#include "js/jsGR2Object.h"
#include "mech/MechWeapon.h"

static void _aimCollider(void* data, dGeomID o1, dGeomID o2);

// this is atan (1/1000) .. or 1 meter granularity at 1000m
#define	MOUSE_GRANULARITY 0.05729576041450060899707537401941f

enum MECH_VEHICLE_CONTROL_FLAG
{
	MV_CONTROL_FORWARD				= 1<<0,
	MV_CONTROL_BACKWARD				= 1<<1,
	MV_CONTROL_FORWARD_AUTO			= 1<<2,
	MV_CONTROL_TURBO				= 1<<3,
	MV_CONTROL_FIRE1				= 1<<4,
	MV_CONTROL_FIRE2				= 1<<5,
	MV_CONTROL_FREELOOK				= 1<<6,
	MV_CONTROL_JUMPJETS				= 1<<7,
	MV_CONTROL_SLOW					= 1<<8,
	MV_CONTROL_TRAVELMODE			= 1<<9
};

enum MECH_VEHICLE_LOGIC_FLAG
{
	MV_LOGIC_IDLE					= 1<<0,
	MV_LOGIC_MOVE					= 1<<1,
	MV_LOGIC_TURN					= 1<<2,
	MV_LOGIC_JUMP					= 1<<3
};

enum MECH_VEHICLE_CONDITION_FLAG
{
	MV_CONDITION_ENGINE				= 1<<0,
	MV_CONDITION_LFOOT				= 1<<1,
	MV_CONDITION_RFOOT				= 1<<2,
	MV_CONDITION_TURRET				= 1<<3,
	MV_CONDITION_REVERSE			= 1<<4,
	MV_CONDITION_COLLISION			= 1<<5,
	MV_CONDITION_FALLING			= 1<<6,
	MV_CONDITION_JUMPJETS			= 1<<7,
	MV_CONDITION_LANDED				= 1<<8,
	MV_CONDITION_NOWEAPONS			= 1<<9,
	MV_CONDITION_IDLE				= 1<<10,
	MV_CONDITION_ENGINE_DISABLED	= 1<<16,
	MV_CONDITION_TURRET_DISABLED	= 1<<17,
	MV_CONDITION_TURRET_DESTROYED	= 1<<18,
	MV_CONDITION_GUNS_DISABLED		= 1<<19,
	MV_CONDITION_NOMOVEMENT			= 1<<20,
	MV_CONDITION_TURBO_DAMAGED		= 1<<21
};

#define MV_CONTROL_FORWARD_ALL		MV_CONTROL_FORWARD|MV_CONTROL_FORWARD_AUTO
#define MV_CONTROL_FIRE_ALL			MV_CONTROL_FIRE1|MV_CONTROL_FIRE2
#define MV_SHARED_CONDITIONS		MV_CONDITION_ENGINE|MV_CONDITION_FALLING|MV_CONDITION_JUMPJETS|MV_CONDITION_IDLE

enum MECH_VEHICLE_DAMAGE_FLAG
{
	MV_DAMAGE_TURRET				= 1<<0,
	MV_DAMAGE_ENGINE				= 1<<1,
	MV_DAMAGE_BASE					= 1<<2,
	MV_DAMAGE_LLEG					= 1<<3,
	MV_DAMAGE_RLEG					= 1<<4
};

MechVehicle::MechVehicle(vehVehicle* vehicle, OBJECT* pObject) :
	gr2Vehicle(vehicle, pObject),
	_eyeLookAt(10.0f),
	_eyeAdjust(0),
	_eyeVOffset(-10.0f),
	_eyeVPad(0),
	_eyeHOffset(-20.0f),
	_eyeHPad(0),
	_eyeOffsetWanted(1.0f),
	_eyeOffset(1.0f),
	_aimOffset(0),
	_eCamTurret(0),
	_tCamTurret(0),
	_animSpeed(0),
	_turbo(0),
	_camTraverse(0),
	_camElevate(0),
	_aimChanged(false),
	_aimDistance(0),
	_traverseAccuracy(1.0f),
	_elevateAccuracy(1.0f),
	_targeter(NULL),
	_power(0),
	_maxPower(0)
{
	memset(&_camera, 0, sizeof(MechCamera));
	_aimPosition.x = _aimPosition.y = _aimPosition.z = 0;
	_aimDirection.x = _aimDirection.y = _aimDirection.z = 0;
	_target.Reset();
}

MechVehicle::~MechVehicle()
{
	_weapons.clear();

	if(_targeter)
		(_targeter->*_targeterCallback)();
}

UINT32 MechVehicle::ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve)
{
	UINT32 handled = 0;

	// only first person vehicles can handle input
	if(IsThirdPerson())
		return handled;

	// allow our attachments to handle the input 1st
	Attachments::iterator it = _attached.begin();
	for(; it != _attached.end(); it++)
	{
		gr2Attachment& attachment = *it;

		if(attachment.object)
		{
			gr2GameObject* go = dynamic_cast<gr2GameObject*>(attachment.object);
			if(go)
			{
				if(!HasCondition(MV_CONDITION_NOWEAPONS) || !go->IsObjectType(GOT_WEAPON))
					go->ProcessControlInputs(param, id, reserve);
			}
		}
	}

	mlVehicleData* data = GetGameVehicleData();

	// record the holddown states
	switch (id)
    {
		case IMAP_MOM_TOGGLE_HUD:
		{
			/*
			phui::View* found = uiViewport()->FindView("mechtarget");
			_target.view = (found) ? dynamic_cast<TargetView*>(found): NULL;

			if(_target.view && !_target.vehicle && _target.view->IsVisible())
				_target.view->Hide();
			*/
		}
		break;
		case IMAP_MECH_TARGET:
		{
			// find the target closet to my current boresight
			if(param)
				FindMechTarget();
		}
		break;
		case IMAP_MECH_TARGET_ENEMY:
		{
			// cycles through all available enemies
			if(param)
				FindMechTarget((_vehicle->VehicleType->WeaponSpec.side() == 1) ? 2: 1);
		}
		break;
		case IMAP_MECH_TARGET_FRIENDLY:
		{
			// cycles through all available friendlies
			if(param)
				FindMechTarget(_vehicle->VehicleType->WeaponSpec.side());
		}
		break;
		case IMAP_MECH_TARGET_CLEAR:
		{
			// cycles through all available friendlies
			if(param && _target.vehicle)
				ClearTarget();
		}
		break;
		case IMAP_MECH_SLOW:
		{
			if(param)
				ToggleControl(MV_CONTROL_SLOW);
		}
		break;
		case IMAP_MECH_FREELOOK:
		{
			if(param)
			{
				//GetAimPosition(NULL);
				_camera.freelook = true;
				_camera.sync = false;
				_camera.savedTraverse = _camera.wantedTraverse;
				_camera.savedElevate = _camera.wantedTraverse;
			}
			else
			{
				_camera.freelook = false;
				_camera.sync = true;
				_camera.wantedTraverse = _camera.savedTraverse;
				_camera.wantedTraverse = _camera.savedElevate;
			}

			// enables the mouse to orbit without the turrets following
			// release and the camera orbits back to line up behind the current turret facing
		}
		break;
		case IMAP_MECH_CAM_TRAVERSE:
		{
			if(!_camera.sync && (_camera.freelook || !HasCondition(MV_CONDITION_TURRET_DISABLED)))
			{
				float maxLeft = 360 + _camera.maxLeft; // maxLeft is negative

				if(!_camera.freelook && (_camera.wantedTraverse < maxLeft) && (_camera.wantedTraverse > _camera.maxRight))
				{
					_camera.wantedTraverse = (_camera.wantedTraverse > 180) ? maxLeft: _camera.maxRight;
				}
				else
				{
					imap_callback_ext_data* extdata = (imap_callback_ext_data*)reserve;

					_camera.wantedTraverse += (extdata->deltaInput * MOUSE_GRANULARITY);

					if(_camera.wantedTraverse < 0)
						_camera.wantedTraverse = 360 + _camera.wantedTraverse;
					else if(_camera.wantedTraverse > 360)
						_camera.wantedTraverse = _camera.wantedTraverse - 360;

					if(!_camera.freelook && (_camera.wantedTraverse < maxLeft) && (_camera.wantedTraverse > _camera.maxRight))
						_camera.wantedTraverse = (_camera.wantedTraverse > 180) ? maxLeft: _camera.maxRight;
				}
			}

			/*
			float traverse = extdata->deltaInput * MOUSE_GRANULARITY;

			if(traverse > 0) // right
			{
				if(_camera.freelook)
				{
					if(_camera.wantedTraverse + traverse > 180)
						_camera.wantedTraverse = -180 + ((_camera.wantedTraverse + traverse) - 180);
					else
						_camera.wantedTraverse += traverse;
				}
				else
				{
					if(_camera.wantedTraverse + traverse < _camera.maxRight)
						_camera.wantedTraverse += traverse;
					else
						_camera.wantedTraverse = _camera.maxRight;
				}
			}
			else if(traverse < 0) // left
			{
				if(_camera.freelook)
				{
					if(_camera.wantedTraverse + traverse < 180)
						_camera.wantedTraverse = 180 + ((_camera.wantedTraverse + traverse) + 180);
					else
						_camera.wantedTraverse += traverse;
				}
				else
				{
					if(_camera.wantedTraverse + traverse > _camera.maxLeft)
						_camera.wantedTraverse += traverse;
					else
						_camera.wantedTraverse = _camera.maxLeft;
				}
			}
			*/
		}
		break;
		case IMAP_MECH_CAM_ELEVATE:
		{
			if(!_camera.sync && (_camera.freelook || !HasCondition(MV_CONDITION_TURRET_DISABLED)))
			{
				float maxDown = 360 + _camera.maxDown; // maxDown is negative

				imap_callback_ext_data* extdata = (imap_callback_ext_data*)reserve;

				_camera.wantedElevate += (extdata->deltaInput * MOUSE_GRANULARITY);

				if(_camera.wantedElevate < 0)
					_camera.wantedElevate = 360 + _camera.wantedElevate;
				else if(_camera.wantedElevate > 360)
					_camera.wantedElevate = _camera.wantedElevate - 360;

				if((_camera.wantedElevate < maxDown) && (_camera.wantedElevate > _camera.maxUp))
					_camera.wantedElevate = (_camera.wantedElevate > 180) ? maxDown: _camera.maxUp;
			}
			/*
			imap_callback_ext_data* extdata = (imap_callback_ext_data*)reserve;

			_camera.wantedElevate += extdata->deltaInput * MOUSE_GRANULARITY;

			if(_camera.wantedElevate < 0)
				_camera.wantedElevate = 360 + _camera.wantedElevate;
			else if(_camera.wantedElevate > 360)
				_camera.wantedElevate = _camera.wantedElevate - 360;

			if(_camera.wantedElevate < 360 + _camera.maxDown) // maxDown is a negative number
				_camera.wantedElevate = 360 + _camera.maxDown;
			else if(_camera.wantedElevate > _camera.maxUp)
				_camera.wantedElevate = _camera.maxUp;

			_camera.wantedElevate = 10;
			*/

			/*
			float elevate = extdata->deltaInput * MOUSE_GRANULARITY;
			// orbits the camera up and down

			if(elevate > 0) // down
			{
				if(_camera.wantedElevate + elevate < _camera.maxDown)
					_camera.wantedElevate += elevate;
				else
					_camera.wantedElevate = _camera.maxDown;
			}
			else if(elevate < 0) // up
			{
				if(_camera.wantedElevate + elevate > _camera.maxUp)
					_camera.wantedElevate += elevate;
				else
					_camera.wantedElevate = _camera.maxUp;
			}
			*/
		}
		break;
		case IMAP_MECH_ZOOM_LEVEL:
		{
			if(param)
			{
				if(_eyeOffsetWanted == 1.0f)
					_eyeOffsetWanted = .45f;
				else if(_eyeOffsetWanted == .45f)
					_eyeOffsetWanted = .1f;
				else if(_eyeOffsetWanted == .1f)
					_eyeOffsetWanted = 1.0f;
			}
		}
		break;
		/*
		case IMAP_MECH_ACTIVATE1:
		{
			// activates item 1
			if(param && _weapons[0])
				_weapons[0]->Toggle();
		}
		break;
		case IMAP_MECH_ACTIVATE2:
		{
			// activates item 2
			if(param && _weapons[1])
				_weapons[1]->Toggle();
		}
		break;
		*/
		case IMAP_MECH_ACTIVEUP:
		case IMAP_MECH_ACTIVEDOWN:
		{
			// moves the selected item up one
			if(param)
			{
				int selected = 0;
				int current = -1;

				// figure out whats selected
				for(int i = 0; i < (int)_weapons.size(); i++)
				{
					if(_weapons[i]->IsEnabled())
					{
						selected++;
						current = i;
					}
				}

				if(selected == _weapons.size())
				{
					// disable all weapons except the first one
					for(int i = 0; i < (int)_weapons.size(); i++)
						_weapons[i]->SetEnabled(!i);
				}
				else if(current == _weapons.size() - 1)
				{
					// enable all weapons
					for(int i = 0; i < (int)_weapons.size(); i++)
						_weapons[i]->SetEnabled(true);
				}
				else
				{
					for(int i = 0; i < (int)_weapons.size(); i++)
						_weapons[i]->SetEnabled(i == (current + 1));
				}
			}
		}
		break;
		case IMAP_TMP_RANGEFIND:
		{
			//GetAimPosition(&_aimPosition);
		}
		break;
		case IMAP_MECH_FORWARD:
		{
			if(param)
				SetControls(MV_CONTROL_FORWARD, MV_CONTROL_BACKWARD);
			else
				ClearControl(MV_CONTROL_FORWARD_ALL);
		}
		break;
		case IMAP_MECH_BACKWARD:
		{
			if(param)
				SetControls(MV_CONTROL_BACKWARD, MV_CONTROL_FORWARD_ALL);
			else
				ClearControl(MV_CONTROL_BACKWARD);
		}
		break;
		case IMAP_MECH_FORWARD_AUTO:
		{
			if(param)
				ToggleControl(MV_CONTROL_FORWARD_AUTO);
		}
		break;
		/*
		case IMAP_HOLD_GUNSIGHT_RANGE_INC:
			if(param) _eyeOffsetWanted = max(_eyeOffsetWanted - _eyeAdjust, .1f);
		break;
		case IMAP_HOLD_GUNSIGHT_RANGE_DEC:
			if(param) _eyeOffsetWanted = min(_eyeOffsetWanted + _eyeAdjust, 1.0f);
		break;
		*/
		/*
		case IMAP_ASX_GAS:
		{
			handled = 1;

			if(HasFlag(GFT_CONDITION, MV_CONDITION_ENGINE))
			{
				FLOAT64 gas = (FLOAT64)(param + 1000) / 2000;

				if(param >= 0)
					_throttle += .1f;
				else
					_throttle -= .1f;

				if(_throttle > 1.0f)
					_throttle = 1.0f;
				else if(_throttle < -.5f)
					_throttle = -.5f;

				if(_throttle >= -.1 && _throttle <= .1)
					_throttle = 0;
			}
		}
		break;
		*/
		/*
		case IMAP_ASX_CENTERBRAKE:
		{
			_throttle = 0;
		}
		break;
		*/
		case IMAP_HOLD_MAINTRIGGER:
			//(data->PrimaryTriggerDown) ? SetState(GFT_CONTROL, MV_CONTROL_FIRE1): ClearState(GFT_CONTROL, MV_CONTROL_FIRE1);
		break;
		case IMAP_HOLD_SECONDTRIGGER:
			//(data->SecondaryTriggerDown) ? SetState(GFT_CONTROL, MV_CONTROL_FIRE2): ClearState(GFT_CONTROL, MV_CONTROL_FIRE2);
		break;
		case IMAP_ASX_TURRET_TRAVERSE:
			//SetTurret(GVTA_HORIZONTAL_AXIS, param); // * fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier;
			/*
			if (turnOK)
			{
				if (mouseLookOn)
				{
					if (!mlMouseLookAndTorsoUseSameKeymap())
					{
						fpFPSAxisRotation (param, id);
					}
				}
				else
				{
					fpFPSAxisRotation (param, id);
				}
			}
			*/
		break;
		case IMAP_ASX_TURRET_ELEVATE:
			//SetTurret(GVTA_VERTICAL_AXIS, param); // * fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier;
			/*
			if (mouseLookOn)
			{
				if (!mlMouseLookAndTorsoUseSameKeymap())
				{
					fpFPSAxisRotation (param, id);
				}
			}
			else
			{
				fpFPSAxisRotation (param, id);
			}
			*/
		break;
		case IMAP_MECH_CROUCH:
			// mech crouching?
		break;
		/*
		case IMAP_MECH_JUMP:
		{
			if(param && _jumpJets && _turboExpended < _turboDuration/2)
			{
				if(_turboDuration && _turboDuration > _turboExpended)
				{
					SetControl(MV_CONTROL_JUMPJETS);
				}
			}
			else
			{
				ClearControl(MV_CONTROL_JUMPJETS);
			}
		}
		break;
		*/
		default:
			Write_Log (LOG_FPS, LOG_INFO, "Unrecognized input received %d", id);
		break;
	}

	return handled;
}

void MechVehicle::HandleStateEvent(GR2_STATE_EVENT e, gr2State* s)
{
	switch(s->flagType)
	{
		case GFT_ACTION:
		{
			switch(e)
			{
				case GSE_START:
				{
					switch(s->value)
					{
						case MV_LOGIC_MOVE:
						{
							gr2AnimationState* animState = (gr2AnimationState*)s->states[0];
							gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, _currentSpeed/_animSpeed);
						}
						break;
						case MV_LOGIC_TURN:
						{
							gr2AnimationState* animState = (gr2AnimationState*)s->states[0];

							if(_currentTurn > 0)
							{
								gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, animState->speed);
							}
							else
							{
								// start from the end
								gr2BlendControlSetLastFrame(GetSkeleton(), animState->blend);
								gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, -animState->speed);
							}
						}
						break;
					}
				}
				break;
				case GSE_UPDATE:
				{
					switch(s->value)
					{
						case MV_LOGIC_MOVE:
						{
							gr2AnimationState* animState = (gr2AnimationState*)s->states[0];
							gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, _currentSpeed/_animSpeed);
						}
						break;
						case MV_LOGIC_TURN:
						{
							gr2AnimationState* animState = (gr2AnimationState*)s->states[0];

							if(_currentTurn > 0)
								gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, animState->speed);
							else
								gr2BlendControlSetSpeed(GetSkeleton(), animState->blend, -animState->speed);
						}
						break;
					}
				}
				break;
			}
		}
		break;
		case GFT_CONTROL:
		break;
		case GFT_CONDITION:
		{
			switch(e)
			{
				case GSE_START:
				{
					switch(s->value)
					{
						case MV_CONDITION_COLLISION:
						{
							if(_throttle)
							{
								for(int i = 0; i < (int)s->states.size(); i++)
								{
									if(s->states[i]->type == GST_EFFECT)
									{
										gr2EffectState* state = (gr2EffectState*)s->states.at(i);

										// update the collision state's collide point
										state->position.x = _lastCollision.x;
										state->position.y = _lastCollision.y;
										state->position.z = _lastCollision.z;
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
}

bool MechVehicle::Init()
{
	if(gr2Vehicle::Init())
	{
		SetState("engine");
		SetState("idle");

		_camera.elevateRate		= 750.0f;
		_camera.traverseRate	= 750.0f;

		for(int i = 0; i < (int)_turrets.size(); i++)
		{
			gr2Turret& turret = _turrets[i];

			if(turret.spec->traverse.type == gr2TurretRotationSpec::TURRET_TYPE_TRAVERSE)
			{
				_camera.maxLeft		= turret.spec->traverse.minrot;
				_camera.maxRight	= turret.spec->traverse.maxrot;
			}

			if(turret.spec->elevate.type == gr2TurretRotationSpec::TURRET_TYPE_ELEVATE)
			{
				// these values are flipped
				_camera.maxDown		= -turret.spec->elevate.maxrot;
				_camera.maxUp		= -turret.spec->elevate.minrot;
			}
		}

		_absMaxTurn = _maxTurn;
		_power = _maxPower;
	}

	return true;
}

void MechVehicle::Update(FLOAT64 delta)
{
	bool isFirstPerson = IsFirstPerson();

	if(isFirstPerson)
	{
		_throttle = 0;
		_maxTurn = _absMaxTurn;

		if(HasControl(MV_CONTROL_FORWARD_ALL))
		{
			_throttle = 1.0f;

			if(HasControl(MV_CONTROL_SLOW))
			{
				_throttle = .5f;
			}
			else if(HasControl(MV_CONTROL_TRAVELMODE) && !HasCondition(MV_CONDITION_TURBO_DAMAGED))
			{
				_throttle = 2.0f; // needs to be exposed
			}
			else if(HasControl(MV_CONTROL_TURBO) && !HasCondition(MV_CONDITION_TURBO_DAMAGED))
			{
				_throttle = _turbo;
			}
		}
		else if(HasControl(MV_CONTROL_BACKWARD))
		{
			_throttle = -.5f;
		}

		if(HasControl(MV_CONTROL_JUMPJETS) && !HasCondition(MV_CONDITION_TURBO_DAMAGED))
		{
			static float maxLift = 0;
			_currentLift += (float)(20.0f * delta);

			if(_currentLift > maxLift)
				maxLift = _currentLift;

			SetState("jump");
		}
		else
		{
			_currentLift -= (float)(20.0f * delta);

			if(_currentLift < 0)
				_currentLift = 0;

			ClearState("jump");
		}

		if(HasCondition(MV_CONDITION_ENGINE_DISABLED))
			_throttle = 0;

		if(HasCondition(MV_CONDITION_NOMOVEMENT))
		{
			_throttle = 0;
			_vehicle->ModelData->Steering = 0;
		}

		if(_throttle)
		{
			float adjustment = max((1.0f - (_currentSpeed/_maxSpeed)), 0);
			_maxTurn = (_absMaxTurn * .75f) + (adjustment * (.25f * _absMaxTurn));
		}

		if(HasCondition(MV_CONDITION_TURRET_DISABLED))
		{
			for(int i = 0; i < (int)_turrets.size(); i++)
				_turrets.at(i).disabled = true;

			if(!_camera.freelook && !_camera.sync && GetAccuracy() != 1.0f)
				_camera.sync = true;
		}

		// force the legs not to move
		if((!_currentSpeed && !_currentTurn) || (IsFalling() && _aglAltitude > 15.0f))
			SetCondition(MV_CONDITION_IDLE);
		else
			ClearCondition(MV_CONDITION_IDLE);
	}

	if(HasCondition(MV_CONDITION_TURRET_DESTROYED))
	{
		for(int i = 0; i < (int)_weapons.size(); i++)
		{
			if(_weapons[i]->IsEnabled())
				_weapons[i]->Toggle();

			_weapons[i]->SetActive(false);
			_weapons[i]->Hide();
		}
	}

	gr2Vehicle::Update(delta);

	if(_currentSpeed && !HasCondition(MV_CONDITION_IDLE))
	{
		SetState("move");
	}
	else
	{
		if(_currentTurn && !HasCondition(MV_CONDITION_IDLE))
			SetState("turn");
		else
			SetState("idle");
	}

	if(isFirstPerson)
	{
		CalculateAimPosition();

		/*
		mlVehicleData* data = GetGameVehicleData();

		if(!HasCondition(MV_CONDITION_TURRET_DESTROYED|MV_CONDITION_GUNS_DISABLED))
		{
			if(data->PrimaryTriggerDown && !HasControl(MV_CONTROL_FIRE1))
			{
				SetState(GFT_CONTROL, MV_CONTROL_FIRE1);
				FireWeapons(true);
			}
		}

		if(!data->PrimaryTriggerDown && HasControl(MV_CONTROL_FIRE1))
		{
			ClearState(GFT_CONTROL, MV_CONTROL_FIRE1);
			FireWeapons(false);
		}
		*/
	}
	else
	{
		// we currently only use 2 weapon mounts
		/*
		if(_vehicle->MountIsFiring[0] && !HasControl(MV_CONTROL_FIRE1))
		{
			SetState(GFT_CONTROL, MV_CONTROL_FIRE1);
			FireWeapons(true, 0);
		}
		else if(!_vehicle->MountIsFiring[0] && HasControl(MV_CONTROL_FIRE1))
		{
			ClearState(GFT_CONTROL, MV_CONTROL_FIRE1);
			FireWeapons(false, 0);
		}

		if(_vehicle->MountIsFiring[1] && !HasControl(MV_CONTROL_FIRE2))
		{
			SetState(GFT_CONTROL, MV_CONTROL_FIRE2);
			FireWeapons(true, 1);
		}
		else if(!_vehicle->MountIsFiring[1] && HasControl(MV_CONTROL_FIRE2))
		{
			ClearState(GFT_CONTROL, MV_CONTROL_FIRE2);
			FireWeapons(false, 1);
		}
		*/

		if(HasCondition(MV_CONDITION_JUMPJETS))
		{
			gr2State* jumpState = GetState("jump");

			if(!jumpState->Active())
			{
				ClearCondition(MV_CONDITION_JUMPJETS);
				SetState("jump");
			}
		}
		else
		{
			ClearState("jump");
		}

		//Write_Debug("<<< Mech Vehicle Position = %f, Rendered = %f", _vehicle->PosData.Pos.z, _vehicle->PosData.RenderPos.z);
	}
}

void MechVehicle::Draw()
{
	gr2Vehicle::Draw();

	// draw a debug line if we have an aim position
	if(false) //if(true || _aimChanged)
	{
		int id = 0;
		FLOAT64VECTOR temp, temp1, temp2;

		temp = _aimPosition;

		{
			temp.z *= -1;

			temp1.x = temp.x - 2;
			temp1.y = temp.y;
			temp1.z = temp.z;

			temp2.x = temp.x + 2;
			temp2.y = temp.y;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "aimx", colorREDALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y - 2;
			temp1.z = temp.z;

			temp2.x = temp.x;
			temp2.y = temp.y + 2;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "aimy", colorREDALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y;
			temp1.z = temp.z - 2;

			temp2.x = temp.x;
			temp2.y = temp.y;
			temp2.z = temp.z + 2;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "aimz", colorREDALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

		}

		piVECTOR offset;

		offset.x = _eyeMatrix._41;
		offset.y = _eyeMatrix._42;
		offset.z = _eyeMatrix._43;

		if(GetAbsoluteBonePosition("COM", &temp, &offset))
		{
			temp.z *= -1;

			temp1.x = temp.x - 2;
			temp1.y = temp.y;
			temp1.z = temp.z;

			temp2.x = temp.x + 2;
			temp2.y = temp.y;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "camx", colorMAGENTAALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y - 2;
			temp1.z = temp.z;

			temp2.x = temp.x;
			temp2.y = temp.y + 2;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "camy", colorMAGENTAALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y;
			temp1.z = temp.z - 2;

			temp2.x = temp.x;
			temp2.y = temp.y;
			temp2.z = temp.z + 2;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "camz", colorMAGENTAALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

		}

		offset.x = _lookAt.x;
		offset.y = _lookAt.y;
		offset.z = -_lookAt.z;

		if(false && GetAbsoluteBonePosition("COM", &temp, &offset))
		{
			temp.z *= -1;

			temp1.x = temp.x - 2;
			temp1.y = temp.y;
			temp1.z = temp.z;

			temp2.x = temp.x + 2;
			temp2.y = temp.y;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "lookx", colorCYANALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y - 2;
			temp1.z = temp.z;

			temp2.x = temp.x;
			temp2.y = temp.y + 2;
			temp2.z = temp.z;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "looky", colorCYANALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

			temp1.x = temp.x;
			temp1.y = temp.y;
			temp1.z = temp.z - 2;

			temp2.x = temp.x;
			temp2.y = temp.y;
			temp2.z = temp.z + 2;

			id = draw3dSetWorldDebugLine(&temp1, &temp2, "lookz", colorCYANALPHA);
			draw3dSetDebugObjectFlipZ(id, FALSE);
			draw3dSetDebugObjectAlwaysOnTop(id, TRUE);

		}

		_aimChanged = false;
	}

	bool drawbb = false;

	if(!IsFirstPerson() && drawbb)
	{
		OBJECT* o = GetGameObject();
		vehVehicle* v = GetGameVehicle();

        piVECTOR v1 = o->bbox[0];
        piVECTOR v2 = o->bbox[1];

		v1.z *= -1.0f;
		v2.z *= -1.0f;

		math3dVectorMultiply(&v1, &v1, &o->mat);
		math3dVectorMultiply(&v2, &v2, &o->mat);

		mathAddVectors(&v1, &v1, &o->pos);
        mathAddVectors(&v2, &v2, &o->pos);

        //piSetWorldMatrix(&identity);

        piVECTOR p1, p2;

        p1.x = v1.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorBLUEALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorYELLOWALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1.x = v1.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorREDALPHA);

        p1.x = v1.x; p1.y = v1.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorBLUEALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorYELLOWALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v1.x; p2.y = v1.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1.x = v2.x; p1.y = v1.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorREDALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorBLUEALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v2.x; p2.y = v1.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1.x = v1.x; p1.y = v2.y; p1.z = v2.z;
        p2.x = v2.x; p2.y = v2.y; p2.z = v2.z;
        draw3dLine(&p1, &p2, colorREDALPHA);

        p1.x = v2.x; p1.y = v2.y; p1.z = v1.z;
        p2.x = v1.x; p2.y = v2.y; p2.z = v1.z;
        draw3dLine(&p1, &p2, colorBLUEALPHA);
	}

	if(IsFirstPerson())
		CalculateHUDTargetInfo();
}

void MechVehicle::PreDrawCallback(gr2ModelRef model)
{
	if(false && _target.vehicle)
	{
		CalculateHUDTargetInfo();
		/*
		FLOAT64VECTOR tpos;
		tpos.x = tpos.y = tpos.z = 0;

		if(_target.vehicle->GetAbsoluteBonePosition("COM", &tpos))
		{
			//mathVectorMultiply64(&tpos, &tpos, &GetGameVehicleData()->VehicleAttInvMat);

			float sx = -1;
			float sy = -1;

			if(GetWorldToScreen(&tpos, &sx, &sy))
			{
				int i = 0;
			}
		}
		*/
	}

	gr2Vehicle::PreDrawCallback(model);
}

void MechVehicle::UpdateEyeMatrix(FLOAT64 delta)
{
	if(_eyeOffsetWanted != _eyeOffset)
	{
		float lastOffset = _eyeOffset;

		if(_eyeOffset > _eyeOffsetWanted)
			_eyeOffset -= (float)delta * _eyeAdjust * 1.0f;
		else if(_eyeOffset < _eyeOffsetWanted)
			_eyeOffset += (float)delta * _eyeAdjust * 1.0f;

		// sync if we overshot
		if(lastOffset < _eyeOffsetWanted && _eyeOffset > _eyeOffsetWanted)
			_eyeOffset = _eyeOffsetWanted;
		else if(lastOffset > _eyeOffsetWanted && _eyeOffset < _eyeOffsetWanted)
			_eyeOffset = _eyeOffsetWanted;
	}

	if(!_camera.freelook && !_camera.sync && HasCondition(MV_CONDITION_TURRET_DISABLED))
		return;

	if(!_camera.sync && _currentTurn)
	{
		float maxLeft = 360 + _camera.maxLeft; // maxLeft is negative

		_camera.wantedTraverse += RAD_TO_DEG(_currentTurn);

		if(_camera.wantedTraverse < 0)
			_camera.wantedTraverse = 360 + _camera.wantedTraverse;
		else if(_camera.wantedTraverse > 360)
			_camera.wantedTraverse = _camera.wantedTraverse - 360;

		if(!_camera.freelook && (_camera.wantedTraverse < maxLeft) && (_camera.wantedTraverse > _camera.maxRight))
			_camera.wantedTraverse = (_camera.wantedTraverse > 180) ? maxLeft: _camera.maxRight;
	}

	float _camTraverseAngle = (_camera.wantedTraverse > 180) ? -360 + _camera.wantedTraverse: _camera.wantedTraverse;
	float _camElevateAngle	= (_camera.wantedElevate > 180) ? -360 + _camera.wantedElevate: _camera.wantedElevate;

	if(_camTraverseAngle != _camera.traverse)
	{
		float traversed = (float)delta * _camera.traverseRate;

		// 1st 2 blocks handle sign flips
		if(_camTraverseAngle >= 0 && _camera.traverse < 0 && (_camera.traverse - traversed) < -180)
		{
			_camera.traverse = max(360 + (_camera.traverse - traversed), _camTraverseAngle);
		}
		else if(_camTraverseAngle < 0 && _camera.traverse >= 0 && (_camera.traverse + traversed) > 180)
		{
			_camera.traverse = min(-360 + (_camera.traverse + traversed), _camTraverseAngle);
		}
		else
		{
			if(_camTraverseAngle > _camera.traverse)
				_camera.traverse = min(_camera.traverse + traversed, _camTraverseAngle);
			else
				_camera.traverse = max(_camera.traverse - traversed, _camTraverseAngle);
		}
	}
	else if(!_camera.freelook)
	{
		_camera.sync = false;
	}

	if(_camElevateAngle != _camera.elevate)
	{
		if(_camElevateAngle > _camera.elevate)
			_camera.elevate = min(_camera.elevate + ((float)delta * _camera.elevateRate), _camElevateAngle);
		else
			_camera.elevate = max(_camera.elevate - ((float)delta * _camera.elevateRate), _camElevateAngle);
	}

	// we calculate the lookAt position based on the turret
	// we calculate the eye position based on the turret

	piVECTOR lookDir	= {0,1,0}; // start off by looking straight ahead
	piVECTOR lookAt		= {0,0,0}; // what point we want to look at
	piVECTOR eyePoint	= {0,0,0}; // where the camera is
	piVECTOR start		= {0,0,-8}; // approximately 10 meters off the ground (around the neck)

	// temporary to test fps style mech camera..ugh
	/*
	if(!HasFlag(GFT_CONDITION, MV_CONDITION_TURRET_DESTROYED))
	{
		std::string component = "turret";
		const char* mesh = (_damageModel->GetComponentLevel(component) == 0) ? "mesh_torso": "damage1_mesh_torso";

		if(_eyeOffset == .1f)
			gr2SetMeshActiveByName(GetLOD(), mesh, false);
		else
			gr2SetMeshActiveByName(GetLOD(), mesh, true);
	}
	*/

	piMATRIX rotMatrix	= identity;
	piMATRIX tempMatrix = identity;

	// get the camera traverse rotation matrix
	GenerateRotation(AROUND_Z, _camera.traverse - 35, &tempMatrix);
	math3dMatrixMultiply(&rotMatrix,&tempMatrix,&rotMatrix);

	// get the camera elevation rotation matrix
	GenerateRotation(AROUND_X, -_camera.elevate + 42, &tempMatrix);
	math3dMatrixMultiply(&rotMatrix,&tempMatrix,&rotMatrix);

	// convert it to a look direction vector
	math3dMatrixToVector(&lookDir, &rotMatrix);

	// normalize it
	mathReallyFastNormalize(lookDir);

	// scale it by 10 meters forward
	math3dScaleVector(&lookDir, &lookDir, _eyeLookAt);

	// move the lookAt point to start plus scaled lookDir
	math3dAddVectors(&lookAt, &start, &lookDir);

	// re-normalize
	mathReallyFastNormalize(lookDir);

	// reverse and scale backwards
	math3dScaleVector(&lookDir, &lookDir, _eyeHPad + (_eyeOffset * _eyeHOffset));

	// move the camera point to behind
	math3dAddVectors(&eyePoint, &start, &lookDir);

	// move it up some more so we're
	// slightly looking down
	eyePoint.z += _eyeVPad + (_eyeOffset * _eyeVOffset);

	//eyePoint.x = eyePoint.y = eyePoint.z = 0;

	// set the look at and eye position
	SetEyePosition(&eyePoint);
	LookAt(&lookAt);

	// test look at code
	if(false && _weapons.size())
	{
		lookAt.x = lookAt.y = lookAt.z = 0;
		lookDir.x = lookDir.y = lookDir.z = 0;

		piMATRIX fireMatrix = identity;

		if(_weapons[0]->GetFirePoint(&fireMatrix))
		{
			fireMatrix._41 = 0;
			fireMatrix._42 = 0;
			fireMatrix._43 = 0;

			math3dMatrixMultiply(&fireMatrix, &fireMatrix, &_vehicle->ModelData->VehicleAttMat);
			math3dMatrixToVector(&lookDir, &fireMatrix);
			mathReallyFastNormalize(lookDir);

			// scale it by 10 meters forward
			math3dScaleVector(&lookDir, &lookDir, 1000.0f);

			// move the lookAt point to start plus scaled lookDir
			math3dAddVectors(&lookAt, &start, &lookDir);

			// update the loook at point
			LookAt(&lookAt);
		}
	}

	// let the underlying class point the eye at the lookat point
	gr2Vehicle::UpdateEyeMatrix(delta);
}

/*
void MechVehicle::UpdateEyeMatrix(FLOAT64 delta)
{
	// we calculate the lookAt position based on the turret
	// we calculate the eye position based on the turret

	piVECTOR lookDir	= {0,1,0}; // start off by looking straight ahead
	piVECTOR lookAt		= {0,0,0}; // what point we want to look at
	piVECTOR eyePoint	= {0,0,0}; // where the camera is
	piVECTOR start		= {0,0,-10}; // approximately 10 meters off the ground (around the neck)

	// take turret rotation into account
	if(_turrets.size())
	{
		gr2Turret& turret = _turrets.at(0);

		if(turret.traverse || turret.elevate)
		{
			piMATRIX rotMatrix	= identity;
			piMATRIX tempMatrix = identity;

			// get the turret traverse rotation matrix
			if(turret.traverse)
			{
				GenerateRotation(AROUND_Z, turret.traverse - 37, &tempMatrix);
				math3dMatrixMultiply(&rotMatrix,&tempMatrix,&rotMatrix);
			}

			// get the turret elevation rotation matrix
			if(turret.elevate)
			{
				GenerateRotation(AROUND_X, -turret.elevate + 45, &tempMatrix);
				math3dMatrixMultiply(&rotMatrix,&tempMatrix,&rotMatrix);
			}

			// convert it to a look direction vector
			math3dMatrixToVector(&lookDir, &rotMatrix);
			// normalize it
			mathReallyFastNormalize(lookDir);
		}
	}

	// scale it by 10 meters forward
	math3dScaleVector(&lookDir, &lookDir, _eyeLookAt);

	// move the lookAt point to start plus scaled lookDir
	math3dAddVectors(&lookAt, &start, &lookDir);

	// re-normalize
	mathReallyFastNormalize(lookDir);

	// reverse and scale backwards
	math3dScaleVector(&lookDir, &lookDir, _eyeHPad + (_eyeOffset * _eyeHOffset));

	// move the camera point to behind
	math3dAddVectors(&eyePoint, &start, &lookDir);

	// move it up some more so we're
	// slightly looking down
	eyePoint.z += _eyeVPad + (_eyeOffset * _eyeVOffset);

	// set the look at and eye position
	SetEyePosition(&eyePoint);
	LookAt(&lookAt);

	// let the underlying class point the eye at the lookat point
	gr2Vehicle::UpdateEyeMatrix(delta);
}
*/

// intercept weapons so we have a ready list
void MechVehicle::AttachObject(gr2Object* object, const char* from, const char* to, bool owner)
{
	if(object->IsObjectType(GOT_WEAPON))
		_weapons.push_back(dynamic_cast<MechWeapon*>(object));

	gr2Vehicle::AttachObject(object, from, to, owner);
}

// removed weapons from our ready list
void MechVehicle::RemoveObject(gr2Object* object, bool freeOwned)
{
	if(object->IsObjectType(GOT_WEAPON))
	{
		MechWeaponList::iterator fit = _weapons.begin();
		for(; fit != _weapons.end(); fit++)
		{
			if(*fit == dynamic_cast<MechWeapon*>(object))
			{
				_weapons.erase(fit);
				break;
			}
		}
	}

	gr2Vehicle::RemoveObject(object, freeOwned);
}

#define IS_PARAM(x) (!stricmp(name.c_str(), x))

void MechVehicle::AddConfigParam(const std::string& name, const std::string& value)
{
	if(IS_PARAM("maxspeed"))			_maxSpeed		= (float)atof(value.c_str());
	else if(IS_PARAM("cruise"))			_cruise			= (float)atof(value.c_str());
	else if(IS_PARAM("maxaccel"))		_maxAccel		= (float)atof(value.c_str());
	else if(IS_PARAM("maxturn"))		_maxTurn		= (float)atof(value.c_str());
	else if(IS_PARAM("lookat"))			_eyeLookAt		= (float)atof(value.c_str());
	else if(IS_PARAM("eyeadjust"))		_eyeAdjust		= (float)atof(value.c_str());
	else if(IS_PARAM("eyevoffset"))		_eyeVOffset		= (float)atof(value.c_str());
	else if(IS_PARAM("eyevpad"))		_eyeVPad		= (float)atof(value.c_str());
	else if(IS_PARAM("eyehoffset"))		_eyeHOffset		= (float)atof(value.c_str());
	else if(IS_PARAM("eyehpad"))		_eyeHPad		= (float)atof(value.c_str());
	else if(IS_PARAM("aimoffset"))		_aimOffset		= (float)atof(value.c_str());
	else if(IS_PARAM("ecamturret"))		_eCamTurret		= atoi(value.c_str());
	else if(IS_PARAM("tcamturret"))		_tCamTurret		= atoi(value.c_str());
	else if(IS_PARAM("animspeed"))		_animSpeed		= (float)atof(value.c_str());
	else if(IS_PARAM("turbo"))			_turbo			= (float)atof(value.c_str());
	else if(IS_PARAM("power"))			_maxPower		= (UINT32)atoi(value.c_str());
}

void MechVehicle::HandleAnimEvent(gr2ModelRef model, SINT32 type, SINT32 event)
{
	if(model != GetSkeleton())
		return;

	switch (gr2GetEventType(model, event))
	{
		case ANIM_EVENT_LFOOT:
			if(!IsFalling())
			{
				ClearState("lfoot");
				SetState("lfoot");
			}
			break;
		case ANIM_EVENT_RFOOT:
			if(!IsFalling())
			{
				ClearState("rfoot");
				SetState("rfoot");
			}
			break;
		default:
			gr2Vehicle::HandleAnimEvent(model, type, event);
			break;
	}
}

const FLOAT64VECTOR* MechVehicle::GetAimPosition() const
{
	return &_aimPosition;
}

FLOAT32 MechVehicle::GetAimRange()
{
	return _hitDistance; //_aimDistance;
	/*
	FLOAT64VECTOR position;

	if(GetAbsoluteBonePosition("COM", &position))
	{
		FLOAT64VECTOR distance;
		mathSubVectors(&distance, &position, &_aimPosition);
		return (FLOAT32)mathExactVectorLength(&distance);
	}

	return 0;
	*/
}

void MechVehicle::CalculateAimPosition()
{
	/*
	1. camera pos from lookat pos to get dir vector
	2. normalize direction vector
	3. calculate camera pos world position
	4. subtract the physics world offset from the camera world pos
	5. rotate direction vector by vehicle's matrix
	6. create a ray geom and find the physics contact point
	7. add the physics world offset to the contact point to get the world position
	*/

	_aimChanged = false;

	FLOAT64VECTOR& physicsOffset	= physicsGetWorldOffset();
	piVECTOR physicsCam				= {0,0,0};
	piVECTOR physicsLook			= {0,0,0};
	//piVECTOR direction				= {0,0,0};

	FLOAT64VECTOR cam, look;
	piVECTOR offset;

	offset.x = _eyeMatrix._41;
	offset.y = _eyeMatrix._42;
	offset.z = _eyeMatrix._43;

	if(GetAbsoluteBonePosition("COM", &cam, &offset))
	{
		physicsCam.x = (FLOAT32)(cam.x - physicsOffset.x);
		physicsCam.y = (FLOAT32)(cam.y - physicsOffset.y);
		physicsCam.z = (FLOAT32)(cam.z - physicsOffset.z);
	}

	offset.x = _lookAt.x;
	offset.y = _lookAt.y;
	offset.z = -_lookAt.z;

	if(GetAbsoluteBonePosition("COM", &look, &offset))
	{
		physicsLook.x = (FLOAT32)(look.x - physicsOffset.x);
		physicsLook.y = (FLOAT32)(look.y - physicsOffset.y);
		physicsLook.z = (FLOAT32)(look.z - physicsOffset.z);
	}

	mathSubVectors(&_aimDirection, &physicsLook, &physicsCam);

	mathReallyFastNormalize(_aimDirection);

	// let's set the default aim position
	{
		// scale the direction vector 500 meters out
		mathScaleVector(&_aimDirection, &_aimDirection, 500);

		// add it to the current cam position to get the
		// default aim point
		mathAddVectors(&_aimPosition, &cam, &_aimDirection);

		// re-normalize the direction vector
		mathReallyFastNormalize(_aimDirection);
	}

	// now lets shoot the ray to see if it hits anything out to 5000 meters

	dGeomID ray = dCreateRay(0, 5000);
	_aimDistance = 5000;

	if(ray)
	{
		dGeomRaySet(ray, physicsCam.x, physicsCam.y, physicsCam.z, _aimDirection.x, _aimDirection.y, _aimDirection.z);
		dGeomSetCollideBits(ray, COLLIDER_TERRAIN_MASK);
		dGeomSetData(ray, this);

		// try to collide with a vehicle 1st
		dSpaceCollide2(ray, (dGeomID)colliderVehicleSpace(), this, _aimCollider);

		// if no vehicles were hit, check the terrain
		if(!_aimChanged)
			dSpaceCollide2(ray, (dGeomID)colliderTerrainSpace(), this, _aimCollider);

		dGeomDestroy(ray);
	}

	///////////////////////////////
	// stuff below here isn't used
	///////////////////////////////

	/*
	piVECTOR dir					= {0,0,0};
	piVECTOR cam					= {_eyeMatrix._41, _eyeMatrix._42, _eyeMatrix._43};
	FLOAT64VECTOR camPos			= {0,0,0};

	// get the direction we're looking at
	math3dSubtractVectors(&dir, &_lookAt, &cam);

	// normalize it
	mathReallyFastNormalize(dir);

	//dir.z *= -1;
	cam.z *= -1;

	if(GetAbsoluteBonePosition("COM", &camPos, &cam))
	{
		FLOAT64VECTOR& physicsOffset	= physicsGetWorldOffset();

		camPos.x -= physicsOffset.x;
		camPos.y -= physicsOffset.y;
		camPos.z -= physicsOffset.z;
	}

	// need to rotate dir by veh
	math3dVectorMultiply(&dir, &dir, &GetGameVehicleData()->VehicleAttMat);

	// normalize it again
	mathReallyFastNormalize(dir);

	dir.z *= -1;
	//camPos.z *= -1;

	dGeomID ray = dCreateRay(0, 5000);

	if(ray)
	{
		dGeomRaySet(ray, camPos.x, camPos.y, camPos.z, dir.x, dir.y, dir.z);
		dGeomSetCollideBits(ray, TERRAIN_COLLIDE_MASK);
		dGeomSetData(ray, this);
		dSpaceCollide2(ray, (dGeomID)physicsStaticSpace(), this, _aimCollider);
	}
	*/

	/*
	// gluUnproject code below

	GLdouble sx, sy, sz, px, py, pz;
	sx = sy = sz = 0;

	px = (GLdouble)1024/2;
	py = (GLdouble)768/2;
	pz = 0;

	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	if(gluUnProject(px, py, pz, modelMatrix, projMatrix, viewport, &sx, &sy, &sz))
	{
		int stop = 0;
	}
	*/

	// gluProject code below
	/*
	piVECTOR offset = {0,0,0};

	offset.x = _eyeMatrix._41;
	offset.y = _eyeMatrix._42;
	offset.z = _eyeMatrix._43;

	if(GetAbsoluteBonePosition("COM", &_aimPosition, &offset))
	{
		FLOAT64VECTOR& physicsOffset = physicsGetWorldOffset();
		piVECTOR lookDir;

		mathSubVectors(&lookDir, &_lookAt, &offset);
		mathReallyFastNormalize(lookDir);

		mathVectorMultiply64(&position, &flashPosition, vehAttMat);

		_aimPosition.x -= physicsOffset.x;
		_aimPosition.y -= physicsOffset.y;
		_aimPosition.z -= physicsOffset.z;


	}
	_eyeMatrix
	*/

	/*
	GLdouble sx, sy, sz;
	sx = sy = sz = 0;

	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	FLOAT64VECTOR point;

	point.x = (FLOAT64)_lookAt.x;
	point.y = (FLOAT64)_lookAt.y;
	point.z = (FLOAT64)_lookAt.z;

	if(IsFirstPerson())
	{
		point.x += _vehicle->ModelData->VehiclePos.x;
		point.y += _vehicle->ModelData->VehiclePos.y;
		point.z += _vehicle->ModelData->VehiclePos.z;
	}

	if(gluProject(point.x, point.y, point.z, modelMatrix, projMatrix, viewport, &sx, &sy, &sz))
	{
		pVector->x = (FLOAT32)sx;
		pVector->y = (FLOAT32)sy;
		pVector->z = (FLOAT32)sz;
	}
	*/
}

void MechVehicle::CalculateHUDTargetInfo()
{
	// brute force for now
	phui::View* found = uiViewport()->FindView("mechtarget");
	_target.view = (found) ? dynamic_cast<TargetView*>(found): NULL;

	if(!_target.view)
		return;

	if(!_target.vehicle)
	{
		if(_target.view->IsVisible())
			_target.view->Hide();

		return;
	}

	if(!_target.view->IsVisible())
		_target.view->Show();

	static FLOAT32 points[24]; // 8 points * 3 coords (x,y,z)

	float sx, sy, sz, x1, x2, y1, y2;
	sx = sy = sz = x1 = x2 = y1 = y2 = 0;

	// get our game data
	OBJECT* o = _target.vehicle->GetGameObject();

	// get our bbox min and max
	piVECTOR bl = o->bbox[1];
	piVECTOR tr = o->bbox[0];

	// temp vectors
	piVECTOR final;

	// get them on the same vertical plane as us
	bl.z *= -1;
	tr.z *= -1;

	// x				y					z
	points[0] = bl.x;	points[1] = bl.y;	points[2] = bl.z;	// point 1 bottom back left
	points[3] = tr.x;	points[4] = bl.y;	points[5] = bl.z;	// point 2 bottom back right
	points[6] = tr.x;	points[7] = tr.y;	points[8] = bl.z;	// point 3 bottom front right
	points[9] = bl.x;	points[10] = tr.y;	points[11] = bl.z;	// point 4 bottom front left
	points[12] = bl.x;	points[13] = bl.y;	points[14] = tr.z;	// point 5 top back left
	points[15] = tr.x;	points[16] = bl.y;	points[17] = tr.z;	// point 6 top back right
	points[18] = tr.x;	points[19] = tr.y;	points[20] = tr.z;	// point 7 top front right
	points[21] = bl.x;	points[22] = tr.y;	points[23] = tr.z;	// point 8 top front left

	// reset the world!
	piSetWorldMatrix(&identity);

	for(int i = 0; i < 24; i += 3)
	{
		final.x = points[i];
		final.y = points[i+1];
		final.z = points[i+2];

		// rotate the point
		math3dVectorMultiply(&final, &final, &o->mat);

		if(GetWorldToScreen(&final, &sx, &sy, &sz))
		{
			if(!i)
			{
				x1 = x2 = sx;
				y1 = y2 = sy;
			}
			else
			{
				if(sx < x1) x1 = sx;
				else if(sx > x2) x2 = sx;
				if(sy < y1) y1 = sy;
				else if(sy > y2) y2 = sy;
			}
		}
	}

	_target.width	= x2 - x1;
	_target.height	= y2 - y1;
	_target.x		= x1;
	_target.y		= y1;
	_target.z		= sz;

	_target.view->MoveTo(_target.x, _target.y);
	_target.view->ResizeTo(_target.width, _target.height);

	if(_target.z > 1.0f) // behind us
	{
		_target.view->Hide();
	}
	else // set the color based on side
	{
		if(_vehicle->Side == _target.vehicle->GetGameVehicle()->Side)
			_target.view->SetTextColor(0, 1, 0, 1);
		else
			_target.view->SetTextColor(1, 0, 0, 1);
	}
}

void MechVehicle::ClearTargeter()
{
	_targeter = NULL;
}

void MechVehicle::SetTargeter(MechVehicle* by, TARGET_DESTROYED_CALLBACK callback)
{
	_targeter = by;
	_targeterCallback = callback;
}

void MechVehicle::OnTargetDestroyed()
{
	_target.vehicle = NULL;
	uiRunScript("game.oncleartarget()");
}

void MechVehicle::ClearTarget()
{
	if(_target.vehicle)
	{
		MechVehicle* mech = dynamic_cast<MechVehicle*>(_target.vehicle);
		if(mech) mech->ClearTargeter();
		_target.vehicle = NULL;
	}

	uiRunScript("game.oncleartarget()");
}

UINT32 MechVehicle::GetCollisionTimeLeft() const
{
	int left = ((_spawnTime + 60000) - frametime)/1000;
	return (left > 0) ? (UINT32)left: 0;
}

FLOAT32 MechVehicle::GetAccuracy() const
{
	return (_traverseAccuracy + _elevateAccuracy)/2;
}

const MechVehicle::MechTarget& MechVehicle::GetMechTarget() const
{
	return _target;
}

void MechVehicle::FindMechTarget(int side)
{
	vehVehicle* NextTarget	= NULL;
	gr2Vehicle* BestTarget	= NULL;
	float BestKey			= 50000.0f;

	FLOAT64VECTOR camPos, tarPos, tarDir;
	piVECTOR temp;

	// get the vehicle list head
	vehGetVehicleHead(&NextTarget);

	// reset the last target if need be
	if(_target.vehicle)
	{
		MechVehicle* mech = dynamic_cast<MechVehicle*>(_target.vehicle);
		if(mech) mech->ClearTargeter();
		_target.vehicle = NULL;
	}

	// only calculate this stuff if we're boresight targeting
	if(!side)
	{
		temp.x = _eyeMatrix._41;
		temp.y = _eyeMatrix._42;
		temp.z = _eyeMatrix._43;

		// lookup the
		GetAbsoluteBonePosition("COM", &camPos, &temp);
	}

	// now lets look for our next target
	for(;NextTarget; NextTarget = NextTarget->Next)
	{
		// I can't be my own target
		if(NextTarget == _vehicle) continue;

		gr2Vehicle* vehicle = gr2GetVehicle(NextTarget);

		// only interested in gr2 vehicles for now
		if(!vehicle) continue;

		if(side) // side matching
		{
			// skip this vehicle if its not the side we're looking for
			if(NextTarget->Side == side)
			{
				FLOAT32 distance = vehicle->GetRangeToPlayer();

				// if its less than our current best, use this target
				if(!BestTarget || distance < BestKey)
				{
					BestTarget = vehicle;
					BestKey = distance;
				}
			}
		}
		else // boresight matching
		{
			// grab its world position
			if(vehicle->GetAbsoluteBonePosition("COM", &tarPos))
			{
				// get the vector from our cam position to the vehicle
				mathSubVectors(&tarDir, &camPos, &tarPos);
				mathNormalize(&tarDir);

				temp.x = (float)tarDir.x;
				temp.y = (float)tarDir.y;
				temp.z = (float)tarDir.z;

				// get the angle to the vehicle
				FLOAT32 angle = 360 - RAD_TO_DEG(acos(mathVectDotProduct(&_aimDirection, &temp)));

				// if its less than our current best, use this target
				if(!BestTarget || angle < BestKey)
				{
					BestTarget = vehicle;
					BestKey = angle;
				}
			}
		}
	}

	// if we found a best target, set it up
	if(BestTarget)
	{
		_target.vehicle = BestTarget;

		// set the destroy handler in case it despawns
		MechVehicle* mech = dynamic_cast<MechVehicle*>(_target.vehicle);
		if(mech) mech->SetTargeter(this, &MechVehicle::OnTargetDestroyed);

		// let the UI know
		uiRunScript("game.onnewtarget()");
	}
}

void MechVehicle::UpdateTurrets()
{
	if(IsFirstPerson() && !_camera.freelook)
	{
		mlVehicleData* data = GetGameVehicleData();
		bool negateTraverse	= (_camera.traverse < 0);
		bool negateElevate	= (_camera.elevate >= 0);

		for(int i = 0; i < (int)_turrets.size(); i++)
		{
			gr2Turret& turret = _turrets.at(i);

			if(turret.bone < 0) continue;

			FLOAT64VECTOR bone;

			if(!GetAbsoluteBonePosition(turret.bone, &bone))
				continue;

			FLOAT64VECTOR aimDir, mechFacing;
			piVECTOR facingDir, finalDir;

			// get a direction vector to the aim point from the bone
			mathSubVectors(&aimDir, &_aimPosition, &bone);
			//mathNormalize(&aimDir);

			mechFacing.x = 0;
			mechFacing.y = 1;
			mechFacing.z = 0;

			mathVectorMultiply64(&mechFacing, &mechFacing, &data->VehicleAttMat);

			facingDir.x = (float)mechFacing.x;
			facingDir.y = (float)mechFacing.y;
			facingDir.z = 0;

			math3dNormalize(&facingDir);

			// lets handle traverse rotations
			if(turret.spec->traverse.tid >= 0)
			{
				float angle = turret.manualTraverse;

				if(turret.spec->traverse.synced)
				{
					finalDir.x = (float)aimDir.x;
					finalDir.y = (float)aimDir.y;
					finalDir.z = 0;

					math3dNormalize(&finalDir);

					angle = RAD_TO_DEG(acos(mathVectDotProduct(&facingDir, &finalDir)));
				}

				float traverse	= (angle / 180.0f) * .5f;
				UINT32 tid		= turret.spec->traverse.tid;

				if(negateTraverse)
				{
					traverse *= -1;
					angle *= -1;
				}

				if(!_camera.freelook && !_camera.sync && HasCondition(MV_CONDITION_TURRET_DISABLED))
					_camera.traverse = angle;

				data->WantedTurretTraverse[tid] = traverse;
				data->TurretTraverseDeltaInput[tid] = 1;

				if(data->WantedTurretTraverse[tid] != data->TurretTraverse[tid])
				{
					data->TurretTraverseRate[tid] = ((traverse < data->TurretTraverse[tid]) ? -1: 1) * turret.efficiency;

					float diff = (float)fabs((data->WantedTurretTraverse[tid] < data->TurretTraverse[tid]) ? data->TurretTraverse[tid] - data->WantedTurretTraverse[tid]: data->WantedTurretTraverse[tid] - data->TurretTraverse[tid]);

					_traverseAccuracy = (diff <= .5f) ? 1.0f - (diff/.5f): 0;
				}
				else
				{
					data->TurretTraverseRate[tid] = 0;
					_traverseAccuracy = 1.0f;
				}

				static int printtTime = 0;

				if(false && frametime > printtTime)
				{
					uiChatPrintLine("Turret Traverse is %.2f,%.2f", _camera.traverse, angle);
					printtTime = frametime + 500;
				}
			}

			if(turret.spec->elevate.tid >= 0)
			{
				float angle = turret.manualElevate;

				if(turret.spec->elevate.synced)
				{
					negateElevate = (aimDir.z < 0) ? true: false;

					//mathVectorMultiply64(&mechFacing, &mechFacing, &data->VehicleAttInvMat);
					mathVectorMultiply64(&aimDir, &aimDir, &data->VehicleAttInvMat);

					facingDir.x = 0; //(float)mechFacing.x;
					facingDir.y = 1; //(float)mechFacing.y;
					facingDir.z = 0; //(float)mechFacing.z;

					//math3dNormalize(&facingDir);

					finalDir.x = (float)aimDir.x;
					finalDir.y = (float)aimDir.y;
					finalDir.z = (float)aimDir.z;

					math3dNormalize(&finalDir);

					float z = finalDir.z;
					finalDir.z = 0;

					double	dp		= mathVectDotProduct(&facingDir, &finalDir);
					double ac		= acos(dp);
					angle			= (float)RAD_TO_DEG(ac);

					piMATRIX zRot = identity;
					GenerateRotation(2, (finalDir.x > 0) ? -angle: angle, &zRot);

					math3dVectorMultiply(&finalDir, &finalDir, &zRot);

					finalDir.z		= z;

					//math3dNormalize(&finalDir);

					dp				= mathVectDotProduct(&facingDir, &finalDir);
					ac				= acos(dp);
					angle			= (float)RAD_TO_DEG(ac);
				}

				float elevate	= (angle / 180.0f) * .5f;
				UINT32 tid		= turret.spec->elevate.tid;

				if(negateElevate)
				{
					elevate *= -1;
					angle *= -1;
				}

				if(!_camera.freelook && !_camera.sync && HasCondition(MV_CONDITION_TURRET_DISABLED))
					_camera.elevate = angle;

				data->WantedTurretElevate[tid] = elevate;
				data->TurretElevateDeltaInput[tid] = 1;

				if(data->WantedTurretElevate[tid] != data->TurretElevate[tid])
				{
					data->TurretElevateRate[tid] = ((elevate < data->TurretElevate[tid]) ? -1: 1) * turret.efficiency;

					float diff = (float)fabs((data->WantedTurretElevate[tid] < data->TurretElevate[tid]) ? data->TurretElevate[tid] - data->WantedTurretElevate[tid]: data->WantedTurretElevate[tid] - data->TurretElevate[tid]);

					_elevateAccuracy = (diff <= .5f) ? 1.0f - (diff/.5f): 0;
				}
				else
				{
					data->TurretElevateRate[tid] = 0;
					_elevateAccuracy = 1.0f;
				}

				static int printeTime = 0;
				static float el = 0;

				if(false && frametime > printeTime && el != _camera.elevate)
				{
					el = _camera.elevate;
					uiChatPrintLine("Turret Elevate is %.2f,%.2f,%s", _camera.elevate, angle, (negateElevate) ? "true": "false");
					printeTime = frametime + 200;
				}
			}
		}

		/*
		// we should calculate the traverse and elevate angle for each turret

		mlVehicleData* data = GetGameVehicleData();
		FLOAT64 traverse = (_camera.traverse / 180.0f) * .5f;
		FLOAT64 elevate = -((_camera.elevate /180.0f) * .5f);

		for(int i = 0; i < (int)_turrets.size(); i++)
		{
			gr2Turret& turret = _turrets.at(i);

			if(turret.bone < 0) continue;

			// lets handle traverse rotations
			if(turret.spec->traverse.type == gr2TurretRotationSpec::TURRET_TYPE_TRAVERSE)
			{
				UINT32 tid = turret.spec->traverse.tid;

				data->WantedTurretTraverse[tid] = traverse;
				data->TurretTraverseDeltaInput[tid] = 1;

				if(data->WantedTurretTraverse[tid] != data->TurretTraverse[tid])
					data->TurretTraverseRate[tid] = (traverse < data->TurretTraverse[tid]) ? -1: 1;
				else
					data->TurretTraverseRate[tid] = 0;
			}

			if(turret.spec->elevate.type == gr2TurretRotationSpec::TURRET_TYPE_ELEVATE)
			{
				UINT32 tid = turret.spec->elevate.tid;

				data->WantedTurretElevate[tid] = elevate;
				data->TurretElevateDeltaInput[tid] = 1;

				if(data->WantedTurretElevate[tid] != data->TurretElevate[tid])
					data->TurretElevateRate[tid] = (elevate < data->TurretElevate[tid]) ? -1: 1;
				else
					data->TurretElevateRate[tid] = 0;
			}
		}

		FLOAT64VECTOR bone;
		piVECTOR offset;

		offset.x = _eyeMatrix._41;
		offset.y = _eyeMatrix._42;
		offset.z = _eyeMatrix._43;

		if(GetAbsoluteBonePosition("Bone_Torso", &bone, &offset))
		{
			FLOAT64VECTOR aimDir;
			piVECTOR finalDir, facingDir;

			// get a direction vector to the aim point from the bone
			mathSubVectors(&aimDir, &_aimPosition, &bone);

			finalDir.x = (float)aimDir.x;
			finalDir.y = (float)aimDir.y;
			finalDir.z = (float)aimDir.z;

			facingDir.x = 0;
			facingDir.y = 1;
			facingDir.z = 0;

			mathReallyFastNormalize(finalDir);

			// we want the xy plane for traverse
			aimDir.z = 0;

			float angle = RAD_TO_DEG(acos(mathVectDotProduct(&finalDir, &facingDir)));
			int stop = 1;
		}
		*/
	}

	gr2Vehicle::UpdateTurrets();
}

void MechVehicle::HandleAimContact(dGeomID ray, dGeomID hit)
{
	if(hit == _collider) return;

	const int MAX_CONTACTS = 3;
	dContact contacts[MAX_CONTACTS];
	FLOAT64VECTOR& offset = physicsGetWorldOffset();

	int numc = dCollide(ray, hit, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

	for (int i = 0; i < numc; i++)
	{
		dContact* contact = &contacts[i];

		if(contact->geom.depth < _aimDistance)
		{
			_aimPosition.x	= contact->geom.pos[0] + offset.x;
			_aimPosition.y	= contact->geom.pos[1] + offset.y;
			_aimPosition.z	= contact->geom.pos[2] + offset.z;
			_aimDistance	= (FLOAT32)contact->geom.depth;
			_hitDistance	= _aimDistance;
			_aimChanged		= true;
		}

		break;
	}
}

void MechVehicle::SetFalling(bool yesno)
{
	if(yesno)
		SetCondition(MV_CONDITION_FALLING);
	else
		ClearCondition(MV_CONDITION_FALLING);
}

bool MechVehicle::IsFalling()
{
	return HasCondition(MV_CONDITION_FALLING);
}

void MechVehicle::SetMaxTurn(float mt)
{
	_absMaxTurn = mt;
}

float MechVehicle::GetMaxTurn(bool fromSpec) const
{
	return _absMaxTurn;
}

UINT32 MechVehicle::GetSharedMask()
{
	return MV_SHARED_CONDITIONS;
}

UINT32 MechVehicle::GetPower()
{
	return _power;
}

UINT32 MechVehicle::GetMaxPower()
{
	return _maxPower;
}

bool MechVehicle::ProcessPower(SINT32 amount, UINT32 minpower, UINT32 maxpower)
{
	if((minpower && _power < minpower) || (maxpower && _power > maxpower))
		return false;

	if(amount < 0) // taking power
	{
		if((SINT32)_power + amount < 0) // not enough power
		{
			_power = 0;
			return false;
		}
		else
		{
			_power += amount;
			return true;
		}
	}
	else // giving power
	{
		_power += amount;

		if(_power > _maxPower)
			_power = _maxPower;

		return true;
	}
}

bool MechVehicle::HasPower(UINT32 amount)
{
	return (amount >= _power);
}

void MechVehicle::InitJSProps(jsGR2Object* ui, int index)
{
	// our property names
	char* propNames[] =
	{
		"speed", "throttle", "power", "maxpower", "accuracy",
		"grace", "range", "callsign", "sameside", "lift"
	};

	JSPropertySpec spec;

	for(int i = 0; i < MechVehicle::TOTAL_PROPS; i++)
	{
		spec.name		= propNames[i];
		spec.tinyid		= i+index;
		spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		ui->AddGR2Property(spec);
	}

	gr2Vehicle::InitJSProps(ui, MechVehicle::TOTAL_PROPS+index);
}

JSBool MechVehicle::GetJSProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case SPEED_PROP:
		{
			*val = INT_TO_JSVAL(0);
		}
		break;
		case THROTTLE_PROP:
		{
			JS_NewDoubleValue(ctx, _throttle, val);
		}
		break;
		case POWER_PROP:
		{
			*val = INT_TO_JSVAL(_power);
		}
		break;
		case MAXPOWER_PROP:
		{
			*val = INT_TO_JSVAL(_maxPower);
		}
		break;
		case ACCURACY_PROP:
		{
			JS_NewDoubleValue(ctx, GetAccuracy(), val);
		}
		break;
		case GRACE_PROP:
		{
			*val = INT_TO_JSVAL(GetCollisionTimeLeft());
		}
		break;
		case RANGE_PROP:
		{
			*val = INT_TO_JSVAL((int)GetRangeToPlayer());
		}
		break;
		case CALLSIGN_PROP:
		{
			*val = phui::js::NewString((const char*)_vehicle->Handle);
		}
		break;
		case SAMESIDE_PROP:
		{
			gr2Vehicle* persona = gr2GetPersonaVehicle();
			*val = BOOLEAN_TO_JSVAL(persona->GetGameVehicle()->Side == _vehicle->Side);
		}
		break;
		case LIFT_PROP:
		{
			*val = INT_TO_JSVAL((int)_currentLift);
		}
		break;
		default:
			return gr2Vehicle::GetJSProperty(ctx, propid-MechVehicle::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

gr2Vehicle* MechVehicle::CreateMech(vehVehicle* vehicle, OBJECT* pObject)
{
	return new MechVehicle(vehicle, pObject);
}

void _aimCollider(void* data, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2((dGeomID)o1, (dGeomID)o2, data, _aimCollider);
		return;
	}

	if(data)
	{
		MechVehicle* vehicle = (MechVehicle*)data;

		if(dGeomGetClass(o1) == 5)
			vehicle->HandleAimContact(o1, o2);
		else if(dGeomGetClass(o2) == 5)
			vehicle->HandleAimContact(o2, o1);
	}
}
