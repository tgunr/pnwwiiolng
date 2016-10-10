#include <syscfg.h> // Path is project specific.

#include "client/GameClient.h"
#include "draw/draw2d.h"
#include "draw/draw3d.h"
#include "ez/EZManager.h"
#include "fio/fio.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSinternal.h"
#include "fp/fpMain.h"
#include "fp/fpPPO.h"
#include "fp/fpTPSPhysics.h"
#include "fx/FXInterface.h"
#include "fx/fxMgr.h"
#include "fx/fxMisc.h"
#include "fx/fxSnd.h"
#include "fx/postRender.h"
#include "gr2/gr2Anim.h"
#include "gr2/gr2Main.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "include/vars.h"
#include "locl/loclMain.h"
#include "map/mapArenaDescription.h"
#include "map/mapPersonaSummary.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "math/mathvect.h"
#include "math/Matrixd.h"
#include "ml/mlgauges.h"
#include "ml/mlhead.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "ob/obdraw.h"
#include "ob/obmain.h"
#include "ob/obMgr.h"
#include "ob/obpool.h"
#include "ogl/oglDefs.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/ogl.h"
#include "ogl/oglMaterial.h"
#include "ogrp/ogrptype.h"
#include "pers/persbombsight.h"
#include "pers/persgunsight.h"
#include "pers/pershud.h"
#include "pers/persmain.h"
#include "pers/persmsg.h"
#include "pers/persrank.h"
#include "pi/pi.h"
#include "rn/rnmain.h"
#include "scen/ScenarioManager.h"
#include "sdm/StratDB.h"
#include "shader/glsl.h"
#include "spt/Random.h"
#include "trn/trncd.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"
#include "trn/trnsky.h"
#include "trn/trnvars.h"
#include "tut/TutorialManager.h"
#include "uimap/uiMap.h"
#include "ui/uiChat.h"
#include "ui/uiGameEvents.h"
#include "ui/uiMain.h"
#include "veh/vehanim.h"
#include "veh/vehcntrl.h"
#include "veh/vehmain.h"
#include "veh/vehupdate.h"
#include "veh/vehVisDamage.h"
#include "wwii/wwiiServerTrackedObject.h"		// The base STO type

#include "dip.h"
#include "strt/strtCPFlags.h"

#define DEFAULT_INTERNAL_FOV 90.0
#define DEFAULT_EXTERN_FOV 90.0

#define _INTERNAL_CAMERA 0
#define _EXTERNAL_CAMERA 1

#define _EXTERNAL_VIEW_DISTANCE 50 * ScaleFactor
#define _EXTERNAL_VIEW_DISTANCE_RUBBER_BAND 20 * ScaleFactor

//#ifdef BETA_RELEASE
#define _EXTERN_VIEW_SLEW_RATE (FLOAT32)mlDEG_TO_RAD(135)
//#else
//#undef _EXTERN_VIEW_SLEW_RATE
//#endif

#define _EXTERNAL_LOCKED 0
#define _EXTERNAL_RUBBERBAND 1
#define _EXTERNAL_FIXED 2

#define _INTERNAL_NORMAL 0
#define _INTERNAL_INSTRUMENT 1
#define _INTERNAL_GUNSIGHT 2

// ===========
// persona health defines

#define _BRAIN_DAMAGE_PER_JOULE 0.025f
#define _BRAIN_GAIN_AT_NEUTRAL_G 0.1f
#define _BRAIN_GAIN_AT_ONE_GEE_OFF 0.0f
#define _BRAIN_GAIN_EACH_ADDITIONAL_POSG -0.1f
#define _BRAIN_GAIN_EACH_ADDITIONAL_NEGG -0.3f
#define _BRAIN_WHITEOUT_START 0.3f
#define _BRAIN_BLACKOUT_FULL 0.5f
#define _BRAIN_BRAINDAMAGE 0.8f
#define _BRAIN_STRENGTH_STARTLOSS 0.4f
#define _BRAIN_STRENGTH_FULLLOSS 0.7f
#define _BRAIN_NOMORE_CONTROL 0.7f

#define _ATP_RUN_GAIN -0.01f
#define _ATP_TROT_GAIN -0.005f
#define _ATP_WALK_GAIN 0.001f
#define _ATP_STAND_GAIN 0.002f
#define _ATP_SIT_GAIN 0.003f
#define _ATP_LIEDOWN_GAIN 0.004f
#define _ATP_CRAWL_GAIN 0.001f
#define _ATP_INVEHICLE_GAIN 0.003f
//#define _ATP_PULLPOS_3G -0.003f
// 4 g's will break even with ATP gain
#define _ATP_PULLPOS_3G -0.00173f
#define _ATP_PULLNEG_3G -0.03f
#define _ATP_INJURY_HEAD_PER_JOULE 0.025f
#define _ATP_INJURY_TORSO_PER_JOULE 0.0033333f
#define _ATP_INJURY_ARMS_PER_JOULE 0.0016666f
#define _ATP_INJURY_LEGS_PER_JOULE 0.0022222f
#define _ATP_INJURY_TORSOHEAD_BOOST_PER_JOULE -0.01f
#define _ATP_INJURY_ARMSLEGS_BOOST_PER_JOULE 0.01f
#define _ATP_INJURY_ARMSLEGS_BOOST_RECOVERY 0.001f
#define _ATP_INJURY_INSTANTDROP_PER_JOULE -0.01f
#define _ATP_INJURY_INSTANTDROP_REGAIN 0.1f
#define _ATP_MIN_REQUIRED_FOR_ACTION 0.8f

#define _MAX_BLOODPRESSURE 4.5f // 4.5 g max adjust
#define _MIN_BLOODPRESSURE -0.5f // -0.5 g min adjust
#define _BP_LAG_MIN_TO_MAX 2.0f // 2 seconds to get from 0.0 to 4.5 gs
#define _BP_LAG_MAX_TO_MIN 0.1f // 0.1 seconds to recover from 4.5 gs

#define _BLOOD_LOSS_PER_JOULE_HEAD 0.0125f
#define _BLOOD_LOSS_PER_JOULE_TORSO 0.0016666f
#define _BLOOD_LOSS_PER_JOULE_ARMS 0.000833333f
#define _BLOOD_LOSS_PER_JOULE_LEGS 0.0011111f
#define _BLEED_RATE_HALFLIFE 30.0f // ten seconds to bleed to new blood level unless catastrophic

#define _REDOUT_RED 0.7f
#define _REDOUT_GREEN 0.094f
#define _REDOUT_BLUE 0.008f

#define DEF_MAX_DEATH_CAM   40.0f

#define SHAKE_HEAD_DURATION 1200

#define HEARTBEAT_VOLUME  .05f
#define HEARTBEAT_DURATION  3100
#define HEARTBEAT_THRESHOLD .6f

#define BREATH_THRESHOLD .75f
#define BREATH_VMIN  .0005f
#define BREATH_VMAX .005f
#define BREATH_DURATION 780
#define BREATH_TMIN 500
#define BREATH_TMAX 1200
#define BREATH_CHECK 1000
#define BREATH_SCALE 10
#define BREATH_ADD 3


// ===========

// horrible hack for vehicle analysis and floaters: from mlMain.cpp
extern char	buffer[256];
extern char lastbuffer[256];

static ref_ptr<oglMaterial> basiccolor;

static UINT32 _UpdateHeadAndView(void);
static void _InstallViewControlCallbacks(void);
static UINT32 _GetExternalCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att);
static UINT32 _GetInternalCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att);
static UINT32 _SlewView(SINT32 Param,UINT32 Id,UINT32 Detach);
static UINT32 _RollViewLeft(SINT32 Param,UINT32 Id,UINT32 Detach);
static UINT32 _RollViewRight(SINT32 Param,UINT32 Id,UINT32 Detach);
static UINT32 _AdjustViewAngle(piVECTOR * Att);
static UINT32 _GetPlayerPos(piVECTOR * Pos,piMATRIX * AttMat);
static UINT32 _AimCameraAtPosition(piVECTOR * Att,piVECTOR * Pos,piVECTOR * AimPnt);
static UINT32 _SetExtFOV(SINT32 Param, UINT32 Id, UINT32 Detach);
static UINT32 _SetInstrumentsView(SINT32 Param, UINT32 Id, UINT32 Detach);
static UINT32 _SetGunsightsView(SINT32 Param, UINT32 Id, UINT32 Detach);
static UINT32 _SetInstrumentView(UINT32 ViewNum);
static UINT32 _SetGunsightView(UINT32 ViewNum);
static UINT32 _SetNormalInternalView(void);
static LOGICAL _DrawGunsightTree(OBJECT * Object,SINT32 DoSibs);

static UINT32 _UpdateExternalCameraTarget(bool start = false);
static UINT32 _UpdateCockpitCameraPos(void);
static UINT32 _UpdatePersonaHealths(void);
static UINT32 _UpdatePersonaSounds(void);
static LOGICAL _CheckForBlocks(UINT32 Type,SINT32 Position);
static LOGICAL _BlockView(void);

static UINT32 _SetGunsightToUserDraw(OBJECT * Object,UINT32 IncludeSibs);
static LOGICAL _DrawGunsightObject(OBJECT * Object);

#if(0)
static UINT32 _DrawRedoutBlackoutOld(void);
#endif
static UINT32 _DrawRedoutBlackoutNew(void);
static UINT32 _CheckUnderground(void);

static int _DoResupplyCheck(void);
static UINT32 _DoScenarioResupplyCheck(void);
static UINT32 _DoGroundResupplyCheck(void);
static UINT32 _DoStaticResupplyCheck(void);
LOGICAL _IsValidResupplyVehicle(const vehVehicle* player, const vehVehicle* resupplier, FLOAT64VECTOR* lastPos);
LOGICAL _IsValidResupplyBuilding(StratDB::StratObject* resupplier);
StratDB::StratObject* _LookupResupplyBuilding();

static void _playheartbeat(FLOAT32 volume, SINT32 duration);
static void _playbreathing(FLOAT32 volume, FLOAT32 pitch, SINT32 duration);
static void _playsound(const char* path, SINT32& id, FLOAT32 volume, FLOAT32 pitch, SINT32 duration);

static FLOAT32 _VehicleG;

static persPersona _Persona;
static UINT32 _UseGyroStabilizedEyes = 0;
UINT32 _CameraMode;
static ExternalCameraAdjust _ExternalCameraAdjuster = nullptr;
static ExternalCameraPivot _ExternalCameraPivot = nullptr;

static UINT32 _ExternalViewMode;
static UINT32 _InternalViewMode;
//static piVECTOR _ExternalFixedCameraPos;
static UINT32 _CameraNeverSet;
static UINT32 _InDeathCam;

static FLOAT32 _FogScaleMultiplier = 1.0f;

static piVECTOR _ViewAtt;
static piVECTOR _CameraPos;
static piVECTOR _ViewTarget;
static piVECTOR _ViewTargetMove;

static SINT32 _SlewViewLRHeldDown; // left right
static SINT32 _SlewViewUDHeldDown; // up down
static SINT32 _SlewViewIOHeldDown; // in out

static FLOAT32 _ExternalFieldOfView;
static FLOAT32 _ExternalZoom = -20.0f;
static FLOAT32 _InternalFieldOfView = DEFAULT_INTERNAL_FOV;
static int _shaketime = 0;
static int _shakeduration = 0;
static SINT32 _exitTime = 0;

static SINT32 _resupplyCheck;	// when to check next
static SINT32 _resupplyAirCheck;	// when to check airplanes next
static SINT32 _resupplyBreak;	// if set, don't do any resupplying until the break is over (after firing, or when full)
static SINT32 _resupplyTick;	// resupply interval
static SINT32 _resupplyAirTick;	// _resupplyTick for airplanes
static SINT32 _resupplyPause;	// delay before starting
static SINT32 _resupplyRadius; // how close you need to be to a truck in meters
static SINT32 _resupplyRadiusAir; // how close you need to be to a airfield in meters
static SINT32 _resupplyRadiusSea; // how close you need to be to a port in meters
static int _resupplyState;		// whats our current status

static SINT32 _targetBreaths = 0;
static SINT32 _breaths = 0;


static OBJECT * _GenericHead; // head object used for granny viewpoints

static bool doDeathCamOnNextUpdate = false;

static persPersonaHealth _PersonaHealth[ML_MAX_CREW_POSITIONS];

#ifdef BETA_RELEASE
#define HEALTH_HISTORY_SIZE 600
static persPersonaHealth _PersonaHealthHistory[ML_MAX_CREW_POSITIONS][HEALTH_HISTORY_SIZE];
static FLOAT32 _GHistory[HEALTH_HISTORY_SIZE];
#endif

static FLOAT32 g_fShakeMagnitude = 0.0f;

//////////////////////////////////////////////////////////////////////
// Flush the parameters in a persPersonaHealth structure (which are
// now all coverted)

void
persPersonaHealth::clear()
{
	BrainOxyLevel = 0;
	BrainOxyMaxLevel = 0;
	ATPLevel = 0;
	ATPCurrentLevel = 0;
	ATPMaxLevel = 0;
	BloodPressure = 0;
	BloodLevel = 0;
	BloodCurrentLevel = 0;
	PayAttentionToPersona = FALSE;
}

vehVehicle* Player::GetVehicle()
{
	return playerGetVehicle();
}

void Player::ShakeHead()
{
	persShakeHead();
}

float Player::GetEnergy() const
{
	float BrainOxy, ATPLevel, BloodPressure, BloodLevel;
	persGetHealthStats(&BrainOxy,&ATPLevel,&BloodPressure,&BloodLevel);
	return ATPLevel;
}

float Player::GetHealth() const
{
	float BrainOxy, ATPLevel, BloodPressure, BloodLevel;
	persGetHealthStats(&BrainOxy,&ATPLevel,&BloodPressure,&BloodLevel);
	return BloodLevel;
}

int Player::GetCountry() const
{
	return (int)PlayerClient()->PersonaSelected().m_countryID;
}

int Player::GetSide() const
{
	return (PlayerClient()->PersonaSelected().m_countryID == 4) ? 2: 1;
}

int Player::GetRangeSetting() const
{
	int range = 200;
	vehVehicle* vehicle = playerGetVehicle();

	if(vehicle && vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		fpFPSGetHUDAmmoData(HUD_AMMO_DATA_RANGE, &range);

	return range;
}

Player* playerGetPlayer()
{
	static Player player;
	return &player;
}

vehVehicle* playerGetVehicle()
{
	return persGetPersonaVehiclePtr();
}

UINT32 persInit(void)
{
	_ExternalFieldOfView = DEFAULT_EXTERN_FOV;

	_GenericHead = nullptr;

	persmsgInit();
	persrankLoad("ranklist.xml", (xmlInputOpenCallback)fioOpen, (xmlInputReadCallback)fioRead, (xmlInputCloseCallback)fioClose);

	// setup resupply variables
	_resupplyCheck		= 0;
	_resupplyAirCheck	= 0;
	_resupplyBreak		= 0;
	_resupplyTick		= 5000;
	_resupplyAirTick	= 4 * _resupplyTick;
	_resupplyPause		= 20000;
	_resupplyRadius		= 10; // in meters
	_resupplyRadiusAir	= 500; // in meters
	_resupplyRadiusSea	= 800; // in meters
	_resupplyState		= GAMEEVENT_RESUPPLY_NA; // are we in range of the truck?

	ARENA_VARS::iterator ait = g_arenaDescription.find("resupply_tick");
	if(ait != g_arenaDescription.end())
		_resupplyTick = atoi(ait->second.c_str());

	ait = g_arenaDescription.find("resupply_airtick");
	if(ait != g_arenaDescription.end())
		_resupplyAirTick = _resupplyTick * atoi(ait->second.c_str());

	ait = g_arenaDescription.find("resupply_pause");
	if(ait != g_arenaDescription.end())
		_resupplyPause = atoi(ait->second.c_str());

	ait = g_arenaDescription.find("resupply_radius");
	if(ait != g_arenaDescription.end())
		_resupplyRadius = atoi(ait->second.c_str());

	ait = g_arenaDescription.find("resupply_radius_air");
	if(ait != g_arenaDescription.end())
		_resupplyRadiusAir = atoi(ait->second.c_str());

	ait = g_arenaDescription.find("resupply_radius_sea");
	if(ait != g_arenaDescription.end())
		_resupplyRadiusSea = atoi(ait->second.c_str());

	// go ahead and square it
	_resupplyRadius		= _resupplyRadius*_resupplyRadius;
	_resupplyRadiusAir	= _resupplyRadiusAir*_resupplyRadiusAir;
	_resupplyRadiusSea	= _resupplyRadiusSea*_resupplyRadiusSea;

	// init default external cam position
	persSetExternalCameraPos(0.0f, 20.0f, 15.0f, false);

	if(!ogl::UseDeprecatedPath())
	{
		glslProgram* pgm = GLSL.getProgram("basiccolor");

		if(!pgm)
		{
			pgm = GLSL.loadProgram("basiccolor", nullptr, nullptr);

			pgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/draw/basiccolor.vert").get());
			pgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/draw/basiccolor.frag").get());

			pgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
			pgm->AddBindAttribLocation("in_color", oglVertexBuffer::ATTRIBUTE_COLOR);

			pgm->Apply();
		}

		basiccolor = new oglMaterial;
		basiccolor->SetMaterial(-1);
		basiccolor->SetShader("basiccolor");
	}

	return 0;
}

UINT32 persKill(void)
{
	persmsgKill();

	if (_GenericHead != nullptr)
	{
		ogrpFreeObjectTree(_GenericHead);
		_GenericHead = nullptr;
	}

	return 0;
}

UINT32 persUpdate(void)
{
	// update the player's vehicle controls (hack for now)
	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->ModelData != nullptr)
		{
			if (_Persona.VehicleAttached->VehicleObjectData != nullptr)
			{
				if (_Persona.VehicleAttached->VehicleObjectData->PlaneObject != nullptr)
				{
					if(_Persona.VehicleAttached->VehicleType->Type != VEH_TYPE_FLOATER)
					{
						mlUpdate(_Persona.VehicleAttached->ModelData,
								 _Persona.VehicleAttached->VehicleObjectData->PlaneObject);
					}

					if (_CameraMode == _INTERNAL_CAMERA && _InternalViewMode == _INTERNAL_GUNSIGHT)
					{
						persUpdateGunsightView();
					}
					else if (_CameraMode == _INTERNAL_CAMERA && _InternalViewMode == _INTERNAL_INSTRUMENT)
					{
						persUpdateInstGunsightView();
					}
					else if(_CameraMode == _EXTERNAL_CAMERA)
					{
						_UpdateExternalCameraTarget();
					}
				}
			}
		}

		if (_Persona.VehicleAttached->VehicleDeadTime != 0 && !_InDeathCam && !vehmainHasBailedOut())
		{
			persSetDeathCam();
		}
	}
	else
	{
		if (_Persona.PainSoundId > 0)
		{
			fxSndStopSound(_Persona.PainSoundId);
			_Persona.PainSoundId = 0;
		}
	}

	_UpdateHeadAndView();

	_UpdateCockpitCameraPos();

	_UpdatePersonaHealths();

	_UpdatePersonaSounds();

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->VehicleType != nullptr)
		{
			if (_Persona.VehicleAttached->VehicleType->BombSightPosition != 0)
			{
				if (_Persona.Position == _Persona.VehicleAttached->VehicleType->BombSightPosition)
				{
					persBombsightUpdate();
				}
			}
		}
	}

	if(uiInGame()) // Online &&
	{
		int nextResupplyState = _DoResupplyCheck();

		if(nextResupplyState != _resupplyState)
			_resupplyState = uiGameEvent(nextResupplyState);
	}

	if(frametime > (_shaketime + _shakeduration))
	{
		persShakeCamera(-1); // reset it
	}
	else
	{
		int diff = frametime - _shaketime;
		float t = 1.0f - float(diff) / float(_shakeduration);

		t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;

		float shake = g_fShakeMagnitude * t;

		if(shake <= 0.0001f)
			persShakeCamera(-1);
	}

	if(doDeathCamOnNextUpdate)
	{
		doDeathCamOnNextUpdate = false;

		if(!persIsInDeathCam())
			persSetDeathCam();
	}

	if(_exitTime && frametime >= _exitTime)
	{
		if(uiViewport()->FindView("exitvehicle") == nullptr)
		{
			if(SCEN_MGR->IsActive())
				uiLoadView("scenarios/exitvehicle");
			else
				uiLoadView("exitvehicle");

			_exitTime = 0;
		}
	}

	return 0;
}

static UINT32 _SetFinalExtFOVandFog(FLOAT32 efov)
{
	_ExternalFieldOfView = efov;
	FLOAT32	 horizon = piLongLikeFloat(FogEnd) * 2.0f;
	piSetFieldOfView(_ExternalFieldOfView, 1.0f, horizon);
	trnUpdateTerrainObjectLODs(1);

	return 0;
}

// FIXME - the results of this function aren't used
static UINT32 _UpdateCockpitCameraPos(void)
{
piVECTOR Pos;

	if (_Persona.VehicleAttached == nullptr)
	{
		_Persona.CockpitCameraPos.x = _Persona.CockpitCameraPos.y = _Persona.CockpitCameraPos.z = 0;
		return 0;
	}

	if (_Persona.VehicleAttached->ModelData == nullptr)
	{
		_Persona.CockpitCameraPos.x = _Persona.CockpitCameraPos.y = _Persona.CockpitCameraPos.z = 0;
		return 0;
	}

	// if we can't find an eyepoint, there isn't a virtual cockpit, so place the eye
	// at the center of the vehicle
	_Persona.CockpitCameraPos.x = (FLOAT32)-_Persona.VehicleAttached->ModelData->RenderPos.x;
	_Persona.CockpitCameraPos.y = (FLOAT32)-_Persona.VehicleAttached->ModelData->RenderPos.y;
	_Persona.CockpitCameraPos.z = (FLOAT32)_Persona.VehicleAttached->ModelData->RenderPos.z;

	if (_Persona.PersonaEyeDOF != nullptr)
	{
		// get the basic position
		Pos.x = _Persona.PersonaEyeDOF->dof->dofmatrix2._41;
		Pos.y = _Persona.PersonaEyeDOF->dof->dofmatrix2._42;
		Pos.z = _Persona.PersonaEyeDOF->dof->dofmatrix2._43;

		// adjust it by DOFs above in the tree
		math3dVectorMultiply(&Pos,&Pos,&_Persona.PersonaEyeDOF->matint);

		_Persona.CockpitCameraPos = Pos;
	}


	return 0;
}

UINT32 _UpdateExternalCameraTarget(bool start)
{
	static SINT32 _lastTargetTime = 0;

	if(start)
	{
		_lastTargetTime = frametime;
	}
	else if(_lastTargetTime > 0)
	{
		// move the camera towards the target
		FLOAT32 move = (FLOAT32)(frametime - _lastTargetTime)/1000.0f;

		if(_ViewTarget.x != 0)
		{
			FLOAT32 xMove = move * _ViewTargetMove.x;

			if(xMove * xMove >= _ViewTarget.x * _ViewTarget.x)
				xMove = _ViewTarget.x;

			_ViewAtt.x += mlDEG_TO_RAD(xMove);
			_ViewTarget.x += -xMove;

			if((_ViewTarget.x >= 0 && xMove < 0) || (_ViewTarget.x < 0 && xMove > 0))
				_ViewTarget.x = 0;
		}

		if(_ViewTarget.y != 0) // zoom is in meters
		{
			FLOAT32 yMove = move * _ViewTargetMove.y;
			FLOAT32 yDiff = _ExternalZoom - _ViewTarget.y;

			if(yMove * yMove >= yDiff * yDiff)
			{
				// we're done
				_ExternalZoom = _ViewTarget.y;
				_ViewTarget.y = 0.0f;
			}
			else
			{
				_ExternalZoom += yMove;
			}

			//if((_ViewTarget.y >= 0 && yMove < 0) || (_ViewTarget.y < 0 && yMove > 0))
			//	_ViewTarget.y = 0;
		}

		if(_ViewTarget.z != 0)
		{
			FLOAT32 zMove = move * _ViewTargetMove.z;

			if(zMove * zMove >= _ViewTarget.z * _ViewTarget.z)
				zMove = _ViewTarget.z;

			_ViewAtt.z += mlDEG_TO_RAD(zMove);
			_ViewTarget.z += -zMove;

			if((_ViewTarget.z >= 0 && zMove < 0) || (_ViewTarget.z < 0 && zMove > 0))
				_ViewTarget.z = 0;
		}

		if(_ViewTarget.x || _ViewTarget.y || _ViewTarget.z)
		{
			_lastTargetTime = frametime;
		}
		else
		{
			if(_lastTargetTime)
			{
				if(_InDeathCam == 2)
					persSetExitTime(frametime);
			}

			TUT_MGR->DoEvent(TUT_EVENT_CAMERA);
			_lastTargetTime = 0;
		}
	}

	return 0;
}

UINT32 persAttachPlayerToVehicle(vehVehicle* Vehicle,UINT32 Position)
{
	OBJECT * Object;
	OBJECT * VehicleObject;
	vehVehicle* LastVehicle;
	FLOAT32 DummyFloat;
	static SCHAR s[128];

	_InternalFieldOfView = DEFAULT_INTERNAL_FOV;
	_InDeathCam = 0;

	LastVehicle = _Persona.VehicleAttached;

	if (Vehicle != _Persona.VehicleAttached)
		persResetHealths();

	_Persona.VehicleAttached = Vehicle;

	if (Vehicle == nullptr)
	{
		return 0;
	}


	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleObjectData->PlaneObject == nullptr)
	{
		return 0;
	}

	OBJECT* o = Vehicle->VehicleObjectData->PlaneObject;

	if(o->modelInstance)
	{
		// Reset for proper camera distance when doing external camera view.
		_ExternalZoom = o->modelInstance->GetBounds().radius() * 1.5f;
	}
	else
	{
		_ExternalZoom = o->maxrad;
	}

	memset(_Persona.DamagePlayTime,0,sizeof(SINT32)*MAX_DAMAGE_SOUND_QUE);

	VehicleObject = Vehicle->VehicleObjectData->PlaneObject;
	sprintf((char*)s,"posit%d",Position+1);
	ogrpFindSubObjectByName(&Object,VehicleObject,(char*)s);
	_Persona.PersonaPosition = Object;
	_Persona.Position = Position;
	_Persona.FPSIsDead = 0;

	if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
	{
		_Persona.PersonaObject = VehicleObject;
		Object = _GenericHead;
	}
	else
	{
		_Persona.PersonaObject = nullptr;
	}

	if (Object != nullptr)
	{
		ogrpFindSubObjectByDOFVar(&Object,Object,ML_DOF_ELEMENT_BODY);
		_Persona.PersonaBodyDOF = Object;
	}
	else
	{
		_Persona.PersonaBodyDOF = nullptr;
	}

	if (Object != nullptr)
	{
		ogrpFindSubObjectByDOFVar(&Object,Object,ML_DOF_ELEMENT_HEAD);
		_Persona.PersonaHeadDOF = Object;
	}
	else
	{
		_Persona.PersonaHeadDOF = nullptr;
	}

	if (Object != nullptr)
	{
		ogrpFindSubObjectByDOFVar(&Object,Object,ML_DOF_ELEMENT_EYE);
		_Persona.PersonaEyeDOF = Object;
		_Persona.PersonaEyeDOF->flag2 |= OBJECT_DOF_USECHILDMATRIX;

	}
	else
	{
		_Persona.PersonaEyeDOF = nullptr;
	}
	_Persona.PersonaEye = _Persona.PersonaEyeDOF;
	// get the instrument view, if it exists for the position
	sprintf((char*)s,"iview%d",Position+1);
	Object = nullptr;
	_Persona.InstrumentViewDOF = nullptr;
	ogrpFindSubObjectByName(&Object,Vehicle->VehicleObjectData->PlaneObject,(char*)s);
	if (Object != nullptr)
	{
		if (Object->flags & OBJECT_DOF)
		{
			if (Object->dof != nullptr && Object->comment != nullptr)
			{
				_Persona.InstrumentViewDOF = Object;


				_Persona.InstrumentViewFOV[0] =
					_Persona.InstrumentViewFOV[1] =
					_Persona.InstrumentViewFOV[2] =
					_Persona.InstrumentViewFOV[3] = 0.0f;

				sscanf(Object->comment,"%s\n%f,%f,%f\n%f\n%f\n%f\n%f",
						s,
						&DummyFloat,
						&DummyFloat,
						&DummyFloat,
						&_Persona.InstrumentViewFOV[0],
						&_Persona.InstrumentViewFOV[1],
						&_Persona.InstrumentViewFOV[2],
						&_Persona.InstrumentViewFOV[3]);

				if (_Persona.InstrumentViewFOV[3] > 0.001)
				{
					_Persona.InstrumentViewNum = 4;
				}
				else
				if (_Persona.InstrumentViewFOV[2] > 0.001)
				{
					_Persona.InstrumentViewNum = 3;
				}
				else
				if (_Persona.InstrumentViewFOV[1] > 0.001)
				{
					_Persona.InstrumentViewNum = 2;
				}
				else
				{
					_Persona.InstrumentViewNum = 1;
				}

				if (Vehicle->VehicleType->InstGunsightFile[Position][0] != 0)
				{
					_Persona.InstrumentViewNum = 2;
				}

				_SetGunsightToUserDraw(Object,0);
			}
		}
	}

	// get the gunsight view, if it exists for the position
	sprintf((char*)s,"gview%d",Position+1);
	Object = nullptr;
	_Persona.GunsightViewDOF = nullptr;
	ogrpFindSubObjectByName(&Object,Vehicle->VehicleObjectData->PlaneObject,(char*)s);
	if (Object != nullptr)
	{
		if (Object->flags & OBJECT_DOF)
		{
			if (Object->dof != nullptr && Object->comment != nullptr)
			{
				_Persona.GunsightViewDOF = Object;

				_Persona.GunsightViewFOV[0] =
					_Persona.GunsightViewFOV[1] =
					_Persona.GunsightViewFOV[2] =
					_Persona.GunsightViewFOV[3] = 0.0f;
				sscanf(Object->comment,"%s\n%f,%f,%f\n%f\n%f\n%f\n%f",
						s,
						&DummyFloat,
						&DummyFloat,
						&DummyFloat,
						&_Persona.GunsightViewFOV[0],
						&_Persona.GunsightViewFOV[1],
						&_Persona.GunsightViewFOV[2],
						&_Persona.GunsightViewFOV[3]);

				if (_Persona.GunsightViewFOV[3] > 0.001)
				{
					_Persona.GunsightViewNum = 4;
				}
				else
				if (_Persona.GunsightViewFOV[2] > 0.001)
				{
					_Persona.GunsightViewNum = 3;
				}
				else
				if (_Persona.GunsightViewFOV[1] > 0.001)
				{
					_Persona.GunsightViewNum = 2;
				}
				else
				{
					_Persona.GunsightViewNum = 1;
				}

				if (Vehicle->VehicleType->GunsightFile[Position][0] != 0)
				{
					_Persona.GunsightViewNum = 2;
				}

				_SetGunsightToUserDraw(Object,0);
			}
		}
	}

	// ensure we're in "inplane" mode
	if (Vehicle->ModelData != nullptr)
	{
		mlObjectSetInplaneObject(Vehicle->VehicleObjectData->PlaneObject,_Persona.Position+1);
	}

	// finally set up controls
	// first read in control file
	vehCntrlReadControlFile(Vehicle->VehicleType,Position);

	imapRemoveControlCallbacks();

	vehCntrlInstallControlCallbacks();

	_InstallViewControlCallbacks();

	if(EZ_MGR)
		EZ_MGR->InstallInputCallbacks();

	if (LastVehicle != Vehicle)
	{
		mlResetControlStuff();
	}

	/*
	if(_CameraMode == _EXTERNAL_CAMERA)
	{
		// reset the visible object data
		persSetInternalCamera(1,0,0);
		persSetExternalCamera(1,0,0);
	}
	else
	{
		persSetInternalCamera(1,0,0);
		persSetExternalCameraAdjuster(nullptr);
		persSetExternalCameraPivot(nullptr);
	}
	*/

	persSetInternalCamera(1,0,0);
	persSetExternalCameraAdjuster(nullptr);
	persSetExternalCameraPivot(nullptr);

	memset(&_ViewTarget, 0, sizeof(piVECTOR));
	memset(&_ViewTargetMove, 0, sizeof(piVECTOR));

	if(Online)
	{
		g_bVehicleAttached = TRUE;
	}

	// kill these in case they were playing from the last spawn in
	// fxSndStopSound checks if it's a valid id and returns early, so just call it without checking
	fxSndStopSound(_Persona.PainSoundId);
	fxSndStopSound(_Persona.earringSoundID);
	fxSndStopSound(_Persona.heartbeatID);
	fxSndStopSound(_Persona.shellshockID);
	fxSndStopSound(_Persona.breathID);

	// 0 is a valid sound id, but is converted to -1 in the sound system
	_Persona.PainSoundId = 0;
	_Persona.earringSoundID = 0;
	_Persona.heartbeatID = 0;
	_Persona.shellshockID = 0;
	_Persona.breathID = 0;

	_exitTime = 0;

	return 0;
}

static bool _DrawGunsightObjectDummy(OBJECT* o)
{
	return true;
}

static UINT32 _SetGunsightToUserDraw(OBJECT * Object,UINT32 IncludeSibs)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(Object->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				ObjAtt->drawcallback = _DrawGunsightObjectDummy;
				ObjAtt->flags |= OBJECT_USERDRAW;
				ObjAtt->flag2 |= OBJECT_ZDISABLE;

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}


		if (ObjSib->child != nullptr)
		{
			_SetGunsightToUserDraw(ObjSib->child,1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && IncludeSibs == 1);

	return 0;
}


#if USE_STATESET
LOGICAL persDrawGunsight(void)
{
	// so _DrawStats gets cleared
	persPrepGunsightDraw();

    if(!_Persona.VehicleAttached)
        return FALSE;

    if(!_Persona.VehicleAttached->VehicleType)
        return FALSE;

	if(_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
	{
		fpFPSDrawGunsight();
		return TRUE;
	}

	if(_CameraMode != _INTERNAL_CAMERA)
        return FALSE;

	if(_Persona.GunsightViewDOF == nullptr)
        return FALSE;

	if(_Persona.VehicleAttached->ModelData == nullptr)
	    return FALSE;

	if(_InternalViewMode == _INTERNAL_GUNSIGHT)
	{
		if(_CheckForBlocks(_Persona.VehicleAttached->VehicleType->GunsightBlocked[_Persona.Position], _Persona.Position))
		{
			_BlockView();
		}
		else
		{
			if(!persGunsightDraw(_Persona.GunsightViewDOF, 0))
			{
				_DrawGunsightTree(_Persona.GunsightViewDOF, 0);
			}
		}
	}

	if(_InternalViewMode == _INTERNAL_INSTRUMENT)
	{
		if(_CheckForBlocks(_Persona.VehicleAttached->VehicleType->InstGunsightBlocked[_Persona.Position], _Persona.Position))
		{
			_BlockView();
		}
		else
		{
			if(persGunsightDraw(_Persona.InstrumentViewDOF, 1) == FALSE)
			{
				_DrawGunsightTree(_Persona.InstrumentViewDOF, 0);
			}
		}
	}

	return TRUE;
}
#else
LOGICAL persDrawGunsight(void)
{
	// so _DrawStats gets cleared
	persPrepGunsightDraw();

	if (_Persona.VehicleAttached == nullptr)
	{
		return 0;
	}

	if (_Persona.VehicleAttached->VehicleType == nullptr)
	{
		return 0;
	}

	if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
	{
		fpFPSDrawGunsight();
		return TRUE;
	}

	if (_CameraMode != _INTERNAL_CAMERA)
	{
		return 0;
	}

	if (_Persona.GunsightViewDOF == nullptr)
	{
		return 0;
	}

	if (_Persona.VehicleAttached->ModelData == nullptr)
	{
		return 0;
	}

	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

	if (_InternalViewMode == _INTERNAL_GUNSIGHT)
	{
		if (_CheckForBlocks(_Persona.VehicleAttached->VehicleType->GunsightBlocked[_Persona.Position],_Persona.Position)
					== TRUE)
		{
			_BlockView();
		}
		else
		{
			if (persGunsightDraw(_Persona.GunsightViewDOF,0) == FALSE)
			{
				_DrawGunsightTree(_Persona.GunsightViewDOF,0);
			}
		}
	}

	if (_InternalViewMode == _INTERNAL_INSTRUMENT)
	{
		if (_CheckForBlocks(_Persona.VehicleAttached->VehicleType->InstGunsightBlocked[_Persona.Position],_Persona.Position)
					== TRUE)
		{
			_BlockView();
		}
		else
		{
			if (persGunsightDraw(_Persona.InstrumentViewDOF,1) == FALSE)
			{
				_DrawGunsightTree(_Persona.InstrumentViewDOF,0);
			}
		}
	}

	piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

	return TRUE;
}
#endif

static LOGICAL _BlockView(void)
{
piMATRIX SavedViewMat;
piVECTOR V1,V2,V3,V4;

	piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_ALWAYS);
	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

	SavedViewMat = *piGetViewMatrix();

	piSetViewMatrix(&IdentityMatrix);
	piSetWorldMatrix(&IdentityMatrix);

	V1.x = -100.0f;
	V1.y = -100.0f;
	V1.z = 10.0f;
	V2.x = 100.0f;
	V2.y = -100.0f;
	V2.z = 10.0f;
	V3.x = 100.0f;
	V3.y = 100.0f;
	V3.z = 10.0f;
	V4.x = -100.0f;
	V4.y = 100.0f;
	V4.z = 10.0f;

	if(ogl::UseDeprecatedPath())
		piDisableTexture();
	else
		basiccolor->Apply();

	draw3dTexturedQuad(&V4,&V3,&V2,&V1,colorBLACKALPHA);

	piSetViewMatrix(&SavedViewMat);

	piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);

	return TRUE;
}

static LOGICAL _CheckForBlocks(UINT32 Type,SINT32 Position)
{
	if (Type == 0)
	{
		return FALSE;
	}

	if (Type == VEH_TURRET_NEUTRAL_ON_CANOPYOPEN)
	{
		if (_Persona.VehicleAttached->StateAnimation.CanopyPos[Position] < 0.01f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	if (Type == VEH_TURRET_NEUTRAL_ON_CANOPYCLOSED)
	{
		if (_Persona.VehicleAttached->StateAnimation.CanopyPos[Position] > 0.99f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	if (Type == VEH_TURRET_NEUTRAL_ON_HATCHOPEN)
	{
		if (_Persona.VehicleAttached->StateAnimation.HatchPos[Position] < 0.01f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	if (Type == VEH_TURRET_NEUTRAL_ON_HATCHCLOSED)
	{
		if (_Persona.VehicleAttached->StateAnimation.HatchPos[Position] > 0.99f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	if (Type == VEH_TURRET_NEUTRAL_ON_BOMBBAYOPEN)
	{
		if (_Persona.VehicleAttached->StateAnimation.BombBayDoorPos < 0.01f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	if (Type == VEH_TURRET_NEUTRAL_ON_BOMBBAYCLOSED)
	{
		if (_Persona.VehicleAttached->StateAnimation.BombBayDoorPos > 0.99f)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	return FALSE;
}

static LOGICAL _DrawGunsightTree(OBJECT * Object,SINT32 DoSibs)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				if (ObjAtt->flags & OBJECT_VISIBLE &&
					!(ObjAtt->flags & OBJECT_SKIPDRAWING) &&
					!(ObjAtt->flag2 & OBJECT_HIDDEN))
				{
					_DrawGunsightObject(ObjAtt);
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}


		if (ObjSib->child != nullptr)
		{
			_DrawGunsightTree(ObjSib->child,1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && DoSibs != 0);


	return 0;
}

static LOGICAL _DrawGunsightObject(OBJECT * Object)
{
	if(ogl::UseDeprecatedPath())
		obdrawDrawSingleObject(Object);
	else
	{
		oglSetWorldMatrix(&Object->mat);
		Object->material->Apply();

		ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, sizeof(piUNIFIED_VERTEX), Object->RenderColors().ptr);
		obdrawDrawObject(Object);
	}

	return TRUE;
}

static UINT32 _GetInternalCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att)
{
mlVehicleData * VehicleData;
piMATRIX* Mat;
piMATRIX TempMat,TempMat1;
piVECTOR TempPos,TempAtt;
UINT32 IsPersona;
OBJECT * TempObj;

	IsPersona = 0;
	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->InUse != FALSE)
		{
			if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
			{
				IsPersona = 1;
			}
		}
	}

	if (_Persona.PersonaEye != nullptr)
	{
		// get basic look attitude
		if (_InternalViewMode == _INTERNAL_INSTRUMENT && IsPersona == 0)
		{
			TempObj = _Persona.InstrumentViewDOF;
			while (TempObj->flags & OBJECT_NOTOBJECT && TempObj->child != nullptr)
			{
				TempObj = TempObj->child;
			}

			TempMat = _Persona.InstrumentViewDOF->dof->dofmatrix2;
			math3dMatrixMultiply(&TempMat,&TempMat,&TempObj->mat);
			Mat = &TempMat;
		}
		else if (_InternalViewMode == _INTERNAL_GUNSIGHT && IsPersona == 0)
		{
			TempObj = _Persona.GunsightViewDOF;
			while (TempObj->flags & OBJECT_NOTOBJECT && TempObj->child != nullptr)
			{
				TempObj = TempObj->child;
			}

			TempMat = _Persona.GunsightViewDOF->dof->dofmatrix2;
			math3dMatrixMultiply(&TempMat,&TempMat,&TempObj->mat);
			Mat = &TempMat;
		}
		else
		{
			Mat = &(_Persona.PersonaEye->mat);

			// since this is based on object center we need to move it to
			// the eye point
			if (_Persona.PersonaEyeDOF != nullptr)
			{
				math3dMatrixMultiply(Mat,&(_Persona.PersonaEyeDOF->dof->dofmatrix2),Mat);
			}
		}


		// first invert the positional component


		Pos->x = -Mat->_41;
		Pos->y = -Mat->_42;
		Pos->z = -Mat->_43;

		Att->x = Att->y = Att->z = 0;
		if (_Persona.VehicleAttached != nullptr)
		{
			if (_Persona.VehicleAttached->ModelData != nullptr)
			{
				if (IsPersona == 1)
				{
					TempMat1 = IdentityMatrix;
					TempMat1._11 = -1.0f;
					TempMat1._33 = -1.0f;
				}

				mathEul_FromMatrix(Att,Mat,EulOrdXYZs);

				Att->x = Att->x;
				Att->y = -Att->y;
				Att->z = -Att->z;

			}
		}

		if (_InternalViewMode == _INTERNAL_NORMAL)
		{
			Att->x -= PI/2;
		}
		else
		{
			Att->x += PI/2;
		}


		if (Att->x > PI)
		{
			Att->x -= PI*2;
		}
		if (Att->x < -PI)
		{
			Att->x += PI*2;
		}
	}
	else
	{
		Att->x = Att->y = Att->z = 0;
		Pos->x = Pos->y = Pos->z = 0;

		if (_Persona.VehicleAttached != nullptr)
		{
			if (_Persona.VehicleAttached != nullptr)
			{
				if (_Persona.VehicleAttached->VehicleType != nullptr)
				{
					if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
					{

						VehicleData = _Persona.VehicleAttached->ModelData;
						TempMat = IdentityMatrix;

						fpFPSGetEyeMatrix(&TempMat);

						TempPos.x = TempPos.y = TempPos.z = 0;
						TempAtt.x = TempAtt.y = TempAtt.z = 0;

						TempMat._43 = -TempMat._43;

						Mat = &TempMat;

						math3dMatrixMultiply(&TempMat,&TempMat,&_Persona.PersonaObject->mat);
						mathEul_FromMatrix(Att,Mat,EulOrdXYZs);

						Att->z = -Att->z;

						// if we're sitting, we're slave to our vehicle
						if (fpFPSIsTrooperSitting(nullptr))
						{
							Att->x = -Att->x;
							Att->y = -Att->y;
						}

						Pos->x = -TempMat._41;
						Pos->y = -TempMat._42;
						Pos->z = -TempMat._43;

					}
					else if (_Persona.VehicleAttached->ModelData != nullptr)
					{
						// Eliminate zig-zag using double.
						FLOAT64 px, py, pz;

						VehicleData = _Persona.VehicleAttached->ModelData;

						Att->x = (float)VehicleData->VehicleAtt.Pitch;
						Att->y = (float)VehicleData->VehicleAtt.Roll;
						Att->z = -(float)VehicleData->VehicleAtt.Yaw;

						px = -VehicleData->VehiclePos.x * ScaleFactor;
						py = -VehicleData->VehiclePos.y * ScaleFactor;
						pz =  VehicleData->VehiclePos.z * ScaleFactor;

						px += WorldOriginOffset.x;
						py += WorldOriginOffset.y;

						Pos->x = (FLOAT32)px;
						Pos->y = (FLOAT32)py;
						Pos->z = (FLOAT32)pz;
					}
				}
			}
		}
		Att->x -= PI/2;

	}

	// do screen shake
	if (frametime - _Persona.ShakeHeadStartTime < SHAKE_HEAD_DURATION)
	{
		FLOAT32 DeltaTime = (frametime - _Persona.ShakeHeadStartTime) / (FLOAT32)SHAKE_HEAD_DURATION;
		Att->z += (FLOAT32)sin(DeltaTime * 2 * PI) * 0.1f;
	}

	if(g_fShakeMagnitude > 0)
	{
		Att->x += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;
		Att->y += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;
		Att->z += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;
	}

	return 0;
}

#define _SPRINGCONSTANT 100.0
#define _SHOCKCONSTANT 10.0
static UINT32 _GetExternalCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att)
{
piMATRIX Mat;
piMATRIX AttMat;
piVECTOR Vec;
piVECTOR Att1;
piVECTOR AimPnt;
FLOAT32 Delta;
piVECTOR TargetPosBase;
FLOAT64VECTOR TargetPos,TargetVel,TargetAcc;
FLOAT64VECTOR Acc;
FLOAT64VECTOR WorldOriginOffset;
piVECTOR TempPos;
SINT32 TempNum;
FLOAT32 MinHeight;
static FLOAT64VECTOR _LastPos,_LastVel,_RelativePos;
static FLOAT64VECTOR /*_TargetLastPos,*/_TargetLastVel;
static UINT32 _LastUpdateTime;
//static piVECTOR _LastAtt[10];
static piVECTOR _LastVec;

	Delta = ((FLOAT32)frametime - (FLOAT32)_LastUpdateTime) / 1000.0f;

	AimPnt.x = AimPnt.y = AimPnt.z = 0.0f;
	AimPnt.y = 10.0f;

	Att1.x = Att1.y = Att1.z = 0.0f;
	//Att1.y = 2.0f;

#ifdef _EXTERN_VIEW_SLEW_RATE
	FLOAT32 slewRate = _EXTERN_VIEW_SLEW_RATE * Delta;
	FLOAT32 zoomMod = (dipKEYDOWN(buffer,DIK_LSHIFT) || dipKEYDOWN(buffer,DIK_RSHIFT)) ? 20.0f : 8.0f;

	Att1.z += _SlewViewLRHeldDown * slewRate;
	Att1.x += _SlewViewUDHeldDown * slewRate;
	_ExternalZoom += _SlewViewIOHeldDown * slewRate * zoomMod;

#endif

	_AdjustViewAngle(&Att1);

	if (_ExternalViewMode == _EXTERNAL_LOCKED)
	{
		if(!_ExternalCameraPivot || !_ExternalCameraPivot(Pos, &AttMat))
			_GetPlayerPos(Pos,&AttMat);

		Delta = (FLOAT32)((frametime - _LastUpdateTime) / 1000.0);
		_LastUpdateTime = frametime;

		FLOAT32 xdis = 0.0f;
		FLOAT32 ydis = _ExternalZoom; //-_EXTERNAL_VIEW_DISTANCE;
		FLOAT32 zdis = 0.0f;

/*
#if RELEASE_CLIENT == 1
		vehVehicle*veh = persGetPersonaVehiclePtr();
		if(veh && veh->VehicleObjectData)
		{
			xdis = veh->VehicleObjectData->Radius;
			ydis = veh->VehicleObjectData->Radius;
			zdis = veh->VehicleObjectData->Radius;
		}
#endif
*/
		Vec.x = xdis;
		Vec.y = ydis;
		Vec.z = zdis;

		Att1.x = _ViewAtt.x;
		Att1.y = 0.0;
		Att1.z = 0.0;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		Att1.x = 0.0;
		Att1.z = _ViewAtt.z;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		Att1.x = 0.0;
		Att1.z = 0.0;
		Att1.y = _ViewAtt.y;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		AimPnt = *Pos;
		Pos->x -= Vec.x;
		Pos->y -= Vec.y;
		Pos->z += Vec.z;
	}

	if (_ExternalViewMode == _EXTERNAL_RUBBERBAND)
	{
		Delta = (FLOAT32)((frametime - _LastUpdateTime) / 1000.0);

		_GetPlayerPos(&TargetPosBase,&AttMat);
		*Pos = TargetPosBase;

		trnGetWorldOriginOffset(&WorldOriginOffset);
		TargetPos.x = TargetPosBase.x - WorldOriginOffset.x;
		TargetPos.y = TargetPosBase.y - WorldOriginOffset.y;
		TargetPos.z = TargetPosBase.z - WorldOriginOffset.z;


		Vec.x = Vec.y = Vec.y = 0;
		Vec.y = _ExternalZoom; //-_EXTERNAL_VIEW_DISTANCE_RUBBER_BAND;

		Att1.x = _ViewAtt.x;
		Att1.y = 0.0;
		Att1.z = 0.0;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		Att1.x = 0.0;
		Att1.z = _ViewAtt.z;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		Att1.x = 0.0;
		Att1.z = 0.0;
		Att1.y = _ViewAtt.y;
		mathEul_ToMatrix(&Mat,EulOrdXYZr,&Att1);
		math3dVectorMultiply(&Vec,&Vec,&Mat);

		math3dVectorMultiply(&Vec,&Vec,&AttMat);

		_RelativePos.x -= (_LastVec.x - Vec.x);
		_RelativePos.y -= (_LastVec.y - Vec.y);
		_RelativePos.z += (_LastVec.z - Vec.z);
		_LastVec = Vec;

		AimPnt = *Pos;
		Pos->x -= Vec.x;
		Pos->y -= Vec.y;
		Pos->z += Vec.z;
		if ((UINT32)frametime != (UINT32)_LastUpdateTime)
		{
			mathSubVectors(&TargetVel,&TargetPos,&_LastPos);
			TargetVel.x /= Delta;
			TargetVel.y /= Delta;
			TargetVel.z /= Delta;
			_LastPos = TargetPos;

			// now do rubber band effect
			mathSubVectors(&TargetAcc,&TargetVel,&_TargetLastVel);
			_TargetLastVel = TargetVel;

			if (Delta > 2.0 || fabs(_RelativePos.x) > 100.0 ||
							   fabs(_RelativePos.y) > 100.0 ||
							   fabs(_RelativePos.z) > 100.0)
			{
				_RelativePos.x = 0.0;
				_RelativePos.y = 0.0;
				_RelativePos.z = 0.0;

				_LastVel.x = 0.0;
				_LastVel.y = 0.0;
				_LastVel.z = 0.0;

				_LastPos = TargetPos;
				_TargetLastVel = TargetVel;
			}
			else
			{

				Acc.x = -TargetAcc.x * 200.0;
				Acc.y = -TargetAcc.y * 200.0;
				Acc.z = -TargetAcc.z * 200.0;

				_LastVel.x += Acc.x * Delta;
				_LastVel.y += Acc.y * Delta;
				_LastVel.z += Acc.z * Delta;

				_LastVel.x -= _RelativePos.x * _SPRINGCONSTANT * Delta;
				_LastVel.y -= _RelativePos.y * _SPRINGCONSTANT * Delta;
				_LastVel.z -= _RelativePos.z * _SPRINGCONSTANT * Delta;

				_LastVel.x -= _LastVel.x * _SHOCKCONSTANT * Delta;
				_LastVel.y -= _LastVel.y * _SHOCKCONSTANT * Delta;
				_LastVel.z -= _LastVel.z * _SHOCKCONSTANT * Delta;

				_RelativePos.x += _LastVel.x * Delta;
				_RelativePos.y += _LastVel.y * Delta;
				_RelativePos.z += _LastVel.z * Delta;

				if (_RelativePos.x > 101.0)
				{
					_RelativePos.x = 101.0;
				}
				if (_RelativePos.y > 101.0)
				{
					_RelativePos.y = 101.0;
				}
				if (_RelativePos.z > 101.0)
				{
					_RelativePos.z = 101.0;
				}
				if (_RelativePos.x < -101.0)
				{
					_RelativePos.x = -101.0;
				}
				if (_RelativePos.y < -101.0)
				{
					_RelativePos.y = -101.0;
				}
				if (_RelativePos.z < -101.0)
				{
					_RelativePos.z = -101.0;
				}

			}
		}


		_LastUpdateTime = frametime;

		Pos->x += (FLOAT32)_RelativePos.x;
		Pos->y += (FLOAT32)_RelativePos.y;
		Pos->z += (FLOAT32)_RelativePos.z;
	}

	// let's calculate a better aim point based on vehicle
	vehVehicle* veh = persGetPersonaVehiclePtr();
	FLOAT32 zAdjust = 0.0f;

	if(veh)
	{
		if(veh->VehicleType->Type == VEH_TYPE_PERSON)
		{
			zAdjust = (TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() - .5f);

			if(AimPnt.z + zAdjust < 0)
				zAdjust = -AimPnt.z;
		}
		else
		{
			zAdjust = veh->VehicleObjectData->HighestPoint/2.0f;
		}
	}
	else
	{
		zAdjust = 1.0f;
	}

	if(!_ExternalCameraAdjuster || !_ExternalCameraAdjuster(Pos, &AimPnt))
		AimPnt.z += zAdjust;

	// ensure camera never goes underground
	if (_ExternalViewMode != _EXTERNAL_FIXED)
	{
		TempPos = *Pos;
		TempPos.x = -TempPos.x;
		TempPos.y = -TempPos.y;
		TempPos.z = -TempPos.z;

		trnopCollideDown(&TempPos,&TempNum,&TempPos);

		rnmainGetZBufferNearVal(&MinHeight);
		MinHeight *= 1.5;
		if (-TempPos.z > Pos->z - MinHeight)
		{
			Pos->z = -TempPos.z + MinHeight;
		}

		_AimCameraAtPosition(&Att1,Pos,&AimPnt);

		*Att = Att1;
	}
	else
	// aim camera if applicable
//	if (_ExternalViewMode == _EXTERNAL_FIXED)
	{
#ifdef _EXTERN_VIEW_SLEW_RATE
		_CameraPos.z += Att1.x/_EXTERN_VIEW_SLEW_RATE/100.0f;
#endif
		*Pos = _CameraPos;

		_GetPlayerPos(&AimPnt,&AttMat);
		_AimCameraAtPosition(Att,Pos,&AimPnt);

	}

	if(g_fShakeMagnitude > 0)
	{
		Att->x += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;;
		Att->y += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;;
		Att->z += (FLOAT32)mathGetFloat32ChaoticNumber() * g_fShakeMagnitude;;
	}

	return 0;
}

static UINT32 _AimCameraAtPosition(piVECTOR * Att,piVECTOR * Pos,piVECTOR * AimPnt)
{
piMATRIX TempMat,MatFinal,TempMat1;
piVECTOR Att1;
piVECTOR ViewVec;

	mathSubVectors(&ViewVec,AimPnt,Pos);

	//FLOAT32 zSave = ViewVec.z;

	// move it a little further out than the aim point
	//mathScaleVector(&ViewVec, &ViewVec, 1.3f);
	//ViewVec.z = zSave;

	Att1.x = -PI/2;
	Att1.y = 0;
	Att1.z = 0;

	mathEul_ToMatrix(&MatFinal,EulOrdXYZr,&Att1);

	Att1.x = 0;
	Att1.y = 0;

	Att1.z = (FLOAT32)atan2(ViewVec.x,-ViewVec.y);

	mathEul_ToMatrix(&TempMat1,EulOrdXYZr,&Att1);

	Att1.x = -(FLOAT32)atan2(ViewVec.z, sqrt(ViewVec.x * ViewVec.x + ViewVec.y * ViewVec.y));
	Att1.z = 0;

	mathEul_ToMatrix(&TempMat,EulOrdXYZs,&Att1);

	math3dMatrixMultiply(&TempMat,&TempMat1,&TempMat);


	math3dMatrixMultiply(&TempMat,&TempMat,&MatFinal);
//	TempMat = MatFinal;

	mathEul_FromMatrix(Att,&TempMat,EulOrdXYZr);

	return 0;
}

static UINT32 _GetPlayerPos(piVECTOR * Pos,piMATRIX * AttMat)
{
	vehVehicle* VehicleData = persGetPersonaVehiclePtr();
	if (VehicleData == nullptr)
	{
		return 0;
	}

	FLOAT64VECTOR WorldOffsetOrigin;
	FLOAT64VECTOR TempPos;
	piVECTOR TempAtt;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	if (VehicleData->ModelData != nullptr)
	{
		TempAtt.x = (FLOAT32)VehicleData->ModelData->RenderAtt.Pitch;
		TempAtt.y = (FLOAT32)VehicleData->ModelData->RenderAtt.Roll;
		TempAtt.z = (FLOAT32)VehicleData->ModelData->RenderAtt.Yaw;

		if(_CameraMode == _EXTERNAL_CAMERA)
		{
			TempPos.x = VehicleData->ModelData->VehiclePos.x;
			TempPos.y = VehicleData->ModelData->VehiclePos.y;
			TempPos.z = VehicleData->ModelData->VehiclePos.z;
		}else
		{
			TempPos.x = VehicleData->ModelData->RenderPos.x;
			TempPos.y = VehicleData->ModelData->RenderPos.y;
			TempPos.z = VehicleData->ModelData->RenderPos.z;
		}
	}
	else
	{
		TempPos.x = VehicleData->PosData.RenderPos.x;
		TempPos.y = VehicleData->PosData.RenderPos.y;
		TempPos.z = VehicleData->PosData.RenderPos.z;

		TempAtt.x = (FLOAT32)VehicleData->PosData.RenderAtt.Pitch;
		TempAtt.y = (FLOAT32)VehicleData->PosData.RenderAtt.Roll;
		TempAtt.z = (FLOAT32)VehicleData->PosData.RenderAtt.Yaw;
	}

	mathSubVectors(&TempPos,&TempPos,&WorldOffsetOrigin);

	Pos->x = -(FLOAT32)TempPos.x;
	Pos->y = -(FLOAT32)TempPos.y;
	Pos->z = (FLOAT32)TempPos.z;

	mathEul_ToMatrix(AttMat,EulOrdXYZs,&TempAtt);

	return 0;
}

UINT32 persGetCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att)
{
	if (_CameraMode == _INTERNAL_CAMERA)
	{
		_GetInternalCameraPosAndAtt(Pos,Att);
	}
	if (_CameraMode == _EXTERNAL_CAMERA)
	{
		_GetExternalCameraPosAndAtt(Pos,Att);
	}

	return 0;
}

static UINT32 _UpdateHeadAndView(void)
{
mlVehicleData * VehicleData;
piMATRIX * Mat;
piMATRIX Matrix,Matrix1,Matrix2;
FLOAT64VECTOR Pos;
FLOAT64ATT AttChange;
FLOAT64VECTOR Displacement;
piVECTOR Att1;
FLOAT64ATT HeadAtt;
FLOAT64ATT EyeAtt;
FLOAT64 DeltaTime;
piMATRIX Mat1;
static SINT32 _LastCallTime;

	if (_Persona.VehicleAttached == nullptr)
	{
		return 1;
	}

	if (_Persona.VehicleAttached->ModelData == nullptr)
	{
		return 1;
	}
	if (_Persona.PersonaEyeDOF == nullptr)
	{
		return 1;
	}

	if (_Persona.PersonaHeadDOF == nullptr)
	{
		return 1;
	}

	DeltaTime = (frametime - _LastCallTime) / 1000.0;
	_LastCallTime = frametime;

	VehicleData = _Persona.VehicleAttached->ModelData;
	Mat = &(_Persona.PersonaHeadDOF->dof->dofmatrix2); // origin position and attitude for eye
	mlUtilityGetModelPositionFromObjectMatrix(&Pos,Mat);

	// correct for center of mass
	Pos.x -= VehicleData->VehicleCenterOfMass.x;
	Pos.y -= VehicleData->VehicleCenterOfMass.y;
	Pos.z -= VehicleData->VehicleCenterOfMass.z;

	// compute head attitude matrix
	Mat1 = _Persona.PersonaHeadDOF->dof->dofmatrix2;
	math3dMatrixMultiply(&Mat1,&Mat1,&_Persona.PersonaHeadDOF->matint);
	Mat1._41 = Mat1._42 = Mat1._43 = 0.0f;

	Mat1._21 = -Mat1._21;
	Mat1._22 = -Mat1._22;
	Mat1._23 = -Mat1._23;

	// first update the head position based on vehicle operation
	mlGetHeadPos(&Displacement,
					&AttChange,
					VehicleData,
					&Pos,
					&Mat1);

	// accentuate head bobbing when in scope
	if (FieldOfView < 45.0f)
	{
		FLOAT32 Mult;

		Mult = 1;

		AttChange.Pitch *= Mult;
		AttChange.Roll *= Mult;
		AttChange.Yaw *= Mult;

		Att1.x = -(float)AttChange.Pitch/2;
		Att1.y = (float)AttChange.Roll/2;
		Att1.z = (float)AttChange.Yaw/2;
		mathEul_ToMatrix(&Matrix,EulOrdXYZs,&Att1);

	}
	else
	{
		AttChange.Pitch = AttChange.Roll = AttChange.Yaw = 0.0;

		Matrix = IdentityMatrix;
	}

//	Matrix = IdentityMatrix;
	Matrix._41 = (float)(Displacement.x) * ScaleFactor;
	Matrix._42 = (float)(Displacement.y) * ScaleFactor;
	Matrix._43 = -(float)(Displacement.z) * ScaleFactor;

	// disabled for e3
//	Matrix = IdentityMatrix;
//	Matrix1 = IdentityMatrix;
//	Matrix2 = IdentityMatrix;
	_Persona.PersonaBodyDOF->dof->dofposmatrix = Matrix;
	_Persona.PersonaBodyDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;


	Att1.x = -(float)AttChange.Pitch;
	Att1.y = (float)AttChange.Roll;
	Att1.z = (float)AttChange.Yaw;
	mathEul_ToMatrix(&Matrix,EulOrdXYZs,&Att1);

	// disabled for e3
//	Matrix = IdentityMatrix;


AttChange.Roll = AttChange.Pitch = AttChange.Yaw = 0.0;
Displacement.x = Displacement.y = Displacement.z = 0.0;
	if (_InternalViewMode == _INTERNAL_NORMAL)
	{
		mlheadGetHeadView(&HeadAtt,&EyeAtt);
	}
	else
	{
		HeadAtt.Pitch = HeadAtt.Roll = HeadAtt.Yaw = 0.0f;
		EyeAtt.Pitch = EyeAtt.Roll = EyeAtt.Yaw = 0.0f;
	}
	HeadAtt.Pitch = (_Persona.LastHeadAtt.Pitch * 0.05 + HeadAtt.Pitch * DeltaTime) /
					(0.05+DeltaTime);
	HeadAtt.Roll = (_Persona.LastHeadAtt.Roll * 0.05 + HeadAtt.Roll * DeltaTime) /
					(0.05+DeltaTime);
	HeadAtt.Yaw = (_Persona.LastHeadAtt.Yaw * 0.05 + HeadAtt.Yaw * DeltaTime) /
					(0.05+DeltaTime);
	_Persona.LastHeadAtt = HeadAtt;
	Att1.x = -(float)HeadAtt.Pitch;
	Att1.y = -(float)HeadAtt.Roll;
	Att1.z = -(float)HeadAtt.Yaw;
	mathEul_ToMatrix(&Matrix1,EulOrdXYZs,&Att1);

	Matrix2 = Matrix;
	math3dMatrixMultiply(&Matrix,&Matrix1,&Matrix);

	_Persona.PersonaHeadDOF->dof->dofposmatrix = Matrix;
	_Persona.PersonaHeadDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

	EyeAtt.Pitch = (_Persona.LastEyeAtt.Pitch * 0.03 + EyeAtt.Pitch * DeltaTime) /
					(0.03+DeltaTime);
	EyeAtt.Roll = (_Persona.LastEyeAtt.Roll * 0.03 + EyeAtt.Roll * DeltaTime) /
					(0.03+DeltaTime);
	EyeAtt.Yaw = (_Persona.LastEyeAtt.Yaw * 0.03 + EyeAtt.Yaw * DeltaTime) /
					(0.03+DeltaTime);
	_Persona.LastEyeAtt = EyeAtt;
	Att1.x = -(float)EyeAtt.Pitch;
	Att1.y = -(float)EyeAtt.Roll;
	Att1.z = -(float)EyeAtt.Yaw;
	mathEul_ToMatrix(&Matrix,EulOrdXYZs,&Att1);
	if (_UseGyroStabilizedEyes)
	{
		math3dInvertMatrix(&Matrix2,&Matrix2);
		math3dMatrixMultiply(&Matrix,&Matrix,&Matrix2);
	}

	_Persona.PersonaEyeDOF->dof->dofposmatrix = Matrix;
	_Persona.PersonaEyeDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

	return 0;
}

persPersona* persGetPersonaDataPtr()
{
	return &_Persona;
}

vehVehicle* persGetPersonaVehiclePtr()
{
	return _Persona.VehicleAttached;
}

vehVehicle* persGetPersonaPilotedOnlineVehicle()
{
	if(!Online) return nullptr;

	vehVehicle* vehicle = _Persona.VehicleAttached;
	if(
		vehicle
		&& vehicle->InUse
		&& vehicle->ModelData
		&& vehicle->VehicleObjectData
		&& vehicle->ModelData->MultiCrewPos <= 1
		)
		return vehicle;

	return nullptr;
}

UINT32 persGetPersonaPosition(UINT32 * Position)
{
	if(EZ_MGR)
	{
		// see if we have overriden the game position temporarily
		SINT32 pos = EZ_MGR->GetGamePosition();
		if(pos >= 0) return *Position = (UINT32)pos;
	}

	*Position = _Persona.Position;

	return 0;
}

UINT32 persSetPersonaPositionCallback(SINT32 param,UINT32 id,UINT32 Detach)
{
	if (param != 1 || vehmainHasBailedOut())
	{
		return 0;
	}

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->VehicleType != nullptr)
		{
			if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
			{
				return 0;
			}
		}
	}

	// this call performs actions that needs to happen when a position change occurs
	mlResetPositionState();

	switch(id)
	{
		case IMAP_MOM_POSITION_1:
			persSetPersonaPosition(0);
			break;
		case IMAP_MOM_POSITION_2:
			persSetPersonaPosition(1);
			break;
		case IMAP_MOM_POSITION_3:
			persSetPersonaPosition(2);
			break;
		case IMAP_MOM_POSITION_4:
			persSetPersonaPosition(3);
			break;
		case IMAP_MOM_POSITION_5:
			persSetPersonaPosition(4);
			break;
		case IMAP_MOM_POSITION_6:
			persSetPersonaPosition(5);
			break;
		case IMAP_MOM_POSITION_7:
			persSetPersonaPosition(6);
			break;
		case IMAP_MOM_POSITION_8:
			persSetPersonaPosition(7);
			break;
		case IMAP_MOM_POSITION_9:
			persSetPersonaPosition(8);
			break;
		case IMAP_MOM_POSITION_10:
			persSetPersonaPosition(9);
			break;
	}

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &id, &param);

	return 0;
}

UINT32 persSetPersonaPosition(UINT32 InPosition)
{
UINT32 Position;
UINT32 LowPosition;
UINT32 i;
OBJECT * TempObj;

	// if we're multicrewed, we need to make sure player can go to position
	// rules are: if position doesn't exist, scan for first position available.
	// if position is for other player, scan for first position available
	// ------------------------------------------------------------------------
	// New Rules: ticket #3142: If position does not exist, don't scan for next
	// available. Available positions are now located in .turret file. Positions
	// will use MultiCrewPos for available positions.
	// ------------------------------------------------------------------------
	Position = 0;
	LowPosition = 0;

	// Remove TESTING_VALID_POSITION code when testing is verified.
#define TESTING_VALID_POSITION 1
#if TESTING_VALID_POSITION
	bool MultiCrewVehicle = false;

	if(_Persona.VehicleAttached->VehicleType)
	{
		if(_Persona.VehicleAttached->VehicleType->VehicleIsMultiCrew)
		{
			MultiCrewVehicle = true;
		}
	}

	if(!MultiCrewVehicle)
		InPosition = 0;
	else
	{
		UINT32* pos = _Persona.VehicleAttached->VehicleType->MultiCrewPos;
		bool found = false;

		for(UINT32 i = 0; i < ML_MAX_CREW_POSITIONS; i++)
		{
			// This position is invalid
			if(pos[i] == 0)
				continue;

			// Looped far enough. If here, position has not been found
			if(i > InPosition)
				break;

			if(i == InPosition)
			{
				found = true;
				break;
			}
		}

		// Invalid key press. Don't move.
		if(!found)
			return FALSE;
	}
#endif

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->ModelData != nullptr)
		{
			if (_Persona.VehicleAttached->ModelData->MultiCrewPos == 0)
			{
				Position = InPosition;
			}
			else
			{
				if (_Persona.VehicleAttached->VehicleType != nullptr)
				{
					if (_Persona.VehicleAttached->VehicleType->MultiCrewPos[InPosition] !=
							_Persona.VehicleAttached->ModelData->MultiCrewPos)
					{
						for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
						{
							if (_Persona.VehicleAttached->VehicleType->MultiCrewPos[i] ==
									_Persona.VehicleAttached->ModelData->MultiCrewPos)
							{
								Position = i;
								LowPosition = i;
								break;
							}
						}
					}
					else
					{
						Position = InPosition;
					}
				}
			}
		}
	}

	persAttachPlayerToVehicle(_Persona.VehicleAttached,Position);
	if (_Persona.VehicleAttached == nullptr)
	{
		return 1;
	}

	if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if (Position != 0)
		{
			persAttachPlayerToVehicle(_Persona.VehicleAttached,0);
		}

		return 0;
	}

	// if in deathcam, disable position changes
	if (_Persona.VehicleAttached->VehicleDeadTime != 0 || vehmainHasBailedOut())
	{
		return 0;
	}

	// check if we don't have an eyepoint, if not, we force to position zero
	if (_Persona.PersonaEyeDOF == nullptr)
	{
		persAttachPlayerToVehicle(_Persona.VehicleAttached,LowPosition);
		Position = LowPosition;
	}


	// load up any gunsights
	persClearGunsights();

	if (_Persona.VehicleAttached->VehicleType->GunsightFile[Position][0] != 0)
	{
		persLoadGunsight(_Persona.VehicleAttached->VehicleType->GunsightFile[Position]);

		if (_Persona.VehicleAttached->VehicleType->GunsightAlignmentObj[Position][0] != 0)
		{
			TempObj = nullptr;
			ogrpFindSubObjectByName(&TempObj,_Persona.VehicleAttached->VehicleObjectData->PlaneObject,
						_Persona.VehicleAttached->VehicleType->GunsightAlignmentObj[Position]);

			if (TempObj != nullptr)
			{
				ogrpFindSubObjectByDOFVarAndSubVar(&TempObj,TempObj,ML_DOF_CONNECT,0);

				if (TempObj != nullptr)
				{
					ogrpFindSubObjectByDOFVar(&TempObj,TempObj,ML_DOF_FIREPOINT);

					if (TempObj != nullptr)
					{
						persAlignGunsight(TempObj,_Persona.GunsightViewDOF,
								_Persona.VehicleAttached->VehicleType->GunsightAlignmentRange[Position]);
					}
				}
			}
		}
	}

	if (_Persona.VehicleAttached->VehicleType->InstGunsightFile[Position][0] != 0)
	{
		persLoadInstGunsight(_Persona.VehicleAttached->VehicleType->InstGunsightFile[Position]);
		if (_Persona.VehicleAttached->VehicleType->InstGunsightAlignmentObj[Position][0] != 0)
		{
			TempObj = nullptr;
			ogrpFindSubObjectByName(&TempObj,_Persona.VehicleAttached->VehicleObjectData->PlaneObject,
						_Persona.VehicleAttached->VehicleType->InstGunsightAlignmentObj[Position]);

			if (TempObj != nullptr)
			{
				ogrpFindSubObjectByDOFVarAndSubVar(&TempObj,TempObj,ML_DOF_CONNECT,0);

				if (TempObj != nullptr)
				{
					ogrpFindSubObjectByDOFVar(&TempObj,TempObj,ML_DOF_FIREPOINT);

					if (TempObj != nullptr)
					{
						persAlignInstGunsight(TempObj,_Persona.InstrumentViewDOF,
								_Persona.VehicleAttached->VehicleType->GunsightAlignmentRange[Position]);
					}
				}
			}
		}
	}

	mlCenterMouseLook();

	if(_Persona.VehicleAttached->ModelData->CrewData.SetBombbayDoor[Position])
	{
		vehAnimChangeVehicleStateByName(_Persona.VehicleAttached, (UCHAR*)"bombbaydoor",
						_Persona.VehicleAttached->ModelData->CrewData.BombbayDoorState[Position]);
	}

	return 0;
}

UINT32 persResetPersonaSettings()
{
	// for every position, reset the
	// starting ordnance number; we do this because
	// the last vehicle used may not have the same
	// ordnance options as this vehicle
	for(UINT32 i = 0; i < ML_MAX_CREW_POSITIONS; i++)
		persSetPositionOrdnanceNum(i, 0);

	// reset other persona data here

	return 0;
}

OBJECT* persGetPersonaVehicleObjectDataPtr()
{
	if (_Persona.VehicleAttached != nullptr && _Persona.VehicleAttached->VehicleObjectData != nullptr)
	{
		return _Persona.VehicleAttached->VehicleObjectData->PlaneObject;
	}
	return nullptr;
}

mlVehicleData* persGetPersonaModelDataPtr()
{
	return (_Persona.VehicleAttached != nullptr) ? _Persona.VehicleAttached->ModelData : nullptr;
}

UINT32 persDetachFromVehicle(vehVehicle* Vehicle)
{
	if (_Persona.VehicleAttached == Vehicle && Vehicle != nullptr)
	{
		persDetachPersona();
		persGunsightClearData();

		if(Online)
		{
			g_bVehicleAttached = FALSE;
		}
	}

	return 0;
}

UINT32 persDetachPersona(void)
{
	_Persona.PersonaPosition = nullptr;
	_Persona.PersonaBodyDOF = nullptr;
	_Persona.PersonaHeadDOF = nullptr;
	_Persona.PersonaEye = nullptr;
	_Persona.PersonaEyeDOF = nullptr;
	_Persona.Position = 0;
	_Persona.VehicleAttached = nullptr;

	persSetExternalCameraAdjuster(nullptr);
	vehcntrlDisableAllControls();

	return 0;
}

UINT32 persToggleCamera()
{
	if (_Persona.VehicleAttached == nullptr || _Persona.VehicleAttached->VehicleType == nullptr)
		return 0;

	if (_CameraMode == _INTERNAL_CAMERA)
	{
		if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
		{
			fpSetExternal(_Persona.VehicleAttached->ModelData);
		}

		persSetExternalCamera(1, 0, 0);
	}
	else
	{
		if(_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
		{
			fpSetInternal(_Persona.VehicleAttached->ModelData);
		}

		persSetInternalCamera(1, 0, 0);
	}

	//uiChatPrintLine("Toggled external camera: %u", _CameraMode);

	return 1;
}

UINT32 persToggleInternalExternalViews(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	if (_Persona.VehicleAttached == nullptr || _Persona.VehicleAttached->VehicleType == nullptr)
		return 0;

#ifndef BETA_RELEASE
	if(!Online || TUT_MGR->IsActive() || SuperFloat)
#endif
	{
		if(Param == 1)
			persToggleCamera();
	}

	return 0;
}

UINT32 persSetExternalCamera(SINT32 Param,UINT32 Id,UINT32 Detach)
{
FLOAT32	horizon;

	if (Param != 1)
	{
		return 0;
	}

	_CameraMode = _EXTERNAL_CAMERA;
	_ExternalViewMode = _EXTERNAL_LOCKED;
	_ExternalFieldOfView = DEFAULT_EXTERN_FOV;

	horizon = piLongLikeFloat(FogEnd) * 2.0f;
	piSetFieldOfView(_ExternalFieldOfView, 1.0f, horizon);
	trnUpdateTerrainObjectLODs(1);

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->VehicleObjectData != nullptr)
		{
			if (_Persona.VehicleAttached->VehicleObjectData->PlaneObject != nullptr)
			{

				// note: this is a hack to hide close-in problems
				if (_Persona.VehicleAttached->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
				{
					obmainClearFlag2InThisTree(_Persona.VehicleAttached->VehicleObjectData->PlaneObject,OBJECT_ISCOCKPIT);
				}

				mlObjectSetExternObject(_Persona.VehicleAttached->VehicleObjectData->PlaneObject);
				vehAnimResetVehicleStateAnimations(_Persona.VehicleAttached);
			}
		}
	}

	_SlewViewLRHeldDown = 0;
	_SlewViewUDHeldDown = 0;
	_SlewViewIOHeldDown = 0;

	/*
	_SlewViewLeftHeldDown = 0;
	_SlewViewRightHeldDown = 0;
	_SlewViewUpHeldDown = 0;
	_SlewViewDownHeldDown = 0;
	*/

	return 0;
}

void persSetExternalCameraPos(FLOAT32 horAxis, FLOAT32 verAxis, FLOAT32 zoom, bool checkMode)
{
	if(checkMode && _CameraMode != _EXTERNAL_CAMERA)
		persToggleCamera(); // turn on the external camera

	if(verAxis > 90.0f)
		verAxis = 90.0f;
	else if(verAxis < -90.0f)
		verAxis = -90.0f;

	if(zoom < 0)
		zoom = 0.0f;

	_ViewAtt.x = mlDEG_TO_RAD(-verAxis); // -90.0f is up and 90.0f is down, so we reverse what was sent in
	_ViewAtt.y = 0.0f;
	_ViewAtt.z = mlDEG_TO_RAD(horAxis);

	_ExternalZoom = zoom;

	// set the default fov
	_SetFinalExtFOVandFog(DEFAULT_EXTERN_FOV);

	// disable any current camera pans
	persSetExternalCameraTarget(0.0f, 0.0f, 0.0f, 0.0f);
	persSetExternalCameraPivot(nullptr);

	// let the UI know we did it
	TUT_MGR->DoEvent(TUT_EVENT_CAMERA);
}

void persSetExternalCameraTarget(FLOAT32 horDeg, FLOAT32 verDeg, FLOAT32 zoom, FLOAT32 duration)
{
	_ViewTarget.x		= -verDeg;
	_ViewTarget.y		= _ExternalZoom + -zoom;
	_ViewTarget.z		= horDeg;

	//if(_ViewTarget.y > -1.0f)
	//	_ViewTarget.y = -1.0f;

	// calculate move amounts in degrees per second
	_ViewTargetMove.x = (duration) ? -verDeg / duration : 0;
	_ViewTargetMove.y = (duration) ? -zoom / duration : 0;
	_ViewTargetMove.z = (duration) ? horDeg / duration : 0;

	// init our targeting
	_UpdateExternalCameraTarget(true);
}

void persSetExternalCameraAdjuster(ExternalCameraAdjust a)
{
	_ExternalCameraAdjuster = a;
}

void persSetExternalCameraPivot(ExternalCameraPivot p)
{
	_ExternalCameraPivot = p;
}

UINT32 persSetInternalCamera(SINT32 Param,UINT32 Id,UINT32 Detach)
{
FLOAT32	horizon;

	trnmainGetHorizon(&horizon);

	if (Param != 1)
	{
		return 0;
	}

	_CameraMode = _INTERNAL_CAMERA;

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->VehicleObjectData != nullptr)
		{
			if (_Persona.VehicleAttached->VehicleObjectData->PlaneObject != nullptr)
			{
				// note: this is a hack to hide close-in problems
				if (_Persona.VehicleAttached->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
				{
					obmainSetFlag2InThisTree(_Persona.VehicleAttached->VehicleObjectData->PlaneObject,OBJECT_ISCOCKPIT);
				}

				mlObjectSetInplaneObject(_Persona.VehicleAttached->VehicleObjectData->PlaneObject,
										_Persona.Position+1);
			}
		}
	}
	_InternalViewMode = _INTERNAL_NORMAL;
	_SetNormalInternalView();

	_SlewViewLRHeldDown = 0;
	_SlewViewUDHeldDown = 0;
	_SlewViewIOHeldDown = 0;

	/*
	_SlewViewLeftHeldDown = 0;
	_SlewViewRightHeldDown = 0;
	_SlewViewUpHeldDown = 0;
	_SlewViewDownHeldDown = 0;
	*/

	return 0;
}

static UINT32 _SlewView(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	// if we're panning, don't allowing this
	if(mathExactVectorLength2(&_ViewTarget) != 0)
	{
		_SlewViewLRHeldDown	= 0;
		_SlewViewUDHeldDown	= 0;
		_SlewViewIOHeldDown	= 0;
	}
	else
	{
		switch(Id)
		{
			case IMAP_HOLD_SLEWVIEWLEFT:
				_SlewViewLRHeldDown = (Param) ? 1 : 0;
			break;
			case IMAP_HOLD_SLEWVIEWRIGHT:
				_SlewViewLRHeldDown = (Param) ? -1 : 0;
			break;
			case IMAP_HOLD_SLEWVIEWUP:
				_SlewViewUDHeldDown = (Param) ? -1 : 0;
			break;
			case IMAP_HOLD_SLEWVIEWDOWN:
				_SlewViewUDHeldDown = (Param) ? 1 : 0;
			break;
			case IMAP_HOLD_SLEWVIEWIN:
				_SlewViewIOHeldDown = (Param) ? -1 : 0;
			break;
			case IMAP_HOLD_SLEWVIEWOUT:
				_SlewViewIOHeldDown = (Param) ? 1 : 0;
			break;
		}
	}

	return 0;
}
/*
static UINT32 _SlewViewLeft(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if (Param == 1)
	{
		_SlewViewLeftHeldDown = 1;
	}
	else
	{
		_SlewViewLeftHeldDown = 0;
	}

	return 0;
}

static UINT32 _SlewViewRight(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if (Param == 1)
	{
		_SlewViewRightHeldDown = 1;
	}
	else
	{
		_SlewViewRightHeldDown = 0;
	}

	return 0;
}

static UINT32 _SlewViewUp(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if (Param == 1)
	{
		_SlewViewUpHeldDown = 1;
	}
	else
	{
		_SlewViewUpHeldDown = 0;
	}

	return 0;
}

static UINT32 _SlewViewDown(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if (Param == 1)
	{
		_SlewViewDownHeldDown = 1;
	}
	else
	{
		_SlewViewDownHeldDown = 0;
	}

	return 0;
}
*/

static UINT32 _RollViewLeft(SINT32 Param,UINT32 Id,UINT32 Detach)
{
piVECTOR Att;

	Att.x = 0;
	Att.z = 0;
	Att.y = (FLOAT32)0.10;

	_AdjustViewAngle(&Att);

	return 0;
}

static UINT32 _RollViewRight(SINT32 Param,UINT32 Id,UINT32 Detach)
{
piVECTOR Att;

	Att.x = 0;
	Att.z = 0;
	Att.y = -(FLOAT32)0.10;

	_AdjustViewAngle(&Att);

	return 0;
}

void persSetView(UINT32 type, UINT32 level)
{
	// levels come in 0 based but need to be one based for the old code to work
	if(type == _INTERNAL_INSTRUMENT)
	{
		if (level < _Persona.InstrumentViewNum)
		{
			_SetInstrumentView(level);
			_Persona.InstrumentViewCurrent = level;
			_Persona.GunsightViewCurrent = 0;
			return;
		}
	}
	else if(type == _INTERNAL_GUNSIGHT)
	{
		if (level < _Persona.GunsightViewNum)
		{
			_SetGunsightView(level);
			_Persona.GunsightViewCurrent = level;
			_Persona.InstrumentViewCurrent = 0;
			return;
		}
	}
	else if(type == _INTERNAL_NORMAL)
	{
		_SetNormalInternalView();
		return;
	}

	// if we made it to here, fall back to our normal view
	// and return false
	_SetNormalInternalView();

	return;
}

static UINT32 _SetGunsightsView(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	if (Param == 0)
	{
		return 0;
	}
	if (_Persona.GunsightViewCurrent < _Persona.GunsightViewNum)
	{
		_SetGunsightView(_Persona.GunsightViewCurrent);
		_Persona.GunsightViewCurrent++;
	}
	else
	{
		_SetNormalInternalView();
	}

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &Id, &Param);

	_Persona.InstrumentViewCurrent = 0;

	return 0;
}

static UINT32 _SetInstrumentsView(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	if (Param == 0)
	{
		return 0;
	}
	if (_Persona.InstrumentViewCurrent < _Persona.InstrumentViewNum)
	{
		_SetInstrumentView(_Persona.InstrumentViewCurrent);
		_Persona.InstrumentViewCurrent++;
	}
	else
	{
		_SetNormalInternalView();
	}

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &Id, &Param);

	_Persona.GunsightViewCurrent = 0;


	return 0;
}

static UINT32 _SetExtFOV(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	UINT32 IsEye = 0;

	// if we're panning, don't allowing this
	if (Param == 0 || mathExactVectorLength2(&_ViewTarget) != 0)
		return 0;

	if (_Persona.VehicleAttached  && _Persona.VehicleAttached->VehicleType &&
		_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_FLOATER)
	{
		IsEye = 1;
	}

	if(Id == IMAP_TMP_CHANGE_FOV_DOWN)
	{
		if 		(_ExternalFieldOfView < 5.0f) 	_ExternalFieldOfView = 5.0f;
		else if (_ExternalFieldOfView < 10.0f) 	_ExternalFieldOfView = 10.0f;
		else if (_ExternalFieldOfView < 15.0f)	_ExternalFieldOfView = 15.0f;
		else if (_ExternalFieldOfView < 30.0f)	_ExternalFieldOfView = 30.0f;
		else if (_ExternalFieldOfView < 45.0f)	_ExternalFieldOfView = 45.0f;
		else if (_ExternalFieldOfView < 60.0f)	_ExternalFieldOfView = 60.0f;
		else if (_ExternalFieldOfView < 75.0f)	_ExternalFieldOfView = 75.0f;
		else if (_ExternalFieldOfView < 90.0f)	_ExternalFieldOfView = 90.0f;


		if 		(_InternalFieldOfView < 5.0f) 	_InternalFieldOfView = 5.0f;
		else if (_InternalFieldOfView < 10.0f) 	_InternalFieldOfView = 10.0f;
		else if (_InternalFieldOfView < 15.0f)	_InternalFieldOfView = 15.0f;
		else if (_InternalFieldOfView < 30.0f)	_InternalFieldOfView = 30.0f;
		else if (_InternalFieldOfView < 45.0f)	_InternalFieldOfView = 45.0f;
		else if (_InternalFieldOfView < 60.0f)	_InternalFieldOfView = 60.0f;
		else if (_InternalFieldOfView < 75.0f)	_InternalFieldOfView = 75.0f;
		else if (_InternalFieldOfView < 90.0f)	_InternalFieldOfView = 90.0f;

		_SetFinalExtFOVandFog(_ExternalFieldOfView);
	}
	else  if(Id == IMAP_TMP_CHANGE_FOV_UP)
	{
		if 		(_ExternalFieldOfView > 75.0f)	_ExternalFieldOfView = 75.0f;
		else if (_ExternalFieldOfView > 60.0f)	_ExternalFieldOfView = 60.0f;
		else if (_ExternalFieldOfView > 45.0f)	_ExternalFieldOfView = 45.0f;
		else if (_ExternalFieldOfView > 30.0f)	_ExternalFieldOfView = 30.0f;
		else if (_ExternalFieldOfView > 15.0f)	_ExternalFieldOfView = 15.0f;
		else if (_ExternalFieldOfView > 10.0f) 	_ExternalFieldOfView = 10.0f;
		else if	(_ExternalFieldOfView > 5.0f) 	_ExternalFieldOfView = 5.0f;
		else if	(_ExternalFieldOfView > 2.5f) 	_ExternalFieldOfView = 2.5f;

		if 		(_InternalFieldOfView > 75.0f)	_InternalFieldOfView = 75.0f;
		else if (_InternalFieldOfView > 60.0f)	_InternalFieldOfView = 60.0f;
		else if (_InternalFieldOfView > 45.0f)	_InternalFieldOfView = 45.0f;
		else if (_InternalFieldOfView > 30.0f)	_InternalFieldOfView = 30.0f;
		else if (_InternalFieldOfView > 15.0f)	_InternalFieldOfView = 15.0f;
		else if (_InternalFieldOfView > 10.0f) 	_InternalFieldOfView = 10.0f;
		else if	(_InternalFieldOfView > 5.0f) 	_InternalFieldOfView = 5.0f;
		else if	(_InternalFieldOfView > 2.5f) 	_InternalFieldOfView = 2.5f;

		_SetFinalExtFOVandFog(_ExternalFieldOfView);
	}
	else
	{
		if (_CameraMode == _EXTERNAL_CAMERA || IsEye)
		{
			if 		(_ExternalFieldOfView < 5.0f) 	_ExternalFieldOfView = 5.0f;
			else if (_ExternalFieldOfView < 10.0f) 	_ExternalFieldOfView = 10.0f;
			else if (_ExternalFieldOfView < 15.0f)	_ExternalFieldOfView = 15.0f;
			else if (_ExternalFieldOfView < 30.0f)	_ExternalFieldOfView = 30.0f;
			else if (_ExternalFieldOfView < 45.0f)	_ExternalFieldOfView = 45.0f;
			else if (_ExternalFieldOfView < 60.0f)	_ExternalFieldOfView = 60.0f;
			else if (_ExternalFieldOfView < 75.0f)	_ExternalFieldOfView = 75.0f;
			else if (_ExternalFieldOfView < 90.0f)	_ExternalFieldOfView = 90.0f;
			else if (_ExternalFieldOfView >= 90.0f)	_ExternalFieldOfView = 2.5f;

			_SetFinalExtFOVandFog(_ExternalFieldOfView);
		}
	}

	// Only adjust if linear fog.
	bool linearfog = false;

#ifdef USE_LINEAR_FOG
	linearfog = true;
#endif

	// fog change
	if (_CameraMode != _EXTERNAL_CAMERA && !IsEye && linearfog)
	{
		FogScaleSelector = (FogScaleSelector + 1) & 3;
		switch (FogScaleSelector)
		{
			case 1:	//if (_FogScaleMultiplier > 0.9999f)
				_FogScaleMultiplier = 0.75f;
			break;
			case 2:	//if (_FogScaleMultiplier > 0.7499f)
				_FogScaleMultiplier = 0.5f;
			break;
			case 3:	//if (_FogScaleMultiplier > 0.4999f)
				_FogScaleMultiplier = 0.25f;
			break;
			default:
				_FogScaleMultiplier = 1.0f;
			break;
		}

		trnskyUpdate();
		trnUpdateTerrainObjectLODs(1);
	}

	return 0;
}

FLOAT32 persGetFogScaleMultiplier(void)
{
	return _FogScaleMultiplier;
}

static UINT32 _AdjustViewAngle(piVECTOR * Att)
{
	_ViewAtt.x += Att->x;
	_ViewAtt.z += Att->z;
	_ViewAtt.y += Att->y;

	/*
	static FLOAT32 h, v, z;

	if(v != _ViewAtt.x || h != _ViewAtt.z || z != _ExternalZoom)
	{
		v = _ViewAtt.x;
		h = _ViewAtt.z;
		z = _ExternalZoom;

		FLOAT64 th, tv;

		th = mlRAD_TO_DEG(h);
		tv = mlRAD_TO_DEG(v);

		gr2NormalizeAngle(th);
		gr2NormalizeAngle(tv);

		uiChatPrintLine("Camera Attitude is %.2f %.2f %.2f", th, tv, z);
	}
	*/

	return 0;

	/*
	piMATRIX Mat,Mat1;

	mathEul_ToMatrix(&Mat,EulOrdXYZs,&_ViewAtt);
	mathEul_ToMatrix(&Mat1,EulOrdXYZs,Att);
	math3dMatrixMultiply(&Mat,&Mat,&Mat1);

	mathEul_FromMatrix(&_ViewAtt,&Mat,EulOrdXYZs);

	return 0;
	*/
}

bool _tpsCamera(piVECTOR* pos, piVECTOR* aim)
{
	vehVehicle* vehicle = persGetPersonaVehiclePtr();

	if(vehicle)
	{
		Vec3d forward		= Vec3d(0,1.0f,0);
		Vec3d right			= Vec3d(1.0f,0,0);
		Matrixf rotate		= Matrixf::rotate(PI, 0, 0, 1);

		Matrixd attitude	= Matrixd(vehicle->ModelData->VehicleAttMat);
		Vec3d position		= Vec3d(vehicle->ModelData->VehiclePos);

		bool trooper		= (vehicle->VehicleType->Type == VEH_TYPE_PERSON);

		forward = forward * attitude;
		right = right * attitude;

		if(trooper)
		{
			forward = forward * rotate;
			right = right * rotate;
		}
		else
		{
			UINT32 seat = 0;
			persGetPersonaPosition(&seat);
			FLOAT64 traverse = -(vehicle->ModelData->TurretTraverse[seat] * 2.0f);

			if(traverse)
			{
				rotate	= Matrixf::rotate(mlDEG_TO_RAD((float)traverse * 180.0f), 0, 0, 1);
				forward = forward * rotate;
				right	= right * rotate;
			}
		}

		Vec3d p = forward;

		p *= 10.0f;

		Vec3d aimPos = position + p;
		FLOAT64 moveBack = _ViewAtt.z;
		FLOAT64 moveRight = .2f;

		if(trooper && vehicle->GetAnim()->currentPoint == TPS_POINTTYPE_AIM)
		{
			static FLOAT64 b = .2f;
			static FLOAT64 r = .2f;
			moveBack = b;
			moveRight = r;
		}

		p = forward;
		p *= (-1.0f * moveBack);
		p += (right * moveRight);

		position = position + p;

		position.z() += (trooper) ? 1.8f : 2.5f;

		if(trooper)
			position.z() += fpFPSGetHeight();

		if(pos)
		{
			pos->x = -float(position.x() - WorldOriginOffset.x);
			pos->y = -float(position.y() - WorldOriginOffset.y);
			pos->z = float(position.z());
		}

		if(aim)
		{
			aim->x = -float(aimPos.x() - WorldOriginOffset.x);
			aim->y = -float(aimPos.y() - WorldOriginOffset.y);
			aim->z = float(aimPos.z());
		}
	}

	return true;
}

UINT32 persSetExternalLockedCamera(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	persSetExternalCameraAdjuster(nullptr);
	_ExternalViewMode = _EXTERNAL_LOCKED;
	return 0;
}

UINT32 persSetExternalRubberBandCamera(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_ExternalViewMode	= _EXTERNAL_LOCKED;

	if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
	{
		_ViewAtt.x			= 0.0f;
		_ViewAtt.y			= 0.0f;
		_ViewAtt.z			= 2.5f;
	}
	else
	{
		_ViewAtt.x			= 0.0f;
		_ViewAtt.y			= 0.0f;
		_ViewAtt.z			= 7.5f;
	}

	persSetExternalCameraAdjuster(_tpsCamera);
	return 0;
}

UINT32 persSetExternalFixedCamera(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	persSetExternalCameraAdjuster(nullptr);
	_ExternalViewMode = _EXTERNAL_FIXED;
	if (_CameraNeverSet == 0)
	{
		persSetFixedCameraPos(0,0,0);
		_CameraNeverSet = 1;
	}

	return 0;
}

UINT32 persSetFixedCameraPos(SINT32 Param,UINT32 Id,UINT32 Detach)
{
piMATRIX DummyMat;

	_GetPlayerPos(&_CameraPos,&DummyMat);
	_CameraPos.z += 1.0f;

	return 0;
}

UINT32 persAdjustExternalFixedCamera(piVECTOR* Adjustment)
{
	_CameraPos.x -= Adjustment->x;
	_CameraPos.y -= Adjustment->y;
	_CameraPos.z -= Adjustment->z;
	return 0;
}

static void _InstallViewControlCallbacks(void)
{
	if (_Persona.VehicleAttached && _Persona.VehicleAttached->VehicleType &&
		_Persona.VehicleAttached->VehicleType->Type != VEH_TYPE_FLOATER)
	{
		imapInstallControlCallback(IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS,persToggleInternalExternalViews);
		imapInstallControlCallback(IMAP_MOM_EXTERNAL_VIEW, persSetExternalCamera);
		imapInstallControlCallback(IMAP_MOM_INTERNAL_VIEW, persSetInternalCamera);
	}

	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWLEFT,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWRIGHT,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWUP,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWDOWN,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWIN,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_SLEWVIEWOUT,_SlewView);
	imapInstallControlCallback(IMAP_HOLD_ROLLVIEWLEFT,_RollViewLeft);
	imapInstallControlCallback(IMAP_HOLD_ROLLVIEWRIGHT,_RollViewRight);

	imapInstallControlCallback(IMAP_MOM_EXTERNAL_LOCKED,persSetExternalLockedCamera);
	imapInstallControlCallback(IMAP_MOM_EXTERNAL_RUBBERBAND,persSetExternalRubberBandCamera);
	imapInstallControlCallback(IMAP_MOM_EXTERNAL_FIXED,persSetExternalFixedCamera);
	imapInstallControlCallback(IMAP_MOM_SET_FIXED_CAMERA,persSetFixedCameraPos);

	imapInstallControlCallback(IMAP_TMP_CHANGE_FOV,_SetExtFOV);

	if(!EZ_MGR)
	{
		imapInstallControlCallback(IMAP_TMP_INSTRUMENT_VIEW,_SetInstrumentsView);
		imapInstallControlCallback(IMAP_TMP_GUNSIGHT_VIEW,_SetGunsightsView);
	}

	imapInstallControlCallback(IMAP_TMP_CHANGE_FOV_UP,_SetExtFOV);
	imapInstallControlCallback(IMAP_TMP_CHANGE_FOV_DOWN,_SetExtFOV);

	// ui map scaling
	imapInstallControlCallback(IMAP_MOM_UI_MAP_ZOOMIN,uiMapScaleAllMaps);
	imapInstallControlCallback(IMAP_MOM_UI_MAP_ZOOMOUT,uiMapScaleAllMaps);
}

UINT32 persPersonaIsCreatedOnHost(void)
{

	_Persona.PersonaInstalledOnHost = 1;

	uiChatPrintLine(loclGetLocalizationStringByID(loclPERSONA_CREATED_ON_HOST_MSG));

	return 0;
}

UINT32 persIsPersonaCreated(void)
{
	return _Persona.PersonaInstalledOnHost;
}

UINT32 persGetPlayerVelocity(piVECTOR * Vel)
{

	Vel->x = Vel->y = Vel->z = 0.0;

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	if (Vehicle == nullptr)
	{
		return 1;
	}
	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 1;
	}

	OBJECT* Object = Vehicle->VehicleObjectData->HitMap;
	if (Object == nullptr)
	{
		Object = Vehicle->VehicleObjectData->PlaneObject;
	}

	if (Object == nullptr)
	{
		return 0;
	}

	while (Object->child != nullptr)
	{
		Object = Object->child;
	}

	if (_CameraMode == _EXTERNAL_CAMERA &&
		_ExternalViewMode == _EXTERNAL_FIXED)
	{
		Vel->x = Vel->y = Vel->z = 0.0;
	}
	else
	{
		Vel->x = Object->vel.x;
		Vel->y = Object->vel.y;
		Vel->z = Object->vel.z;
	}

	return 0;
}

static UINT32 _SetNormalInternalView(void)
{
FLOAT32		horizon;
FLOAT32		fov;

//	trnmainGetHorizon(&horizon);
	horizon = piLongLikeFloat(FogEnd) * 2.0f;

	if (_CameraMode != _INTERNAL_CAMERA)
	{
		return 0;
	}

	fov = _InternalFieldOfView;

	vehVehicle* vehicle = persGetPersonaVehiclePtr();
	if (vehicle != nullptr)
	{
		if (vehicle->VehicleType != nullptr)
		{
			if (vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				fov = 75.0;
			}
		}
	}

	if (_Persona.PersonaEyeDOF == nullptr)
	{
		piSetFieldOfView(fov, 0.1f, horizon);

		return 0;
	}

	_Persona.GunsightViewCurrent = 0;
	_Persona.InstrumentViewCurrent = 0;
	_InternalViewMode = _INTERNAL_NORMAL;
	piSetFieldOfView(fov, 0.1f, horizon);
	trnUpdateTerrainObjectLODs(1);

	// hide scope gunsight
	if (_Persona.GunsightViewDOF != nullptr)
	{
		obmainClearFlagsInThisTree(_Persona.GunsightViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.GunsightViewDOF,0);
	}
	// hide instrument view gunsight
	if (_Persona.InstrumentViewDOF != nullptr)
	{
		obmainClearFlagsInThisTree(_Persona.InstrumentViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.InstrumentViewDOF,0);
	}

	return 0;
}

static UINT32 _SetInstrumentView(UINT32 ViewNum)
{
FLOAT32	horizon;
FLOAT32 FOV;

//	trnmainGetHorizon(&horizon);
	horizon = piLongLikeFloat(FogEnd) * 2.0f;

	if (_CameraMode != _INTERNAL_CAMERA)
	{
		return 0;
	}

	if (_Persona.InstrumentViewDOF == nullptr)
	{
		return 0;
	}

	_InternalViewMode = _INTERNAL_INSTRUMENT;

	if (ViewNum == 1)
	{
		persGunSightSetBaseFOV(45.0);
	}
	else
	{
		persGunSightSetBaseFOV(90.0);
	}
	if (persGunsightGetInstFieldOfView(&FOV) == TRUE)
	{
		piSetFieldOfView(FOV,0.1f,horizon);
	}
	else
	{
		piSetFieldOfView(_Persona.InstrumentViewFOV[ViewNum], 0.1f, horizon);
	}
	trnUpdateTerrainObjectLODs(1);

	// hide scope gunsight
	if (_Persona.GunsightViewDOF != nullptr)
	{
		obmainClearFlagsInThisTree(_Persona.GunsightViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.GunsightViewDOF,0);
	}
	// show scope instrument view
	if (_Persona.InstrumentViewDOF != nullptr)
	{
		obmainSetFlagsInThisTree(_Persona.InstrumentViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.InstrumentViewDOF,0);
	}

	return 0;
}

static UINT32 _SetGunsightView(UINT32 ViewNum)
{
FLOAT32	horizon;
FLOAT32 FOV;

//	trnmainGetHorizon(&horizon);
	horizon = piLongLikeFloat(FogEnd) * 2.0f;

	if (_CameraMode != _INTERNAL_CAMERA)
	{
		return 0;
	}

	if (_Persona.GunsightViewDOF == nullptr)
	{
		return 0;
	}

	_InternalViewMode = _INTERNAL_GUNSIGHT;
	if (ViewNum == 1)
	{
		persGunSightSetBaseFOV(45.0);
	}
	else
	{
		persGunSightSetBaseFOV(90.0);
	}
	if (persGunsightGetGunSightFieldOfView(&FOV) == TRUE)
	{
		piSetFieldOfView(FOV,0.1f,horizon);
	}
	else
	{
		piSetFieldOfView(_Persona.GunsightViewFOV[ViewNum], 0.1f, horizon);
	}
	trnUpdateTerrainObjectLODs(1);

	// show scope gunsight
	if (_Persona.GunsightViewDOF != nullptr)
	{
		obmainSetFlagsInThisTree(_Persona.GunsightViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.GunsightViewDOF,0);
	}
	// hide scope instrument view
	if (_Persona.InstrumentViewDOF != nullptr)
	{
		obmainClearFlagsInThisTree(_Persona.InstrumentViewDOF, OBJECT_VISIBLE);
		obAdjustObjectTreeVisListStatus(_Persona.InstrumentViewDOF,0);
	}

	return 0;
}

LOGICAL persSetGunsightView(FLOAT32 FieldOfView)
{
FLOAT32	horizon;

//	trnmainGetHorizon(&horizon);
	horizon = piLongLikeFloat(FogEnd) * 2.0f;
	_InternalViewMode = _INTERNAL_GUNSIGHT;
	piSetFieldOfView(FieldOfView, 0.1f, horizon);
	trnUpdateTerrainObjectLODs(1);

	return(TRUE);
}

void persSetNormalView(FLOAT32 FieldOfView)
{
	FLOAT32 horizon = piLongLikeFloat(FogEnd) * 2.0f;
	_InternalViewMode = _INTERNAL_NORMAL;
	piSetFieldOfView(FieldOfView, 0.1f, horizon);
	trnUpdateTerrainObjectLODs(1);
}

UINT32 persGetCrewIsDead(UINT32 Position)
{
	return (_PersonaHealth[Position].BrainOxyLevel > _BRAIN_BRAINDAMAGE);
}

UINT32 persGetCrewDeadForKillScore(UINT32 Position)
{
	if (_PersonaHealth[Position].BloodLevel > 0.375f)
	{
		return 1;
	}
	if (_PersonaHealth[Position].BrainOxyLevel > _BRAIN_BRAINDAMAGE)
	{
		return 1;
	}

	return 0;
}

UINT32 persResetVehicleG(void)
{
	_VehicleG = 1.0;

	return 0;
}

UINT32 persSetVehicleG(FLOAT32 VehicleG, vehVehicle* Vehicle)
{
	if (Vehicle == _Persona.VehicleAttached && Vehicle != nullptr)
	{
		// HACK: This isn't being properly accumulated for ground vehicles on rough terrain.
		if ( _Persona.VehicleAttached->VehicleType != nullptr && _Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_GROUND )
			_VehicleG = 1.0;
		else
			_VehicleG = VehicleG;
	}

	return 0;
}

static UINT32 _UpdatePersonaHealths(void)
{
	UINT32 i;
	FLOAT32 Delta;
	FLOAT32 DeltaTime;
	FLOAT32 OxyGain;
	FLOAT32 BrainLoss;
	FLOAT32 BrainStrength,FatigueStrength;
	LOGICAL IsFPS;
	FLOAT32 ATPGain;

	FLOAT32 TempNum;
	FLOAT64 Coefficient;
	FLOAT32 BPMax;

	static SINT32 _LastHealthUpdateTime;

	BrainStrength = FatigueStrength = 1.0f;
	_Persona.ControlsDisabled = FALSE;
	_Persona.BlackoutPercent = 0.0f;
	_Persona.Redout = 0;

	DeltaTime = (_LastHealthUpdateTime != 0) ? (frametime - _LastHealthUpdateTime) / 1000.0f : 0.0f;

	// sanity check
	if (DeltaTime > 0.5f)
		DeltaTime = 0.5f;

	_LastHealthUpdateTime = frametime;

	for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		persPersonaHealth& Health = _PersonaHealth[i];
		FLOAT32 brainOxyLevel = Health.BrainOxyLevel.Get();	// Covert access

		// brain & brain oxy
		if(_Persona.VehicleAttached != nullptr)
		{
			if(brainOxyLevel > _BRAIN_NOMORE_CONTROL)
				Delta = 1.0f - ((1.0f - Health.BloodCurrentLevel) * 2.0f - 1.0f); // hack to prevent g death
			else
				Delta = _VehicleG - Health.BloodPressure;

			if (Delta > 0.0f && Delta < 1.0f)
				OxyGain = (1.0f - Delta) * _BRAIN_GAIN_AT_NEUTRAL_G + Delta   * _BRAIN_GAIN_AT_ONE_GEE_OFF;

			if (Delta >= 1.0f)
				OxyGain = _BRAIN_GAIN_EACH_ADDITIONAL_POSG * (Delta - 1.0f)   + _BRAIN_GAIN_AT_ONE_GEE_OFF;

			if (Delta <= 0.0f && Delta > -1.0f)
				OxyGain = (1.0f - -Delta) * _BRAIN_GAIN_AT_NEUTRAL_G + -Delta * _BRAIN_GAIN_AT_ONE_GEE_OFF;

			if (Delta <= -1.0f)
				OxyGain = _BRAIN_GAIN_EACH_ADDITIONAL_NEGG * (-Delta - 1.0f)  + _BRAIN_GAIN_AT_ONE_GEE_OFF;

			if (OxyGain > 0.0)
				OxyGain *= (1.5f - Health.ATPCurrentLevel) / 1.5f;

			brainOxyLevel += -OxyGain * DeltaTime;
		}

		const FLOAT32 oxyMaxLevel = Health.BrainOxyMaxLevel;
		if (brainOxyLevel < oxyMaxLevel)
			brainOxyLevel = oxyMaxLevel;

		if (brainOxyLevel > 1.0f)
			brainOxyLevel = 1.0f;

		if (brainOxyLevel > _BRAIN_BRAINDAMAGE)
		{
			BrainLoss = brainOxyLevel - _BRAIN_BRAINDAMAGE;
			BrainLoss *= DeltaTime;

			brainOxyLevel += BrainLoss;
			if (brainOxyLevel > 1.0f)
				brainOxyLevel = 1.0f;

			if (brainOxyLevel < oxyMaxLevel)
				brainOxyLevel = oxyMaxLevel;
		}

		// handle effects of brain oxygen level
		if (i == _Persona.Position)
		{
			_Persona.Redout = (_VehicleG >= 0.0) ? 0 : 1;


			if (brainOxyLevel > _BRAIN_WHITEOUT_START &&
				brainOxyLevel < _BRAIN_BLACKOUT_FULL)
			{
				_Persona.BlackoutPercent =
						(brainOxyLevel - _BRAIN_WHITEOUT_START) /
						(_BRAIN_BLACKOUT_FULL - _BRAIN_WHITEOUT_START);
			}
			if (brainOxyLevel >= _BRAIN_BLACKOUT_FULL)
				_Persona.BlackoutPercent = 1.0f;


			if (brainOxyLevel >= _BRAIN_NOMORE_CONTROL)
				_Persona.ControlsDisabled = TRUE;

			if (brainOxyLevel >= _BRAIN_STRENGTH_FULLLOSS)
				BrainStrength = 0.0f;
			if (brainOxyLevel <= _BRAIN_STRENGTH_FULLLOSS &&
				brainOxyLevel > _BRAIN_STRENGTH_STARTLOSS)
			{
				BrainStrength = (brainOxyLevel - _BRAIN_STRENGTH_STARTLOSS) /
								(_BRAIN_STRENGTH_FULLLOSS - _BRAIN_STRENGTH_STARTLOSS);
//				BrainStrength = 1.0f - (FLOAT32)sqrt(BrainStrength);
				BrainStrength = 1.0f - BrainStrength * BrainStrength;
			}
		}

		// fatigue
		IsFPS = FALSE;
		if (_Persona.VehicleAttached != nullptr && _Persona.VehicleAttached->VehicleObjectData != nullptr &&
			_Persona.VehicleAttached->VehicleObjectData->PlaneObject != nullptr &&
			_Persona.VehicleAttached->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
		{
			IsFPS = TRUE;
		}

		Health.BrainOxyLevel = brainOxyLevel;
		// need to do something with fps differently
		if (IsFPS)
		{
			fpFPSGetATPGain(&ATPGain, DeltaTime);
		}
		else
		{
			ATPGain = _ATP_INVEHICLE_GAIN;

			// factor in G's:
			if (_VehicleG > 0.0)
			{
				TempNum = _VehicleG / 3.0f;
				TempNum *= TempNum;
				TempNum *= _ATP_PULLPOS_3G;

				ATPGain += TempNum;
			}
			else
			{
				TempNum = -_VehicleG / 3.0f;
				TempNum *= TempNum;
				TempNum *= _ATP_PULLNEG_3G;

				ATPGain += TempNum;
			}
		}

		Health.ATPCurrentLevel -= ATPGain * DeltaTime;
		if (Health.ATPCurrentLevel < Health.ATPMaxLevel)
			Health.ATPCurrentLevel = Health.ATPMaxLevel;

		if (Health.ATPCurrentLevel > 1.0)
			Health.ATPCurrentLevel = 1.0;

		if (Health.ATPLevel > Health.ATPCurrentLevel)
		{
			Health.ATPLevel -= _ATP_INJURY_INSTANTDROP_REGAIN * DeltaTime;
			if (Health.ATPLevel < Health.ATPCurrentLevel)
				Health.ATPLevel = Health.ATPCurrentLevel;
		}
		if (Health.ATPLevel < Health.ATPCurrentLevel)
		{
			Health.ATPLevel += _ATP_INJURY_INSTANTDROP_REGAIN * DeltaTime;
			if (Health.ATPLevel > Health.ATPCurrentLevel)
				Health.ATPLevel = Health.ATPCurrentLevel;
		}

		if (i == _Persona.Position)
		{
			FatigueStrength = (FLOAT32)sqrt(1.0f - Health.ATPLevel);
			if (Health.ATPLevel > _ATP_MIN_REQUIRED_FOR_ACTION)
				_Persona.ControlsDisabled = TRUE;

			// redout from pain
			_Persona.PainLevel = 0.0f;
			_Persona.PainLevel2 = 0.0f;
			if (Health.ATPCurrentLevel < 1.0)
			{
				if (Health.ATPLevel > Health.ATPCurrentLevel)
				{
					_Persona.PainLevel2 = (Health.ATPLevel - Health.ATPCurrentLevel) /
										(1.0f - Health.ATPCurrentLevel);
					_Persona.PainLevel2 *= 5.0;
					if (_Persona.PainLevel2 > 1.0f)
						_Persona.PainLevel2 = 1.0f;
				}
			}
			else
			{
				_Persona.PainLevel = 1.0f;
			}
			if (_Persona.PainLevel < Health.ATPMaxLevel)
				_Persona.PainLevel = Health.ATPCurrentLevel;
		}

		// blood and blood pressure

		if (Health.BloodCurrentLevel < Health.BloodLevel)
		{
			Delta = Health.BloodLevel - Health.BloodCurrentLevel;
			// compute coefficient that yields correct halftime
			Coefficient = 0.69314718 / _BLEED_RATE_HALFLIFE;
			TempNum = Delta * (1.0f - (FLOAT32)exp(-Coefficient * DeltaTime));

			Health.BloodCurrentLevel += TempNum;

			if (Health.BloodCurrentLevel > 1.0f)
				Health.BloodCurrentLevel = 1.0f;
		}

		BPMax = _MAX_BLOODPRESSURE;
		BPMax *= (1.0f - Health.BloodCurrentLevel) * 2.0f - 1.0f;

		if (Health.BloodPressure > _VehicleG)
		{
			Health.BloodPressure -= 1.0f / _BP_LAG_MAX_TO_MIN *
									_MAX_BLOODPRESSURE *
									(1.0f - Health.BloodCurrentLevel) *
									DeltaTime;
			if (Health.BloodPressure < _VehicleG)
				Health.BloodPressure = _VehicleG;
		}
		if (Health.BloodPressure < _VehicleG)
		{
			Health.BloodPressure += 1.0f / _BP_LAG_MIN_TO_MAX *
									_MAX_BLOODPRESSURE *
									(1.0f - Health.BloodCurrentLevel) *
									DeltaTime;
			if (Health.BloodPressure > _VehicleG)
				Health.BloodPressure = _VehicleG;
		}
		if (BPMax < 0.0)
		{
			Health.BloodPressure = BPMax;
		}
		else
		{
			if (Health.BloodPressure > BPMax)
				Health.BloodPressure = BPMax;

			if (Health.BloodPressure < _MIN_BLOODPRESSURE)
				Health.BloodPressure = _MIN_BLOODPRESSURE;
		}
	}

	_Persona.Strength = BrainStrength * FatigueStrength *
					(1.0f - _Persona.PainLevel);

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis)
	{
		piTLVERTEX	Vertex[HEALTH_HISTORY_SIZE];
		memset(Vertex, 0x0, HEALTH_HISTORY_SIZE * sizeof(piTLVERTEX));

		_Persona.BlackoutPercent = 0.0f;
		_Persona.ControlsDisabled = 0;
		_Persona.PainLevel = 0.0f;
		_Persona.PainLevel2 = 0.0f;
		_Persona.Strength = 1.0f;

		//Memmove/memcpy here causes memory crash with smartheap
		for(i = 0; i < ML_MAX_CREW_POSITIONS; i++)
		{
			for(int q = 1; q < HEALTH_HISTORY_SIZE; q++)
			{
				_PersonaHealthHistory[i][q] = _PersonaHealthHistory[i][q-1];
			}

			_PersonaHealthHistory[i][0] = _PersonaHealth[i];
		}

		memmove(&_GHistory[1], _GHistory, (HEALTH_HISTORY_SIZE-1) * sizeof(FLOAT32));
		_GHistory[0] = _VehicleG;

		if(ogl::UseDeprecatedPath())
		{
			piDisableTexture();
			piDisableMaterial();
		}
		else
		{
			basiccolor->Apply();
		}

		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, HEALTH_HISTORY_SIZE);
		for(i = 0; i < HEALTH_HISTORY_SIZE; i++)
		{
			persPersonaHealth& Health = _PersonaHealthHistory[_Persona.Position][i];
			Vertex[i].sx = (FLOAT32)(1000 - i);
			Vertex[i].sy = 15 + Health.BrainOxyLevel * 100;
			Vertex[i].sz = 0;
			Vertex[i].color = colorBLUEALPHA;
		}
		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, HEALTH_HISTORY_SIZE);
		for(i = 0; i < HEALTH_HISTORY_SIZE; i++)
		{
			persPersonaHealth& Health = _PersonaHealthHistory[_Persona.Position][i];
			Vertex[i].sx = (FLOAT32)(1000 - i);
			Vertex[i].sy = 15 + Health.BloodPressure * 25;
			Vertex[i].sz = 0;
			Vertex[i].color = colorREDALPHA;
		}
		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, HEALTH_HISTORY_SIZE);
		for(i = 0; i < HEALTH_HISTORY_SIZE; i++)
		{
			Vertex[i].sx = (FLOAT32)(1000 - i);
			Vertex[i].sy = 15 + _GHistory[i] * 10;
			Vertex[i].sz = 0;
			Vertex[i].color = colorGREENALPHA;
		}
		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, HEALTH_HISTORY_SIZE);
		for(i = 0; i < HEALTH_HISTORY_SIZE; i++)
		{
			persPersonaHealth& Health = _PersonaHealthHistory[_Persona.Position][i];
			Vertex[i].sx = (FLOAT32)(1000 - i);
			Vertex[i].sy = 15 + Health.BrainOxyMaxLevel * 100;
			Vertex[i].sz = 0;
			Vertex[i].color = colorBLUE50;
		}
		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, HEALTH_HISTORY_SIZE);

		Vertex[0].sx = 1000; Vertex[0].sy = 15 + _BRAIN_BRAINDAMAGE * 100; Vertex[0].color = colorWHITEALPHA;
		Vertex[1].sx = 400;  Vertex[1].sy = 15 + _BRAIN_BRAINDAMAGE * 100; Vertex[1].color = colorWHITEALPHA;
		piRenderScene(piPT_LINELIST, piVT_TLVERTEX, &Vertex, 2);
	}
#endif

	// handle fps death
	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	if (Vehicle != nullptr)
	{
		if (Vehicle->VehicleType != nullptr)
		{
			if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				if (_Persona.BlackoutPercent > 0.999 ||
					_PersonaHealth[_Persona.Position].ATPMaxLevel > 0.99)
				{
					if (_Persona.FPSIsDead == 0)
					{
						// tell animation that we're dead
						fpFPSKillTrooper();
						_Persona.FPSIsDead = 1;
					}
				}
			}
		}
	}

	return 0;
}

UINT32 persGetHealthStats(FLOAT32 * BrainOxy,
						  FLOAT32 * ATPLevel,
						  FLOAT32 * BloodPressure,
						  FLOAT32 * BloodLevel)
{

	if (BrainOxy != nullptr)
	{
		*BrainOxy = (1.0f - _PersonaHealth[_Persona.Position].BrainOxyLevel);
	}
	if (ATPLevel != nullptr)
	{
		*ATPLevel = (1.0f - _PersonaHealth[_Persona.Position].ATPLevel);
	}
	if (BloodPressure != nullptr)
	{
		*BloodPressure = _PersonaHealth[_Persona.Position].BloodPressure;
	}
	if (BloodLevel != nullptr)
	{
		*BloodLevel = (1.0f - _PersonaHealth[_Persona.Position].BloodCurrentLevel);
	}

	return 0;
}

UINT32 persGetMaxBrainOxy(FLOAT32 * MaxBrainOxy, FLOAT32* BlackoutFullPercent)
{
	if (MaxBrainOxy != nullptr)
	{
		*MaxBrainOxy = (1.0f - _PersonaHealth[_Persona.Position].BrainOxyMaxLevel);
	}

	if (BlackoutFullPercent != nullptr)
	{
		*BlackoutFullPercent = (1.0f - _BRAIN_BLACKOUT_FULL);
	}

	return 0;
}

UINT32 persDrawRedoutBlackout(void)
{
	piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

	//if ((SupportedSrcBlendModes & piBLENDCAPS_INVSRCCOLOR) &&
	//	(SupportedDestBlendModes & piBLENDCAPS_SRCCOLOR))
	if ((SupportedSrcBlendModes & piBLENDCAPS_SRCALPHA) &&
		(SupportedDestBlendModes & piBLENDCAPS_ONE))
	{
		_DrawRedoutBlackoutNew();
	}
	else
	{
#if(0)
		_DrawRedoutBlackoutOld();
#endif
	}

	piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

	return 0;
}

#if(0)
static UINT32 _DrawRedoutBlackoutOld(void)
{
	static UINT32 Color;
	static FLOAT32 Red,Green,Blue;
	static FLOAT32 AlphaValue;

	// redout/blackout
	if (_Persona.BlackoutPercent > 0.001f && _CameraMode == _INTERNAL_CAMERA)
	{
		if (_Persona.Redout == 1)
		{
			Red = 1.0f;
			Green = 0.0f;
			Blue = 0.0f;
		}
#if(0)
		else
		{
			Red = 0.5f;
			Green =0.5f;
			Blue = 0.5f;
		}
#endif
		else if (_Persona.BlackoutPercent > 0.9f)
		{
			AlphaValue = 1.0f;
			Red *= 1.0f - (_Persona.BlackoutPercent - 0.5f) * 2.0f;
			Green *= 1.0f - (_Persona.BlackoutPercent - 0.5f) * 2.0f;
			Blue *= 1.0f - (_Persona.BlackoutPercent - 0.5f) * 2.0f;
		}
		else
		{
			AlphaValue = _Persona.BlackoutPercent * 0.75f;
		}

		if(g_postRenderFilter)
		{
			PostRenderFilter::GetInstance().setblackout(Red, Green, Blue, AlphaValue);
		}
		else
		{
#if platOS == OS_MACOS
			Color = piRGBA(Red,Green,Blue,fmin(AlphaValue, 0.75));
#else
			Color = piRGBA(Red,Green,Blue, std::min(AlphaValue, 0.75f));
#endif
			draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);
		}
	}
	// pain level
	else if (_Persona.PainLevel > 0.001f && _CameraMode == _INTERNAL_CAMERA)
	{
		Red = 0.5f;
		Green = 0.0f;
		Blue = 0.0f;
		AlphaValue = _Persona.PainLevel * _Persona.PainLevel + _Persona.PainLevel2;
		if (AlphaValue > 1.0f)
		{
			AlphaValue = 1.0f;
		}

		if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
			AlphaValue = .25f;

		if(g_postRenderFilter)
		{
			PostRenderFilter::GetInstance().setblackout(Red, Green, Blue, AlphaValue);
		}
		else
		{
			Color = piRGBA(Red,Green,Blue, std::min(AlphaValue, 0.75f));
			draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);
		}
	}

	_CheckUnderground();

	return 0;
}
#endif

// uses the multiplier blend mode
static UINT32 _DrawRedoutBlackoutNew(void)
{
	UINT32 Color;
	FLOAT32 Red,Green,Blue;
	FLOAT32 AlphaValue;
	FLOAT32 SmokeLevel;

	if (_Persona.VehicleAttached != nullptr)
	{
		SmokeLevel = _Persona.VehicleAttached->StateAnimation.CockpitSmokeLevel;

		if (_CameraMode == _INTERNAL_CAMERA && SmokeLevel > 0.8f)
		{
			Red = 0.0f;
			Green = 0.0f;
			Blue = 0.0f;
			AlphaValue = (SmokeLevel - 0.8f) / 0.25f;

			Color = piRGBA(Red,Green,Blue,AlphaValue);
			draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);
		}
	}
	else
	{
		return 0;
	}

#if(0)
	LOGICAL isFPS = 0;

	static UINT32	Redout=0;
	static FLOAT32	Percent=0.0f;
	static SINT32	lastPainTime;

	FLOAT32	painTimeFader;

	if ((Percent + 0.001f) < _Persona.BlackoutPercent)
	{
		Redout = _Persona.Redout;
	}

	Percent = _Persona.BlackoutPercent;
	if (Percent <= 0.001f)
	{
		Redout = _Persona.Redout;
	}

	if (_Persona.VehicleAttached != nullptr && _Persona.VehicleAttached->VehicleObjectData != nullptr &&
		_Persona.VehicleAttached->VehicleObjectData->PlaneObject != nullptr &&
		_Persona.VehicleAttached->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
	{
		isFPS = TRUE;
	}

	if (isFPS)
	{
		painTimeFader = 1.0f - ((frametime - lastPainTime)/1000.0f)/ fpFPSGetRedScreenTime();
		if (painTimeFader < 0)
			painTimeFader = 0;
	}
	else
		painTimeFader = 1.0f;

	// pain level
	if (_Persona.PainLevel > 0.001f && _CameraMode == _INTERNAL_CAMERA)
	{
		FLOAT32	baseAlpha;

		// arbitrary, copied from what was here before..
		baseAlpha = _Persona.PainLevel * 0.85f;

		if (isFPS)
		{
			if (_Persona.FPSIsDead)
			{
				baseAlpha = 1.0f;
			}
			else
			if (baseAlpha > fpFPSGetRedScreenMinAlpha())
			{
				baseAlpha = max (fpFPSGetRedScreenMinAlpha(), baseAlpha * painTimeFader);
			}
		}
		else
		{
			baseAlpha = max (0.15f, baseAlpha * painTimeFader);
		}

		Color = piRGBA(_REDOUT_RED,
						_REDOUT_GREEN,
						_REDOUT_BLUE,
						baseAlpha);
		draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);

		// pain shock
		if (_Persona.PainLevel2 > 0)
		{
			Color = piRGBA(_REDOUT_RED,
							_REDOUT_GREEN,
							_REDOUT_BLUE,
							_Persona.PainLevel2);
			draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);

			lastPainTime = frametime;
		}

		if (_Persona.PainLevel > 0.85f)
		{
			Color = piRGBA(_REDOUT_RED,
							_REDOUT_GREEN,
							_REDOUT_BLUE,
							(_Persona.PainLevel - 0.85f) * (1.0f/0.15f) * painTimeFader);
			draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);
		}
	}

	// redout/blackout
	if (_Persona.BlackoutPercent > 0.001f && _CameraMode == _INTERNAL_CAMERA)
	{
		if (Redout != 1)
		{
			Color = piRGBA(0.0f,
							0.0f,
							0.0f,
							_Persona.BlackoutPercent);
		}
		else
		{
			Color = piRGBA(_REDOUT_RED,
							_REDOUT_GREEN,
							_REDOUT_BLUE,
							_Persona.BlackoutPercent);
		}

		draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,Color);
	}
#endif

	_CheckUnderground();

	return 0;
}

static UINT32 _CheckUnderground(void)
{

	// and, of course, disable visibility if the camera goes underground
	// anything 10cm above the ground and below counts
	if (_Persona.VehicleAttached != nullptr && _Persona.VehicleAttached->ModelData != nullptr &&
		(_Persona.VehicleAttached->VehicleType->Type != VEH_TYPE_FLOATER && !SuperFloat))
	{
		piVECTOR TempVec,TempVec2;

		TempVec.x = -playpos.x;
		TempVec.y = -playpos.y;
		TempVec.z = -playpos.z;

		TempVec2 = TempVec;
		TempVec2.z += 30000.0f;
//		TempVec.z += 0.1f;

		trncdIgnoreTreeCollide(TRUE);

		if (trncdCollideDownMediumList(&TempVec,&TempVec2) == FALSE)
		{
			// only do this if there's something to collide with
			TempVec.z -= 10000.0f;
			if (trncdCollideDownMediumList(&TempVec,&TempVec2) == TRUE)
			{
				draw2dTransparentRectangle(0,0,piGetWidth(),piGetHeight(),zdepthLAYER7,colorBLACKALPHA);
			}
		}

		trncdIgnoreTreeCollide(FALSE);
	}

	return 0;
}

UINT32 persUpdateCockpitSmoke(void)
{
	if (_CameraMode == _EXTERNAL_CAMERA)
	{
		return 0;
	}
#if(0)
FLOAT32 TimeBurning;
FLOAT32 SmokeOpacity;
FLOAT64VECTOR WorldOriginOffset;
FLOAT64VECTOR Pos;
piVECTOR RotationRate;
FLOAT32 Delta,SmokeLevel;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	// cockpit smoke effect
	if (_Persona.VehicleAttached != nullptr)
	{
		SmokeOpacity = 0.0f;

		if (_Persona.VehicleAttached->StateAnimation.TankOnFire == TRUE)
		{
			TimeBurning = (FLOAT32)(frametime - _Persona.VehicleAttached->StateAnimation.TankOnFireStartTime) /
										1000.0f;
			if (TimeBurning > 8.0f)
			{
				SmokeOpacity = 1.0f;
			}
			else
			{
				SmokeOpacity = TimeBurning / 8.0f;
			}

		}

		if (_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel > 0.0f)
		{
			Delta = (FLOAT32)(frametime - _Persona.VehicleAttached->StateAnimation.MainGunLastUpdateTime) /
							1000.0f;
			_Persona.VehicleAttached->StateAnimation.MainGunLastUpdateTime = frametime;

			_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel -= Delta * 0.01f;
			if (_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel < 0.0f)
			{
				_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel = 0.0f;
			}

			if (_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel >
					_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel)
			{
				_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel =
					_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel;
			}

			if (_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel >
				_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel)
			{
				_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel +=
					0.1f * Delta;
				if (_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel >
					_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel)
				{
					_Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel =
						_Persona.VehicleAttached->StateAnimation.MainGunSmokeLevel;
				}
			}

			SmokeLevel = _Persona.VehicleAttached->StateAnimation.LastMainGunSmokeLevel;

			if (SmokeLevel > 0.5f)
			{
				SmokeLevel = 0.5f;
			}

			if (_InternalViewMode == _INTERNAL_NORMAL ||
				_InternalViewMode == _INTERNAL_INSTRUMENT)
			{
				SmokeOpacity += SmokeLevel;
				if (SmokeOpacity > 1.0f)
				{
					SmokeOpacity = 1.0f;
				}
			}
		}

		if (SmokeOpacity > 0.01f)
		{
			Pos.x = playdir.x * 0.06 - playpos.x;
			Pos.y = playdir.y * 0.06 - playpos.y;
			Pos.z = playdir.z * 0.06 - playpos.z;
			Pos.x += WorldOriginOffset.x;
			Pos.y += WorldOriginOffset.y;
			Pos.z = -(Pos.z + WorldOriginOffset.z);

			RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
			RotationRate.y = mathGetFloat32ChaoticNumber() * 1.0f * 2.0f - 1.0f;
			RotationRate.z = 0.0f;

			if (SmokeOpacity > 0.5f)
			{
				FXCreateNewParticle(&Pos,
									nullptr,
									0.0f,
									nullptr,
									(SmokeOpacity - 0.5f) / 0.5f,
									0.0f,
									0.0f,
									0.0f,
									0.50f,
									0.50f,
									0.50f,
									0.50f,
									frametime,
									frametime + 50,
									frametime + 50,
									frametime + 50,
									11,
									&RotationRate,
									PARTICLE_FLAG_USESROTATION |
										PARTICLE_FLAG_INCOCKPIT_PARTICLE |
										PARTICLE_FLAG_PRIORITY |
										PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME,
									nullptr);

				Pos.x += playdir.x * 0.001;
				Pos.y += playdir.y * 0.001;
				Pos.z += playdir.z * 0.001;

				FXCreateNewParticle(&Pos,
									nullptr,
									0.0f,
									nullptr,
									SmokeOpacity,
									0.0f,
									0.0f,
									0.0f,
									0.50f,
									0.50f,
									0.50f,
									0.50f,
									frametime,
									frametime + 50,
									frametime + 50,
									frametime + 50,
									12,
									&RotationRate,
									PARTICLE_FLAG_USESROTATION |
										PARTICLE_FLAG_INCOCKPIT_PARTICLE |
										PARTICLE_FLAG_PRIORITY |
										PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME,
									nullptr);

			}
			else
			{
				FXCreateNewParticle(&Pos,
									nullptr,
									0.0f,
									nullptr,
									SmokeOpacity,
									0.0f,
									0.0f,
									0.0f,
									0.50f,
									0.50f,
									0.50f,
									0.50f,
									frametime,
									frametime + 50,
									frametime + 50,
									frametime + 50,
									12,
									&RotationRate,
									PARTICLE_FLAG_USESROTATION |
										PARTICLE_FLAG_INCOCKPIT_PARTICLE |
										PARTICLE_FLAG_PRIORITY |
										PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME,
									nullptr);
			}
			_Persona.VehicleAttached->StateAnimation.CockpitSmokeLevel = SmokeOpacity;
		}

	}
#endif

	return 0;
}

UINT32 persGetControlStatus(FLOAT32 * StrengthControl, LOGICAL * ControlsDisabled)
{
	if (StrengthControl != nullptr)
	{
		*StrengthControl = _Persona.Strength;
	}
	if (ControlsDisabled != nullptr)
	{
		*ControlsDisabled = _Persona.ControlsDisabled;
	}

	return 0;
}

UINT32 persGetGDistortion(piMATRIX * RetMat)
{
piMATRIX Mat,Mat1,Mat2;
FLOAT64VECTOR Acc;
FLOAT64 AccVal;
piVECTOR Att;

	if (_Persona.PersonaBodyDOF == nullptr ||
		_Persona.VehicleAttached == nullptr ||
		_Persona.VehicleAttached->ModelData == nullptr ||
		_CameraMode == _EXTERNAL_CAMERA)
	{
		*RetMat = IdentityMatrix;
		return 0;
	}

	Mat = IdentityMatrix;

	Acc = _Persona.VehicleAttached->ModelData->VehicleAcc;
	Acc.x = -Acc.x;
	Acc.z -= 9.8;
	AccVal = mathExactVectorLength(&Acc);

	Att.x = -(FLOAT32)atan2(Acc.y,Acc.z);
	Att.y = Att.z = 0.0f;
	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att);
	mathVectorMultiply64(&Acc,&Acc,&Mat1);
	Att.x = 0.0f;
	Att.y = -(FLOAT32)atan2(Acc.z,Acc.x);
	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&Att);
	mathVectorMultiply64(&Acc,&Acc,&Mat2);
	math3dMatrixMultiply(&Mat1,&Mat1,&Mat2);

	Mat = Mat1;
	Mat2 = IdentityMatrix;
	Mat2._11 = 0.995f + 0.002f * (FLOAT32)(fabs(Acc.x) / 9.8);
	math3dMatrixMultiply(&Mat,&Mat,&Mat2);
	math3dInvertMatrix(&Mat1,&Mat1);
	math3dMatrixMultiply(&Mat,&Mat,&Mat1);


	*RetMat = Mat;
	return 0;
}

UINT32 persApplyDamage(FLOAT64 Damage,UINT32 Position, UINT32 Location, LOGICAL ATPOnly)
{
	if(vehmainHasBailedOut())
		return 0;

	if (Position >= ML_MAX_CREW_POSITIONS)
	{
		return 0;
	}

    Write_Debug( "Damage %f position %u location %u atponly %u\n", Damage, Position, Location, ATPOnly );

	persPersonaHealth& Health = _PersonaHealth[Position];
	if (Location == PERS_HITLOCATION_HEAD)
	{
		if (ATPOnly == FALSE)
		{
			Health.BrainOxyMaxLevel += (FLOAT32)Damage * _BRAIN_DAMAGE_PER_JOULE;
			Health.BrainOxyLevel += (FLOAT32)Damage * _BRAIN_DAMAGE_PER_JOULE;
			if (Health.BrainOxyMaxLevel > 1.0)
			{
				Health.BrainOxyMaxLevel = 1.0;
			}
			if (Health.BrainOxyLevel > 1.0)
			{
				Health.BrainOxyLevel = 1.0;
			}
			Health.BloodLevel += (FLOAT32)Damage * _BLOOD_LOSS_PER_JOULE_HEAD;
			Health.ATPMaxLevel += _ATP_INJURY_HEAD_PER_JOULE * (FLOAT32)Damage;
		}

		Health.ATPLevel += _ATP_INJURY_HEAD_PER_JOULE * (FLOAT32)Damage;
		Health.ATPLevel -= _ATP_INJURY_INSTANTDROP_PER_JOULE * (FLOAT32)Damage;
		if (Health.ATPMaxLevel > 1.0)
		{
			Health.ATPMaxLevel = 1.0;
		}
		if (Health.ATPLevel > 1.0)
		{
			Health.ATPLevel = 1.0;
		}
	}
	if (Location == PERS_HITLOCATION_TORSO)
	{
		if (ATPOnly == FALSE)
		{
			Health.BloodLevel += (FLOAT32)Damage * _BLOOD_LOSS_PER_JOULE_TORSO;
			Health.ATPMaxLevel += _ATP_INJURY_TORSO_PER_JOULE * (FLOAT32)Damage;
		}

		Health.ATPLevel += _ATP_INJURY_TORSO_PER_JOULE * (FLOAT32)Damage;
		Health.ATPLevel -= _ATP_INJURY_INSTANTDROP_PER_JOULE * (FLOAT32)Damage;
		if (Health.ATPMaxLevel > 1.0)
		{
			Health.ATPMaxLevel = 1.0;
		}
		if (Health.ATPLevel > 1.0)
		{
			Health.ATPLevel = 1.0;
		}
	}
	if (Location == PERS_HITLOCATION_ARMS)
	{
		if (ATPOnly == FALSE)
		{
			Health.BloodLevel += (FLOAT32)Damage * _BLOOD_LOSS_PER_JOULE_ARMS;
			Health.ATPMaxLevel += _ATP_INJURY_ARMS_PER_JOULE * (FLOAT32)Damage;
		}

		Health.ATPLevel += _ATP_INJURY_ARMS_PER_JOULE * (FLOAT32)Damage;
		Health.ATPLevel -= _ATP_INJURY_INSTANTDROP_PER_JOULE * (FLOAT32)Damage;
		if (Health.ATPMaxLevel > 1.0)
		{
			Health.ATPMaxLevel = 1.0;
		}
		if (Health.ATPLevel > 1.0)
		{
			Health.ATPLevel = 1.0;
		}
	}
	if (Location == PERS_HITLOCATION_LEGS)
	{
		if (ATPOnly == FALSE)
		{
			Health.BloodLevel += (FLOAT32)Damage * _BLOOD_LOSS_PER_JOULE_LEGS;
			Health.ATPMaxLevel += _ATP_INJURY_LEGS_PER_JOULE * (FLOAT32)Damage;
		}

		Health.ATPLevel += _ATP_INJURY_LEGS_PER_JOULE * (FLOAT32)Damage;
		Health.ATPLevel -= _ATP_INJURY_INSTANTDROP_PER_JOULE * (FLOAT32)Damage;
		if (Health.ATPMaxLevel > 1.0)
		{
			Health.ATPMaxLevel = 1.0;
		}
		if (Health.ATPLevel > 1.0)
		{
			Health.ATPLevel = 1.0;
		}
	}

	if(Damage)
		fpPPOCancel();

	return 0;
}

UINT32 _UpdatePersonaSounds(void)
{
FLOAT32 Volume;
vehVehicle* Vehicle;
mlPositionData * CrewData;
SINT32 PosNum;
FLOAT64VECTOR Pos;
SINT32 SoundNum;
UINT32 i;
FLOAT32 Pitch;

	Volume = 1.0f;

	if (_CameraMode == _INTERNAL_CAMERA)
	{
		if (_Persona.BlackoutPercent > 0.5f)
		{
			Volume *= 1.0f - (_Persona.BlackoutPercent - 0.5f) / 0.5f;
		}
	}

	if(_Persona.earringSoundID > 0)
	{
		Volume *= 0.1f;
	}

	fxSndSetOverallVolume(Volume);

	if (_Persona.PainLevel > 0.001f && _Persona.PainSoundId == 0)
	{
		SINT32 iID = fxSndGetNewSound();

		if(iID != -1)
		{
			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 0.0f);
			fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
			fxSndSetSoundName(iID, "sounds\\painsound.wav");
			fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

			if(fxSndInitSound(iID))
			{
				_Persona.PainSoundId = iID;
			}
		}
	}
	if (_Persona.PainLevel <= 0.001f && _Persona.PainSoundId > 0)
	{
		fxSndStopSound(_Persona.PainSoundId);
		_Persona.PainSoundId = 0;
	}

	if (_Persona.PainSoundId > 0)
	{
		Volume = _Persona.PainLevel;
		if (Volume > 1.0f)
		{
			Volume = 1.0f;
		}

		fxSndChangeVolume(_Persona.PainSoundId,Volume);
	}

	Volume = 1.0f;

	if (_CameraMode == _INTERNAL_CAMERA)
	{
		PosNum = _Persona.Position;
		if (_Persona.VehicleAttached == nullptr)
		{
			return 0;
		}
		if (_Persona.VehicleAttached->ModelData == nullptr)
		{
			return 0;
		}
		Vehicle = _Persona.VehicleAttached;
		CrewData = &_Persona.VehicleAttached->ModelData->CrewData;

		Volume *= persCanopyVolumeMultiplier(PosNum);
		Volume *= persHatchVolumeMultiplier(PosNum);

		// handle engine
#if platOS != OS_MACOS
		FLOAT32 TempNum;

		TempNum = (1.0f - _Persona.VehicleAttached->EngineIdleVolume) *
					(CrewData->EngineIdleVol[PosNum]) + (1.0f - CrewData->EngineIdleVol[PosNum]);
		Volume *= TempNum;

		TempNum = (1.0f - _Persona.VehicleAttached->EnginePowerVolume) *
					(CrewData->EngineFullPowerVol[PosNum]) + (1.0f - CrewData->EngineFullPowerVol[PosNum]);
		Volume *= TempNum;
#endif

	}
	fxSndSetExternalVolume(Volume);

	LOGICAL hit = FALSE;

	// handle hit sounds
	if (_Persona.VehicleAttached != nullptr)
	{
		for (i=0;i<MAX_DAMAGE_SOUND_QUE;i++)
		{
			if (_Persona.DamagePlayTime[i] != 0)
			{
				if (frametime - _Persona.DamagePlayTime[i] >= 0)
				{
					SINT32 ID = fxSndGetNewSound();

					if(ID != -1)
					{
						// play the sound
						Pitch = 1.0f + 0.4f * mathGetFloat32ChaoticNumber() - 0.2f;
						SoundNum = (UINT32)(mathGetChaoticNumber()) % 3;
						Pos.x = _Persona.DamagePlayLocation[i].x;
						Pos.y = _Persona.DamagePlayLocation[i].y;
						Pos.z = _Persona.DamagePlayLocation[i].z;

						fxSndSetPitch(ID, Pitch);
						fxSndSetVolume(ID, _Persona.DamageVolume[i]);
						fxSndSetPriority(ID, IDE_FP_PRIORITY_MANDATORY);
						fxSndSetSoundName(ID, _Persona.VehicleAttached->VehicleType->HitSounds[SoundNum]);
						fxSndSetSoundType(ID, TYPE_FINITE_SOUND);

						switch(_Persona.VehicleAttached->VehicleType->Type)
						{
						case VEH_TYPE_PERSON:
						case VEH_TYPE_AIR:
						case VEH_TYPE_GROUND:
							{
								fxSndSetPlayType(ID, DEF_PLAYTYPE_HEAD);
								fxSndSetVolume(ID, 1.0f);
								break;
							}
						default:
							{
								fxSndSetPlayType(ID, DEF_PLAYTYPE_OBJECT);
								fxSndSetPosition(ID, Pos.x, Pos.y, Pos.z);
								fxSndSetEmitterObject(ID, _Persona.DamagePlayObject[i]);
								fxSndSetMinDistance(ID, 10.0f);
								fxSndSetMaxDistance(ID, 50.0f);
								break;
							}
						}

						fxSndInitSound(ID);

					}

					_Persona.DamagePlayTime[i] = 0;

					hit = TRUE;
				}
			}
		}
	}

	/*
	bool fps		= persIsPersonaVehicleType(VEH_TYPE_PERSON);
	bool tired	= (fps && _PersonaHealth[0].ATPLevel >= .2f) ? true : false;

	//.78s breath1.wav
	//2.87s breath2

	if(hit || tired)
	{
		if(_Persona.heartbeatID > 0)
		{
			fxSndChangeVolume(_Persona.heartbeatID, HEARTBEAT_MAX);
		}
		else
		{
			// Avoid repetitive annoying sound
			if(fps)
			{
				_playheartbeat(HEARTBEAT_MAX, 3000);
			}
		}
	*/
	FLOAT32 atp		= 1.0f - _PersonaHealth[0].ATPCurrentLevel;
	FLOAT32 atpMax	= 1.0f - _PersonaHealth[0].ATPMaxLevel;
	FLOAT32 atpPctg	= atp / atpMax;

	bool fps			= (persIsPersonaVehicleType(VEH_TYPE_PERSON) && !persIsInDeathCam());
	bool tired		= (fps && atpPctg <= BREATH_THRESHOLD) ? true : false;
	bool fatigued	= (fps && atpPctg <= HEARTBEAT_THRESHOLD) ? true : false;

	if(!persIsCrewDead() && (hit || tired || fatigued || _targetBreaths))
	{
		if(hit || (fps && fatigued))
			_playheartbeat(HEARTBEAT_VOLUME, HEARTBEAT_DURATION);

		if(fps)
		{
			if(frametime >= _Persona.breathCheck)
			{
				if(tired && fpFPSIsMoving(.1f))
				{
					_targetBreaths += BREATH_ADD;

					if(_targetBreaths > BREATH_SCALE)
						_targetBreaths = BREATH_SCALE;
				}

				_Persona.breathCheck = frametime + BREATH_CHECK;
			}

			if(hit)
			{
				if(_targetBreaths < BREATH_SCALE)
					_targetBreaths = BREATH_SCALE;

				if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
				{
					fxMiscStartHitConcussion(1.0f, 1000);
				}
			}

			if(_targetBreaths && frametime >= _Persona.breathNext)
			{
				++_breaths;

				// time to breath
				FLOAT32 scaler		= (FLOAT32)_breaths / (FLOAT32)BREATH_SCALE;
				FLOAT32 volume		= BREATH_VMIN + ((BREATH_VMAX - BREATH_VMIN) * scaler);
				FLOAT32 pitch			= 1.0f;
				SINT32 duration		= BREATH_DURATION;
				SINT32 breathBreak	= (int)(BREATH_TMAX - ((BREATH_TMAX - BREATH_TMIN) * scaler));

				_playbreathing(volume, pitch, duration);
				_Persona.breathNext = frametime + duration + breathBreak;

				--_targetBreaths;

				if(_targetBreaths < 0)
					_targetBreaths = 0;

				if(_breaths > _targetBreaths)
					_breaths = _targetBreaths;
			}
		}

#if(0)
		if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
		{
			fxMiscStartHitConcussion(1.0f, 1000);
		}
#endif
	}

	return 0;
}

UINT32 persAddHitSoundsForHitComponent(
							OBJECT * HitMapObject,
							SINT32 NumHits,
							FLOAT32 DamageAccrued)
{
SINT32 i,j;
FLOAT32 Damage;

	if (HitMapObject == nullptr)
	{
		return 0;
	}
	if (NumHits == 0)
	{
		return 0;
	}

	if (_Persona.VehicleAttached == nullptr)
	{
		return 0;
	}

	if (NumHits == 1)
	{
		// all damage goes into 1 slot
		for (i=0;i<MAX_DAMAGE_SOUND_QUE;i++)
		{
			if (_Persona.DamagePlayTime[i] == 0)
			{
				_Persona.DamagePlayTime[i] = frametime;
				_Persona.DamagePlayLocation[i] = HitMapObject->pos;
				_Persona.DamagePlayLocation[i].z = -_Persona.DamagePlayLocation[i].z;
				_Persona.DamagePlayObject[i] = HitMapObject;
				if (_Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy > 0.001f)
				{
					_Persona.DamageVolume[i] = DamageAccrued / _Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy;
				}
				else
				{
					_Persona.DamageVolume[i] = 1.0f;
				}

				if (_Persona.DamageVolume[i] > 1.0f)
				{
					_Persona.DamageVolume[i] = 1.0f;
				}


				break;
			}
		}
	}
	else
	{
		// 70% of damage goes into 1st hit, rest into remaining damage
		// 1st hit:
		Damage = DamageAccrued * 0.7f;
		for (i=0;i<MAX_DAMAGE_SOUND_QUE;i++)
		{
			if (_Persona.DamagePlayTime[i] == 0)
			{
				_Persona.DamagePlayTime[i] = frametime;
				_Persona.DamagePlayLocation[i] = HitMapObject->pos;
				_Persona.DamagePlayLocation[i].z = -_Persona.DamagePlayLocation[i].z;
				_Persona.DamagePlayObject[i] = HitMapObject;
				if (_Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy > 0.001f)
				{
					_Persona.DamageVolume[i] = Damage / _Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy;
				}
				else
				{
					_Persona.DamageVolume[i] = 1.0f;
				}

				if (_Persona.DamageVolume[i] > 1.0f)
				{
					_Persona.DamageVolume[i] = 1.0f;
				}


				break;
			}
		}
		// remaining
		for (j=1;j<NumHits;j++)
		{
			Damage = DamageAccrued / (NumHits-1);
			Damage *= mathGetFloat32ChaoticNumber() + 0.5f;

			for (i=0;i<MAX_DAMAGE_SOUND_QUE;i++)
			{
				if (_Persona.DamagePlayTime[i] == 0)
				{
					_Persona.DamagePlayLocation[i] = HitMapObject->pos;
					_Persona.DamagePlayLocation[i].z = -_Persona.DamagePlayLocation[i].z;
					_Persona.DamagePlayObject[i] = HitMapObject;
					if (_Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy > 0.001f)
					{
						_Persona.DamageVolume[i] = Damage / _Persona.VehicleAttached->VehicleType->HitMaxVolumeEnergy;
					}
					else
					{
						_Persona.DamageVolume[i] = 1.0f;
					}

					if (_Persona.DamageVolume[i] > 1.0f)
					{
						_Persona.DamageVolume[i] = 1.0f;
					}

					_Persona.DamagePlayTime[i] = frametime + (UINT32)(mathGetChaoticNumber()) % 500;

					break;
				}
			}
		}
	}

	return 0;
}

#define POS_VALID(pos) ((pos).x || (pos).y || (pos).z)
#define POS_MOVED(pos,chk) (((pos).x != (chk).x) || ((pos).y != (chk).y) || ((pos).z != (chk).z))
#define POS_RESET(pos) ((pos).x = (pos).y = (pos).z = 0)

int _DoResupplyCheck(void)
{
	// only check every once in a while (host determined)
	if(frametime < _resupplyCheck)
		return _resupplyState;

	// saved for use with our fire time check
//	const SINT32 lastCheckTime = _resupplyCheck;

	// set the next check time; normally check for something new every second
	_resupplyCheck = frametime + 1000;

	// no vehicle, no resupply
	if(_Persona.VehicleAttached == nullptr || _Persona.VehicleAttached->ModelData == nullptr)
		return GAMEEVENT_RESUPPLY_NA;

	// can't be dead
	if(persIsCrewDead())
		return GAMEEVENT_RESUPPLY_NA;

	if(SCEN_MGR->IsActive() && SCEN_MGR->HasFlag(ScenarioManager::RESUPPLY_ME))
	{
		if(_Persona.VehicleAttached->VehicleType->Type != VEH_TYPE_FLOATER)
			return _DoScenarioResupplyCheck();
	}
	else
	{
		switch(_Persona.VehicleAttached->VehicleType->Type)
		{
			case VEH_TYPE_GROUND:
			case VEH_TYPE_PERSON:
			{
				return _DoGroundResupplyCheck();

				/*
				UINT32 state = _DoTruckResupplyCheck();

				if(state == GAMEEVENT_RESUPPLY_NA)
					state = _DoFRUResupplyCheck();

				return state;
				*/
			}
			break; // take this out to left ground forces resupply from buildings
			default:
				return _DoStaticResupplyCheck(); // checks vehicles against against facilities
			break;
		}
	}

	// vehicle type not supported
	return GAMEEVENT_RESUPPLY_NA;
}

static bool _findResupplyFRU(ServerTrackedObject& sto, void* cookie)
{
	if(!cookie) return false;

	vehVehicle* player = persGetPersonaVehiclePtr();

	if(sto.type() == ServerTrackedObject::_UMS && wwiiSide(sto.flags()) == player->Side)
	{
		const WorldPosition_m& pos = sto.position();

		Vec3d sPos = Vec3d(pos.x(), pos.y(), pos.z());
		Vec3d pPos = Vec3d(player->ModelData->VehiclePos);

		Vec3d diff = sPos - pPos;

		if(diff.length2() <= (10.0f * 10.0f))
		{
			*(UINT32*)cookie = sto.uid();
			return true;
		}
	}

	return false;
}

UINT32 _DoGroundResupplyCheck(void)
{
	static vehVehicle* VehicleResupplier = nullptr;

	static FLOAT64VECTOR supplierPos	= {0,0,0}; // my supplier's position
	static FLOAT64VECTOR playerPos		= {0,0,0}; // my position

	// we have to look for any trucks that are stopped near us

	// shortcut validity checking...
	// if we got the new resupplier directly
	// from the vehicle list, no need to validate it again
	bool confirmedValid = false;

	if(VehicleResupplier == nullptr)
	{
		// we don't have a resupplier, try to find a new one
		vehVehicle*VehicleHead, *CurrentVehicle;
		vehGetVehicleHead(&VehicleHead);

		for (CurrentVehicle = VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
		{
			if(_IsValidResupplyVehicle(_Persona.VehicleAttached, CurrentVehicle, &supplierPos))
			{
				// found one, break!
				VehicleResupplier = CurrentVehicle;
				confirmedValid = true;
				break;
			}
		}
	}

	UINT32 STOResupplier = 0;

	// look for a valid resupply sto if we didn't find a vehicle resupplier
	if(!VehicleResupplier)
		wwiiForEachSTO(_findResupplyFRU, &STOResupplier);

	// is the resupply vehicle valid
	if(!VehicleResupplier ||																		// we don't have a resupplier available
	  (!confirmedValid &&																			// we didn't get them directly from the list
	  (!vehValid(VehicleResupplier) ||																// they're not a valid vehicle anymore
	   !_IsValidResupplyVehicle(_Persona.VehicleAttached, VehicleResupplier, &supplierPos))))		// they're not a valid resupplier
	{
		if(!STOResupplier)
		{
			// we did have a resupplier, but they're no longer valid
			// so we prevent resupplier for a while
			if(VehicleResupplier)
				_resupplyBreak = frametime + _resupplyPause;

			VehicleResupplier = nullptr;

			// resupplying not available
			return GAMEEVENT_RESUPPLY_NA;
		}
	}

	// if we made it this far, there was a valid truck OR sto near enough to us;
	// by valid, we mean not moving, within range and hasn't fired recently

	// set the check time to the tick
	_resupplyCheck = frametime + _resupplyTick;

	// now lets check to see if we've fired recently
	// if so prevent resupplying for a while
	if(_Persona.VehicleAttached->LastFireTime > frametime - 10000)
		_resupplyBreak = frametime + _resupplyPause;

	bool ridingResupplier = (VehicleResupplier &&
							 _Persona.VehicleAttached->RideVehicleHostIndex &&
							 _Persona.VehicleAttached->RideVehicleHostIndex == VehicleResupplier->VehicleHostIndex);

	piVECTOR velocity = {0,0,0};
	persGetPlayerVelocity(&velocity);

	// check my movement; if ok skip the first time here; make em wait one tick to start
	// we give movement a threshold because the vehicle can bounce around a bit
	// if we're riding the current resupplier, disregard any velocity or movement
	if((ridingResupplier || mathExactVectorLength(&velocity) < .5f) && _resupplyState >= GAMEEVENT_RESUPPLY_READY)
	{
		// this will never hit right now since we're not doing "have I moved" checking
		if(!ridingResupplier && POS_VALID(playerPos) && POS_MOVED(playerPos, _Persona.VehicleAttached->ModelData->VehiclePos))
		{
			// reset the player position
			POS_RESET(playerPos);

			// make em wait
			_resupplyBreak = frametime + _resupplyPause;
		}

		// ok, if we're not in a break, lets try to resupply
		if(!_resupplyBreak || frametime >= _resupplyBreak)
		{
			// reset the break if we had one
			_resupplyBreak = 0;

			// everything is good, do a resupply
			if(vehResupplyWeapons(_Persona.VehicleAttached))
			{
				// we've resupplied at least 1 weapon, check again in _resupplyTick time
				_resupplyCheck = frametime + _resupplyTick;

				// let the UI know we've sent received a resupply
				uiGameEvent(GAMEEVENT_RESUPPLY_RECEIVED);

				// save the current player resupply position
				/* this is used for "have I moved" checking, which we're not doing right now
				if(false && !POS_VALID(playerPos))
				{
					playerPos.x = _Persona.VehicleAttached->ModelData->VehiclePos.x;
					playerPos.y = _Persona.VehicleAttached->ModelData->VehiclePos.y;
					playerPos.z = _Persona.VehicleAttached->ModelData->VehiclePos.z;
				}

				if(false && !POS_VALID(supplierPos))
				{
					supplierPos.x = CurrentResupplier->PosData.Pos.x;
					supplierPos.y = CurrentResupplier->PosData.Pos.y;
					supplierPos.z = CurrentResupplier->PosData.Pos.z;
				}
				*/

				// let the UI know we're now resupplying
				return GAMEEVENT_RESUPPLY_RESUPPLYING;
			}
			else
			{
				// we must be full; lets reset
				VehicleResupplier = nullptr;

				// reset the supply positions
				POS_RESET(supplierPos);
				POS_RESET(playerPos);

				// make em wait the delay length
				_resupplyBreak = frametime + _resupplyPause;
			}
		}
	}

	// we're ready for resupply
	return GAMEEVENT_RESUPPLY_READY;
}

UINT32 _DoStaticResupplyCheck(void)
{
	static StratDB::StratObject* CurrentResupplier = nullptr;

	// we have to look for any resupply buildings that are near us
	if(CurrentResupplier == nullptr)
		CurrentResupplier = _LookupResupplyBuilding();

	// is the resupply building valid
	if(!CurrentResupplier || !_IsValidResupplyBuilding(CurrentResupplier))
	{
		// we did have a resupplier, but they're no longer valid
		// so we prevent resupplier for a while
		if(CurrentResupplier)
			_resupplyBreak = frametime + _resupplyPause;

		CurrentResupplier = nullptr;

		// resupplying not available
		return GAMEEVENT_RESUPPLY_NA;
	}

	// if we made it this far, there was a valid building near enough to us;
	// by valid, we mean same side

	// set the check time to the tick
	_resupplyCheck = frametime + _resupplyTick;

	// now lets check to see if we've fired recently
	// if so prevent resupplying for a while
	if(_Persona.VehicleAttached->LastFireTime > frametime - 10000)
		_resupplyBreak = frametime + _resupplyPause;

	piVECTOR velocity = {0,0,0};
	persGetPlayerVelocity(&velocity);

	// check my movement; if ok skip the first time here; make em wait one tick to start
	// we give movement a threshold because the vehicle can bounce around a bit
	// if we're riding the current resupplier, disregard any velocity or movement
	if(mathExactVectorLength(&velocity) < .5f && _resupplyState >= GAMEEVENT_RESUPPLY_READY)
	{
		// planes are resupplied on the backend of a longer wait period
		if(!_resupplyAirCheck)
			_resupplyAirCheck = frametime + _resupplyAirTick;

		// ok, if we're not in a break, lets try to resupply
		if(!_resupplyBreak || frametime >= _resupplyBreak)
		{
			// reset the break if we had one
			_resupplyBreak = 0;

			if(persIsPersonaVehicleType(VEH_TYPE_AIR) && frametime < _resupplyAirCheck)
			{
				// let the ui know we're trying to resupply the plane
				return GAMEEVENT_RESUPPLY_RESUPPLYING;
			}
			else
			{
				// everything is good, do a resupply
				if(vehResupplyWeapons(_Persona.VehicleAttached))
				{
					// we've resupplied at least 1 weapon, check again in _resupplyTick time
					_resupplyCheck = frametime + _resupplyTick;

					// let the UI know we've sent received a resupply
					uiGameEvent(GAMEEVENT_RESUPPLY_RECEIVED);

					// return the current state
					return GAMEEVENT_RESUPPLY_RESUPPLYING;
				}
				else
				{
					// we must be full; lets reset
					CurrentResupplier = nullptr;

					// make em wait the delay length
					_resupplyBreak = frametime + _resupplyPause;
					_resupplyAirCheck = 0;
				}
			}
		}
		else
		{
			// move the air resupply time back if we had a pause
			_resupplyAirCheck = 0;
		}
	}

	// we're ready for resupply
	return GAMEEVENT_RESUPPLY_READY;
}

UINT32 _DoScenarioResupplyCheck(void)
{
	// if we made it this far, there was a valid building near enough to us;
	// by valid, we mean same side

	// set the check time to the tick
	_resupplyCheck = frametime + _resupplyTick;

	// now lets check to see if we've fired recently
	// if so prevent resupplying for a while
	if(_Persona.VehicleAttached->LastFireTime > frametime - 1000)
		_resupplyBreak = frametime + _resupplyPause;

	piVECTOR velocity = {0,0,0};
	persGetPlayerVelocity(&velocity);

	// check my movement; if ok skip the first time here; make em wait one tick to start
	// we give movement a threshold because the vehicle can bounce around a bit
	// if we're riding the current resupplier, disregard any velocity or movement
	if(mathExactVectorLength(&velocity) < .5f && _resupplyState >= GAMEEVENT_RESUPPLY_READY)
	{
		// planes are resupplied on the backend of a longer wait period
		if(!_resupplyAirCheck)
			_resupplyAirCheck = frametime + _resupplyAirTick;

		// ok, if we're not in a break, lets try to resupply
		if(!_resupplyBreak || frametime >= _resupplyBreak)
		{
			// reset the break if we had one
			_resupplyBreak = 0;

			if(persIsPersonaVehicleType(VEH_TYPE_AIR) && frametime < _resupplyAirCheck)
			{
				// let the ui know we're trying to resupply the plane
				return GAMEEVENT_RESUPPLY_RESUPPLYING;
			}
			else
			{
				// everything is good, do a resupply
				if(vehResupplyWeapons(_Persona.VehicleAttached))
				{
					// we've resupplied at least 1 weapon, check again in _resupplyTick time
					_resupplyCheck = frametime + _resupplyTick;

					// let the UI know we've sent received a resupply
					uiGameEvent(GAMEEVENT_RESUPPLY_RECEIVED);

					// return the current state
					return GAMEEVENT_RESUPPLY_RESUPPLYING;
				}
				else
				{
					// we must be full; lets reset
					// make em wait the delay length
					_resupplyBreak = frametime + _resupplyPause;
					_resupplyAirCheck = 0;
				}
			}
		}
		else
		{
			// move the air resupply time back if we had a pause
			_resupplyAirCheck = 0;
		}
	}

	// we're ready for resupply
	return GAMEEVENT_RESUPPLY_READY;
}

LOGICAL _IsValidResupplyVehicle(const vehVehicle* player, const vehVehicle* resupplier, FLOAT64VECTOR* lastPos)
{
	if(resupplier != _Persona.VehicleAttached && // can't resupply yourself
	   !((vehVehicle*)resupplier)->isHulked() && // can't be hulked; had to cast away constness
		resupplier->Side == _Persona.VehicleAttached->Side && // same side
		resupplier->VehicleType->WeaponSpec.wCategory == 2 &&	// ground
		resupplier->VehicleType->WeaponSpec.wClass == 6)		// truck
	{
		// we have a possible resupply truck

		// first lets make sure they haven't fired for at least resupply tick time
		if(frametime > (resupplier->LastFireTime + _resupplyTick))
		{
			// lets check to see if they're close enough
			FLOAT64VECTOR vecDistance, vecPersona;

			vecPersona.x = _Persona.VehicleAttached->ModelData->VehiclePos.x;
			vecPersona.y = _Persona.VehicleAttached->ModelData->VehiclePos.y;
			vecPersona.z = _Persona.VehicleAttached->ModelData->VehiclePos.z;

			mathSubVectors(&vecDistance, &resupplier->PosData.Pos, &vecPersona);

			// this probably doesn't work for large vehicles like ships
			// since it calculates the position from the center point
			// might need bounding boxes for that situation to work
			if(mathVectDotProduct(&vecDistance, &vecDistance) <= _resupplyRadius)
			{
				if(player->RideVehicleHostIndex && player->RideVehicleHostIndex == resupplier->VehicleHostIndex)
				{
					// we're riding this resupply truck, ignore their motion
					return 1;
				}
				else
				{
					// lets check their velocity; must be zero
					const FLOAT64VECTOR& vehVelocity = resupplier->PosData.Vel;
					if(mathExactVectorLength(&vehVelocity) < .5f)
					{
						// let's make sure they haven't moved
						if(!POS_VALID(*lastPos) || !POS_MOVED(*lastPos, resupplier->PosData.Pos))
							return 1; // they aren't moving, haven't fire or moved, and are close enough, good supplier
					}
				}
			}
		}
	}

	return 0;
}

StratDB::StratObject* _LookupResupplyBuilding()
{
	StratDB::StratObject* cp		= StratDB::FindStratObject(_Persona.VehicleAttached->ModelData->closestCp);
	UINT32 vehType					= _Persona.VehicleAttached->VehicleType->Type;

	// must be same side as me
	if(!cp || cp->Side() != _Persona.VehicleAttached->Side)
		return nullptr;

	// cp can't be contested
	if(cp->StratFlags() & CP_STAT_CONTENTION)
		return nullptr;

	// bail out early if the flags indicate a failure
	if(vehType == VEH_TYPE_AIR && !(cp->StratFlags() & CP_STAT_HAS_AIRBASE))
		return nullptr;
	else if(vehType == VEH_TYPE_SEA && !(cp->StratFlags() & CP_STAT_HAS_NAVYBASE))
		return nullptr;

	StratDB::StratObject* closest	= nullptr;
	FLOAT64 lastDistance2			= 0;
	FLOAT64VECTOR stratPos, diff;

	// loop over facs, then buildings trying to find the right one; a close spawner
	StratDB::StratObject::ChildList facs = cp->ChildStratObjects();

	for(int f = 0; f < (int)facs.size(); f++)
	{
		StratDB::StratObject* fac = facs[f];

		if((vehType == VEH_TYPE_AIR && fac->StratType() == SDM_FACILITY_AIRBASE) ||
		   (vehType == VEH_TYPE_SEA && fac->StratType() == SDM_FACILITY_NAVALBASE))
		{
			// loop over and find the spawner
			StratDB::StratObject::ChildList buildings = fac->ChildStratObjects();

			for(int b = 0; b < (int)buildings.size(); b++)
			{
				StratDB::StratObject* building = buildings[b];

				if(building->StratType() == SDM_BUILDING_SPAWN || trncdSpawnDataExists(building->Filename()))
				{
					// we need to find the closest one
					stratPos.x = (FLOAT64)building->PosX();
					stratPos.y = (FLOAT64)building->PosY();
					stratPos.z = _Persona.VehicleAttached->ModelData->VehiclePos.z; // we just use our vert plane since it doesn't need to be super exact

					// get the diff vector
					mathSubVectors(&diff, &_Persona.VehicleAttached->ModelData->VehiclePos, &stratPos);

					// find the square of its length
					FLOAT64 distance2 = mathExactVectorLength2(&diff);

					// if we haven't found one yet or its closer, use it
					if(!closest || distance2 < lastDistance2)
					{
						closest			= building;
						lastDistance2	= distance2;
					}
				}
			}
		}
	}

	return closest;
}

LOGICAL _IsValidResupplyBuilding(StratDB::StratObject* resupplier)
{
	// resupplier will be a building (a spawner)
	if(!resupplier ||
		resupplier->StratClass() != SDM_CLASS_BUILDING ||
	   (resupplier->StratType() != SDM_BUILDING_SPAWN && !trncdSpawnDataExists(resupplier->Filename())))
		return FALSE;

	StratDB::StratObject* parent = resupplier->ParentStratObject();

	while(parent && parent->StratClass() != SDM_CLASS_CP)
		parent = parent->ParentStratObject();

	if(!parent || parent->StratClass() != SDM_CLASS_CP)
		return FALSE;

	if(parent->Side() != _Persona.VehicleAttached->Side)
		return FALSE;

	if(parent->StratFlags() & CP_STAT_CONTENTION)
		return FALSE;

	// ok, lets check our distance to it

	// lets check to see if they're close enough
	FLOAT64VECTOR vecDistance, resupplyPos;
	FLOAT64 radius = 0;

	switch(_Persona.VehicleAttached->VehicleType->Type)
	{
		case VEH_TYPE_AIR:
			radius = _resupplyRadiusAir;
		break;
		case VEH_TYPE_SEA:
			radius = _resupplyRadiusSea;
		break;
		default:
			radius = _resupplyRadius;
		break;
	}

	resupplyPos.x = (FLOAT64)resupplier->PosX();
	resupplyPos.y = (FLOAT64)resupplier->PosY();
	resupplyPos.z = _Persona.VehicleAttached->ModelData->VehiclePos.z;

	mathSubVectors(&vecDistance, &_Persona.VehicleAttached->ModelData->VehiclePos, &resupplyPos);

	if(mathVectDotProduct(&vecDistance, &vecDistance) > radius)
		return FALSE;

	// looks good!
	return TRUE;
}

UINT32 persGetResupplyState()
{
	return _resupplyState;
}

LOGICAL persIsOverSupplied()
{
	return vehmainIsOverSupplied(_Persona.VehicleAttached);
}

LOGICAL persIsFatigued()
{
	return (_Persona.heartbeatID > 0);
}

LOGICAL persIsTired()
{
	return (_Persona.breathID > 0);
}

void persResetHealths(void)
{
	for (int i = 0 ; i < ML_MAX_CREW_POSITIONS ; ++i )
		_PersonaHealth[i].clear();
}

void persShakeHead(const char* message)
{
	// only shake your head if your not already shaking your head
	if(frametime - _Persona.ShakeHeadStartTime > SHAKE_HEAD_DURATION)
	{
		_Persona.ShakeHeadStartTime = frametime;

		if(message)
			uiGameEvent(GAMEEVENT_ALERT, 1, uiLocString(message).c_str());
		else
			uiGameEvent(GAMEEVENT_ALERT, 1, uiLocString("common.headshake").c_str());
	}
}

void persKillCrew(void)
{
	if(vehmainHasBailedOut())
		return;

	if (_Persona.VehicleAttached != nullptr)
	{
		if (_Persona.VehicleAttached->ModelData != nullptr)
		{
			_Persona.VehicleAttached->ModelData->KillCrew = TRUE;
		}
	}

	for (size_t i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		_PersonaHealth[i].BrainOxyMaxLevel = 1;
		_PersonaHealth[i].BloodLevel = 1;
	}

	// let the tutorial layer know we've been killed
	TUT_MGR->DoEvent(TUT_EVENT_DIED);

	// make sure and cancel any ppos that are being built
	fpPPOCancel();
}

bool persAllowHUDUpdate(void)
{
	return (_Persona.BlackoutPercent <= 0.3f);
}

bool persCanVehicleExit(void)
{
	if (PN_UNLIKELY(!_Persona.VehicleAttached))
		return false;
	auto model = _Persona.VehicleAttached->ModelData;
	if (PN_UNLIKELY(!model))
		return false;

	const UINT32 MultiCrewPos = _Persona.VehicleAttached->ModelData->MultiCrewPos;
	if (MultiCrewPos == 0)
	{
		// exit is determined by 1st crew survival
		return (_PersonaHealth[0].BrainOxyMaxLevel > _BRAIN_BLACKOUT_FULL);
	}

	for (size_t i = 0; i < ML_MAX_CREW_POSITIONS; ++i)
	{
		// see if one of the crewmembers associated with the
		// multicrew persona is alive, if so, no exit
		if (_Persona.VehicleAttached->VehicleType->MultiCrewPos[i] == MultiCrewPos)
		{
			if (_PersonaHealth[i].BrainOxyMaxLevel <= _BRAIN_BLACKOUT_FULL)
			{
				return false;
			}
		}
	}

	return true;
}

bool persIsCrewDead(void)
{
	if (PN_UNLIKELY(!_Persona.VehicleAttached))
		return false;

	if (_Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON)
	{
		return (_PersonaHealth[0].BrainOxyMaxLevel >= _BRAIN_STRENGTH_FULLLOSS || _PersonaHealth[0].BloodLevel >= 0.5f || fpFPSIsDead());
	}

	bool AnyCrew = false;
	for (size_t i = 0; i < ML_MAX_CREW_POSITIONS; ++i)
	{
		if (_PersonaHealth[i].PayAttentionToPersona != 0)
		{
			AnyCrew = true;
			if (_PersonaHealth[i].BrainOxyMaxLevel < _BRAIN_STRENGTH_FULLLOSS && _PersonaHealth[i].BloodLevel < 0.5f)
				return false; // At least one crew member is alive.
		}
	}

	return AnyCrew;
}

void persSetPayAttentionFlag(UINT32 Position)
{
	_PersonaHealth[Position].PayAttentionToPersona = 1;
}

extern void persSetExitTime(SINT32 when)
{
	_exitTime = when;
}

void persSetDeathCamForNextUpdate()
{
	doDeathCamOnNextUpdate = true;
}

bool _persDeathCamera(piVECTOR* pos, piVECTOR* aim)
{
	static bool init = false;
	static SINT32 respawnTime = 0;

	vehVehicle* v = persGetPersonaVehiclePtr();

	if(!pos)
		init = false;

	if(!init && v)
	{
		respawnTime = frametime + 4000;
		init = true;
	}

	if(SCEN_MGR->IsActive() && v && !v->VehicleDeadTime)
	{
		if(frametime > respawnTime && !uiFindView("despawn"))
		{
			SCEN_MGR->SetFlag(ScenarioManager::RESPAWN_ME);
			vehExitVehicle(1,0,0);
			v->VehicleDeadTime = frametime - 4000; // make it exit right away
		}
	}

	return true;
}

void persSetDeathCam(UINT32 mode)
{
	if(vehmainHasBailedOut())
		return;

	// reset our ppo if we had one
	fpPPOKill();

	// determine start position and rotation
	//persSetExternalCameraPos(90.0f, 20.0f, 10.0f);
	//persSetExternalCameraTarget(90.0f, 50.0f, -10.0f, 5.0f);

	vehVehicle* v = persGetPersonaVehiclePtr();

	_InDeathCam = mode;

	if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
	{
		// set our ragdoll camera
		persSetExternalCameraAdjuster(fpTPSDeathCamera);
		persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), (float)50.0f, (float)2.0f);

		if(_ExternalCameraAdjuster)
			_ExternalCameraAdjuster(nullptr, nullptr);

		if(mode == 1)
		{
			persSetExternalCameraPivot(fpTPSDeathPivot);
			persSetExternalCameraTarget(1800.0f, 0.0f, 0.0f, 30.0f);
			persSetExternalCameraAdjuster(nullptr);

			//gr2AnimTrooper* anim = v->VehicleObjectData->PlaneObject->animModel;
			//if(anim) anim->twist.Straighten(anim);
		}
	}
	else
	{
		bool hulked = (v != nullptr && v->isHulked());

		if(persIsPersonaVehicleType(VEH_TYPE_GROUND))
		{
			if(hulked && mode == 2)
			{
				persSetExternalCameraAdjuster(_persDeathCamera);
				persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), 20.0f, (hulked) ? 20.0f: 8.0f);

				if(_ExternalCameraAdjuster)
					_ExternalCameraAdjuster(nullptr, nullptr);
			}
			else
			{
				persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), 20.0f, (hulked) ? 20.0f: 8.0f);
				persSetExternalCameraAdjuster(nullptr);
			}
		}
		else if(persIsPersonaVehicleType(VEH_TYPE_AIR))
		{
			if(hulked && mode == 2)
			{
				persSetExternalCameraAdjuster(vehVisDamageCamera);
				persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), (float)50.0f, (float)2.0f);

				if(_ExternalCameraAdjuster)
					_ExternalCameraAdjuster(nullptr, nullptr);
			}
			else
			{
				persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), 20, 15);
				persSetExternalCameraAdjuster(nullptr);
			}
		}
		else if(persIsPersonaVehicleType(VEH_TYPE_SEA))
		{
			persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), 20, 50);
		}

		if(v && mathExactVectorLength2(&v->ModelData->VehicleVel) < 5.0f * 5.0f)
			persSetExternalCameraTarget(1000.0f, 0, 0, 30.0f); // rotate around the horizontal plane for 30 seconds if we're slow or not moving
	}
}

//////////////////////////////////////
// Function: persIsInDeathCam
// Description: Are we in death cam?
/////////////////////////////////////
LOGICAL persIsInDeathCam()
{
	return _InDeathCam == 1;
}

UINT32 persGetCameraMode()
{
	return _CameraMode;
}

LOGICAL persIsCameraInternalMode()
{
	return _CameraMode == _INTERNAL_CAMERA;
}

LOGICAL persIsCameraExternalMode()
{
	return _CameraMode == _EXTERNAL_CAMERA;
}

void persGetCameraPosition(piVECTOR* pos)
{
	*pos = _CameraPos;
}

void persShakeCamera(FLOAT32 mag, SINT32 duration)
{
	if(vehmainHasBailedOut() || mag <= .0001f || duration <= 0) // resets shake
	{
		g_fShakeMagnitude = 0;
		_shaketime = 0;
		_shakeduration = 0;
	}
	else if(g_fShakeMagnitude || !_shaketime) // don't overshake!
	{
		g_fShakeMagnitude = mag;
		_shaketime = frametime;
		_shakeduration = duration;
	}
}

//////////////////////////////////////////////////////////////////////
// Function: persIsPersonaVehicleType
// Description: Test for persona type, i.e., VEH_TYPE_AIR, VEH_TYPE_GROUND, etc...
//////////////////////////////////////////////////////////////////////
LOGICAL persIsPersonaVehicleType(UINT32 uType)
{
	if(_Persona.VehicleAttached == nullptr)
	{
		return FALSE;
	}

	if(_Persona.VehicleAttached->VehicleType == nullptr)
	{
		return FALSE;
	}

	if(_Persona.VehicleAttached->VehicleType->Type == uType)
	{
		return TRUE;
	}

	return FALSE;
}

void persSetPositionOrdnanceNum (UINT32 position, UINT32 ordNum)
{
	_Persona.PositionData[position].ordnanceNum = ordNum;
}

UINT32 persGetPositionOrdnanceNum (UINT32 position)
{
	return _Persona.PositionData[position].ordnanceNum;
}

LOGICAL persIsCanopyExposed(UINT32 position)
{
	if(position >= ML_MAX_CREW_POSITIONS) return FALSE;
	if(_Persona.VehicleAttached->ModelData->CrewData.AlwaysExposed[position]) return TRUE;

	if(_Persona.VehicleAttached->ModelData->CrewData.ExposedByCanopy[position])
		return persIsCanopyOpen(position);

	return FALSE;
}

LOGICAL persIsHatchExposed(UINT32 position)
{
	if(position >= ML_MAX_CREW_POSITIONS) return FALSE;
	if(_Persona.VehicleAttached->ModelData->CrewData.AlwaysExposed[position]) return TRUE;

	if(_Persona.VehicleAttached->ModelData->CrewData.ExposedByHatch[position])
		return persIsHatchOpen(position);

	return FALSE;
}

LOGICAL persIsCanopyOpen(UINT32 position)
{
	if(position >= ML_MAX_CREW_POSITIONS) return FALSE;

	if(_Persona.VehicleAttached->ModelData->CrewData.DefaultCanopyPosition[0] == 0)
		return (_Persona.VehicleAttached->StateAnimation.CanopyPos[position] != 0);
	else
		return (_Persona.VehicleAttached->StateAnimation.CanopyPos[position] == 0);

	return FALSE;
}

LOGICAL persIsHatchOpen(UINT32 position)
{
	if(position >= ML_MAX_CREW_POSITIONS) return FALSE;

	if(_Persona.VehicleAttached->ModelData->CrewData.DefaultHatchPosition[0] == 0)
		return (_Persona.VehicleAttached->StateAnimation.HatchPos[position] != 0);
	else
		return (_Persona.VehicleAttached->StateAnimation.HatchPos[position] == 0);

	return FALSE;
}

FLOAT32 persCanopyVolumeMultiplier(UINT32 PosNum)
{
	if(_Persona.VehicleAttached->ModelData->CrewData.AlwaysExposed[PosNum]) return 1.0f;

	mlPositionData* CrewData = &_Persona.VehicleAttached->ModelData->CrewData;
	FLOAT32 mult, t;

	if(CrewData->DefaultCanopyPosition[PosNum] == 0)
		t = _Persona.VehicleAttached->StateAnimation.CanopyPos[PosNum];
	else
		t = 1.0f - _Persona.VehicleAttached->StateAnimation.CanopyPos[PosNum];

	mult = CrewData->CanopyClosedSoundVol[PosNum];
	mult += (FLOAT32) (t * (CrewData->CanopyOpenSoundVol[PosNum] - CrewData->CanopyClosedSoundVol[PosNum]));
	mult = 1.0f - mult;

	return mult;
}

FLOAT32 persHatchVolumeMultiplier(UINT32 PosNum)
{
	if(_Persona.VehicleAttached->ModelData->CrewData.AlwaysExposed[PosNum]) return 1.0f;

	mlPositionData* CrewData = &_Persona.VehicleAttached->ModelData->CrewData;
	FLOAT32 mult, t;

	if(CrewData->DefaultHatchPosition[PosNum] == 0)
		t = _Persona.VehicleAttached->StateAnimation.HatchPos[PosNum];
	else
		t = 1.0f - _Persona.VehicleAttached->StateAnimation.HatchPos[PosNum];

	mult = CrewData->HatchClosedSoundVol[PosNum];
	mult += (FLOAT32) (t * (CrewData->HatchOpenSoundVol[PosNum] - CrewData->HatchClosedSoundVol[PosNum]));
	mult = 1.0f - mult;

	return mult;
}

static void _killSound(void* data)
{
	if(data)
	{
		SINT32* soundID = (SINT32*)data;
		*soundID = 0;
	}
}

#define CONCUSSION_SOUND_LIMIT  5000

static void _soundUpdate(FXSOUND* s)
{
}

void _playearring(FLOAT32 volume, SINT32 duration)
{
	if(persIsInDeathCam())
		return;

#if FRES
	// Do nothing for now
#else
	_playsound("sounds/ear_ringing.wav", _Persona.earringSoundID, volume * .5f, .7f, duration);

	/*
	SINT32 id = fxSndGetNewSound();

	if(id == -1)
	{
		return;
	}

	fxSndSetPitch(id, 0.7f);

	fxSndSetVolume(id, volume * 0.5f);
	fxSndSetPriority(id, IDE_FP_PRIORITY_LOW);
	fxSndSetSoundType(id, TYPE_INFINITE_SOUND);
	fxSndSetSoundName(id, "sounds//ear_ringing.wav");
	fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
	fxSndStopSoundDelayed(id, duration);
	fxSndSetKillCallback(id, _killSound);
	fxSndSetCreateTime(id, frametime + 50);
	fxSndPushFlags(id, SOUND_NO_OVERALL);
	fxSndSetUserUpdate(id, _soundUpdate);

	fxSndSetKillData(id, &_Persona.earringSoundID);

	// Required after settings
	if(fxSndInitSound(id))
	{
		_Persona.earringSoundID = id;
	}
	*/
#endif
}

void _playheartbeat(FLOAT32 volume, SINT32 duration)
{
	if(persIsInDeathCam())
		return;

#if FRES
	// Do nothing for now
#else
	_playsound("sounds/heartbeat.wav", _Persona.heartbeatID, volume, 1.0f, duration);
	/*
	SINT32 id = fxSndGetNewSound();

	if(id == -1)
	{
		return;
	}

	fxSndSetPitch(id, 1.0f);

	fxSndSetVolume(id, volume);
	fxSndSetPriority(id, IDE_FP_PRIORITY_LOW);
	fxSndSetSoundType(id, TYPE_INFINITE_SOUND);
	fxSndSetSoundName(id, "sounds//heartbeat.wav");
	fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
	fxSndStopSoundDelayed(id, duration);
	fxSndSetKillCallback(id, _killSound);
	fxSndSetCreateTime(id, frametime + 50);
	fxSndPushFlags(id, SOUND_NO_OVERALL);
	fxSndSetUserUpdate(id, _soundUpdate);

	fxSndSetKillData(id, &_Persona.heartbeatID);

	// Required after settings
	if(fxSndInitSound(id))
	{
		_Persona.heartbeatID = id;
	}
	*/
#endif
}

void _playbreathing(FLOAT32 volume, FLOAT32 pitch, SINT32 duration)
{
	if(persIsInDeathCam())
		return;

#if FRES
	// Do nothing for now
#else
	_playsound("sounds/troop/breath1.wav", _Persona.breathID, volume, pitch, duration);

	/*
	SINT32 id = fxSndGetNewSound();

	if(id == -1)
	{
		return;
	}

	fxSndSetPitch(id, pitch);
	fxSndSetVolume(id, volume);
	fxSndSetPriority(id, IDE_FP_PRIORITY_LOW);
	fxSndSetSoundType(id, TYPE_INFINITE_SOUND);
	fxSndSetSoundName(id, "sounds//troop//breath1.wav");
	fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
	fxSndStopSoundDelayed(id, duration);
	fxSndSetKillCallback(id, _killSound);
	fxSndSetCreateTime(id, frametime + 50);
	fxSndPushFlags(id, SOUND_NO_OVERALL);
	fxSndSetUserUpdate(id, _soundUpdate);

	fxSndSetKillData(id, &_Persona.breathID);

	// Required after settings
	if(fxSndInitSound(id))
	{
		_Persona.breathID = id;
	}
	*/
#endif
}

void _playshellshock(FLOAT32 volume, SINT32 duration)
{
#if FRES
	// Do nothing for now
#else
	//_playsound("sounds//troop//breath1.wav", _Persona.shellshockID, volume, 1.0f, duration);

	_targetBreaths = BREATH_SCALE;
	_breaths = 0;

	/*
	SINT32 id = fxSndGetNewSound();

	if(id == -1)
	{
		return;
	}

	fxSndSetPitch(id, 1.0f);

	fxSndSetVolume(id, volume);
	fxSndSetPriority(id, IDE_FP_PRIORITY_LOW);
	fxSndSetSoundType(id, TYPE_INFINITE_SOUND);
	fxSndSetSoundName(id, "sounds//shellshock.wav");
	fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
	fxSndStopSoundDelayed(id, duration);
	fxSndSetKillCallback(id, _killSound);
	fxSndSetCreateTime(id, frametime + 50);
	fxSndPushFlags(id, SOUND_NO_OVERALL);
	fxSndSetUserUpdate(id, _soundUpdate);

	fxSndSetKillData(id, &_Persona.shellshockID);

	// Required after settings
	if(fxSndInitSound(id))
	{
		_Persona.shellshockID = id;
	}
	*/
#endif
}

void _playsound(const char* path, SINT32& id, FLOAT32 volume, FLOAT32 pitch, SINT32 duration)
{
	if(id > 0)
	{
		fxSndChangePitch(id, pitch);
		fxSndChangeVolume(id, volume);
		fxSndStopSoundDelayed(id, duration);
	}
	else
	{
		SINT32 newID = fxSndGetNewSound();

		if(newID == -1)
		{
			return;
		}

		fxSndSetPitch(newID, pitch);
		fxSndSetVolume(newID, volume);
		fxSndSetPriority(newID, IDE_FP_PRIORITY_LOW);
		fxSndSetSoundType(newID, TYPE_INFINITE_SOUND);
		fxSndSetSoundName(newID, (SCHAR*)path);
		fxSndSetPlayType(newID, DEF_PLAYTYPE_HEAD);
		fxSndStopSoundDelayed(newID, duration);
		fxSndSetKillCallback(newID, _killSound);
		fxSndSetCreateTime(newID, frametime + 50);
		fxSndPushFlags(newID, SOUND_NO_OVERALL);
		fxSndSetUserUpdate(newID, _soundUpdate);

		fxSndSetKillData(newID, &id);

		// Required after settings
		if(fxSndInitSound(newID))
		{
			id = newID;
		}
	}
}

static inline bool TornadoCheck(const FLOAT64VECTOR& val)
{
	if(_isnan(val.x) || !_finite(val.x))
		return true;

	if(_isnan(val.y) || !_finite(val.y))
		return true;

	if(_isnan(val.z) || !_finite(val.z))
		return true;

	return false;
};

void persSetConcussionEffect(const weapAmmo* ammo)
{
	if(!ammo)
	{
		return;
	}

	if(ammo->AmmoType->ExplosiveEnergy <= 0.0001f)
	{
		return;
	}

	// Don't over shake
	if(g_fShakeMagnitude)
	{
		return;
	}

	if(persIsCrewDead())
	{
		return;
	}

	piVECTOR ammopos;

	// May need adjustment
	FLOAT32 maxeffectdist = pow(ammo->AmmoType->ExplosiveEnergy, 0.215f);

	bool isTrooper = (_Persona.VehicleAttached && _Persona.VehicleAttached->VehicleType->Type == VEH_TYPE_PERSON);

	// if trooper, process the concussion mod
	if(isTrooper)
		maxeffectdist *= fpFPSGetConcussionMod();

	if(TornadoCheck(ammo->Position))
	{
		return;
	}

	ammopos.x = (FLOAT32)(ammo->Position.x - WorldOriginOffset.x);
	ammopos.y = (FLOAT32)(ammo->Position.y - WorldOriginOffset.y);
	ammopos.z = (FLOAT32)(ammo->Position.z - WorldOriginOffset.z);

	piVECTOR diff;

	diff.x = ammopos.x + playpos.x;
	diff.y = ammopos.y + playpos.y;
	diff.z = playpos.z - ammopos.z;

	FLOAT32 distance = math3dFastSqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

	if(distance > maxeffectdist)
	{
		return;
	}

	if(distance < 1.0f)
	{
		distance = 1.0f;
	}


	FLOAT32 t = distance / maxeffectdist;
	FLOAT32 x0 = 0.0;
	FLOAT32 x1 = 1.0f;

	t = t > 1.0f ? 1.0f : t;

	FLOAT32 volume = x0 + t * (x1 - x0);
	volume = 1.0f - volume;

	FLOAT32 shake = 1.0f - t;
	SINT32 duration = FloatToInt(CONCUSSION_SOUND_LIMIT * (1.0f - t));

	duration = duration < 1000 ? 1000 : duration;

	if(!isTrooper)
		fxMiscStartExpConcussion(maxeffectdist * (1.0f - t), duration);

	persShakeCamera((shake > 0.05f) ? 0.05f : shake, duration);

	if(_Persona.earringSoundID > 0)
	{
		if(fxSndGetVolume(_Persona.earringSoundID) < volume)
		{
			fxSndChangeVolume(_Persona.earringSoundID, volume);
		}

		if(fxSndGetVolume(_Persona.heartbeatID) < volume)
		{
			fxSndChangeVolume(_Persona.heartbeatID, volume);
		}

		if(fxSndGetVolume(_Persona.shellshockID) < volume)
		{
			fxSndChangeVolume(_Persona.shellshockID, volume);
		}

		// Set delayed stop time again only.
		fxSndStopSoundDelayed(_Persona.earringSoundID, duration + 1000);
		fxSndStopSoundDelayed(_Persona.heartbeatID, duration + 1000);
		fxSndStopSoundDelayed(_Persona.shellshockID, duration + 1000);
	}
	else
	{
		_playearring(volume, duration + 1000);
		_playheartbeat(volume, duration + 1000);
		_playshellshock(volume, duration + 1000);
	}
}

/*------------------------------------------------------------------
Function: persGetMySupercellAndOctet
Description: Returns your current supercell and octet.
Octet row/column is 0,0 at bottom left and 15,15 at top right
-------------------------------------------------------------------*/
bool persGetMySupercellAndOctet(int& sx, int& sy, int& r, int& c)
{
	if(!_Persona.VehicleAttached || !_Persona.VehicleAttached->ModelData)
	{
		sx = sy = r = c = -1;
		return false;
	}

	FLOAT64VECTOR pos = _Persona.VehicleAttached->ModelData->VehiclePos;
	Vec3d v = Vec3d(pos); //make gcc happy
	trnmainGetSupercellAndOctet(v, sx, sy, r, c);

	return true;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleTank
Description: Are we a tank?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleTank(const vehVehicle* v)
{
    static WEAPON_SPEC tank = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        tank = weapCreateSpec("GV_tank.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == tank.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == tank.wClass)
            {
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleDestroyer
Description: Are we a destroyer?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleDestroyer(const vehVehicle* v)
{
    static WEAPON_SPEC destroyer = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        destroyer = weapCreateSpec("SV_sea.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == destroyer.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == destroyer.wClass)
            {
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleRiverBoat
Description: Are we a river boat?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleRiverBoat(const vehVehicle* v)
{
    static WEAPON_SPEC riverboat = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        riverboat = weapCreateSpec("SV_riverine.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == riverboat.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == riverboat.wClass)
            {
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleTruck
Description: Are we a truck?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleTruck(const vehVehicle* v)
{
    static WEAPON_SPEC truck = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        truck = weapCreateSpec("GV_truck.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == truck.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == truck.wClass)
            {
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleBomber
Description: Are we a bomber?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleBomber(const vehVehicle* v)
{
    static WEAPON_SPEC bomber = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        bomber = weapCreateSpec("AC_bomber.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == bomber.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == bomber.wClass)
            {
                return true;
            }
        }
    }

    return false;
}

/*-------------------------------------------------------------------
Function: persIsPersonaVehicleFighter
Description: Are we a figher plane?
-------------------------------------------------------------------*/
bool persIsPersonaVehicleFighter(const vehVehicle* v)
{
    static WEAPON_SPEC fighter = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        fighter = weapCreateSpec("AC_fighter.1");
    }

    if(v && v->VehicleType)
    {
        if(v->VehicleType->WeaponSpec.wClass == fighter.wClass)
        {
            return true;
        }
    }

    if(_Persona.VehicleAttached)
    {
        if(_Persona.VehicleAttached->VehicleType)
        {
            if(_Persona.VehicleAttached->VehicleType->WeaponSpec.wClass == fighter.wClass)
            {
                return true;
            }
        }
    }

    return false;
}
