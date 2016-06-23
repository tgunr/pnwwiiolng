#pragma once

#include "veh/vehmain.h"
#include "ob/obmain.h"
#include "ml/mltypes.h"
#include "wwii/wwiiCovert.h"

/*
	Main "persona" handling routines

	The "Persona" is the point of view of the player, the "state" they're in, etc
	It is usually attached to a vehicle of some sort

	Originating Programmer: Jonathan Hoof
*/

#define PERS_MAX_POSITIONS ML_MAX_CREW_POSITIONS

#define PERS_HITLOCATION_HEAD 0
#define PERS_HITLOCATION_TORSO 1
#define PERS_HITLOCATION_ARMS 2
#define PERS_HITLOCATION_LEGS 3

#define MAX_DAMAGE_SOUND_QUE 64

typedef bool (*ExternalCameraAdjust)(piVECTOR*, piVECTOR*);
typedef bool (*ExternalCameraPivot)(piVECTOR*, piMATRIX*);

struct persPositionDataElement {
	UINT32	ordnanceNum;
	UINT32	gunsightNum;
};

struct persCommandListElement
{
	UCHAR CommandName[32];
	UINT32 CommandId;
};

struct persPersona
{
	vehVehicle* VehicleAttached;
	UINT32 Position;
	OBJECT * PersonaObject;
	OBJECT * PersonaPosition;
	OBJECT * PersonaBodyDOF;
	OBJECT * PersonaHeadDOF;
	OBJECT * PersonaEye;
	OBJECT * PersonaEyeDOF;

	OBJECT * InstrumentViewDOF;
	FLOAT32 InstrumentViewFOV[4];
	UINT32 InstrumentViewNum;
	UINT32 InstrumentViewCurrent;
	OBJECT * GunsightViewDOF;
	FLOAT32 GunsightViewFOV[4];
	UINT32 GunsightViewNum;
	UINT32 GunsightViewCurrent;


	FLOAT64ATT LastBodyAtt;
	FLOAT64ATT LastHeadAtt;
	FLOAT64ATT LastEyeAtt;

	piVECTOR CockpitCameraPos; // unscaled cockpit relative position

	UINT32 Redout; // Do we display a redout or blackout?
	FLOAT32 BlackoutPercent; // 0.0 = normal, 0.5 = grey/redout, 1.0 = black
	LOGICAL ControlsDisabled; // can the persona control vehicle?
	FLOAT32 PainLevel; // 0.0 = no dramatic pain, 1.0 = pain
	FLOAT32 PainLevel2; // temporary shock pain
	UINT32 FPSIsDead;

	FLOAT32 Strength; // player's current strength, 1.0 = full, 0.0 = none

	UINT32 PersonaInstalledOnHost;

	// damage hit sound queue
	piVECTOR DamagePlayLocation[MAX_DAMAGE_SOUND_QUE];
	OBJECT * DamagePlayObject[MAX_DAMAGE_SOUND_QUE];
	FLOAT32 DamageVolume[MAX_DAMAGE_SOUND_QUE];
	SINT32 DamagePlayTime[MAX_DAMAGE_SOUND_QUE];

	SINT32 PainSoundId;
    SINT32 earringSoundID;
    SINT32 heartbeatID;
    SINT32 shellshockID;
	SINT32 breathID;
	SINT32 breathCheck;
	SINT32 breathNext;

	SINT32 ShakeHeadStartTime;

	persPositionDataElement	PositionData[ML_MAX_CREW_POSITIONS];

};

class persPersonaHealth
{
public:
	persPersonaHealth()
		: BrainOxyLevel(0.0f), BrainOxyMaxLevel(0.0f)
		, ATPLevel(0.0f), ATPCurrentLevel(0.0f), ATPMaxLevel(0.0f)
		, BloodPressure(0.0f), BloodLevel(0.0f), BloodCurrentLevel(0.0f)
		, PayAttentionToPersona(FALSE) {}
	~persPersonaHealth() {}

	void clear();

public:
	// these are 0.0 = normal, 1.0 = dead
	Covert<FLOAT32> BrainOxyLevel; // current oxy level
	Covert<FLOAT32> BrainOxyMaxLevel; // instantaneous oxy level

	// these are 0.0 = no fatigue, 1.0 = totally fatigues
	Covert<FLOAT32> ATPLevel; // instantaneous ATP level, used for hit effects
	Covert<FLOAT32> ATPCurrentLevel; // current ATP level
	Covert<FLOAT32> ATPMaxLevel; // maximum ATP level

	// pressure is G level
	// blood level 0.0 = full blood, 1.0 = problems
	Covert<FLOAT32> BloodPressure; // blood pressure G rating
	Covert<FLOAT32> BloodLevel; // blood level, where we're headed
	Covert<FLOAT32> BloodCurrentLevel; // current blood level

	Covert<UINT32> PayAttentionToPersona; // used for vehicle kill stuff
};

// start lua compatibility functions and types with OSG

//struct VehicleBase;

class Player
{
public:
	vehVehicle* GetVehicle();
	void ShakeHead();
	float GetEnergy() const;
	float GetHealth() const;
	int GetCountry() const;
	int GetSide() const;
	int GetRangeSetting() const;
};

Player* playerGetPlayer();
vehVehicle* playerGetVehicle();
// end lua compatibility

extern UINT32 persInit(void); // initializes the persona system

extern UINT32 persKill(void); // kills the persona system

extern UINT32 persUpdate(void); // updater for the persona system

// attaches the "persona" to a particular position on a particular vehicle
extern UINT32 persAttachPlayerToVehicle(vehVehicle* Vehicle,UINT32 Position);

// gets the current camera position and attitude matrix
extern UINT32 persGetCameraPosAndAtt(piVECTOR * Pos,piVECTOR * Att);

// returns the persPersona structure pointer for the player's persona
extern persPersona* persGetPersonaDataPtr();

// returns the vehVehicle the player is attached to, or nullptr if not attached
extern vehVehicle* persGetPersonaVehiclePtr();

// If the persona is controlling a populated and ready vehicle,
// and the game is online, then this returns the vehVehicle.
extern vehVehicle* persGetPersonaPilotedOnlineVehicle();

// if the player is attached to a vehicle and that vehicle has an object,
// returns that object, otherwise nullptr
extern OBJECT* persGetPersonaVehicleObjectDataPtr();

// returns the current position number of the player
extern UINT32 persGetPersonaPosition(UINT32 * Position);

// sets the position number for the persona
extern UINT32 persSetPersonaPosition(UINT32 InPosition);

// called after the persona is initially attached to the vehicle
// so any old data can be reset
extern UINT32 persResetPersonaSettings();

// if the player is attached to a vehicle and that vehicle has model data
// returns the model data, otherwise nullptr
extern mlVehicleData* persGetPersonaModelDataPtr();

// checks if the persona is attached to a particular vehicle
// if so it detaches the persona
extern UINT32 persDetachFromVehicle(vehVehicle* Vehicle);

// detaches the persona from the currently attached vehicle, if attached
extern UINT32 persDetachPersona(void);

extern UINT32 persUpdateCockpitSmoke(void);

extern UINT32 persToggleCamera();
extern UINT32 persToggleInternalExternalViews(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persSetExternalCamera(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persSetInternalCamera(SINT32 Param,UINT32 Id,UINT32 Detach);

extern void persSetExternalCameraPos(FLOAT32 horAxis, FLOAT32 verAxis, FLOAT32 zoom, bool checkMode = true);
extern void persSetExternalCameraTarget(FLOAT32 horDeg, FLOAT32 verDeg, FLOAT32 zoom, FLOAT32 duration);
extern void persSetExternalCameraAdjuster(ExternalCameraAdjust a);
extern void persSetExternalCameraPivot(ExternalCameraPivot p);
extern bool persSubmergedDeathCamera(piVECTOR* pos, piVECTOR* aim);

extern UINT32 persPersonaIsCreatedOnHost(void);
extern UINT32 persIsPersonaCreated(void);

extern UINT32 persSetExternalLockedCamera(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persSetExternalRubberBandCamera(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persSetExternalFixedCamera(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persSetFixedCameraPos(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persAdjustExternalFixedCamera(piVECTOR* Adjustment);

extern UINT32 persGetPlayerVelocity(piVECTOR * Vel);
extern UINT32 persGetCrewIsDead(UINT32 Position);

extern UINT32 persSetVehicleG(FLOAT32 VehicleG, vehVehicle* Vehicle);
extern UINT32 persGetHealthStats(FLOAT32 * BrainOxy,
								  FLOAT32 * ATPLevel,
								  FLOAT32 * BloodPressure,
								  FLOAT32 * BloodLevel);
extern UINT32 persGetMaxBrainOxy(FLOAT32 * MaxBrainOxy, FLOAT32* BlackoutFullPercent);

extern UINT32 persDrawRedoutBlackout(void);
extern UINT32 persGetControlStatus(FLOAT32 * StrengthControl, LOGICAL* ControlsDisabled);
extern UINT32 persGetGDistortion(piMATRIX * RetMat);
extern UINT32 persApplyDamage(FLOAT64 Damage,UINT32 Position, UINT32 Location, LOGICAL ATPOnly);
extern UINT32 persAddHitSoundsForHitComponent(
								OBJECT * HitMapObject,
								SINT32 NumHits,
								FLOAT32 DamageAccrued);

extern void persSetView(UINT32 type, UINT32 level);
extern void persSetNormalView(FLOAT32 FieldOfView_);
extern void persResetHealths(void);
extern UINT32 persSetPersonaPositionCallback(SINT32 param, UINT32 id, UINT32 detach);
extern void persShakeHead(const char* message = nullptr);
extern void persKillCrew(void);
extern bool persAllowHUDUpdate(void);

extern bool persCanVehicleExit(void);
extern bool persIsCrewDead(void);
extern FLOAT32 persGetFogScaleMultiplier(void);

extern LOGICAL persDrawGunsight(void);
extern UINT32 persResetVehicleG(void);
extern UINT32 persGetCrewDeadForKillScore(UINT32 Position);

extern void persSetPayAttentionFlag(UINT32 Position);

extern void persSetDeathCam(UINT32 mode = 1);
extern LOGICAL persIsInDeathCam();

extern UINT32 persGetCameraMode();
extern LOGICAL persIsPersonaVehicleType(UINT32 uType);
extern LOGICAL persIsCameraInternalMode();
extern LOGICAL persIsCameraExternalMode();

extern void persGetCameraPosition(piVECTOR*);
extern void persShakeCamera(FLOAT32 mag, SINT32 duration = 1000);
extern void persSetConcussionEffect(const weapAmmo* ammo);

extern bool persGetMySupercellAndOctet(int& sx, int& sy, int& r, int& c);

extern void	persSetPositionOrdnanceNum (UINT32 position, UINT32 ordNum);
extern UINT32	persGetPositionOrdnanceNum (UINT32 position);

extern LOGICAL persIsCanopyOpen(UINT32 position);
extern LOGICAL persIsHatchOpen(UINT32 position);
extern LOGICAL persIsCanopyExposed(UINT32 position);
extern LOGICAL persIsHatchExposed(UINT32 position);
extern FLOAT32 persHatchVolumeMultiplier(UINT32 position);
extern FLOAT32 persCanopyVolumeMultiplier(UINT32 position);

extern UINT32 persGetResupplyState();
extern LOGICAL persIsOverSupplied();
extern LOGICAL persIsFatigued();
extern LOGICAL persIsTired();

extern void persSetDeathCamForNextUpdate();
extern void persSetExitTime(SINT32 when);

extern bool persIsPersonaVehicleTank(const vehVehicle* v = nullptr);
extern bool persIsPersonaVehicleDestroyer(const vehVehicle* v = nullptr);
extern bool persIsPersonaVehicleRiverBoat(const vehVehicle* v = nullptr);
extern bool persIsPersonaVehicleTruck(const vehVehicle* v = nullptr);
extern bool persIsPersonaVehicleBomber(const vehVehicle* v = nullptr);
extern bool persIsPersonaVehicleFighter(const vehVehicle* v = nullptr);
